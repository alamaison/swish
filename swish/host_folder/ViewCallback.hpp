/**
    @file

    Handler for host folder's interaction with Explorer Shell Folder View.

    @if license

    Copyright (C) 2008, 2009, 2010, 2011
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

#ifndef SWISH_HOST_FOLDER_VIEW_CALLBACK_HPP
#define SWISH_HOST_FOLDER_VIEW_CALLBACK_HPP
#pragma once

#include "swish/frontend/winsparkle_shower.hpp" // winsparkle_shower
#include "swish/nse/view_callback.hpp" // CViewCallback

#include <winapi/object_with_site.hpp> // object_with_site
#include <winapi/gui/menu/item/item.hpp>
#include <winapi/shell/pidl.hpp> // apidl_t

#include <comet/ptr.h> // com_ptr
#include <comet/server.h> // simple_object

#include <boost/optional/optional.hpp> // optional

namespace swish {
namespace host_folder {

class CViewCallback :
	public comet::simple_object<nse::CViewCallback, winapi::object_with_site>
{
public:

	CViewCallback(const winapi::shell::pidl::apidl_t& folder_pidl);

private:

	/// @name  SFVM_* message handlers
	// @{

	virtual bool on_window_created(HWND hwnd_view);
	virtual bool on_get_notify(PCIDLIST_ABSOLUTE& pidl_monitor, LONG& events);
	virtual bool on_fs_notify(PCIDLIST_ABSOLUTE pidl, LONG event);
	virtual bool on_merge_menu(QCMINFO& menu_info);
	virtual bool on_selection_changed(SFV_SELECTINFO& selection_info);
	virtual bool on_init_menu_popup(
		UINT first_command_id, int menu_index, HMENU menu);
	virtual bool on_invoke_command(UINT command_id);
	virtual bool on_get_help_text(
		UINT command_id, UINT buffer_size, LPTSTR buffer);
	virtual bool on_get_webview_content(SFV_WEBVIEW_CONTENT_DATA& content_out);
	virtual bool on_get_webview_tasks(SFV_WEBVIEW_TASKSECTION_DATA& tasks_out);

	// @}

	comet::com_ptr<IDataObject> selection();
	void update_menus();

	HWND m_hwnd_view;         ///< Handle to folder view window
	boost::optional<winapi::gui::menu::item> m_tools_menu;
							  ///< Handle to the Explorer 'Tools' menu
	UINT m_first_command_id;  ///< Start of our tools menu ID range
	winapi::shell::pidl::apidl_t m_folder_pidl; ///< Our copy of pidl to owning
	                                            ///< folder
	swish::frontend::winsparkle_shower m_winsparkle; ///< Autoupdate checker
};

}} // namespace swish::host_folder

#endif
