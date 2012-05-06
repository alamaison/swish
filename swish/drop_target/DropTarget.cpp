/**
    @file

    Expose the remote filesystem as an IDropTarget.

    @if license

    Copyright (C) 2009, 2010, 2012  Alexander Lamaison <awl03@doc.ic.ac.uk>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

    @endif
*/

#include "DropTarget.hpp"

#include "swish/drop_target/CopyFileOperation.hpp" // CopyFileOperation
#include "swish/drop_target/CreateDirectoryOperation.hpp"
                                                    // CreateDirectoryOperation
#include "swish/drop_target/Operation.hpp" // Operation
#include "swish/interfaces/SftpProvider.h" // ISftpProvider/Consumer
#include "swish/shell_folder/data_object/ShellDataObject.hpp" // ShellDataObject
#include "swish/shell_folder/SftpDirectory.h" // CSftpDirectory
#include "swish/windows_api.hpp" // SHBindToParent

#include <winapi/com/catch.hpp> // WINAPI_COM_CATCH_AUTO_INTERFACE
#include <winapi/shell/shell.hpp> // strret_to_string, parsing_name_from_pidl,
                                  // bind_to_handler_object, stream_from_pidl

#include <boost/bind.hpp> // bind
#include <boost/cstdint.hpp> // int64_t
#include <boost/function.hpp> // function
#include <boost/foreach.hpp> // BOOST_FOREACH
#include <boost/lambda/lambda.hpp> // _1, _2
#include <boost/numeric/conversion/cast.hpp> // numeric_cast
#include <boost/ptr_container/ptr_vector.hpp> // ptr_vector
#include <boost/shared_ptr.hpp>  // shared_ptr
#include <boost/throw_exception.hpp>  // BOOST_THROW_EXCEPTION

#include <comet/bstr.h> // bstr_t
#include <comet/datetime.h> // datetime_t
#include <comet/ptr.h>  // com_ptr

#include <cassert> // assert
#include <exception> // exception
#include <functional> // unary_function
#include <stdexcept> // logic_error
#include <string>

using swish::remote_folder::create_remote_itemid;
using swish::shell_folder::data_object::ShellDataObject;
using swish::shell_folder::data_object::PidlFormat;

using winapi::shell::bind_to_handler_object;
using winapi::shell::parsing_name_from_pidl;
using winapi::shell::pidl::pidl_t;
using winapi::shell::pidl::apidl_t;
using winapi::shell::pidl::cpidl_t;
using winapi::shell::stream_from_pidl;
using winapi::shell::strret_to_string;

using boost::bind;
using boost::int64_t;
using boost::filesystem::wpath;
using boost::function;
using boost::numeric_cast;
using boost::ptr_vector;
using boost::shared_ptr;

using comet::bstr_t;
using comet::com_error;
using comet::com_ptr;
using comet::datetime_t;

using std::exception;
using std::logic_error;
using std::size_t;
using std::wstring;

namespace swish {
namespace drop_target {

namespace { // private

	/**
	 * Given a DataObject and bitfield of allowed DROPEFFECTs, determine
	 * which drop effect, if any, should be chosen.  If none are
	 * appropriate, return DROPEFFECT_NONE.
	 */
	DWORD determine_drop_effect(
		const com_ptr<IDataObject>& pdo, DWORD allowed_effects)
	{
		if (pdo)
		{
			PidlFormat format(pdo);
			if (format.pidl_count() > 0)
			{
				if (allowed_effects & DROPEFFECT_COPY)
					return DROPEFFECT_COPY;
			}
		}

		return DROPEFFECT_NONE;
	}

	std::pair<wpath, int64_t> stat_stream(const com_ptr<IStream>& stream)
	{
		STATSTG statstg;
		HRESULT hr = stream->Stat(&statstg, STATFLAG_DEFAULT);
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(com_error_from_interface(stream, hr));

		shared_ptr<OLECHAR> name(statstg.pwcsName, ::CoTaskMemFree);
		return std::make_pair(name.get(), statstg.cbSize.QuadPart);
	}

	/**
	 * Return the stream name from an IStream.
	 */
	wpath filename_from_stream(const com_ptr<IStream>& stream)
	{
		return stat_stream(stream).first;
	}

