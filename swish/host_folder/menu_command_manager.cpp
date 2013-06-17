/**
    @file

    Manage complexities of adding and removing menu items in host window.

    @if license

    Copyright (C) 2013  Alexander Lamaison <awl03@doc.ic.ac.uk>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    If you modify this Program, or any covered work, by linking or
    combining it with the OpenSSL project's OpenSSL library (or a
    modified version of that library), containing parts covered by the
    terms of the OpenSSL or SSLeay licenses, the licensors of this
    Program grant you additional permission to convey the resulting work.

    @endif
*/

#include "menu_command_manager.hpp"

#include "swish/frontend/commands/About.hpp"
#include "swish/host_folder/commands/Add.hpp"
#include "swish/host_folder/commands/LaunchAgent.hpp"
#include "swish/host_folder/commands/Remove.hpp"
#include "swish/nse/Command.hpp" // MenuCommandTitleAdapter

#include <winapi/gui/menu/basic_menu.hpp> // find_first_item_with_id
#include <winapi/gui/menu/button/string_button_description.hpp>
#include <winapi/gui/menu/item/command_item_description.hpp>
#include <winapi/gui/menu/item/command_item.hpp>
#include <winapi/gui/menu/item/item_state.hpp> // selectability
#include <winapi/gui/menu/item/separator_item.hpp>
#include <winapi/gui/menu/item/sub_menu_item.hpp>
#include <winapi/trace.hpp> // trace

#include <boost/exception/diagnostic_information.hpp> // diagnostic_information
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <cassert> // assert
#include <stdexcept> // logic_error, runtime_error

using swish::frontend::commands::About;
using swish::host_folder::commands::Add;
using swish::host_folder::commands::LaunchAgent;
using swish::host_folder::commands::Remove;
using swish::nse::MenuCommandTitleAdapter;

using namespace winapi::gui::menu;
using winapi::shell::pidl::apidl_t;
using winapi::trace;
using winapi::window::window;

using comet::com_ptr;

using boost::diagnostic_information;
using boost::optional;

using std::logic_error;
using std::runtime_error;
using std::wstring;

namespace swish {
namespace host_folder {

namespace {

// Menu command ID offsets for Explorer menus
enum MENUOFFSET
{
    MENUIDOFFSET_FIRST = 0,
    MENUIDOFFSET_ADD = MENUIDOFFSET_FIRST,
    MENUIDOFFSET_REMOVE,
    MENUIDOFFSET_LAUNCH_AGENT,
    MENUIDOFFSET_ABOUT,
    MENUIDOFFSET_LAST
};

template<typename DescriptionType, typename HandleCreator>
item item_from_menu(
    const basic_menu<DescriptionType, HandleCreator>& parent_menu,
    UINT menu_id)
{
    basic_menu<DescriptionType, HandleCreator>::iterator position =
        find_first_item_with_id(
            parent_menu.begin(), parent_menu.end(), menu_id);

    if (position != parent_menu.end())
    {
        return *position;
    }
    else
    {
        BOOST_THROW_EXCEPTION(
            runtime_error("Unable to find menu with given ID"));
    }
}

item fallback_menu(const menu_bar& parent_menu)
{
    return item_from_menu(parent_menu, FCIDM_MENU_FILE);
}

/**
 * Get handle to explorer 'Tools' menu.
 *
 * The menu we want to insert into is actually the @e submenu of the
 * Tools menu @e item.  Confusing!
 */
item tools_menu_with_fallback(const menu_bar& parent_menu)
{
    try
    {
        return item_from_menu(parent_menu, FCIDM_MENU_TOOLS);
    }
    catch (const std::exception& e)
    {
        trace("Failed getting Tools menu: %s") % diagnostic_information(e);

        return fallback_menu(parent_menu);
    }
}

/**
 * Get handle to explorer 'Help' menu.
 *
 * The menu we want to insert into is actually the @e submenu of the
 * Help menu @e item.  Confusing!
 */
item help_menu_with_fallback(const menu_bar& parent_menu)
{
    try
    {
        return item_from_menu(parent_menu, FCIDM_MENU_HELP);
    }
    catch (const std::exception& e)
    {
        trace("Failed getting help menu: %s") % diagnostic_information(e);

        return fallback_menu(parent_menu);
    }
}

class merge_tools_command_items
{
public:

    typedef void result_type;

    merge_tools_command_items(
        const optional<window<wchar_t>>& view,
        const apidl_t& folder, UINT first_command_id)
        : m_view(view), m_folder(folder),
          m_first_command_id(first_command_id) {}

