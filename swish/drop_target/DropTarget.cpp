/**
    @file

    Expose the remote filesystem as an IDropTarget.

    @if licence

    Copyright (C) 2009, 2010  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "swish/interfaces/SftpProvider.h" // ISftpProvider/Consumer
#include "swish/shell_folder/data_object/ShellDataObject.hpp" // ShellDataObject
#include "swish/shell_folder/shell.hpp"  // bind_to_handler_object
#include "swish/windows_api.hpp" // SHBindToParent

#include <winapi/com/catch.hpp> // COM_CATCH_AUTO_INTERFACE
#include <winapi/shell/shell.hpp> // strret_to_string

#include <boost/bind.hpp> // bind
#include <boost/function.hpp> // function
#include <boost/shared_ptr.hpp>  // shared_ptr
#include <boost/throw_exception.hpp>  // BOOST_THROW_EXCEPTION

#include <comet/ptr.h>  // com_ptr
#include <comet/bstr.h> // bstr_t

#include <string>
#include <vector>

using swish::shell_folder::data_object::ShellDataObject;
using swish::shell_folder::data_object::PidlFormat;
using swish::shell_folder::bind_to_handler_object;

using winapi::shell::pidl::pidl_t;
using winapi::shell::pidl::apidl_t;
using winapi::shell::pidl::cpidl_t;
using winapi::shell::strret_to_string;

using boost::bind;
using boost::filesystem::wpath;
using boost::function;
using boost::shared_ptr;

using comet::bstr_t;
using comet::com_error;
using comet::com_ptr;

using std::wstring;
using std::vector;

template<> struct comet::comtype<ISftpProvider>
{
	static const IID& uuid() throw() { return IID_ISftpProvider; }
	typedef IUnknown base;
};


template<> struct comet::comtype<ISftpConsumer>
{
	static const IID& uuid() throw() { return IID_ISftpConsumer; }
	typedef IUnknown base;
};

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

	/**
	 * Given a PIDL to a *real* file in the filesystem, return an IStream 
	 * to it.
	 *
	 * @note  This fails with E_NOTIMPL on Windows 2000 and below.
	 */
	com_ptr<IStream> stream_from_shell_pidl(const apidl_t& pidl)
	{
		PCUITEMID_CHILD pidl_child;
		HRESULT hr;
		com_ptr<IShellFolder> folder;
		
		hr = swish::windows_api::SHBindToParent(
			pidl.get(), folder.iid(), reinterpret_cast<void**>(folder.out()),
			&pidl_child);
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(com_error(hr));

		com_ptr<IStream> stream;
		
		hr = folder->BindToObject(
			pidl_child, NULL, stream.iid(),
			reinterpret_cast<void**>(stream.out()));
		if (FAILED(hr))
		{
			hr = folder->BindToStorage(
				pidl_child, NULL, stream.iid(),
				reinterpret_cast<void**>(stream.out()));
			if (FAILED(hr))
				BOOST_THROW_EXCEPTION(com_error(hr));
		}

		return stream;
	}

	/**
	 * Return the stream name from an IStream.
	 */
	wpath filename_from_stream(const com_ptr<IStream>& stream)
	{
		STATSTG statstg;
		HRESULT hr = stream->Stat(&statstg, STATFLAG_DEFAULT);
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(com_error(hr));

		shared_ptr<OLECHAR> name(statstg.pwcsName, ::CoTaskMemFree);
		return name.get();
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
			BOOST_THROW_EXCEPTION(com_error(parent_folder, hr));

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

	const size_t COPY_CHUNK_SIZE = 1024 * 32;

	/**
	 * Write a stream to the provider at the given path.
	 *
	 * If it already exists, we want to ask the user for confirmation.
	 * The poor-mans way of checking if the file is already there is to
	 * try to get the file read-only first.  If this fails, assume the
	 * file noes not already exist.
	 *
	 * @bug  The get may have failed for a different reason or this
	 *       may not work reliably on all SFTP servers.  A safer
	 *       solution would be an explicit stat on the file.
	 *
	 * @bug  Of course, there is a race condition here.  After we check if the
	 *       file exists, someone else may have created it.  Unfortunately,
	 *       there is nothing we can do about this as SFTP doesn't give us
	 *       a way to do this atomically such as locking a file.
	 */
	void copy_stream_to_remote_destination(
		com_ptr<IStream> local_stream, com_ptr<ISftpProvider> provider,
		com_ptr<ISftpConsumer> consumer, const wpath& to_path,
		CopyCallback& callback, function<bool()> cancelled)
	{
		com_ptr<IStream> remote_stream;
		bstr_t target(to_path.string());

		HRESULT hr = provider->GetFile(
			consumer.get(), target.in(), false, remote_stream.out());
		if (SUCCEEDED(hr) && !callback.can_overwrite(to_path))
			return;

		hr = provider->GetFile(
			consumer.get(), target.in(), true, remote_stream.out());
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(com_error(provider, hr));

		// Set both streams back to the start
		LARGE_INTEGER move = {0};
		hr = local_stream->Seek(move, SEEK_SET, NULL);
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(com_error(local_stream, hr));

		hr = remote_stream->Seek(move, SEEK_SET, NULL);
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(com_error(remote_stream, hr));

		// Do the copy in chunks allowing us to cancel the operation
		ULARGE_INTEGER cb;
		cb.QuadPart = COPY_CHUNK_SIZE;
		while (!cancelled())
		{
			ULARGE_INTEGER cbRead = {0};
			ULARGE_INTEGER cbWritten = {0};
			// TODO: make our own CopyTo that propagates errors
			hr = local_stream->CopyTo(
				remote_stream.get(), cb, &cbRead, &cbWritten);
			assert(FAILED(hr) || cbRead.QuadPart == cbWritten.QuadPart);
			if (FAILED(hr))
				BOOST_THROW_EXCEPTION(com_error(local_stream, hr));
			if (cbRead.QuadPart == 0)
				break; // finished
		}
	}

	void create_remote_directory(
		const com_ptr<ISftpProvider>& provider, 
		const com_ptr<ISftpConsumer>& consumer, const wpath& remote_path)
	{
		bstr_t path = remote_path.string();

		HRESULT hr = provider->CreateNewDirectory(
			consumer.get(), path.get_raw());
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(com_error(provider, hr));
	}

	/**
	 * Storage structure for an item in the copy list built by 
	 * build_copy_list().
	 */
	struct CopylistEntry
	{
		CopylistEntry(
			const pidl_t& pidl, wpath relative_path, bool is_folder)
		{
			this->pidl = pidl;
			this->relative_path = relative_path;
			this->is_folder = is_folder;
		}

		pidl_t pidl;
		wpath relative_path;
		bool is_folder;
	};

	void build_copy_list_recursively(
		const apidl_t& parent, const pidl_t& folder_pidl,
		vector<CopylistEntry>& copy_list_out)
	{
		wpath folder_path = parsing_path_from_pidl(parent, folder_pidl);

		copy_list_out.push_back(
			CopylistEntry(folder_pidl, folder_path, true));

		com_ptr<IShellFolder> folder = 
			bind_to_handler_object<IShellFolder>(parent + folder_pidl);

		// Add non-folder contents

		com_ptr<IEnumIDList> e;
		HRESULT hr = folder->EnumObjects(
			NULL, SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN, e.out());
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(com_error(folder, hr));

		cpidl_t item;
		while (hr == S_OK && e->Next(1, item.out(), NULL) == S_OK)
		{
			pidl_t pidl = folder_pidl + item;
			copy_list_out.push_back(
				CopylistEntry(
					pidl, parsing_path_from_pidl(parent, pidl), false));
		}

		// Recursively add folders

		hr = folder->EnumObjects(
			NULL, SHCONTF_FOLDERS | SHCONTF_INCLUDEHIDDEN, e.out());
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(com_error(folder, hr));

		while (hr == S_OK && e->Next(1, item.out(), NULL) == S_OK)
		{
			pidl_t pidl = folder_pidl + item;
			build_copy_list_recursively(parent, pidl, copy_list_out);
		}
	}

	/**
	 * Expand the top-level PIDLs into a list of all items in the hierarchy.
	 */
	void build_copy_list(PidlFormat format, vector<CopylistEntry>& copy_list)
	{
		for (unsigned int i = 0; i < format.pidl_count(); ++i)
		{
			pidl_t pidl = format.relative_file(i);
			try
			{
				// Test if streamable
				com_ptr<IStream> stream;
				stream = stream_from_shell_pidl(format.file(i));
				
				CopylistEntry entry(
					pidl, filename_from_stream(stream), false);
				copy_list.push_back(entry);
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
	}
}

/**
 * Copy the items in the DataObject to the remote target.
 *
 * @param format  IDataObject wrapper holding the items to be copied.
 * @param provider  SFTP connection to copy data over.
 * @param remote_path  Path on the target filesystem to copy items into.
 *                     This must be a path to a @b directory.
 * @param progress  Optional progress dialogue.
 */
void copy_format_to_provider(
	PidlFormat format, com_ptr<ISftpProvider> provider,
	com_ptr<ISftpConsumer> consumer, const wpath& remote_path,
	CopyCallback& callback)
{
	vector<CopylistEntry> copy_list;
	build_copy_list(format, copy_list);

	std::auto_ptr<Progress> auto_progress(callback.progress());

	for (unsigned int i = 0; i < copy_list.size(); ++i)
	{
		if (auto_progress->user_cancelled())
			BOOST_THROW_EXCEPTION(com_error(E_ABORT));

		wpath from_path = copy_list[i].relative_path;
		wpath to_path = remote_path / copy_list[i].relative_path;
		assert(from_path.filename() == to_path.filename());

		if (copy_list[i].is_folder)
		{
			auto_progress->line_path(1, from_path);
			auto_progress->line_path(2, to_path);

			create_remote_directory(provider, consumer, to_path);
			
			auto_progress->update(i, copy_list.size());
		}
		else
		{
			com_ptr<IStream> stream;

			stream = stream_from_shell_pidl(
				format.parent_folder() + copy_list[i].pidl);

			auto_progress->line_path(1, from_path);
			auto_progress->line_path(2, to_path);

			copy_stream_to_remote_destination(
				stream, provider, consumer, to_path, callback,
				bind(&Progress::user_cancelled, boost::ref(*auto_progress)));
			
			auto_progress->update(i, copy_list.size());
		}
	}
}

/**
 * Copy the items in the DataObject to the remote target.
 *
 * @param pdo  IDataObject holding the items to be copied.
 * @param pProvider  SFTP connection to copy data over.
 * @param remote_path  Path on the target filesystem to copy items into.
 *                     This must be a path to a @b directory.
 */
void copy_data_to_provider(
	com_ptr<IDataObject> data_object, com_ptr<ISftpProvider> provider, 
	com_ptr<ISftpConsumer> consumer, const wpath& remote_path,
	CopyCallback& callback)
{
	ShellDataObject data(data_object.get());
	if (data.has_pidl_format())
	{
		copy_format_to_provider(
			PidlFormat(data_object), provider, consumer, remote_path, callback);
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
	com_ptr<ISftpConsumer> consumer, const wpath& remote_path,
	shared_ptr<CopyCallback> callback)
	:
	m_provider(provider), m_consumer(consumer), m_remote_path(remote_path),
		m_callback(callback) {}

STDMETHODIMP CDropTarget::SetSite(IUnknown* pUnkSite)
{
	try
	{
		m_ole_site = pUnkSite;
		m_callback->site(m_ole_site);
	}
	COM_CATCH_AUTO_INTERFACE();

	return S_OK;
}

STDMETHODIMP CDropTarget::GetSite(REFIID riid, void** ppvSite)
{
	try
	{
		if (!ppvSite)
			BOOST_THROW_EXCEPTION(com_error(E_POINTER));
		*ppvSite = NULL;

		HRESULT hr = m_ole_site.get()->QueryInterface(riid, ppvSite);
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(com_error(m_ole_site, hr));
	}
	COM_CATCH_AUTO_INTERFACE();

	return S_OK;
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
	COM_CATCH_AUTO_INTERFACE();

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
	COM_CATCH_AUTO_INTERFACE();

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
	COM_CATCH_AUTO_INTERFACE();

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
				pdo, m_provider, m_consumer, m_remote_path, *m_callback);
	}
	COM_CATCH_AUTO_INTERFACE();

	return S_OK;
}

}} // namespace swish::drop_target
