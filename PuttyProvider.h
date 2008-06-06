/*  Declaration of SFTP data provider using PuTTY.

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

#ifndef PUTTYPROVIDER_H
#define PUTTYPROVIDER_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#pragma once
#include "stdafx.h"
#include "resource.h"       // main symbols

#include "PuttyWrapper.h"

/**
 * The record structure returned by the GetListing() method of CPuttyProvider.
 *
 * This structure represents a single file contained in the directory 
 * specified to GetListing().
 */
[export]
struct Listing {
	BSTR bstrFilename;    ///< Directory-relative filename (e.g. README.txt)
	BSTR bstrPermissions; ///< Unix-style file permissions (e.g. drwxr--r--)
	BSTR bstrOwner;       ///< The user name of the file's owner
	BSTR bstrGroup;       ///< The name of the group to which the file belongs
	ULONG cSize;          ///< The file's size in bytes
	ULONG cHardLinks;     ///< The number of hard links referencing this file
	DATE dateModified;    ///< The date and time at which the file was 
	                      ///< last modified in automation-compatible format
};

// IEnumListing
[
	object,
	uuid("b816a843-5022-11dc-9153-0090f5284f85"),
	helpstring("IEnumListing Interface"),
	pointer_default(unique)
]
__interface IEnumListing : IUnknown
{
	/* Will need local/remote hack if used remotely */
	HRESULT Next (
		[in] ULONG celt,
		[out] Listing *rgelt,
		[out] ULONG *pceltFetched);
	HRESULT Skip ([in] ULONG celt);
	HRESULT Reset ();
	HRESULT Clone ([out] IEnumListing **ppEnum);
};




#define ISftpProvider ISftpProviderUnstable
#define IID_ISftpProvider __uuidof(ISftpProviderUnstable)
#define ISftpConsumer ISftpConsumerUnstable
#define IID_ISftpConsumer __uuidof(ISftpConsumerUnstable)
#define CLSID_CPuttyProvider __uuidof(CPuttyProvider)

// ISftpConsumer
[
	object, oleautomation,
	uuid("99293E0D-C3AB-4b50-8132-329E30216E14"),
	//uuid("b816a844-5022-11dc-9153-0090f5284f85"),
	helpstring("ISftpConsumer Interface"),
	pointer_default(unique)
]
__interface ISftpConsumer
{
	HRESULT OnPasswordRequest(
		[in]          BSTR bstrRequest,
		[out, retval] BSTR *pbstrPassword
	);
	HRESULT OnYesNoCancel(
		[in]          BSTR bstrMessage,
		[in]          BSTR bstrYesInfo,
		[in]          BSTR bstrNoInfo,
		[in]          BSTR bstrCancelInfo,
		[in]          BSTR bstrTitle,
		[out, retval] int *piResult
	);
};

// ISftpProvider
[
	object,
	uuid("93874AB6-D2AE-47c0-AFB7-F59A7507FADA"),
	//uuid("b816a841-5022-11dc-9153-0090f5284f85"),
	helpstring("ISftpProvider Interface"),
	pointer_default(unique)
]
__interface ISftpProvider
{
	HRESULT Initialize(
		[in] ISftpConsumer *pConsumer,
		[in] BSTR bstrUser,
		[in] BSTR bstrHost,
		[in] short uPort
	);
	HRESULT GetListing(
		[in] BSTR bstrDirectory,
		[out, retval] IEnumListing **ppEnum
	);
	
};



/**
 * PuTTY-based SFTP data provider.
 *
 * @todo List pros and cons of this data provider over others.
 */
[
	coclass,
	default(ISftpProvider),
	threading(apartment),
	vi_progid("Swish.PuttyProvider"),
	progid("Swish.PuttyProvider.1"),
	version(1.0),
	uuid("b816a842-5022-11dc-9153-0090f5284f85"),
	helpstring("PuttyProvider Class")
]
class ATL_NO_VTABLE CPuttyProvider :
	public ISftpProvider
{
public:
	/**
	 * Create PuTTY-based data provider component instance.
	 *
	 * The location of the PuTTY SFTP executable (@c psftp.exe) is taken 
	 * from the Registry.
	 *
	 * @see _GetExePath()
	 * @warning
	 *   The Initialize() method must be called before the other methods
	 *   of the object can be used.
	 */
	CPuttyProvider()
	try
		: m_fConstructException(false), m_fInitialized(false),
		  m_pConsumer(NULL), m_Putty(_GetExePath()) {}
	catch (CPuttyWrapper::ChildLaunchException e)
	{
		UNREFERENCED_PARAMETER( e );
		m_fConstructException = true;
		UNREACHABLE;
	}

	/**
	 * Returns constructor success or failure.
	 *
	 * It is possible for the CPuttyWrapper() constructor to throw an exception
	 * in which case this function must return a failure code which will be
	 * passed up to the ATL CreateInstance function.
	 */
	HRESULT FinalConstruct()
	{
		if (m_fConstructException) return E_FAIL;
		return S_OK;
	}
	void FinalRelease()
	{
		if (m_pConsumer)
			m_pConsumer->Release();
	}

	// IPuttyProviderUnstable
	IFACEMETHODIMP Initialize(
		__in ISftpConsumer *pConsumer,
		__in BSTR bstrUser, __in BSTR bstrHost, short uPort );
	IFACEMETHODIMP GetListing(
		__in BSTR bstrDirectory, __out IEnumListing **ppEnum );

private:
	ISftpConsumer *m_pConsumer;    ///< Callback to consuming object
	BOOL m_fInitialized;           ///< Flag if Initialize() has been called
	CPuttyWrapper m_Putty;         ///< Wrapper round psftp command-line
	std::list<Listing> m_lstFiles;
	CString m_strUser;             ///< Holds username for remote connection
	CString m_strHost;             ///< Hold name of remote host
	UINT m_uPort;                  ///< Holds remote port to connect to
	BOOL m_fConstructException;    ///< Was there an exception in constructor?

	HRESULT _HandlePasswordRequests(CString& strCurrentChunk);
	HRESULT _HandleKeyboardInteractive(CString& strCurrentChunk);
	HRESULT _HandleUnknownKeyNotice(CString& strCurrentChunk);
	CString _ExtractLastLine(CString strChunk);
	static CString _GetExePath();
	static DATE _BuildDate(
		__in const CString szMonth, __in const CString szDate, 
		__in const CString szTimeYear);
};

#endif // PUTTYPROVIDER_H