    void operator()(sub_menu_item& sub_menu)
    {
        menu::iterator insert_position = sub_menu.menu().begin();

        // We hope the 1st and 2nd items are map and unmap network drive, so we
        // just skip them.  So that we don't fail completely if the Tools
        // menu is bizarre, we make sure there's actually room to skip first.
        if (sub_menu.menu().size() >= 2)
        {
            insert_position += 2;
        }

        HWND view_handle = (m_view) ? m_view->hwnd() : NULL;

        MenuCommandTitleAdapter<Add> add(view_handle, m_folder);

        // We have to be careful to increment the iterator *after* each calls to
        // insert in case we are inserting at the end.  Doing insert_position++
        // in the call to insert would step off the end;

        command_item_description add_item(
            string_button_description(add.title(NULL).c_str()),
            m_first_command_id + MENUIDOFFSET_ADD);
        sub_menu.menu().insert(add_item, insert_position);
        ++insert_position;

        MenuCommandTitleAdapter<Remove> remove(view_handle, m_folder);

        command_item_description remove_item(
            string_button_description(remove.title(NULL).c_str()),
            m_first_command_id + MENUIDOFFSET_REMOVE);
        remove_item.selectability(selectability::disabled);
        sub_menu.menu().insert(remove_item, insert_position);
        ++insert_position;

        MenuCommandTitleAdapter<LaunchAgent> launch(view_handle, m_folder);

        command_item_description launch_item(
             string_button_description(launch.title(NULL).c_str()),
             m_first_command_id + MENUIDOFFSET_LAUNCH_AGENT);
        launch_item.selectability(selectability::disabled);
        sub_menu.menu().insert(launch_item, insert_position);
        ++insert_position; // must be after insert in case we're at end
    }

    void operator()(command_item&)
    {
        BOOST_THROW_EXCEPTION(
            logic_error("Cannot insert into command item"));
    }

    void operator()(separator_item&)
    {
        BOOST_THROW_EXCEPTION(
            logic_error("Cannot insert into separator"));
    }

private:
    optional<window<wchar_t>> m_view;
    apidl_t m_folder;
    UINT m_first_command_id;
};

class merge_help_command_items
{
public:

    typedef void result_type;

    merge_help_command_items(
        const optional<window<wchar_t>>& view, const apidl_t& /*folder*/,
        UINT first_command_id)
        :
    m_view(view), m_first_command_id(first_command_id) {}

    void operator()(sub_menu_item& sub_menu)
    {
        // Inserting into the bottom of the menu
        menu::iterator insert_position = sub_menu.menu().end();

        MenuCommandTitleAdapter<About> about((m_view) ? m_view->hwnd() : NULL);

        command_item_description about_item(
            string_button_description(about.title(NULL).c_str()),
            m_first_command_id + MENUIDOFFSET_ABOUT);
        sub_menu.menu().insert(about_item, insert_position);
    }

    void operator()(command_item&)
    {
        BOOST_THROW_EXCEPTION(
            logic_error("Cannot insert into command item"));
    }

    void operator()(separator_item&)
    {
        BOOST_THROW_EXCEPTION(
            logic_error("Cannot insert into separator"));
    }

private:
    optional<window<wchar_t>> m_view;
    UINT m_first_command_id;
};

}


menu_command_manager::menu_command_manager(
    QCMINFO& menu_info, const optional<window<wchar_t>>& view,
    const apidl_t& folder)
:
m_view(view), m_folder(folder), m_first_command_id(menu_info.idCmdFirst)
{
    assert(menu_info.idCmdFirst >= FCIDM_SHVIEWFIRST);
    assert(menu_info.idCmdLast <= FCIDM_SHVIEWLAST);
    //assert(::IsMenu(menu_info.hmenu));

    // Try to get a handle to the  Explorer Tools menu and insert
    // add and remove connection menu items into it if we find it
    m_tools_menu = tools_menu_with_fallback(
        menu_handle::foster_handle(menu_info.hmenu));
    m_tools_menu->accept(
        merge_tools_command_items(m_view, m_folder, m_first_command_id));

    // Try to get a handle to the  Explorer Help menu and insert About box
    m_help_menu = help_menu_with_fallback(
        menu_handle::foster_handle(menu_info.hmenu));
    m_help_menu->accept(
        merge_help_command_items(m_view, m_folder, m_first_command_id));

    // Return value of last menu ID plus 1
    menu_info.idCmdFirst += MENUIDOFFSET_LAST; // Added 3 items
}

bool menu_command_manager::invoke(
    UINT command_id, com_ptr<IDataObject> selection)
{
    HWND view_handle = (m_view) ? m_view->hwnd() : NULL;

    if (command_id == MENUIDOFFSET_ADD)
    {
        Add command(view_handle, m_folder);
        command(selection, NULL);
        return true;
    }
    else if (command_id == MENUIDOFFSET_REMOVE)
    {
        Remove command(view_handle, m_folder);
        command(selection, NULL);
        return true;
    }
    else if (command_id == MENUIDOFFSET_LAUNCH_AGENT)
    {
        LaunchAgent command(view_handle, m_folder);
        command(selection, NULL);
        return true;
    }
    else if (command_id == MENUIDOFFSET_ABOUT)
    {
        About command(view_handle);
        command(selection, NULL);
        return true;
    }

    return false;
}

bool menu_command_manager::help_text(
    UINT command_id, wstring& text_out, com_ptr<IDataObject> selection)
{
    HWND view_handle = (m_view) ? m_view->hwnd() : NULL;

    if (command_id == MENUIDOFFSET_ADD)
    {
        Add command(view_handle, m_folder);
        text_out = command.tool_tip(selection);
        return true;
    }
    else if (command_id == MENUIDOFFSET_REMOVE)
    {
        Remove command(view_handle, m_folder);
        text_out = command.tool_tip(selection);
        return true;
    }
    else if (command_id == MENUIDOFFSET_LAUNCH_AGENT)
    {
        LaunchAgent command(view_handle, m_folder);
        text_out = command.tool_tip(selection);
        return true;
    }
    else if (command_id == MENUIDOFFSET_ABOUT)
    {
        About command(view_handle);
        text_out = command.tool_tip(selection);
        return true;
    }
    else
    {
        return false;
    }

}

namespace {

class update_command_items
{
public:

