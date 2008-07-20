/*  Declaration of remote folder contents enumerator via IEnumIDList interface

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

#ifndef REMOTEENUMIDLIST_H
#define REMOTEENUMIDLIST_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "stdafx.h"
#include "resource.h"       // main symbols

#include "RemoteFolder.h"   // For back-reference to parent folder
#include "SftpProvider.h"   // For interface to back-end data providers
#include "PasswordDialog.h" // For password dialog box

struct FILEDATA
{
	BOOL fIsFolder;
	CString strPath;
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
	public IEnumIDList,
	public ISftpConsumer
{
public:
	CRemoteEnumIDList() :
		m_pFolder(NULL), m_fBoundToFolder(false), m_iPos(0)
	{
	}

	DECLARE_PROTECT_FINAL_CONSTRUCT()
	HRESULT FinalConstruct()
	{
		return S_OK;
	}

	void FinalRelease()
	{
		// Release folder that should have been incremented in BindToFolder
		if (m_pFolder) // Possibly NULL if FinalConstruct() failed
			m_pFolder->Release();
	}

	HRESULT BindToFolder( __in IRemoteFolder* pFolder );
	HRESULT ConnectAndFetch(
		PCTSTR szUser, PCTSTR szHost, PCTSTR szPath, USHORT uPort );

	// IEnumIDList
	IFACEMETHODIMP Next(
		ULONG celt,
		__out_ecount_part(celt, *pceltFetched) PITEMID_CHILD *rgelt,
		__out_opt ULONG *pceltFetched );
	IFACEMETHODIMP Skip( DWORD celt );
	IFACEMETHODIMP Reset();
	IFACEMETHODIMP Clone( __deref_out IEnumIDList **ppenum );

private:
	BOOL m_fBoundToFolder;
	IRemoteFolder *m_pFolder;
	std::vector<FILEDATA> m_vListing;
	ULONG m_iPos; // Current position
	CRemotePidlManager m_PidlManager;

	time_t _ConvertDate( __in DATE dateValue ) const;

	/**
	 * User interaction callbacks.
	 * @{
	 */
	HRESULT OnPasswordRequest(
		__in BSTR bstrRequest, __out BSTR *pbstrPassword
	);
	HRESULT OnYesNoCancel(
		__in BSTR bstrMessage,
		__in_opt BSTR bstrYesInfo,
		__in_opt BSTR bstrNoInfo,
		__in_opt BSTR bstrCancelInfo,
		__in_opt BSTR bstrTitle,
		__out int *piResult
	);
	/* @} */
};

#endif // REMOTEENUMIDLIST_H
