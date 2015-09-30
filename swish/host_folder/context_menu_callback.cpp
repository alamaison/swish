/* Copyright (C) 2015  Alexander Lamaison <swish@lammy.co.uk>

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

#include "swish/host_folder/commands/Remove.hpp"
#include "swish/host_folder/context_menu_callback.hpp" // context_menu_callback
#include "swish/nse/command_site.hpp"
#include "swish/shell/shell_item_array.hpp" // shell_item_array_from_data_object

#include <washer/window/window.hpp>

#include <boost/optional/optional.hpp>

#include <shlobj.h> // DFM_CMD_DELETE

using swish::nse::command_site;
using swish::shell::shell_item_array_from_data_object;

using washer::shell::pidl::apidl_t;
using washer::window::window;
using washer::window::window_handle;

using comet::com_ptr;

using boost::optional;

using std::wstring;

namespace swish {
namespace host_folder {

context_menu_callback::context_menu_callback(const apidl_t& folder_pidl)
: m_folder_pidl(folder_pidl) {}

namespace {

    bool do_invoke_command(
        const apidl_t& folder_pidl, HWND hwnd_view,
        com_ptr<IDataObject> selection_data_object, UINT item_offset,
        const wstring& /*arguments*/, int /*window_mode*/,
        com_ptr<IUnknown> context_menu_site)
    {
        if (item_offset == DFM_CMD_DELETE)
        {
            com_ptr<IShellItemArray> selection =
                shell_item_array_from_data_object(selection_data_object);

            // Use given window as a UI owner fallback because, if we compile
            // with pre-Vista support, the OLE site will always be NULL
            optional<window<wchar_t>> fallback_ui_owner;
            if (hwnd_view)
            {
                fallback_ui_owner = window<wchar_t>(
                    window_handle::foster_handle(hwnd_view));
            }

            commands::Remove deletion_command(folder_pidl);
            deletion_command(
                selection,
                command_site(context_menu_site, fallback_ui_owner), NULL);

            return true;
        }
        else
        {
            return false;
        }
    }

}

bool context_menu_callback::invoke_command(
    HWND hwnd_view, com_ptr<IDataObject> selection, UINT item_offset,
    const wstring& arguments)
{
    return do_invoke_command(
        m_folder_pidl, hwnd_view, selection, item_offset, arguments, SW_NORMAL,
        NULL);
}

/**
 * @todo  Take account of the behaviour flags.
 */
bool context_menu_callback::invoke_command(
    HWND hwnd_view, com_ptr<IDataObject> selection, UINT item_offset,
    const wstring& arguments, DWORD /*behaviour_flags*/, UINT /*minimum_id*/,
    UINT /*maximum_id*/, const CMINVOKECOMMANDINFO& invocation_details,
    com_ptr<IUnknown> context_menu_site)
{
    return do_invoke_command(
        m_folder_pidl, hwnd_view, selection, item_offset, arguments,
        invocation_details.nShow, context_menu_site);
}

}} // namespace swish::remote_folder
