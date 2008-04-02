/*  SFTP data provider using PuTTY (psftp.exe).

    Copyright (C) 2008  Alexander Lamaison <awl03@doc.ic.ac.uk>

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
#include "PuttyProvider.h"
#include "remotelimits.h"

#include <ATLComTime.h>

using std::list;

#define READ_STARTUP_MESSAGE _T("psftp: no hostname specified; use \"open host.name\" to connect\r\npsftp> ")
#define OPEN_COMMAND _T("open %s@%s %d\r\n")
#define OPEN_REPLY_HEAD _T("Remote working directory is /")
#define OPEN_REPLY_TAIL _T("\r\npsftp> ")
#define SPACE_DELIMITER _T(" ")

/**
 * Perform initial setup of PuTTY data provider.
 *
 * This function must be called before any other and is used to set the
 * user name, host and port with which to perform the SSH connection.  None
 * of these parameters is optional.
 *
 * @pre The port must be between 0 and 65535 (@ref MAX_PORT) inclusive.
 * @pre The user name (@a bstrUser) and the host name (@a bstrHost) must not
 *      be empty strings.
 *
 * @param bstrUser The user name of the SSH account.
 * @param bstrHost The name of the machine to connect to.
 * @param uPort    The TCP/IP port on which the SSH server is running.
 * @returns 
 *   @c E_INVALIDARG if either string parameter was empty, @c S_OK otherwise.
 */
STDMETHODIMP CPuttyProvider::Initialize(
	BSTR bstrUser, BSTR bstrHost, USHORT uPort )
{
	ASSERT( !m_fConstructException );
	if (::SysStringLen(bstrUser) == 0 || ::SysStringLen(bstrHost) == 0)
		return E_INVALIDARG;

	m_strUser = bstrUser;
	m_strHost = bstrHost;
	m_uPort = uPort;

	ASSERT( !m_strUser.IsEmpty() );
	ASSERT( !m_strHost.IsEmpty() );
	ASSERT( m_uPort <= MAX_PORT );

	m_fInitialized = true;
	return S_OK;
}

/**
* Retrieves a file listing, @c ls, of a given directory.
*
* The listing is returned as an IEnumListing of Listing objects.
*
* @pre The Initialize() method must have been called first
*
* @param [in]  bstrDirectory The absolute path of the directory to list
* @param [out] ppEnum        A pointer to the IEnumListing interface pointer
* @returns 
*   @c S_OK and IEnumListing pointer if successful, @c E_UNEXPECTED if
*   Initialize() was not previously called, @c E_FAIL if other error occurs.
*
* @todo Handle the case where the directory does not exist
* @todo Return a collection rather than an enumerator
* @todo Refactor connection code out of this method
*
* @see Listing for details of what file information is retrieved.
*/
STDMETHODIMP CPuttyProvider::GetListing(
	BSTR bstrDirectory, IEnumListing **ppEnum )
{
	*ppEnum = NULL;
	ASSERT( !m_fConstructException );
	ASSERT(m_fInitialized); // Check that Initialize() was called first
	if (!m_fInitialized)
		return E_UNEXPECTED;
	ASSERT(!m_strUser.IsEmpty());
	ASSERT(!m_strHost.IsEmpty());
	if (m_strUser.IsEmpty() || m_strHost.IsEmpty())
		return E_FAIL;

	typedef CComEnumOnSTL<IEnumListing, &__uuidof(IEnumListing), Listing, _Copy<Listing>, list<Listing> > CComEnumListing;
	CString strCommand;
	CString strActual;
	CString strExpected;

	// Connect
	try
	{
		// Should read: 
		// "psftp: no hostname specified; use open host.name to connect
		//  psftp> "
		strActual = m_Putty.Read();
		strExpected = READ_STARTUP_MESSAGE;
		VERIFY( strExpected == strActual );

		// Should read: 
		// "Remote working directory is /such-and-such-a-path
		//  psftp> "
		strCommand.Format(OPEN_COMMAND, m_strUser, m_strHost, m_uPort);
		m_Putty.Write( strCommand );
		strActual = m_Putty.Read();
		strExpected = OPEN_REPLY_HEAD;
		VERIFY( strExpected == strActual.Left(strExpected.GetLength()) );
		strExpected = OPEN_REPLY_TAIL;
		VERIFY( strExpected == strActual.Right(strExpected.GetLength()) );
	}
	catch (...)
	{
		// TODO: Make this more specific by exception type
		return E_FAIL;
	}

	COLE2T szDirectory(bstrDirectory);
	list<CString> lstLs = m_Putty.RunLS(szDirectory);
	for (list<CString>::iterator itLs=lstLs.begin(); itLs!=lstLs.end(); itLs++)
	{
		// Check format of listing is sensible
		CString strRow = *itLs;
		CString strPermissions, strHardLinks, strOwner, strGroup;
		CString strSize, strMonth, strDate, strTimeYear, strFilename;
		int iPos = 0;
		strPermissions = strRow.Tokenize( SPACE_DELIMITER, iPos );
		ATLASSERT( !strPermissions.IsEmpty() );
		strHardLinks = strRow.Tokenize( SPACE_DELIMITER, iPos );
		ATLASSERT( !strHardLinks.IsEmpty() );
		strOwner = strRow.Tokenize( SPACE_DELIMITER, iPos );
		ATLASSERT( !strOwner.IsEmpty() );
		strGroup = strRow.Tokenize( SPACE_DELIMITER, iPos );
		ATLASSERT( !strGroup.IsEmpty() );
		strSize = strRow.Tokenize( SPACE_DELIMITER, iPos );
		ATLASSERT( !strSize.IsEmpty() );
		strMonth = strRow.Tokenize( SPACE_DELIMITER, iPos );
		ATLASSERT( !strMonth.IsEmpty() );
		strDate = strRow.Tokenize( SPACE_DELIMITER, iPos );
		ATLASSERT( !strDate.IsEmpty() );
		strTimeYear = strRow.Tokenize( SPACE_DELIMITER, iPos );
		ATLASSERT( !strTimeYear.IsEmpty() );
		strFilename = strRow.Mid( iPos );
		ATLASSERT( !strFilename.IsEmpty() );

	
		Listing ltFile;
		::ZeroMemory( &ltFile, sizeof(ltFile) );
		ltFile.bstrFilename = strFilename.AllocSysString();
		ltFile.bstrPermissions = strPermissions.AllocSysString();
		ltFile.bstrOwner = strOwner.AllocSysString();
		ltFile.bstrGroup = strOwner.AllocSysString();
		ltFile.cHardLinks = _ttol(strHardLinks);
		ltFile.cSize = _ttol(strSize);
		ltFile.dateModified = _BuildDate(strMonth, strDate, strTimeYear);

		m_lstFiles.push_back(ltFile);
	}

	// Create instance of Enum from ATL template class and get interface pointer
	CComObject<CComEnumListing> *pEnum = NULL;
	HRESULT hr = CComObject<CComEnumListing>::CreateInstance(&pEnum);
	if (SUCCEEDED(hr))
	{
		pEnum->AddRef();
			hr = pEnum->Init(this->GetUnknown(), m_lstFiles);
			if (SUCCEEDED(hr))
				hr = pEnum->QueryInterface(ppEnum);
		pEnum->Release();
	}

	return hr;
}


