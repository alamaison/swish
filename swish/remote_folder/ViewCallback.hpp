/**
    @file

    Handler for remote folder's interaction with Explorer Shell Folder View.

    @if license

    Copyright (C) 2011  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#ifndef SWISH_REMOTE_FOLDER_VIEW_CALLBACK_HPP
#define SWISH_REMOTE_FOLDER_VIEW_CALLBACK_HPP
#pragma once

#include "swish/nse/view_callback.hpp" // CViewCallback

#include <winapi/object_with_site.hpp> // object_with_site
#include <winapi/shell/pidl.hpp> // apidl_t

#include <comet/server.h> // simple_object

namespace swish {
namespace remote_folder {

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
    virtual bool on_get_webview_content(SFV_WEBVIEW_CONTENT_DATA& content_out);
    virtual bool on_get_webview_tasks(SFV_WEBVIEW_TASKSECTION_DATA& tasks_out);

    // @}

    HWND m_hwnd_view;         ///< Handle to folder view window
    winapi::shell::pidl::apidl_t m_folder_pidl; ///< Our copy of pidl to owning
                                                ///< folder
};

}} // namespace remote::host_folder

#endif
