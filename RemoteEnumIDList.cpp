/*  Expose contents of remote folder as PIDLs via IEnumIDList interface

    Copyright (C) 2007  Alexander Lamaison <awl03@doc.ic.ac.uk>

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
*/

#include "stdafx.h"
#include "RemoteEnumIDList.h"
#include "RemotePidlManager.h"

/*------------------------------------------------------------------------------
 * CRemoteEnumIDList::BindToFolder
 * Save back-reference to folder and increment its reference count to ensure
 * that folder remains alive for at least as long as the enumerator.
 *
 * MUST BE CALLED BEFORE ALL OTHER FUNCTIONS
 *----------------------------------------------------------------------------*/
HRESULT CRemoteEnumIDList::BindToFolder( CRemoteFolder* pFolder )
{
	ATLTRACE("CRemoteEnumIDList::BindToFolder called\n");

	if (pFolder == NULL)
		return E_POINTER;

	// Save back-reference to folder and increment its reference count to
	// ensure that folder remains alive for at least as long as the enumerator
	m_pFolder = pFolder;
	m_pFolder->AddRef();

	return S_OK;
}

/*------------------------------------------------------------------------------
 * CRemoteEnumIDList::Connect
 * Populates the enumerator by connecting to the remote server given in the
 * parameter and fetching the file listing (TODO: currently only simulated).
 *
 * TODO: Ideally, the final EnumIDList will deal with enumeration *only*.
 *       Connection and retrieval will be handled by other objects.
 *----------------------------------------------------------------------------*/
HRESULT CRemoteEnumIDList::Connect( PCWSTR szUser, PCWSTR szHost, 
								    PCWSTR szPath, UINT uPort )
{
	ATLTRACE("CRemoteEnumIDList::BindToFolder called\n");
	ATLASSERT( m_pFolder );

	if (m_pFolder == NULL)
		return E_FAIL;

	// Connect to server
	// Server->Connect()

	// Retrieve file listing
	FILEDATA fdTemp;
	fdTemp.strPath = _T("bob.jpg");
	fdTemp.dtModified = 0x3DE43B0C; // November 26, 2002 at 7:25p PST
	fdTemp.dwPermissions = 0x777;
	fdTemp.fIsFolder = false;
	fdTemp.strOwner = _T("awl03");
	fdTemp.strGroup = _T("awl03");
	fdTemp.strAuthor = _T("awl03");
	fdTemp.uSize = 1481;
	m_vListing.push_back(fdTemp);

	return S_OK;
}

/*------------------------------------------------------------------------------
 * CRemoteEnumIDList::Next
 * Retrieves the specified number of item identifiers in the enumeration
 * sequence and advances the current position by the number of items retrieved.
 *----------------------------------------------------------------------------*/
STDMETHODIMP CRemoteEnumIDList::Next(
	ULONG celt,
	__out_ecount_part(celt, *pceltFetched) PITEMID_CHILD *rgelt,
	__out_opt ULONG *pceltFetched )
{
	ATLTRACE("CRemoteEnumIDList::Next called\n");
	ATLASSERT( m_pFolder );
	if (m_pFolder == NULL)
		return E_FAIL;
	if (!(pceltFetched || celt <= 1))
		return E_INVALIDARG;

	HRESULT hr = S_OK;
	ULONG cFetched = 0;
	for (ULONG i = 0; i < celt; i++)
	{
		ULONG index = m_iPos + i; // Existing offset + current retrieval
		if (index >= m_vListing.size())
		{
			// Ran out of entries before requested number could be fetched
			hr = S_FALSE;
			break;
		}

		// Fetch data and create new PIDL from it
		ATLASSERT( index < m_vListing.size() );
		hr = m_PidlManager.Create(
			m_vListing[index].strPath,
			m_vListing[index].strOwner,
			m_vListing[index].strGroup,
			m_vListing[index].dwPermissions,
			m_vListing[index].uSize,
			m_vListing[index].dtModified,
			m_vListing[index].fIsFolder,
			&rgelt[i]
		);
		if (FAILED(hr))
			break;
		cFetched++;
	}

	*pceltFetched = cFetched;
	m_iPos += cFetched;
	return hr;
}

/*------------------------------------------------------------------------------
 * CRemoteEnumIDList::Skip
 * Skips the specified number of elements in the enumeration sequence.
 *----------------------------------------------------------------------------*/
STDMETHODIMP CRemoteEnumIDList::Skip( DWORD celt )
{
	ATLTRACE("CRemoteEnumIDList::Skip called\n");
	ATLASSERT( m_pFolder );

	m_iPos += celt;

	return S_OK;
}

/*------------------------------------------------------------------------------
 * CRemoteEnumIDList::Reset
 * Returns to the beginning of the enumeration sequence.
 *----------------------------------------------------------------------------*/
STDMETHODIMP CRemoteEnumIDList::Reset()
{
	ATLTRACE("CRemoteEnumIDList::Reset called\n");
	ATLASSERT( m_pFolder );

	m_iPos = 0;

	return S_OK;
}

/*------------------------------------------------------------------------------
 * CRemoteEnumIDList::Clone
 * Creates a new item enumeration object with the same contents and state 
 * as the current one.
 *----------------------------------------------------------------------------*/
STDMETHODIMP CRemoteEnumIDList::Clone( __deref_out IEnumIDList **ppenum )
{
	ATLTRACE("CRemoteEnumIDList::Clone called\n");
	ATLASSERT( m_pFolder );

	// TODO: Implement this

	return E_NOTIMPL;
}

// CRemoteEnumIDList