	/**
	 * Query an item's parent folder for the item's display name relative
	 * to that folder.
	 */
	wstring display_name_of_item(
		const com_ptr<IShellFolder>& parent_folder, const cpidl_t& pidl)
	{
		STRRET strret;
		HRESULT hr = parent_folder->GetDisplayNameOf(
			pidl.get(), SHGDN_INFOLDER | SHGDN_FORPARSING, &strret);
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(com_error_from_interface(parent_folder, hr));

		return strret_to_string<wchar_t>(strret, pidl);
	}

	/**
	 * Return the parsing name of an item.
	 */
	wpath display_name_from_pidl(const apidl_t& parent, const cpidl_t& item)
	{
		com_ptr<IShellFolder> parent_folder = 
			bind_to_handler_object<IShellFolder>(parent);

		return display_name_of_item(parent_folder, item);
	}

	/**
	 * Return the parsing path name for a PIDL relative the the given parent.
	 */
	wpath parsing_path_from_pidl(const apidl_t& parent, const pidl_t& pidl)
	{
		if (pidl.empty())
			return wpath();

		cpidl_t item;
		item.attach(::ILCloneFirst(pidl.get()));

		return display_name_from_pidl(parent, item) / 
			parsing_path_from_pidl(parent + item, ::ILNext(pidl.get()));
	}

	/**
	 * Calculate percentage.
	 *
	 * @bug  Throws if using ludicrously large sizes.
	 */
	int percentage(int64_t done, int64_t total)
	{
		if (total == 0)
			return 100;
		else
			return numeric_cast<int>((done * 100) / total);
	}

	/**
	 * A destination (directory or file) on the remote server given as a
	 * path relative to a PIDL.
	 *
	 * As in an FGD, the path may be multi-level.  The directories named by the
	 * intermediate sections may not exist so care must be taken that the,
	 * destinations are used in the order listed in the FGD which is designed
	 * to make sure they exist.
	 */
	class destination
	{
	public:
		destination(const apidl_t& remote_root, const wpath& relative_path)
			: m_remote_root(remote_root), m_relative_path(relative_path)
		{
			if (relative_path.has_root_directory())
				BOOST_THROW_EXCEPTION(
					logic_error("Path must be relative to root"));
		}

		resolved_destination resolve_destination() const
		{
			apidl_t directory = m_remote_root;

			BOOST_FOREACH(
				wstring intermediate_directory_name,
				m_relative_path.parent_path())
			{
				directory += create_remote_itemid(
					intermediate_directory_name, true, false, L"", L"", 0, 0, 0, 0,
					datetime_t::now(), datetime_t::now());
			}

			return resolved_destination(directory, m_relative_path.filename());
		}

	private:
		apidl_t m_remote_root;
		wpath m_relative_path;
	};

	/**
	 * Functor handles 'intra-file' progress.
	 * 
	 * In other words, it handles the small increments that happen during the
	 * upload of one file amongst many.  We need this to give meaningful
	 * progress when only a small number of files are being dropped.
	 *
	 * @bug  Vulnerable to integer overflow with a very large number of files.
	 */
	class ProgressMicroUpdater : public std::unary_function<int, void>
	{
	public:
		ProgressMicroUpdater(
			Progress& auto_progress, size_t current_file_index,
			size_t total_files)
			: m_auto_progress(auto_progress),
			  m_current_file_index(current_file_index),
			  m_total_files(total_files)
		{}

		void operator()(int percent_done)
		{
			unsigned long long pos =
				(m_current_file_index * 100) + percent_done;
			m_auto_progress.update(pos, m_total_files * 100);
		}

	private:
		Progress& m_auto_progress;
		size_t m_current_file_index;
		size_t m_total_files;
	};

	class PidlCopyList
	{
	public:
		const Operation& operator[](unsigned int i) const
		{
			return m_copy_list.at(i);
		}

		size_t size() const
		{
			return m_copy_list.size();
		}

		void add(const Operation& entry)
		{
			m_copy_list.push_back(entry.clone());
		}

