/* HostFolder context menu implementation.

   Copyright (C) 2015  Alexander Lamaison <swish@lammy.co.uk>

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

#ifndef SWISH_HOST_FOLDER_CONTEXT_MENU_CALLBACK_HPP
#define SWISH_HOST_FOLDER_CONTEXT_MENU_CALLBACK_HPP

#include "swish/nse/default_context_menu_callback.hpp"
                                               // default_context_menu_callback

#include <washer/shell/pidl.hpp> // apidl_t

#include <comet/ptr.h> // com_ptr

#include <string>

#include <ObjIdl.h> // IDataObject
#include <ShObjIdl.h> // CMINVOKECOMMANDINFO
#include <Windows.h> // HWND

namespace swish {
namespace host_folder {

class context_menu_callback : public swish::nse::default_context_menu_callback
{
public:
    context_menu_callback(const washer::shell::pidl::apidl_t& folder_pidl);

private:

    bool invoke_command(
        HWND hwnd_view, comet::com_ptr<IDataObject> selection, UINT item_offset,
        const std::wstring& arguments);

    bool invoke_command(
        HWND hwnd_view, comet::com_ptr<IDataObject> selection, UINT item_offset,
        const std::wstring& arguments, DWORD behaviour_flags, UINT minimum_id,
        UINT maximum_id, const CMINVOKECOMMANDINFO& invocation_details,
        comet::com_ptr<IUnknown> context_menu_site);

    washer::shell::pidl::apidl_t m_folder_pidl;
};

}} // namespace swish::host_folder

#endif
