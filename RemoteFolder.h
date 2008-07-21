/*  Explorer folder handling remote files and folders in a directory.

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

#ifndef REMOTEFOLDER_H
#define REMOTEFOLDER_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "stdafx.h"
#include "resource.h"       // main symbols

#define INITGUID
#include <propkey.h>

#include "RemotePidlManager.h"
#include "HostPidlManager.h"

// This is being used as a type-safety marker interface - may be uneccessary
[
	object,
	uuid("b816a83b-5022-11dc-9153-0090f5284f85"),
	helpstring("IRemoteFolder Interface"),
	pointer_default(unique)
]
__interface IRemoteFolder : IUnknown
{
};


// CRemoteFolder
[
	coclass,
	default(IRemoteFolder),
	threading(apartment),
	vi_progid("Swish.RemoteFolder"),
	progid("Swish.RemoteFolder.1"),
	version(1.0),
	registration_script("RemoteFolder.rgs"),
	uuid("b816a83c-5022-11dc-9153-0090f5284f85"),
	helpstring("RemoteFolder Class")
]
class ATL_NO_VTABLE CRemoteFolder :
	public IRemoteFolder,
	// The IShellFolder2-specific detail-handling methods are not compatible
	// with Win 9x/NT but it supports all those of IShellDetails which are
	public IShellFolder2,
	public IPersistFolder2, // IPersistFolder2 needed for Details expando
	public IShellDetails // This is compatible with 9x/NT unlike IShellFolder2
{
public:
	CRemoteFolder() : m_pidl(NULL) {}

	~CRemoteFolder()
	{
		if (m_pidl)
			m_RemotePidlManager.Delete( m_pidl );
	}

    // IPersist
    IFACEMETHODIMP GetClassID( __out CLSID *pClsid );

	// IPersistFolder
	IFACEMETHODIMP Initialize( __in PCIDLIST_ABSOLUTE pidl );

	// IPersistFolder2
	IFACEMETHODIMP GetCurFolder( __deref_out_opt PIDLIST_ABSOLUTE *ppidl );

	// IShellFolder
	IFACEMETHODIMP BindToObject(
		__in PCUIDLIST_RELATIVE pidl, __in_opt IBindCtx *pbc, __in REFIID riid,
		__out void** ppvOut );
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

	// IShellDetails
	STDMETHOD(GetDetailsOf)( PCUITEMID_CHILD pidl, UINT iColumn, 
							 LPSHELLDETAILS pDetails );
	STDMETHOD(ColumnClick)( UINT iColumn );

private:
    CRemotePidlManager m_RemotePidlManager;
	CHostPidlManager   m_HostPidlManager;
	PIDLIST_ABSOLUTE   m_pidl; // Absolute pidl of this folder object

	CString _GetLongNameFromPIDL( PCIDLIST_ABSOLUTE pidl, BOOL fCanonical );
	CString _GetFilenameFromPIDL( PCUITEMID_CHILD pidl );
	CString _GetFileExtensionFromPIDL( PCUITEMID_CHILD );
	HRESULT _GetRegistryKeysForPidl(
		PCUITEMID_CHILD pidl, 
		__out UINT *pcKeys, __deref_out_ecount(pcKeys) HKEY **paKeys );
	std::vector<CString> _GetExtensionSpecificKeynames( PCTSTR szExtension );
	CString _ExtractPathFromPIDL( PCIDLIST_ABSOLUTE pidl );
	HRESULT _FillDetailsVariant( PCWSTR szDetail, VARIANT *pv );
	HRESULT _FillDateVariant( CTime dtDate, VARIANT *pv );
	HRESULT _FillUI8Variant( ULONGLONG ull, VARIANT *pv );

	/**
	 * Static dispatcher for Default Context Menu callback
	 */
	static HRESULT __callback CALLBACK MenuCallback(
		IShellFolder *psf, HWND hwnd, IDataObject *pdtobj, UINT uMsg, 
		WPARAM wParam, LPARAM lParam )
	{
		return static_cast<CRemoteFolder *>(psf)->OnMenuCallback(
			hwnd, pdtobj, uMsg, wParam, lParam );
	}

	/** @name Default context menu event handlers */
	// @{
	HRESULT OnMenuCallback( HWND hwnd, IDataObject *pdtobj, 
		UINT uMsg, WPARAM wParam, LPARAM lParam );
	HRESULT OnMergeContextMenu(
		IDataObject *pDataObj, UINT uFlags, QCMINFO& info );
	HRESULT OnInvokeCommand(
		IDataObject *pDataObj, int idCmd, PCTSTR pszArgs );
	// @}
};

// Remote folder listing column property IDs
enum PID_SWISH_REMOTE {
	PID_SWISH_REMOTE_GROUP = PID_FIRST_USABLE,
	PID_SWISH_REMOTE_PERMISSIONS
};

// PKEYs for custom swish details/properties
// Swish remote folder FMTID GUID {b816a851-5022-11dc-9153-0090f5284f85}
DEFINE_PROPERTYKEY(PKEY_SwishRemoteGroup, 0xb816a851, 0x5022, 0x11dc, \
				   0x91, 0x53, 0x00, 0x90, 0xf5, 0x28, 0x4f, 0x85, \
				   PID_SWISH_REMOTE_GROUP);
DEFINE_PROPERTYKEY(PKEY_SwishRemotePermissions, 0xb816a851, 0x5022, 0x11dc, \
				   0x91, 0x53, 0x00, 0x90, 0xf5, 0x28, 0x4f, 0x85, \
				   PID_SWISH_REMOTE_PERMISSIONS);

#endif // REMOTEFOLDER_H
