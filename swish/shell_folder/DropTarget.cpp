/**
    @file

    Expose the remote filesystem as an IDropTarget.

    @if licence

    Copyright (C) 2009  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "data_object/ShellDataObject.hpp"  // ShellDataObject
#include "swish/shell_folder/shell.hpp"  // bind_to_handler_object
#include "swish/catch_com.hpp"  // catchCom
#include "swish/exception.hpp"  // com_exception

#include <boost/shared_ptr.hpp>  // shared_ptr
#include <boost/integer_traits.hpp>
#include <boost/throw_exception.hpp>  // BOOST_THROW_EXCEPTION

#include <comet/interface.h>  // uuidof, comtype
#include <comet/ptr.h>  // com_ptr
#include <comet/bstr.h> // bstr_t

#include <string>
#include <vector>

using swish::shell_folder::data_object::ShellDataObject;
using swish::shell_folder::data_object::PidlFormat;
using swish::shell_folder::bind_to_handler_object;
using swish::exception::com_exception;

using ATL::CComPtr;
using ATL::CComBSTR;

using boost::filesystem::wpath;
using boost::shared_ptr;
using boost::integer_traits;

using comet::com_ptr;
using comet::uuidof;
using comet::bstr_t;

using std::wstring;
using std::vector;

namespace { // private

	/**
	 * Given a DataObject and bitfield of allowed DROPEFFECTs, determine
	 * which drop effect, if any, should be chosen.  If none are
	 * appropriate, return DROPEFFECT_NONE.
	 */
	DWORD determine_drop_effect(IDataObject* pdo, DWORD allowed_effects)
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
	 */
	com_ptr<IStream> stream_from_shell_pidl(const CAbsolutePidl& pidl)
	{
		PCUITEMID_CHILD pidl_child;
		HRESULT hr;
		com_ptr<IShellFolder> folder;
		
		hr = ::SHBindToParent(
			pidl, __uuidof(IShellFolder), 
			reinterpret_cast<void**>(folder.out()),
			&pidl_child);
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(com_exception(hr));

		com_ptr<IStream> stream;
		hr = folder->BindToStorage(pidl_child, NULL, __uuidof(IStream),
			reinterpret_cast<void**>(stream.out()));
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(com_exception(hr));

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
			BOOST_THROW_EXCEPTION(com_exception(hr));

		shared_ptr<OLECHAR> name(statstg.pwcsName, ::CoTaskMemFree);
		return name.get();
	}

	/**
	 * Convert a STRRET structure to a string.
	 *
	 * If the STRRET is using its pOleStr member to store the data (rather
	 * than holding it directly or extracting it from the PIDL offset)
	 * the data will be freed.  In other words, this function destroys
	 * the STRRET passed to it.
	 */
	wstring strret_to_string(STRRET& strret, const CChildPidl& pidl)
	{
		vector<wchar_t> buffer(MAX_PATH);
		HRESULT hr = ::StrRetToBufW(
			&strret, pidl, &buffer[0], buffer.size());
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(com_exception(hr));

		buffer[buffer.size() - 1] = L'\0';

		return wstring(&buffer[0]);
	}

	/**
	 * Query an item's parent folder for the item's display name relative
	 * to that folder.
	 */
	wstring display_name_of_item(
		const com_ptr<IShellFolder>& parent_folder,
		const CChildPidl& pidl)
	{
		STRRET strret;
		HRESULT hr = parent_folder->GetDisplayNameOf(
			pidl, SHGDN_INFOLDER | SHGDN_FORPARSING, &strret);
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(com_exception(hr));

		return strret_to_string(strret, pidl);
	}

	/**
	 * Return the folder name from a folder PIDL.
	 */
	wpath folder_name_from_pidl(
		const CChildPidl& item, const CAbsolutePidl parent)
	{
		com_ptr<IShellFolder> parent_folder = 
			bind_to_handler_object<IShellFolder>(parent);

		return display_name_of_item(parent_folder, item);
	}

	void copy_stream_to_remote_destination(
		const com_ptr<IStream>& local_stream, 
		const com_ptr<ISftpProvider>& provider, wpath destination)
	{
		CComBSTR bstrPath = destination.string().c_str();

		com_ptr<IStream> remote_stream;
		HRESULT hr = provider->GetFile(bstrPath, remote_stream.out());
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(com_exception(hr));

		LARGE_INTEGER move = {0};
		hr = local_stream->Seek(move, SEEK_SET, NULL);
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(com_exception(hr));

		hr = remote_stream->Seek(move, SEEK_SET, NULL);
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(com_exception(hr));

		ULARGE_INTEGER cbRead = {0};
		ULARGE_INTEGER cbWritten = {0};
		ULARGE_INTEGER cb;
		cb.QuadPart = integer_traits<ULONGLONG>::const_max;
		// TODO: make our own CopyTo that propagates errors
		hr = local_stream->CopyTo(
			remote_stream.get(), cb, &cbRead, &cbWritten);
		assert(FAILED(hr) || cbRead.QuadPart == cbWritten.QuadPart);
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(com_exception(hr));
	}

	void copy_folder_to_remote_destination(
		const CAbsolutePidl& folder_pidl,
		const com_ptr<ISftpProvider>& provider, wpath destination)
	{
		com_ptr<IShellFolder> folder = 
			bind_to_handler_object<IShellFolder>(folder_pidl);

		bstr_t path = destination.string();

		HRESULT hr = provider->CreateNewDirectory(path.get_raw());
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(com_exception(hr));

		// Copy non-folders

		com_ptr<IEnumIDList> e;
		hr = folder->EnumObjects(
			NULL, SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN, e.out());
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(com_exception(hr));

		CChildPidl item;
		while (e->Next(1, &item, NULL) == S_OK)
		{
			com_ptr<IStream> stream;
			hr = folder->BindToStorage(
				item, NULL, __uuidof(IStream), 
				reinterpret_cast<void**>(stream.out()));
			if (FAILED(hr))
				BOOST_THROW_EXCEPTION(com_exception(hr));

			copy_stream_to_remote_destination(
				stream, provider, 
				destination / filename_from_stream(stream));
		}

		// Copy folders recursively

		hr = folder->EnumObjects(
			NULL, SHCONTF_FOLDERS | SHCONTF_INCLUDEHIDDEN, e.out());
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(com_exception(hr));

		while (e->Next(1, &item, NULL) == S_OK)
		{
			wpath subfolder_name = folder_name_from_pidl(
				item, folder_pidl);

			copy_folder_to_remote_destination(
				CAbsolutePidl(folder_pidl, item), provider, 
				destination / subfolder_name);
		}

	}
}

