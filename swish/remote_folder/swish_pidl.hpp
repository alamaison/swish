/**
    @file

    Operations over complete Swish PIDLs.

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

#ifndef SWISH_REMOTE_FOLDER_SWISH_PIDL_HPP
#define SWISH_REMOTE_FOLDER_SWISH_PIDL_HPP
#pragma once

#include "swish/host_folder/host_pidl.hpp" // find_host_itemid, host_itemid_view
#include "swish/remote_folder/remote_pidl.hpp" // remote_itemid_view

#include <washer/shell/pidl.hpp> // apidl_t
#include <washer/shell/pidl_iterator.hpp> // raw_pidl_iterator

#include <ssh/path.hpp>

namespace swish {
namespace remote_folder {

/**
 * Return the absolute path made by the items in this PIDL.
 * e.g. "/path/dir2/dir2/dir3/filename.ext"
 * The PIDL must contain a host itemid an after that can contain any number of
 * remote item ids, but doesn't have to.
 */
inline ssh::filesystem::path absolute_path_from_swish_pidl(
    const washer::shell::pidl::apidl_t& pidl)
{
    washer::shell::pidl::raw_pidl_iterator item_pos =
        swish::host_folder::find_host_itemid(pidl);

    ssh::filesystem::path path =
        swish::host_folder::host_itemid_view(*item_pos).path();

    if (++item_pos != washer::shell::pidl::raw_pidl_iterator())
    {
        if (remote_itemid_view(*item_pos).valid())
        {
            path /= path_from_remote_pidl(*item_pos);
        }
    }

    return path;
}

}} // namespace swish::remote_folder

#endif
