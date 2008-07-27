/*  IEnumIDList-based enumerator for SFTP remote folder.

    Copyright (C) 2007, 2008  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#ifndef REMOTEENUMIDLIST_H
#define REMOTEENUMIDLIST_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "stdafx.h"
#include "resource.h"        // main symbols

#include "RemoteFolder.h"    // For back-reference to parent folder
#include "SftpProvider.h"    // For interface to back-end data providers
#include "UserInteraction.h" // For implementation of ISftpConsumer
#include "Connection.h"      // For SFTP Connection container

struct FILEDATA
{
	bool fIsFolder;
	CString strFilename;
	CString strOwner;
	CString strGroup;
	CString strAuthor;

	ULONGLONG uSize; // 64-bit allows files up to 16 Ebibytes (a lot)
	time_t dtModified;
	DWORD dwPermissions;
};

// CRemoteEnumIDList
[
	coclass,
	default(IUnknown),
	threading(apartment),
	vi_progid("Swish.RemoteEnumIDList"),
	progid("Swish.RemoteEnumIDList.1"),
	version(1.0),
	uuid("b816a83e-5022-11dc-9153-0090f5284f85"),
	helpstring("RemoteEnumIDList Class")
]
class ATL_NO_VTABLE CRemoteEnumIDList :
	public IEnumIDList
{
public:
	CRemoteEnumIDList() :
		m_pConsumer(NULL), m_fInitialised(false), m_iPos(0)
	{
	}

	DECLARE_PROTECT_FINAL_CONSTRUCT()
	HRESULT FinalConstruct()
	{
		return S_OK;
	}

	void FinalRelease()
	{
		// Release SFTP provider that was passed in Intialize()
		if (m_pProvider)
		{
			m_pProvider->Release();
			m_pProvider = NULL;
		}

		// Release SFTP user interaction handler that was passed in Intialize()
		if (m_pConsumer)
		{
			m_pConsumer->Release();
			m_pConsumer = NULL;
		}
	}

	HRESULT Initialize(
		__in CConnection& conn, __in PCTSTR pszPath, __in SHCONTF grfFlags );

	/**
	 * Creates enumerator instance and fetches directory listing from server. 
	 *
	 * This will AddRef() the folder to ensure it remains alive as long as 
	 * the enumerator needs it.
	 *
	 * @param [in]  conn      SFTP connection container.
	 * @param [in]  pszPath   Path of remote directory to be enumerated.
	 * @param [in]  grfFlags  Flags specifying nature of enumeration.
	 * @param [out] ppReturn  Location in which to return the IEnumIDList.
	 */
	static HRESULT MakeInstance(
		__in CConnection& conn, __in PCTSTR pszPath, __in SHCONTF grfFlags,
		__deref_out IEnumIDList **ppEnumIDList )
	{
		HRESULT hr;

		// Create instance of our folder enumerator class
		CComObject<CRemoteEnumIDList>* pEnum;
		hr = CComObject<CRemoteEnumIDList>::CreateInstance( &pEnum );
		ATLENSURE_RETURN_HR(SUCCEEDED(hr), hr);

		pEnum->AddRef();

		hr = pEnum->Initialize( conn, pszPath, grfFlags );
		ATLASSERT(SUCCEEDED(hr));

		// Return an IEnumIDList interface to the caller.
		if (SUCCEEDED(hr))
		{
			hr = pEnum->QueryInterface( ppEnumIDList );
			ATLASSERT(SUCCEEDED(hr));
		}

		pEnum->Release();
		pEnum = NULL;

		return hr;
	}

	// IEnumIDList
	IFACEMETHODIMP Next(
		ULONG celt,
		__out_ecount_part(celt, *pceltFetched) PITEMID_CHILD *rgelt,
		__out_opt ULONG *pceltFetched );
	IFACEMETHODIMP Skip( DWORD celt );
	IFACEMETHODIMP Reset();
	IFACEMETHODIMP Clone( __deref_out IEnumIDList **ppenum );

private:
	BOOL m_fInitialised;
	ISftpProvider *m_pProvider; ///< Connection to SFTP backend.
	ISftpConsumer *m_pConsumer; ///< User-interaction handler for backend.
	SHCONTF m_grfFlags;         ///< Flags specifying type of file to enumerate
	std::vector<FILEDATA> m_vListing;
	ULONG m_iPos; // Current position
	CRemotePidlManager m_PidlManager;

	HRESULT _Fetch( PCTSTR pszPath );
	time_t _ConvertDate( __in DATE dateValue ) const;
};

#endif // REMOTEENUMIDLIST_H