		void do_copy(
			const apidl_t& remote_destination_root, Progress& progress,
			com_ptr<ISftpProvider> provider, com_ptr<ISftpConsumer> consumer,
			CopyCallback& callback)
			const
		{
			for (unsigned int i = 0; i < size(); ++i)
			{
				if (progress.user_cancelled())
					BOOST_THROW_EXCEPTION(com_error(E_ABORT));

				const Operation& source = (*this)[i];

				destination target(remote_destination_root, source.relative_path());
				resolved_destination resolved_target(target.resolve_destination());

				assert(
					source.relative_path().filename() == 
					resolved_target.as_absolute_path().filename());

				progress.line_path(1, source.relative_path());
				progress.line_path(2, resolved_target.as_absolute_path());
 
				ProgressMicroUpdater micro_updater(progress, i, size());

				source(
					resolved_target, bind(micro_updater, bind(percentage, _1, _2)),
					provider, consumer, callback);

				// We update here as well, fixing the progress to a file boundary,
				// as we don't completely trust the intra-file progress.  A stream
				// could have lied about its size messing up the count.  This
				// will override any such errors.

				progress.update(i, size());
			}
		}

	private:
		ptr_vector<Operation> m_copy_list;
	};


	void build_copy_list_recursively(
		const apidl_t& parent, const pidl_t& folder_pidl,
		PidlCopyList& copy_list_out)
	{
		wpath folder_path = parsing_path_from_pidl(parent, folder_pidl);

		copy_list_out.add(
			CreateDirectoryOperation(parent, folder_pidl, folder_path));

		com_ptr<IShellFolder> folder = 
			bind_to_handler_object<IShellFolder>(parent + folder_pidl);

		// Add non-folder contents

		com_ptr<IEnumIDList> e;
		HRESULT hr = folder->EnumObjects(
			NULL, SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN, e.out());
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(com_error_from_interface(folder, hr));

		cpidl_t item;
		while (hr == S_OK && e->Next(1, item.out(), NULL) == S_OK)
		{
			pidl_t pidl = folder_pidl + item;
			copy_list_out.add(
				CopyFileOperation(
					parent, pidl, parsing_path_from_pidl(parent, pidl)));
		}

		// Recursively add folders

		hr = folder->EnumObjects(
			NULL, SHCONTF_FOLDERS | SHCONTF_INCLUDEHIDDEN, e.out());
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(com_error_from_interface(folder, hr));

		while (hr == S_OK && e->Next(1, item.out(), NULL) == S_OK)
		{
			pidl_t pidl = folder_pidl + item;
			build_copy_list_recursively(parent, pidl, copy_list_out);
		}
	}

