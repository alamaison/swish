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
#include "swish/catch_com.hpp"  // catchCom
#include "swish/exception.hpp"  // com_exception

#include <boost/shared_ptr.hpp>  // shared_ptr
#include <boost/integer_traits.hpp>

#include <string>

using swish::shell_folder::data_object::ShellDataObject;
using swish::exception::com_exception;

using ATL::CComPtr;
using ATL::CComBSTR;

using boost::filesystem::wpath;
using boost::shared_ptr;
using boost::integer_traits;

using std::string;

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
			ShellDataObject data_object(pdo);
			if (data_object.pidl_count() > 0)
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
	CComPtr<IStream> stream_from_shell_pidl(const CAbsolutePidl& pidl)
	{
		PCUITEMID_CHILD pidl_child;
		HRESULT hr;
		CComPtr<IShellFolder> spFolder;
		
		hr = ::SHBindToParent(
			pidl, __uuidof(IShellFolder), reinterpret_cast<void**>(&spFolder),
			&pidl_child);
		if (FAILED(hr))
			throw com_exception(hr);

		CComPtr<IStream> spStream;
		hr = spFolder->BindToStorage(pidl_child, NULL, __uuidof(IStream),
			reinterpret_cast<void**>(&spStream));
		if (FAILED(hr))
			throw com_exception(hr);

		return spStream;
	}

	/**
	 * Return the stream name from an IStream.
	 */
	wpath filename_from_stream(IStream* pstream)
	{
		STATSTG statstg;
		HRESULT hr = pstream->Stat(&statstg, STATFLAG_DEFAULT);
		if (FAILED(hr))
			throw com_exception(hr);

		shared_ptr<OLECHAR> name(statstg.pwcsName, ::CoTaskMemFree);
		return name.get();
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
		IDataObject* pdo, ISftpProvider* pProvider, wpath remote_path)
	{
		HRESULT hr;

		ShellDataObject data_object(pdo);
		for (unsigned int i = 0; i < data_object.pidl_count(); ++i)
		{
			CAbsolutePidl pidl = data_object.GetFile(i);
			
			CComPtr<IStream> spStream = stream_from_shell_pidl(pidl);

			wpath destination = remote_path / filename_from_stream(spStream);

			CComBSTR bstrPath = destination.string().c_str();

			CComPtr<IStream> spRemoteStream;
			hr = pProvider->GetFile(bstrPath, &spRemoteStream);
			if (FAILED(hr))
				throw com_exception(hr);

			LARGE_INTEGER move = {0};
			hr = spStream->Seek(move, SEEK_SET, NULL);
			if (FAILED(hr))
				throw com_exception(hr);

			hr = spRemoteStream->Seek(move, SEEK_SET, NULL);
			if (FAILED(hr))
				throw com_exception(hr);

			ULARGE_INTEGER cbRead = {0};
			ULARGE_INTEGER cbWritten = {0};
			ULARGE_INTEGER cb;
			cb.QuadPart = integer_traits<ULONGLONG>::const_max;
			// TODO: make our own CopyTo that propagates errors
			hr = spStream->CopyTo(
				spRemoteStream, cb,	&cbRead, &cbWritten);
			assert(FAILED(hr) || cbRead.QuadPart == cbWritten.QuadPart);
			if (FAILED(hr))
				throw com_exception(hr);
		}
	}
}

namespace swish {
namespace shell_folder {

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
