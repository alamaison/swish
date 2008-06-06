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
#include <ATLComTime.h> // For COleDateTime wrapper class

/*------------------------------------------------------------------------------
 * CRemoteEnumIDList::BindToFolder
 * Save back-reference to folder and increment its reference count to ensure
 * that folder remains alive for at least as long as the enumerator.
 *
 * MUST BE CALLED BEFORE ALL OTHER FUNCTIONS
 *----------------------------------------------------------------------------*/
HRESULT CRemoteEnumIDList::BindToFolder( IRemoteFolder* pFolder )
{
	ATLTRACE("CRemoteEnumIDList::BindToFolder called\n");

	if (m_fBoundToFolder) // Already called this function
		return E_UNEXPECTED;
	if (pFolder == NULL)
		return E_POINTER;

	// Save back-reference to folder and increment its reference count to
	// ensure that folder remains alive for at least as long as the enumerator
	m_pFolder = pFolder;
	m_pFolder->AddRef();

	m_fBoundToFolder = true;
	return S_OK;
}

/*------------------------------------------------------------------------------
 * CRemoteEnumIDList::ConnectAndFetch
 * Populates the enumerator by connecting to the remote server given in the
 * parameter and fetching the file listing
 *
 * TODO: Ideally, the final EnumIDList will deal with enumeration *only*.
 *       Connection and retrieval will be handled by other objects.
 *----------------------------------------------------------------------------*/
HRESULT CRemoteEnumIDList::ConnectAndFetch( PCTSTR szUser, PCTSTR szHost, 
								            PCTSTR szPath, USHORT uPort )
{
	ATLTRACE("CRemoteEnumIDList::ConnectAndFetch called\n");

	// Must call BindToFolder() first
	if (!m_fBoundToFolder) return E_UNEXPECTED;
	if (!szUser || !szUser[0]) return E_INVALIDARG;
	if (!szHost || !szHost[0]) return E_INVALIDARG;
	if (!szPath || !szPath[0]) return E_INVALIDARG;
	ATLENSURE( m_pFolder );

	// Create SFTP Provider
	ISftpProvider *pProvider;
	HRESULT hr = ::CoCreateInstance(
		CLSID_CPuttyProvider,     // CLASSID for CPuttyProvider.
		NULL,                     // Ignore this.
		CLSCTX_INPROC_SERVER,     // Server.
		IID_ISftpProvider,       // Interface you want.
		(LPVOID *)&pProvider);    // Place to store interface.
	ATLENSURE_RETURN_HR( SUCCEEDED(hr), hr );

	// Get SftpConsumer to pass to PuttyProvider (used for password reqs etc.)
	ISftpConsumer *pConsumer;
	hr = this->QueryInterface(__uuidof(pConsumer), (void**)&pConsumer);
	ATLENSURE_RETURN_HR( SUCCEEDED(hr), hr );

	// Set up Putty provider
	CComBSTR bstrUser(szUser);
	CComBSTR bstrHost(szHost);
	hr = pProvider->Initialize(pConsumer, bstrUser, bstrHost, uPort);
	ATLENSURE_RETURN_HR( SUCCEEDED(hr), hr );
	pConsumer->Release();
	pConsumer = NULL;

	// Get listing enumerator
	IEnumListing *pEnum;
	CComBSTR bstrPath = szPath;
	hr = pProvider->GetListing(bstrPath, &pEnum);
	if (SUCCEEDED(hr))
	{
		do {
			Listing lt;
			hr = pEnum->Next(1, &lt, NULL);
			if (hr == S_OK)
			{
				FILEDATA fd;
				fd.strPath = lt.bstrFilename;
				fd.strOwner = lt.bstrOwner;
				fd.strGroup = lt.bstrGroup;
				fd.uSize = lt.cSize;
				fd.dtModified = (time_t) COleDateTime(lt.dateModified);
				// TODO:
				fd.dwPermissions = 0x777;
				m_vListing.push_back( fd );
			}
		} while (hr == S_OK);
		pEnum->Release();
	}

	// Release data provider component (should destroy psftp process)
	pProvider->Release();

	return hr;
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
	
	// Must call BindToFolder() first
	if (!m_fBoundToFolder) return E_UNEXPECTED;
	if (!(pceltFetched || celt <= 1)) return E_INVALIDARG;
	ATLENSURE( m_pFolder );

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

	// Must call BindToFolder() first
	if (!m_fBoundToFolder) return E_UNEXPECTED;
	ATLENSURE( m_pFolder );

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

	// Must call BindToFolder() first
	if (!m_fBoundToFolder) return E_UNEXPECTED;
	ATLENSURE( m_pFolder );

	m_iPos = 0;

	return S_OK;
}

