/**
    @file

    Explorer folder to handle SFTP connection items.

    @if licence

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

    @endif
*/

#pragma once

#include "SwishFolder.hpp"       // Superclass

#include "resource.h"            // main symbols
#include "swish.h"               // For CHostFolder UUID
#include "HostPidl.h"            // HostItemId handling
#include "swish/CoFactory.hpp"   // CComObject factory

#include "swish/atl.hpp"         // Common ATL setup

#include <initguid.h>  // Make DEFINE_PROPERTYKEY() actually define a key
#include <propkey.h>   // For DEFINE_PROPERTYKEY

#include <vector>

class ATL_NO_VTABLE CHostFolder :
	public IExtractIconW,
	public swish::shell_folder::folder::CSwishFolder
{
public:

	BEGIN_COM_MAP(CHostFolder)
		COM_INTERFACE_ENTRY(IExtractIconW)
		COM_INTERFACE_ENTRY_CHAIN(CSwishFolder)
	END_COM_MAP()

protected:

	CLSID clsid() const;

	void validate_pidl(PCUIDLIST_RELATIVE pidl) const;
	int compare_pidls(
		PCUITEMID_CHILD pidl1, PCUITEMID_CHILD pidl2,
		int column, bool compare_all_fields, bool canonical) const;

	ATL::CComPtr<IShellFolder> subfolder(PCIDLIST_ABSOLUTE pidl) const;

	ATL::CComPtr<IExplorerCommandProvider> command_provider(HWND hwnd);

	ATL::CComPtr<IExtractIconW> extract_icon_w(
		HWND hwnd, PCUITEMID_CHILD pidl);
	ATL::CComPtr<IQueryAssociations> query_associations(
		HWND hwnd, UINT cpidl, PCUITEMID_CHILD_ARRAY apidl);
	ATL::CComPtr<IContextMenu> context_menu(
		HWND hwnd, UINT cpidl, PCUITEMID_CHILD_ARRAY apidl);
	ATL::CComPtr<IDataObject> data_object(
		HWND hwnd, UINT cpidl, PCUITEMID_CHILD_ARRAY apidl);

	ATL::CComPtr<IShellFolderViewCB> folder_view_callback(HWND hwnd);

public: // IShellFolder methods

	IFACEMETHODIMP EnumObjects(
		__in_opt HWND hwnd,
		SHCONTF grfFlags,
		__deref_out_opt IEnumIDList **ppenumIDList);

	STDMETHOD(GetAttributesOf) ( UINT, PCUITEMID_CHILD_ARRAY, SFGAOF* );
	IFACEMETHODIMP GetDisplayNameOf( 
		__in PCUITEMID_CHILD pidl, __in SHGDNF uFlags, __out STRRET *pName);
	IFACEMETHODIMP ParseDisplayName( 
		__in_opt HWND hwnd, __in_opt IBindCtx *pbc, __in PWSTR pwszDisplayName,
		__reserved ULONG *pchEaten, __deref_out_opt PIDLIST_RELATIVE *ppidl, 
		__inout_opt ULONG *pdwAttributes);
    STDMETHOD(SetNameOf)
		( HWND, PCUITEMID_CHILD, LPCOLESTR, DWORD, PITEMID_CHILD* )
        { return E_NOTIMPL; }

	// IShellFolder2
	STDMETHOD(GetDefaultColumnState)( UINT iColumn, SHCOLSTATEF *pcsFlags );
	STDMETHOD(GetDetailsEx)( PCUITEMID_CHILD pidl, const SHCOLUMNID *pscid, 
							 VARIANT *pv );
	STDMETHOD(MapColumnToSCID)( UINT iColumn, PROPERTYKEY *pscid );

	// IExtractIconW
	STDMETHOD(Extract)( LPCTSTR pszFile, UINT nIconIndex, HICON *phiconLarge, 
						HICON *phiconSmall, UINT nIconSize );
	STDMETHOD(GetIconLocation)( UINT uFlags, LPTSTR szIconFile, UINT cchMax, 
								int *piIndex, UINT *pwFlags );

	// IShellDetails
	STDMETHOD(GetDetailsOf)( PCUITEMID_CHILD pidl, UINT iColumn, 
							 LPSHELLDETAILS pDetails );

private:
	std::vector<CHostItem>  m_vecConnData;

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