/*----------------------------------------------------------------------------*
 * Private functions
 *----------------------------------------------------------------------------*/

/**
 * Get the path to the PuTTY executable (@c psftp.exe).
 *
 * This path is based on the path to the Swish DLL stored in the registry.  It
 * is assumed that @c psftp.exe will exist in the same directory.
 *
 * @pre The path to the Swish DLL is stored in the registry in the
 *      @c HKLM\\CLSID\\<__uuidof(CPuttyProvider)>\\InprocServer32 key.
 * @pre The PuTTY SFTP exectuable, @c psftp.exe, exists in the same directory
 *      as the registered Swish DLL.
 * @returns The path to @c psftp.exe is found or an empty string otherwise.
 *
 * @todo Check that it really is an empty string in the failure case.
 */
CString CPuttyProvider::_GetExePath() // static
{
	
	// Construct subkey using CLSID as a string
	CString strSubkey;
	LPOLESTR pszCLSID = NULL;
	::StringFromCLSID( __uuidof(CPuttyProvider), &pszCLSID );
	strSubkey += _T("CLSID\\");
	strSubkey += pszCLSID;
	strSubkey += _T("\\InprocServer32");
	::CoTaskMemFree( pszCLSID );

	// Get path of Swish DLL e.g. C:\Program Files\Swish\Swish.dll
	TCHAR szPath[MAX_PATH];
	VERIFY(
		::SHRegGetPath(HKEY_CLASSES_ROOT, strSubkey, 0, szPath, NULL) == 
		ERROR_SUCCESS);

	// Use to contruct psftp path e.g. C:\Program Files\Swish\psftp.exe
	VERIFY( ::PathRemoveFileSpec(szPath) );
	CString strExePath(szPath);
	strExePath += _T("\\psftp.exe");
	ASSERT( ::PathFileExists(strExePath) );

	return strExePath;
}

/**
 * Build an automation-compatible DATE from the given strings
 *
 * @param strMonth    The desired month in English: e.g. August
 * @param strDate     The date as a number: e.g. 31
 * @param strTimeYear Either the year (e.g. 2008)
 *                    or the time in hour:minute format (e.g. 18:38)
 */
DATE CPuttyProvider::_BuildDate( // static
	const CString strMonth, const CString strDate, const CString strTimeYear)
{
	COleDateTime dateModified;

	CString strTime, strYear;
	if (strTimeYear.Find(_T(':')) == -1)
	{
		// strTimeYear is a year: 2008
		strYear = strTimeYear;
		strTime = _T("");
	}
	else
	{
		// strTimeYear is a time: 18:38
		strTime = strTimeYear + _T(":00");
		strYear.Format(_T("%d"), COleDateTime::GetCurrentTime().GetYear());
	}

	CString strDateTime = 
		strDate + _T(" ") + strMonth + _T(" ") +  strYear + _T(" ") + strTime;
	VERIFY( dateModified.ParseDateTime(strDateTime) );

	return dateModified;
}

// CPuttyProvider