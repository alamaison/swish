/**
    @file

    Host folder overlay icons.

    @if license

    Copyright (C) 2013  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#ifndef SWISH_HOST_FOLDER_OVERLAY_ICON_HPP
#define SWISH_HOST_FOLDER_OVERLAY_ICON_HPP

#include "swish/connection/connection_spec.hpp"
#include "swish/connection/session_pool.hpp"
#include "swish/host_folder/host_itemid_connection.hpp"
                                                  // connection_from_host_itemid

#include <winapi/shell/pidl.hpp> // cpidl_t

#include <shlobj.h> // SHGetIconOverlayIndex

namespace swish {
namespace host_folder {

class overlay_icon
{
public:
    overlay_icon(const winapi::shell::pidl::cpidl_t& item)
        :
    m_connection(connection_from_host_itemid(host_itemid_view(item)))
    {}

    bool has_overlay() const
    {
        return swish::connection::session_pool().has_session(m_connection);
    }

    int index() const
    {
        return ::SHGetIconOverlayIndexW(NULL, IDO_SHGIOI_DEFAULT);
    }

    int icon_index() const
    {
        return INDEXTOOVERLAYMASK(index());
    }

private:
    swish::connection::connection_spec m_connection;
};

}}

#endif