    typedef void result_type;

    update_command_items(
        const optional<window<wchar_t>>& view,
        const apidl_t& folder, com_ptr<IDataObject> selection,
        UINT first_command_id)
        :
    m_view(view), m_folder(folder), m_selection(selection),
    m_first_command_id(first_command_id) {}

    class selectability_setter
    {
    public:
        typedef void result_type;

        selectability_setter(BOOST_SCOPED_ENUM(selectability) selectability)
            : m_selectability(selectability) {}

        void operator()(command_item& item)
        {
            item.selectability(m_selectability);
        }

        template<typename T>
        void operator()(T&)
        {
            BOOST_THROW_EXCEPTION(
                logic_error("Unexpected menu item type"));
        }

    private:
        BOOST_SCOPED_ENUM(selectability) m_selectability;
    };

    void operator()(sub_menu_item& sub_menu)
    {
        HWND view_handle = (m_view) ? m_view->hwnd() : NULL;

        Add add(view_handle, m_folder);
        BOOST_SCOPED_ENUM(selectability) add_state =
            add.disabled(m_selection, false) ?
            selectability::disabled : selectability::enabled;

        item add_item = item_from_menu(
            sub_menu.menu(), m_first_command_id + MENUIDOFFSET_ADD);
        add_item.accept(selectability_setter(add_state));

        Remove remove(view_handle, m_folder);
        BOOST_SCOPED_ENUM(selectability) remove_state =
            remove.disabled(m_selection, false) ?
            selectability::disabled : selectability::enabled;

        item remove_item = item_from_menu(
            sub_menu.menu(), m_first_command_id + MENUIDOFFSET_REMOVE);
        remove_item.accept(selectability_setter(remove_state));

        LaunchAgent launch(view_handle, m_folder);
        BOOST_SCOPED_ENUM(selectability) launch_state =
            launch.disabled(m_selection, false) ?
            selectability::disabled : selectability::enabled;

       item launch_item = item_from_menu(
           sub_menu.menu(), m_first_command_id + MENUIDOFFSET_LAUNCH_AGENT);
       launch_item.accept(selectability_setter(launch_state));

       About about(view_handle);
       BOOST_SCOPED_ENUM(selectability) about_state =
           about.disabled(m_selection, false) ?
           selectability::disabled : selectability::enabled;

       item about_item = item_from_menu(
           sub_menu.menu(), m_first_command_id + MENUIDOFFSET_ABOUT);
       about_item.accept(selectability_setter(about_state));
    }

    void operator()(command_item&)
    {
        BOOST_THROW_EXCEPTION(
            logic_error("Cannot insert into command item"));
    }

    void operator()(separator_item&)
    {
        BOOST_THROW_EXCEPTION(
            logic_error("Cannot insert into separator"));
    }

private:
    optional<window<wchar_t>> m_view;
    apidl_t m_folder;
    com_ptr<IDataObject> m_selection;
    UINT m_first_command_id;
};

}

void menu_command_manager::update_state(com_ptr<IDataObject> selection)
{
    if (!m_tools_menu)
        BOOST_THROW_EXCEPTION(logic_error("Missing menu"));

    m_tools_menu->accept(
        update_command_items(m_view, m_folder, selection, m_first_command_id));
}

}}