namespace swish {
namespace shell_folder {

/**
 * Copy the items in the DataObject to the remote target.
 *
 * @param pdo  IDataObject holding the items to be copied.
 * @param pProvider  SFTP connection to copy data over.
 * @param remote_path  Path on the target filesystem to copy items into.
 *                     This must be a path to a @b directory.
 */
void copy_data_to_provider(
	IDataObject* pdo, ISftpProvider* pProvider, wpath remote_path)
{
	ShellDataObject data_object(pdo);
	if (data_object.has_pidl_format())
	{
		PidlFormat format(pdo);
		for (unsigned int i = 0; i < format.pidl_count(); ++i)
		{
			CRelativePidl pidl = format.relative_file(i);
			if (!::ILIsChild(pidl))
				BOOST_THROW_EXCEPTION(com_exception(E_FAIL));

			com_ptr<IStream> stream;
			try
			{
				stream = stream_from_shell_pidl(format.file(i));
			}
			catch (com_exception)
			{
				// Treating the item as something with an IStream has failed
				// Now we try to treat it as an IShellFolder and hope we
				// have more success

				wpath directory_name = folder_name_from_pidl(
					static_cast<PCITEMID_CHILD>(
						static_cast<PCUIDLIST_RELATIVE>(pidl)),
					format.parent_folder());

				copy_folder_to_remote_destination(
					format.file(i), pProvider, 
					remote_path / directory_name);

				continue;
			}

			copy_stream_to_remote_destination(
				stream, pProvider,
				remote_path / filename_from_stream(stream));
		}
	}
	else
	{
		BOOST_THROW_EXCEPTION(com_exception(E_FAIL));
	}
}

/**
 * Create an instance of the DropTarget initialised with a data provider.
 */
/*static*/ CComPtr<IDropTarget> CDropTarget::Create(
	ISftpProvider* pProvider, const wpath& remote_path)
{
	CComPtr<CDropTarget> sp = sp->CreateCoObject();
	sp->m_spProvider = pProvider;
	sp->m_remote_path = remote_path;
	return sp.p;
}

CDropTarget::CDropTarget()
{
}

CDropTarget::~CDropTarget()
{
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
	if (!pdwEffect)
		return E_INVALIDARG;

	try
	{
		m_spDataObject = pdo;

		*pdwEffect = determine_drop_effect(pdo, *pdwEffect);
	}
	catchCom()
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
	if (!pdwEffect)
		return E_INVALIDARG;

	try
	{
		*pdwEffect = determine_drop_effect(m_spDataObject, *pdwEffect);
	}
	catchCom()
	return S_OK;
}

/**
 * End the drag-and-drop loop for the current DataObject.
 */
STDMETHODIMP CDropTarget::DragLeave()
{
	try
	{
		m_spDataObject = NULL;
	}
	catchCom()
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
	if (!pdwEffect)
		return E_INVALIDARG;

	*pdwEffect = determine_drop_effect(pdo, *pdwEffect);
	m_spDataObject = pdo;

	try
	{
		if (pdo && *pdwEffect == DROPEFFECT_COPY)
			copy_data_to_provider(pdo, m_spProvider, m_remote_path);
	}
	catchCom()

	m_spDataObject = NULL;
	return S_OK;
}

}} // namespace swish::shell_folder
