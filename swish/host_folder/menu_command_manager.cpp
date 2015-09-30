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

#include "menu_command_manager.hpp"

#include "swish/frontend/commands/About.hpp"
#include "swish/host_folder/commands/Add.hpp"
#include "swish/host_folder/commands/CloseSession.hpp"
#include "swish/host_folder/commands/LaunchAgent.hpp"
#include "swish/host_folder/commands/Remove.hpp"
#include "swish/host_folder/commands/Rename.hpp"
#include "swish/nse/command_site.hpp"

#include <washer/gui/menu/basic_menu.hpp> // find_first_item_with_id
#include <washer/gui/menu/button/string_button_description.hpp>
#include <washer/gui/menu/item/command_item_description.hpp>
#include <washer/gui/menu/item/command_item.hpp>
#include <washer/gui/menu/item/item_state.hpp> // selectability
#include <washer/gui/menu/item/separator_item.hpp>
#include <washer/gui/menu/item/sub_menu_item.hpp>
#include <washer/trace.hpp> // trace

#include <boost/exception/diagnostic_information.hpp> // diagnostic_information
#include <boost/foreach.hpp> // BOOST_FOREACH
#include <boost/make_shared.hpp>
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <cassert> // assert
#include <stdexcept> // logic_error, runtime_error

using swish::frontend::commands::About;
using swish::host_folder::commands::Add;
using swish::host_folder::commands::CloseSession;
using swish::host_folder::commands::LaunchAgent;
using swish::host_folder::commands::Remove;
using swish::host_folder::commands::Rename;
using swish::nse::Command;
using swish::nse::command_site;

using namespace washer::gui::menu;
using washer::shell::pidl::apidl_t;
using washer::trace;
using washer::window::window;

using comet::com_ptr;

using boost::diagnostic_information;
using boost::make_shared;
using boost::optional;
using boost::shared_ptr;

using std::logic_error;
using std::map;
using std::runtime_error;
using std::wstring;

namespace swish {
namespace host_folder {

namespace {

typedef std::map<UINT, boost::shared_ptr<swish::nse::Command>>
    menu_id_command_map;

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

void merge_command_items(
    UINT first_command_id, const UINT max_command_id,
    menu destination, menu::iterator insert_position, 
    const menu_id_command_map& commands)
{
    typedef menu_id_command_map::iterator::value_type mapped_command;

    BOOST_FOREACH(const mapped_command& menu_command, commands)
    {
        UINT new_command_id = first_command_id + menu_command.first;
        if (new_command_id > max_command_id)
            BOOST_THROW_EXCEPTION(
                runtime_error("Exeeded permitted merge space"));

        command_item_description item(
            string_button_description(
                menu_command.second->menu_title(NULL)),
            new_command_id);
        
        // TODO: work out how to hide hidden() items.  For the moment we
        // treat them the same as disabled().
        // I don't know how to insert a hidden menu item, but Windows Forms
        // seems to allow it.  Maybe they maintain a list of menu items
        // separate from the menu itself and insert/remove the item to
        // show/hide it.

        BOOST_SCOPED_ENUM(selectability) item_state =
            menu_command.second->state(NULL, false) == Command::state::enabled ?
                selectability::enabled : selectability::disabled;

        item.selectability(item_state);

        // We have to be careful to increment the iterator *after* calling
        // insert in case we are inserting at the end.  Doing 
        // insert_position++ in the call to insert would step off the end.

        destination.insert(item, insert_position);
        ++insert_position;
    }
}

class merge_tools_command_items
{
public:

    typedef void result_type;

    merge_tools_command_items(
        UINT first_command_id, const UINT max_command_id,
        const menu_id_command_map& commands)
        :
    m_first_command_id(first_command_id),
    m_max_command_id(max_command_id), m_commands(commands) {}

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

        merge_command_items(
            m_first_command_id, m_max_command_id, sub_menu.menu(),
            insert_position, m_commands);
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
    UINT m_first_command_id;
    const UINT m_max_command_id;
    menu_id_command_map m_commands;
};

class merge_help_command_items
{
public:

    typedef void result_type;

    merge_help_command_items(
        UINT first_command_id, const UINT max_command_id,
        const menu_id_command_map& commands)
        :
    m_first_command_id(first_command_id),
    m_max_command_id(max_command_id), m_commands(commands) {}

