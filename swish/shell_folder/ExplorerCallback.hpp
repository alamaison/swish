/**
    @file

    Handler for Shell Folder View's interaction with Explorer.

    @if licence

    Copyright (C) 2008, 2009, 2010  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#ifndef SWISH_SHELL_FOLDER_EXPLORERCALLBACK_HPP
#define SWISH_SHELL_FOLDER_EXPLORERCALLBACK_HPP

#pragma once

#include "HostPidl.h"

#include <winapi/object_with_site.hpp> // object_with_site
#include <winapi/shell/pidl.hpp> // apidl_t

#include <comet/ptr.h> // com_ptr
#include <comet/server.h> // simple_object

template<> struct comet::comtype<IShellFolderViewCB>
{
	static const IID& uuid() throw() { return IID_IShellFolderViewCB; }
	typedef IUnknown base;
};

template<> struct comet::comtype<IDataObject>
{
	static const IID& uuid() throw() { return IID_IDataObject; }
	typedef IUnknown base;
};

namespace swish {
namespace shell_folder {

/**
 * SFVM_SELECTIONCHANGED parameter.
 *
 * Undocumented by Microsoft.  Based on public domain code at
 * http://www.whirlingdervishes.com/nselib/mfc/samples/source.php.
 *
 * Copyright (C) 1998-2003 Whirling Dervishes Software.
 */
struct SFVCB_SELECTINFO
{
	UINT uOldState; // 0
	UINT uNewState; // LVIS_SELECTED, LVIS_FOCUSED,...
	LPITEMIDLIST pidl;
};

class CExplorerCallback :
	public comet::simple_object<IShellFolderViewCB, winapi::object_with_site>
{
public:

	typedef IShellFolderViewCB interface_is;

	CExplorerCallback(const winapi::shell::pidl::apidl_t& folder_pidl);

public: // IShellFolderViewCB

	virtual IFACEMETHODIMP MessageSFVCB(
		UINT message, WPARAM wparam, LPARAM lparam);

private:

	/// @name  SFVM_* message handlers
	// @{

	bool on_window_created(HWND hwnd_view);
	bool on_get_notify(PCIDLIST_ABSOLUTE& pidl_monitor, LONG& events);
	bool on_fs_notify(PCIDLIST_ABSOLUTE pidl, LONG event);
	bool on_merge_menu(QCMINFO& menu_info);
	bool on_selection_changed(SFVCB_SELECTINFO& selection_info);
	bool on_init_menu_popup(UINT first_command_id, int menu_index, HMENU menu);
	bool on_invoke_command(UINT command_id);
	bool on_get_help_text(UINT command_id, UINT buffer_size, LPTSTR buffer);

	// @}

	comet::com_ptr<IDataObject> selection();
	void update_menus();

	HWND m_hwnd_view;         ///< Handle to folder view window
	HMENU m_tools_menu;       ///< Handle to the Explorer 'Tools' menu
	UINT m_first_command_id;  ///< Start of our tools menu ID range
	winapi::shell::pidl::apidl_t m_folder_pidl; ///< Our copy of pidl to owning
	                                            ///< folder
	comet::com_ptr<IUnknown> m_ole_site; ///< OLE container site
};

}} // namespace swish::shell_folder

#endif
