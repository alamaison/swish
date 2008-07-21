/*  Explorer folder to handle SFTP connection items.

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

#ifndef HOSTFOLDER_H
#define HOSTFOLDER_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "stdafx.h"
#include "resource.h"       // main symbols

#define INITGUID
#include <propkey.h>

#include "HostPidlManager.h"
#include "RemotePidlManager.h"

// CHostFolder
[
	coclass,
	default(IUnknown),
	threading(apartment),
	vi_progid("Swish.HostFolder"),
	progid("Swish.HostFolder.1"),
	version(1.0),
	registration_script("HostFolder.rgs"),
	uuid("b816a83a-5022-11dc-9153-0090f5284f85"),
	helpstring("HostFolder Class")
]
class ATL_NO_VTABLE CHostFolder :
	// The IShellFolder2-specific detail-handling methods are not compatible
	// with Win 9x/NT but it supports all those of IShellDetails which are
	public IShellFolder2, 
	// IPersistFolder2 needed for Details expando
	public IPersistFolder2, 
	public IExtractIcon
//	public IShellDetails // This is compatible with 9x/NT unlike IShellFolder2
{
public:
	CHostFolder() : m_pidl(NULL) {}

	~CHostFolder()
	{
		if (m_pidl)
			m_HostPidlManager.Delete( m_pidl );
	}

	DECLARE_PROTECT_FINAL_CONSTRUCT()
	HRESULT FinalConstruct()
	{
		return S_OK;
	}
	void FinalRelease()
	{
	}

    // IPersist
    STDMETHOD(GetClassID)( CLSID* );

	// IPersistFolder
    STDMETHOD(Initialize)( PCIDLIST_ABSOLUTE pidl );

	// IPersistFolder2
	STDMETHOD(GetCurFolder)( PIDLIST_ABSOLUTE *ppidl );

	// IShellFolder
    STDMETHOD(BindToObject)(PCUIDLIST_RELATIVE pidl, IBindCtx*, REFIID, void**);
	STDMETHOD(EnumObjects)( HWND, SHCONTF, LPENUMIDLIST* );
    STDMETHOD(CreateViewObject)( HWND, REFIID, void** );
	STDMETHOD(GetAttributesOf) ( UINT, PCUITEMID_CHILD_ARRAY, SFGAOF* );
    STDMETHOD(GetUIObjectOf)
		( HWND, UINT, PCUITEMID_CHILD_ARRAY, REFIID, LPUINT, void** );
	STDMETHOD(CompareIDs)
		( LPARAM, PCUIDLIST_RELATIVE, PCUIDLIST_RELATIVE );
    STDMETHOD(BindToStorage)( PCUIDLIST_RELATIVE, LPBC, REFIID, void** )
        { return E_NOTIMPL; }
    STDMETHOD(GetDisplayNameOf)( PCUITEMID_CHILD, SHGDNF, STRRET* );
    STDMETHOD(ParseDisplayName)
		( HWND, LPBC, LPOLESTR, LPDWORD, PIDLIST_RELATIVE*, LPDWORD )
        { return E_NOTIMPL; }
    STDMETHOD(SetNameOf)
		( HWND, PCUITEMID_CHILD, LPCOLESTR, DWORD, PITEMID_CHILD* )
        { return E_NOTIMPL; }

	// IShellFolder2
	STDMETHOD(EnumSearches)( IEnumExtraSearch **ppEnum );
	STDMETHOD(GetDefaultColumn)( DWORD, ULONG *pSort, ULONG *pDisplay );
	STDMETHOD(GetDefaultColumnState)( UINT iColumn, SHCOLSTATEF *pcsFlags );		STDMETHOD(GetDefaultSearchGUID)( GUID *pguid )
		{ return E_NOTIMPL; }
	STDMETHOD(GetDetailsEx)( PCUITEMID_CHILD pidl, const SHCOLUMNID *pscid, 
							 VARIANT *pv );
	STDMETHOD(MapColumnToSCID)( UINT iColumn, PROPERTYKEY *pscid );

	// IExtractIcon
	STDMETHOD(Extract)( LPCTSTR pszFile, UINT nIconIndex, HICON *phiconLarge, 
						HICON *phiconSmall, UINT nIconSize );
	STDMETHOD(GetIconLocation)( UINT uFlags, LPTSTR szIconFile, UINT cchMax, 
								int *piIndex, UINT *pwFlags );

	// IShellDetails
	STDMETHOD(GetDetailsOf)( PCUITEMID_CHILD pidl, UINT iColumn, 
							 LPSHELLDETAILS pDetails );
	STDMETHOD(ColumnClick)( UINT iColumn );

private:
    CHostPidlManager      m_HostPidlManager;
	CRemotePidlManager    m_RemotePidlManager;
	PIDLIST_ABSOLUTE      m_pidl; // Absolute pidl of this folder object
	std::vector<HOSTPIDL> m_vecConnData;

	CString _GetLongNameFromPIDL( PCUITEMID_CHILD pidl, BOOL fCanonical );
	CString _GetLabelFromPIDL( PCUITEMID_CHILD pidl );
	HRESULT _FillDetailsVariant( PCWSTR szDetail, VARIANT *pv );
	HRESULT _LoadConnectionsFromRegistry();
	HRESULT _LoadConnectionDetailsFromRegistry( PCTSTR szLabel );
};

// Host column property IDs
enum PID_SWISH_HOST {
	PID_SWISH_HOST_USER = PID_FIRST_USABLE,
	PID_SWISH_HOST_PORT
};

// PKEYs for custom swish details/properties
// Swish Host FMTID GUID {b816a850-5022-11dc-9153-0090f5284f85}
DEFINE_PROPERTYKEY(PKEY_SwishHostUser, 0xb816a850, 0x5022, 0x11dc, \
				   0x91, 0x53, 0x00, 0x90, 0xf5, 0x28, 0x4f, 0x85, \
				   PID_SWISH_HOST_USER);
DEFINE_PROPERTYKEY(PKEY_SwishHostPort, 0xb816a850, 0x5022, 0x11dc, \
				   0x91, 0x53, 0x00, 0x90, 0xf5, 0x28, 0x4f, 0x85, \
				   PID_SWISH_HOST_PORT);

#endif // HOSTFOLDER_H
