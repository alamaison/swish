/* Handler for remote folder's interaction with Explorer Shell Folder View.

   Copyright (C) 2008, 2009, 2010, 2011, 2012, 2013, 2015
   Alexander Lamaison <swish@lammy.co.uk>

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

#ifndef SWISH_REMOTE_FOLDER_VIEW_CALLBACK_HPP
#define SWISH_REMOTE_FOLDER_VIEW_CALLBACK_HPP
#pragma once

#include "swish/nse/view_callback.hpp" // CViewCallback

#include <washer/object_with_site.hpp> // object_with_site
#include <washer/shell/pidl.hpp> // apidl_t
#include <washer/window/window.hpp>

#include <comet/server.h> // simple_object

#include <boost/optional/optional.hpp> // optional

namespace swish {
namespace remote_folder {

class CViewCallback :
    public comet::simple_object<nse::CViewCallback, washer::object_with_site>
{
public:

    CViewCallback(const washer::shell::pidl::apidl_t& folder_pidl);

private:

    /// @name  SFVM_* message handlers
    // @{

    virtual bool on_window_created(HWND hwnd_view);
    virtual bool on_get_notify(PCIDLIST_ABSOLUTE& pidl_monitor, LONG& events);
    virtual bool on_fs_notify(PCIDLIST_ABSOLUTE pidl, LONG event);
    virtual bool on_get_webview_content(SFV_WEBVIEW_CONTENT_DATA& content_out);
    virtual bool on_get_webview_tasks(SFV_WEBVIEW_TASKSECTION_DATA& tasks_out);

    // @}

    boost::optional<washer::window::window<wchar_t>> m_owning_folder_view;
    washer::shell::pidl::apidl_t m_folder_pidl;
};

}} // namespace remote::host_folder

#endif
