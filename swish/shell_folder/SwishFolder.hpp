/**
    @file

    Base-class implementing functionality common to all Swish folders

    @if licence

    Copyright (C) 2009  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "Folder.h"  // Superclass

#include "swish/atl.hpp"

namespace swish {
namespace shell_folder {
namespace folder {

class CSwishFolder :
	public swish::shell_folder::folder::CFolder
{
public:

	BEGIN_COM_MAP(CSwishFolder)
		COM_INTERFACE_ENTRY(IShellFolder)
		COM_INTERFACE_ENTRY_CHAIN(CFolder)
	END_COM_MAP()

protected:

	ATL::CComPtr<IUnknown> folder_object(HWND hwnd, REFIID riid);
	ATL::CComPtr<IUnknown> folder_item_object(
		HWND hwnd, REFIID riid, UINT cpidl, PCUITEMID_CHILD_ARRAY apidl);

	/** @name Objects associated with the current folder. */
	// @{
	virtual ATL::CComPtr<IShellView> folder_view(HWND hwnd);
	virtual ATL::CComPtr<IShellDetails> shell_details(HWND hwnd);

	virtual ATL::CComPtr<IDropTarget> drop_target(HWND hwnd);
	// @}


	/** @name Objects associated with items contained in folder. */
	// @{

	virtual ATL::CComPtr<IExtractIconW> extract_icon_w(
		HWND hwnd, PCUITEMID_CHILD pidl);
	virtual ATL::CComPtr<IExtractIconA> extract_icon_a(
		HWND hwnd, PCUITEMID_CHILD pidl);
	
	virtual ATL::CComPtr<IContextMenu> context_menu(
		HWND hwnd, UINT cpidl, PCUITEMID_CHILD_ARRAY apidl);
	
	virtual ATL::CComPtr<IQueryAssociations> query_associations(
		HWND hwnd, UINT cpidl, PCUITEMID_CHILD_ARRAY apidl);
	
	virtual ATL::CComPtr<IDataObject> data_object(
		HWND hwnd, UINT cpidl, PCUITEMID_CHILD_ARRAY apidl);
	// @}

	virtual ATL::CComPtr<IShellFolderViewCB> folder_view_callback(HWND hwnd);
};

}}} // namespace swish::shell_folder::folder
