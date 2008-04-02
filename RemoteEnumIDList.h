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
#include "PuttyProvider.h"  // For putty-based data provider

#define IPuttyProvider IPuttyProviderUnstable
#define IID_IPuttyProvider __uuidof(IPuttyProviderUnstable)
#define CLSID_CPuttyProvider __uuidof(CPuttyProvider)

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


// IRemoteEnumIDList
[
	object,
	uuid("b816a83d-5022-11dc-9153-0090f5284f85"),
	helpstring("IRemoteEnumIDList Interface"),
	pointer_default(unique)
]
__interface IRemoteEnumIDList : IUnknown
{
};


// CRemoteEnumIDList
[
	coclass,
	default(IRemoteEnumIDList),
	threading(apartment),
	vi_progid("Swish.RemoteEnumIDList"),
	progid("Swish.RemoteEnumIDList.1"),
	version(1.0),
	uuid("b816a83e-5022-11dc-9153-0090f5284f85"),
	helpstring("RemoteEnumIDList Class")
]
class ATL_NO_VTABLE CRemoteEnumIDList :
	public IRemoteEnumIDList,
	public IEnumIDList
{
public:
	CRemoteEnumIDList() :
		m_pProvider(NULL), m_pFolder(NULL), m_fBoundToFolder(false), m_iPos(0)
	{
	}

	DECLARE_PROTECT_FINAL_CONSTRUCT()
	HRESULT FinalConstruct()
	{
		// Create instance of CPuttyProvider using CLSID
		HRESULT hr = ::CoCreateInstance(
			CLSID_CPuttyProvider,     // CLASSID for CPuttyProvider.
			NULL,                     // Ignore this.
			CLSCTX_INPROC_SERVER,     // Server.
			IID_IPuttyProvider,       // Interface you want.
			(LPVOID *)&m_pProvider);  // Place to store interface.

		ASSERT( SUCCEEDED(hr) );
		return hr;
	}

	void FinalRelease()
	{
		// Release folder that should have been incremented in BindToFolder
		if (m_pFolder) // Possibly NULL if FinalConstruct() failed
			m_pFolder->Release();

		// Release data provider component (should destroy psftp process)
		if (m_pProvider) // Possibly NULL if FinalConstruct() failed
			m_pProvider->Release();
	}

	HRESULT BindToFolder( CRemoteFolder* pFolder );
	HRESULT Connect(
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
	IPuttyProvider *m_pProvider;
	CRemoteFolder* m_pFolder;
	std::vector<FILEDATA> m_vListing;
	ULONG m_iPos; // Current position
    CRemotePidlManager m_PidlManager;
};

#endif // REMOTEENUMIDLIST_H
