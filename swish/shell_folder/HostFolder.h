/**
    @file

    Explorer folder to handle SFTP connection items.

    @if license

    Copyright (C) 2007, 2008, 2009, 2010, 2013
    Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "swish/host_folder/columns.hpp" // Column

#include "resource.h"            // main symbols
#include "swish/shell_folder/Swish.h" // For CHostFolder UUID
#include "swish/CoFactory.hpp"   // CComObject factory

#include "swish/atl.hpp"         // Common ATL setup

#include <vector>

class ATL_NO_VTABLE CHostFolder :
    public IExtractIconW, public IShellIconOverlay,
    public swish::shell_folder::folder::CSwishFolder<
        swish::host_folder::Column>
{
public:

    BEGIN_COM_MAP(CHostFolder)
        COM_INTERFACE_ENTRY(IExtractIconW)
        COM_INTERFACE_ENTRY(IShellIconOverlay)
        COM_INTERFACE_ENTRY_CHAIN(CSwishFolder)
    END_COM_MAP()

protected:

    CLSID clsid() const;

    void validate_pidl(PCUIDLIST_RELATIVE pidl) const;

    ATL::CComPtr<IShellFolder> subfolder(
        const winapi::shell::pidl::cpidl_t& pidl);
    
    comet::variant_t property(
        const winapi::shell::property_key& key,
        const winapi::shell::pidl::cpidl_t& pidl);

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

public:

    // IShellFolder (via folder_error_adapter)
    virtual IEnumIDList* enum_objects(HWND hwnd, SHCONTF flags);
    
    virtual void get_attributes_of(
        UINT pidl_count, PCUITEMID_CHILD_ARRAY pidl_array,
        SFGAOF* flags_inout);

    virtual STRRET get_display_name_of(
        PCUITEMID_CHILD pidl, SHGDNF uFlags);

    virtual PIDLIST_RELATIVE parse_display_name(
        HWND hwnd, IBindCtx* bind_ctx, const wchar_t* display_name,
        ULONG* attributes_inout);

    virtual PITEMID_CHILD set_name_of(
        HWND hwnd, PCUITEMID_CHILD pidl, const wchar_t* name,
        SHGDNF flags);

    // IShellFolder2 (via folder_error_adapter2)
    virtual SHCOLUMNID map_column_to_scid(UINT column_index);

    // IExtractIconW
    STDMETHOD(Extract)( LPCTSTR pszFile, UINT nIconIndex, HICON *phiconLarge, 
                        HICON *phiconSmall, UINT nIconSize );
    STDMETHOD(GetIconLocation)( UINT uFlags, LPTSTR szIconFile, UINT cchMax, 
                                int *piIndex, UINT *pwFlags );

    // IShellIconOverlay
    STDMETHOD(GetOverlayIndex)(PCUITEMID_CHILD item, int* index);
    STDMETHOD(GetOverlayIconIndex)(PCUITEMID_CHILD item, int* icon_index);
};
