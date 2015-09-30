/* Manage complexities of adding and removing menu items in host window.

   Copyright (C) 2013, 2015  Alexander Lamaison <swish@lammy.co.uk>

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by the
   Free Software Foundation, either version 3 of the License, or (at your
   option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SWISH_HOST_FOLDER_MENU_COMMAND_MANAGER
#define SWISH_HOST_FOLDER_MENU_COMMAND_MANAGER

#include "swish/nse/Command.hpp"

#include <washer/gui/menu/menu.hpp>
#include <washer/shell/pidl.hpp> // apidl_t
#include <washer/window/window.hpp>

#include <comet/ptr.h> // com_ptr

#include <boost/optional/optional.hpp>
#include <boost/shared_ptr.hpp>

#include <map>
#include <string>

#include <ShlObj.h> // QCMINFO

namespace swish {
namespace host_folder {

/**
 * Unlike for webview tasks and command items, the shell doesn't recognise an
 * object to manage collections of menu items.  This class fill that gap in
 * order to keep the logic out of the view callback.
 */
class menu_command_manager
{
public:

    /**
     * Merge items into Explorer menus.
     */
    menu_command_manager(
        QCMINFO& menu_info,
        const boost::optional<washer::window::window<wchar_t>>& view,
        const washer::shell::pidl::apidl_t& folder);

    /**
     * Invoke a command by merge offset.
     */
    bool invoke(
        UINT command_id, comet::com_ptr<IShellItemArray> selection,
        comet::com_ptr<IUnknown> ole_site);

    /**
     * Request tool tip for command by merge offset.
     */
    bool help_text(
        UINT command_id, std::wstring& text_out,
        comet::com_ptr<IShellItemArray> selection);

    /**
     * Refresh command states to match current selection.
     */
    void update_state(comet::com_ptr<IShellItemArray> selection);

private:

    boost::optional<washer::window::window<wchar_t>> m_view;
    ///< Folder view window

    washer::shell::pidl::apidl_t m_folder; ///< Owning folder

    UINT m_first_command_id;  ///< Start of our tools menu ID range

    std::map<UINT, boost::shared_ptr<swish::nse::Command>> m_commands;
    ///< Commands in menu with their menu item ID

    boost::optional<washer::gui::menu::item> m_tools_menu;
    ///< Handle to the Explorer 'Tools' menu

    boost::optional<washer::gui::menu::item> m_help_menu;
    ///< Handle to the Explorer 'Help' menu
};

}}

#endif