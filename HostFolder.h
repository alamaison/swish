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

#pragma once

#include "stdafx.h"
#include "resource.h"       // main symbols

#include "CoFactory.h"
#include "Folder.h"

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
	public CFolder,
	public CCoFactory<CHostFolder>,
	public IExtractIcon
{
public:

	CHostFolder();

protected:

	__override void ValidatePidl(PCUIDLIST_RELATIVE pidl) const throw(...);
	__override CLSID GetCLSID() const;
	__override CComPtr<IShellFolder> CreateSubfolder(
		PCIDLIST_ABSOLUTE pidlRoot)
		const throw(...);
	__override int ComparePIDLs(
		__in PCUIDLIST_RELATIVE pidl1, __in PCUIDLIST_RELATIVE pidl2,
		USHORT uColumn, bool fCompareAllFields, bool fCanonical)
		const throw(...);

	__override CComPtr<IShellFolderViewCB> GetFolderViewCallback()
		const throw(...);

public: // IShellFolder methods

	IFACEMETHODIMP EnumObjects(
		__in_opt HWND hwnd,
		SHCONTF grfFlags,
		__deref_out_opt IEnumIDList **ppenumIDList);

	STDMETHOD(GetAttributesOf) ( UINT, PCUITEMID_CHILD_ARRAY, SFGAOF* );
    STDMETHOD(GetUIObjectOf)
		( HWND, UINT, PCUITEMID_CHILD_ARRAY, REFIID, LPUINT, void** );
	IFACEMETHODIMP GetDisplayNameOf( 
		__in PCUITEMID_CHILD pidl, __in SHGDNF uFlags, __out STRRET *pName);
	IFACEMETHODIMP ParseDisplayName( 
		__in_opt HWND hwnd, __in_opt IBindCtx *pbc, __in PWSTR pszDisplayName,
		__reserved ULONG *pchEaten, __deref_out_opt PIDLIST_RELATIVE *ppidl, 
		__inout_opt ULONG *pdwAttributes);
    STDMETHOD(SetNameOf)
		( HWND, PCUITEMID_CHILD, LPCOLESTR, DWORD, PITEMID_CHILD* )
        { return E_NOTIMPL; }

	// IShellFolder2
	STDMETHOD(GetDefaultColumn)( DWORD, ULONG *pSort, ULONG *pDisplay );
	STDMETHOD(GetDefaultColumnState)( UINT iColumn, SHCOLSTATEF *pcsFlags );
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

private:
    CHostPidlManager      m_HostPidlManager;
	CRemotePidlManager    m_RemotePidlManager;
	std::vector<HOSTPIDL> m_vecConnData;

	/**
	 * Static dispatcher for Default Context Menu callback
	 */
	static HRESULT __callback CALLBACK MenuCallback(
		__in_opt IShellFolder *psf, HWND hwnd, __in_opt IDataObject *pdtobj, 
		UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		ATLENSURE_RETURN(psf);
		return static_cast<CHostFolder *>(psf)->OnMenuCallback(
			hwnd, pdtobj, uMsg, wParam, lParam);
	}

	/** @name Default context menu event handlers */
	// @{
	HRESULT OnMenuCallback( HWND hwnd, IDataObject *pdtobj, 
		UINT uMsg, WPARAM wParam, LPARAM lParam );
	HRESULT OnMergeContextMenu(
		HWND hwnd, IDataObject *pDataObj, UINT uFlags, QCMINFO& info );
	// @}

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