	/**
	 * Expand the top-level PIDLs into a list of all items in the hierarchy.
	 */
	PidlCopyList build_copy_list(PidlFormat format)
	{
		PidlCopyList copy_list;

		for (unsigned int i = 0; i < format.pidl_count(); ++i)
		{
			pidl_t pidl = format.relative_file(i);
			try
			{
				// Test if streamable
				com_ptr<IStream> stream;
				stream = stream_from_pidl(format.file(i));
				
				CopyFileOperation entry(
					format.parent_folder(), pidl, filename_from_stream(stream));
				copy_list.add(entry);
			}
			catch (const com_error&)
			{
				// Treating the item as something with an IStream has failed
				// Now we try to treat it as an IShellFolder and hope we
				// have more success

				build_copy_list_recursively(
					format.parent_folder(), pidl, copy_list);
			}
		}

		return copy_list;
	}
}

/**
 * Copy the items in the DataObject to the remote target.
 *
 * @param format  IDataObject wrapper holding the items to be copied.
 * @param provider  SFTP connection to copy data over.
 * @param remote_root  PIDL to target directory in the remote filesystem
 *                     to copy items into.
 * @param progress  Optional progress dialogue.
 */
void copy_format_to_provider(
	PidlFormat local_source_format, com_ptr<ISftpProvider> provider,
	com_ptr<ISftpConsumer> consumer, const apidl_t& remote_destination_root,
	CopyCallback& callback)
{
	PidlCopyList copy_list = build_copy_list(local_source_format);

	std::auto_ptr<Progress> progress(callback.progress());

	copy_list.do_copy(
		remote_destination_root, *progress, provider, consumer, callback);
}

/**
 * Copy the items in the DataObject to the remote target.
 *
 * @param pdo  IDataObject holding the items to be copied.
 * @param pProvider  SFTP connection to copy data over.
 * @param remote_directory  PIDL to target directory in the remote filesystem
 *                          to copy items into.
 */
void copy_data_to_provider(
	com_ptr<IDataObject> data_object, com_ptr<ISftpProvider> provider, 
	com_ptr<ISftpConsumer> consumer, const apidl_t& remote_directory,
	CopyCallback& callback)
{
	ShellDataObject data(data_object.get());
	if (data.has_pidl_format())
	{
		copy_format_to_provider(
			PidlFormat(data_object), provider, consumer, remote_directory,
			callback);
	}
	else
	{
		BOOST_THROW_EXCEPTION(
			com_error("DataObject doesn't contain a supported format"));
	}
}

/**
 * Create an instance of the DropTarget initialised with a data provider.
 */
CDropTarget::CDropTarget(
	com_ptr<ISftpProvider> provider,
	com_ptr<ISftpConsumer> consumer, const apidl_t& remote_directory,
	shared_ptr<CopyCallback> callback)
	:
	m_provider(provider), m_consumer(consumer),
	m_remote_directory(remote_directory), m_callback(callback) {}

void CDropTarget::on_set_site(com_ptr<IUnknown> ole_site)
{
	m_callback->site(ole_site);
}

/**
 * Indicate whether the contents of the DataObject can be dropped on
 * this DropTarget.
 *
 * @todo  Take account of the key state.
 */
STDMETHODIMP CDropTarget::DragEnter( 
	IDataObject* pdo, DWORD /*grfKeyState*/, POINTL /*pt*/, DWORD* pdwEffect)
{
	try
	{
		if (!pdwEffect)
			BOOST_THROW_EXCEPTION(com_error(E_POINTER));

		m_data_object = pdo;

		*pdwEffect = determine_drop_effect(pdo, *pdwEffect);
	}
	WINAPI_COM_CATCH_AUTO_INTERFACE();

	return S_OK;
}

/**
 * Refresh the choice drop effect for the last DataObject passed to DragEnter.
 * Although the DataObject will not have changed, the key state and allowed
 * effects bitfield may have.
 *
 * @todo  Take account of the key state.
 */
STDMETHODIMP CDropTarget::DragOver( 
	DWORD /*grfKeyState*/, POINTL /*pt*/, DWORD* pdwEffect)
{
	try
	{
		if (!pdwEffect)
			BOOST_THROW_EXCEPTION(com_error(E_POINTER));

		*pdwEffect = determine_drop_effect(m_data_object, *pdwEffect);
	}
	WINAPI_COM_CATCH_AUTO_INTERFACE();

	return S_OK;
}

/**
 * End the drag-and-drop loop for the current DataObject.
 */
STDMETHODIMP CDropTarget::DragLeave()
{
	try
	{
		m_data_object = NULL;
	}
	WINAPI_COM_CATCH_AUTO_INTERFACE();

	return S_OK;
}

/**
 * Perform the drop operation by either copying or moving the data
 * in the DataObject to the remote target.
 *
 * @todo  Take account of the key state.
 */
STDMETHODIMP CDropTarget::Drop( 
	IDataObject* pdo, DWORD /*grfKeyState*/, POINTL /*pt*/, DWORD* pdwEffect)
{
	try
	{
		if (!pdwEffect)
			BOOST_THROW_EXCEPTION(com_error(E_POINTER));

		// Drop doesn't need to maintain any state and is handed a fresh copy
		// of the IDataObject so we can can immediately cancel the one we were
		// using for the other parts of the drag-drop loop
		m_data_object = NULL;

		*pdwEffect = determine_drop_effect(pdo, *pdwEffect);

		if (pdo && *pdwEffect == DROPEFFECT_COPY)
			copy_data_to_provider(
				pdo, m_provider, m_consumer, m_remote_directory, *m_callback);
	}
	WINAPI_COM_CATCH_AUTO_INTERFACE();

	return S_OK;
}

}} // namespace swish::drop_target
