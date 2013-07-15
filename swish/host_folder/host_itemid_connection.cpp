/**
    @file

    Relates host item IDs to SFTP connections.

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

#include "host_itemid_connection.hpp"

#include "swish/connection/connection_spec.hpp"
#include "swish/connection/session_pool.hpp"
#include "swish/host_folder/host_pidl.hpp" // find_host_itemid, host_itemid_view

using swish::connection::connection_spec;
using swish::connection::session_pool;
using swish::host_folder::host_itemid_view;

namespace swish {
namespace host_folder {

connection_spec connection_from_host_itemid(const host_itemid_view& host_itemid)
{
    return connection_spec(
        host_itemid.host(), host_itemid.user(), host_itemid.port());
}

}} // namespace swish::host_folder
