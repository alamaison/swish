/*  Explorer folder handling remote files and folders in a directory.

    Copyright (C) 2007, 2008, 2009  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "Connection.h"     // For SFTP interactive connection objects
#include "RemotePidl.h"     // For RemoteItemId handling
#include "Pool.h"           // For access to SFTP global session pool

#pragma warning (push)
#pragma warning (disable: 4199) // Not injecting base class

[
	coclass,
	default(IUnknown),
	threading(apartment),
	vi_progid("Swish.RemoteFolder"),
	progid("Swish.RemoteFolder.1"),
	version(1.0),
	registration_script("RemoteFolder.rgs"),
	uuid("b816a83c-5022-11dc-9153-0090f5284f85"),
	helpstring("RemoteFolder Class")
]
class ATL_NO_VTABLE CRemoteFolder :
	public CFolder,
	private CCoFactory<CRemoteFolder>
{
public:

	BEGIN_COM_MAP(CRemoteFolder)
		COM_INTERFACE_ENTRY(IShellFolder)
		COM_INTERFACE_ENTRY_CHAIN(CFolder)
	END_COM_MAP()

	/*
	We can assume that the PIDLs contained in this folder (i.e. any PIDL
	relative to it) contain one or more REMOTEPIDLs representing the 
	file-system hierarchy of the target file or folder and may be a child of
	either a HOSTPIDL or another REMOTEPIDL:

	    <Relative (HOST|REMOTE)PIDL>/REMOTEPIDL[/REMOTEPIDL]*
	*/

	/**
	 * Create initialized instance of the CRemoteFolder class.
	 *
	 * @param pidl  Absolute PIDL at which to root the folder instance (passed
	 *              to Initialize).
	 *
	 * @returns Smart pointer to the CRemoteFolder's IShellFolder interface.
	 * @throws  CAtlException if creation fails.
	 */
	static CComPtr<IShellFolder> Create(__in PCIDLIST_ABSOLUTE pidl)
	throw(...)
	{
		CComPtr<CRemoteFolder> spObject = spObject->CreateCoObject();
		
		HRESULT hr = spObject->Initialize(pidl);
		ATLENSURE_SUCCEEDED(hr);
		return spObject;
	}

protected:

	__override void ValidatePidl(PCUIDLIST_RELATIVE pidl) const throw(...);
	__override CLSID GetCLSID() const;
	__override CComPtr<IShellFolder> CreateSubfolder(
		PCIDLIST_ABSOLUTE pidlRoot)
		const throw(...);
	__override int ComparePIDLs(
		__in PCUITEMID_CHILD pidl1, __in PCUITEMID_CHILD pidl2,
		USHORT uColumn, bool fCompareAllFields, bool fCanonical)
		const throw(...);

	__override CComPtr<IShellFolderViewCB> GetFolderViewCallback()
		const throw(...);

public:

	// IShellFolder
	STDMETHOD(EnumObjects)( HWND, SHCONTF, IEnumIDList** );
	STDMETHOD(GetAttributesOf) ( UINT, PCUITEMID_CHILD_ARRAY, SFGAOF* );
    STDMETHOD(GetUIObjectOf)
		( HWND, UINT, PCUITEMID_CHILD_ARRAY, REFIID, LPUINT, void** );
	IFACEMETHODIMP GetDisplayNameOf( 
		__in PCUITEMID_CHILD pidl, __in SHGDNF uFlags, __out STRRET *pName);
	IFACEMETHODIMP ParseDisplayName( 
		__in_opt HWND hwnd, __in_opt IBindCtx *pbc, __in PWSTR pwszDisplayName,
		__reserved ULONG *pchEaten, __deref_out_opt PIDLIST_RELATIVE *ppidl, 
		__inout_opt ULONG *pdwAttributes);
	IFACEMETHODIMP SetNameOf(
		__in_opt HWND hwnd, __in PCUITEMID_CHILD pidl, __in LPCWSTR pwszName,
		SHGDNF uFlags, __deref_out_opt PITEMID_CHILD *ppidlOut);

	// IShellDetails
	IFACEMETHODIMP GetDetailsOf(
		__in_opt PCUITEMID_CHILD pidl, UINT iColumn, __out SHELLDETAILS* psd);

	// IShellFolder2
	IFACEMETHODIMP GetDefaultColumnState( 
		UINT iColumn, __out SHCOLSTATEF* pcsFlags);
	IFACEMETHODIMP GetDetailsEx(
		__in PCUITEMID_CHILD pidl, __in const SHCOLUMNID* pscid,
		__out VARIANT* pv);
	IFACEMETHODIMP MapColumnToSCID(UINT iColumn, __out SHCOLUMNID* pscid);

private:

	typedef std::vector<CRemoteItem> RemotePidls;

	CConnection _GetConnection(
		__in_opt HWND hwnd, __in_z PCWSTR szHost, __in_z PCWSTR szUser, 
		UINT uPort ) throw(...);
	CConnection _CreateConnectionForFolder(
		__in_opt  HWND hwndUserInteraction ) throw(...);
	void _Delete( __in_opt HWND hwnd, __in const RemotePidls& vecDeathRow )
		throw(...);
	void _DoDelete(
		__in_opt HWND hwnd, __in const RemotePidls& vecDeathRow ) throw(...);
	bool _ConfirmDelete(
		__in_opt HWND hwnd, __in BSTR bstrName, __in bool fIsFolder ) throw();
	bool _ConfirmMultiDelete( __in_opt HWND hwnd, size_t cItems ) throw();

	/**
	 * Static dispatcher for Default Context Menu callback
	 */
	static HRESULT __callback CALLBACK MenuCallback(
		__in_opt IShellFolder *psf, HWND hwnd, __in_opt IDataObject *pdtobj, 
		UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		ATLENSURE_RETURN(psf);
		return static_cast<CRemoteFolder *>(psf)->OnMenuCallback(
			hwnd, pdtobj, uMsg, wParam, lParam);
	}

	/** @name Default context menu event handlers */
	// @{
	HRESULT OnMenuCallback( HWND hwnd, IDataObject *pdtobj, 
		UINT uMsg, WPARAM wParam, LPARAM lParam );
	HRESULT OnMergeContextMenu(
		HWND hwnd, IDataObject *pDataObj, UINT uFlags, QCMINFO& info );
	HRESULT OnInvokeCommand(
		HWND hwnd, IDataObject *pDataObj, int idCmd, PCWSTR pwszArgs );
	HRESULT OnInvokeCommandEx(
		HWND hwnd, IDataObject *pDataObj, int idCmd, PDFMICS pdfmics );
	// @}

	/** @name Invoked command handlers */
	// @{
	HRESULT OnCmdDelete( HWND hwnd, IDataObject *pDataObj );
	// @}

};

#pragma warning (pop)