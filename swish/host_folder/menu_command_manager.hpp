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

#ifndef SWISH_HOST_FOLDER_MENU_COMMAND_MANAGER
#define SWISH_HOST_FOLDER_MENU_COMMAND_MANAGER

#include <winapi/shell/pidl.hpp> // apidl_t

#include <comet/ptr.h> // com_ptr

#include <string>

#include <ShlObj.h> // QCMINFO

namespace swish {
namespace host_folder {

class menu_command_manager
{
public:

    /**
     * Merge items into Explorer menus.
     */
    menu_command_manager(QCMINFO& menu_info);

    /**
     * Invoke a command by merge offset.
     */
    bool invoke(UINT command_id);

    /**
     * Request tool tip for command by merge offset.
     */
    bool help_text(UINT command_id, std::wstring& text_out);

    /**
     * Refresh command states to match current selection.
     */
    void update_state(
        HWND hwnd_view, const winapi::shell::pidl::apidl_t& folder_pidl,
        comet::com_ptr<IDataObject> selection);

private:
};

}}

#endif