/*------------------------------------------------------------------------------
 * CRemoteEnumIDList::Clone
 * Creates a new item enumeration object with the same contents and state 
 * as the current one.
 *----------------------------------------------------------------------------*/
STDMETHODIMP CRemoteEnumIDList::Clone( __deref_out IEnumIDList **ppEnum )
{
	ATLTRACE("CRemoteEnumIDList::Clone called\n");
	(void)ppEnum;

	// Must call BindToFolder() first
	if (!m_fBoundToFolder) return E_UNEXPECTED;
	ATLENSURE( m_pFolder );

	// TODO: Implement this

	return E_NOTIMPL;
}

/**
 * Displays UI dialog to get password from user and returns it.
 *
 * @param [in]  bstrRequest    The prompt to display to the user.
 * @param [out] pbstrPassword  The reply from the user - the password.
 *
 * @return E_ABORT if the user chooses Cancel, S_OK otherwise.
 */
STDMETHODIMP CRemoteEnumIDList::OnPasswordRequest(
	BSTR bstrRequest, BSTR *pbstrPassword
)
{
	CString strPrompt = bstrRequest;
	ATLASSERT(strPrompt.GetLength() > 0);

	CPasswordDialog dlgPassword;
	dlgPassword.SetPrompt( strPrompt ); // Pass text through from backend
	if (dlgPassword.DoModal() == IDOK)
	{
		CString strPassword;
		strPassword = dlgPassword.GetPassword();
		*pbstrPassword = strPassword.AllocSysString();
		return S_OK;
	}
	else
		return E_ABORT;
}

/**
 * Display Yes/No/Cancel dialog to the user with given message.
 *
 * @param [in]  bstrMessage    The prompt to display to the user.
 * @param [in]  bstrYesInfo    The explanation of the Yes option.
 * @param [in]  bstrNoInfo     The explanation of the No option.
 * @param [in]  bstrCancelInfo The explanation of the Cancel option.
 * @param [in]  bstrTitle      The title of the dialog.
 * @param [out] piResult       The user's choice.
 *
 * @return E_ABORT if the user chooses Cancel, S_OK otherwise.
*/
STDMETHODIMP CRemoteEnumIDList::OnYesNoCancel(
	BSTR bstrMessage, BSTR bstrYesInfo, BSTR bstrNoInfo, BSTR bstrCancelInfo,
	BSTR bstrTitle, int *piResult
)
{
	// Construct unknown key information message
	CString strMessage = bstrMessage;
	CString strTitle = bstrTitle;
	if (bstrYesInfo && ::SysStringLen(bstrYesInfo) > 0)
	{
		strMessage += _T("\r\n");
		strMessage += bstrYesInfo;
	}
	if (bstrNoInfo && ::SysStringLen(bstrNoInfo) > 0)
	{
		strMessage += _T("\r\n");
		strMessage += bstrNoInfo;
	}
	if (bstrCancelInfo && ::SysStringLen(bstrCancelInfo) > 0)
	{
		strMessage += _T("\r\n");
		strMessage += bstrCancelInfo;
	}

	// Display message box
	int msgboxID = ::MessageBox(
		NULL, strMessage, strTitle, 
		MB_ICONWARNING | MB_YESNOCANCEL | MB_DEFBUTTON3 );

	// Process user choice
	switch (msgboxID)
	{
	case IDYES:
		*piResult = 1; return S_OK;
	case IDNO:
		*piResult = 0; return S_OK;
	case IDCANCEL:
		*piResult = -1; return E_ABORT;
	default:
		*piResult = -2;
		UNREACHABLE;
	}

	return E_ABORT;
}

// CRemoteEnumIDList