    void operator()(sub_menu_item& sub_menu)
    {
        // Inserting into the bottom of the menu
        menu::iterator insert_position = sub_menu.menu().end();

        merge_command_items(
            m_first_command_id, m_max_command_id, sub_menu.menu(),
            insert_position, m_commands);
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
    UINT m_first_command_id;
    const UINT m_max_command_id;
    menu_id_command_map m_commands;
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

    menu_id_command_map tools_menu_commands;

    UINT offset = 0;
    tools_menu_commands[offset++] = make_shared<Add>(m_folder);
    tools_menu_commands[offset++] = make_shared<Remove>(m_folder);
    tools_menu_commands[offset++] = make_shared<Rename>();
    tools_menu_commands[offset++] = make_shared<CloseSession>();
    tools_menu_commands[offset++] = make_shared<LaunchAgent>(m_folder);

    // Try to get a handle to the Explorer Tools menu and insert
    // add and remove connection menu items into it if we find it
    m_tools_menu = tools_menu_with_fallback(
        menu_handle::foster_handle(menu_info.hmenu));
    m_tools_menu->accept(
        merge_tools_command_items(
            m_first_command_id, menu_info.idCmdLast, tools_menu_commands));

    menu_id_command_map help_menu_commands;
    help_menu_commands[offset++] = make_shared<About>();

    // Try to get a handle to the Explorer Help menu and insert About box
    m_help_menu = help_menu_with_fallback(
        menu_handle::foster_handle(menu_info.hmenu));
    m_help_menu->accept(
        merge_help_command_items(
            m_first_command_id, menu_info.idCmdLast, help_menu_commands));

    m_commands.insert(tools_menu_commands.begin(), tools_menu_commands.end());
    m_commands.insert(help_menu_commands.begin(), help_menu_commands.end());

    // Return value of last menu ID plus 1
    // The following works because maps are sorted so rbegin points to the
    // last and highest item
    if (m_commands.rbegin() != m_commands.rend())
    {
        menu_info.idCmdFirst += m_commands.rbegin()->first + 1;
    }
    
    // if no commands were added, leave idCmdFirst alone
}

bool menu_command_manager::invoke(
    UINT command_id, com_ptr<IShellItemArray> selection,
    com_ptr<IUnknown> ole_site)
{
    menu_id_command_map::iterator pos = m_commands.find(command_id);
    if (pos != m_commands.end())
    {
        // Use given window as a UI owner fallback in case the SFV callback
        // object was get an OLE site set
        (*(pos->second))(selection, command_site(ole_site, m_view), NULL);
        return true;
    }
    else
    {
        return false;
    }
}

bool menu_command_manager::help_text(
    UINT command_id, wstring& text_out, com_ptr<IShellItemArray> selection)
{
    menu_id_command_map::iterator pos = m_commands.find(command_id);
    if (pos != m_commands.end())
    {
        text_out = pos->second->tool_tip(selection);
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
        com_ptr<IShellItemArray> selection, UINT first_command_id,
        const menu_id_command_map& commands)
        :
    m_selection(selection), m_first_command_id(first_command_id),
    m_commands(commands) {}

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
        typedef menu_id_command_map::iterator::value_type mapped_command;

        BOOST_FOREACH(const mapped_command& menu_command, m_commands)
        {
            BOOST_SCOPED_ENUM(selectability) command_state =
                menu_command.second->state(m_selection, false)
                == Command::state::enabled ?
                    selectability::enabled : selectability::disabled;

            item menu_item = item_from_menu(
                sub_menu.menu(), m_first_command_id + menu_command.first);
            menu_item.accept(selectability_setter(command_state));
        }
    }

    void operator()(command_item&)
    {
        BOOST_THROW_EXCEPTION(logic_error("Cannot insert into command item"));
    }

    void operator()(separator_item&)
    {
        BOOST_THROW_EXCEPTION(logic_error("Cannot insert into separator"));
    }

private:
    com_ptr<IShellItemArray> m_selection;
    UINT m_first_command_id;
    menu_id_command_map m_commands;
};

}

void menu_command_manager::update_state(com_ptr<IShellItemArray> selection)
{
    if (!m_tools_menu)
        BOOST_THROW_EXCEPTION(logic_error("Missing menu"));

    m_tools_menu->accept(
        update_command_items(selection, m_first_command_id, m_commands));
}

}}
