/**
    @file

    Relates PIDLs to SFTP connections.

    @if license

    Copyright (C) 2007, 2008, 2009, 2010, 2011, 2013
    Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#ifndef SWISH_REMOTE_FOLDER_PIDL_CONNECTION_HPP
#define SWISH_REMOTE_FOLDER_PIDL_CONNECTION_HPP
#pragma once

#include "swish/provider/sftp_provider.hpp" // sftp_provider

#include <winapi/shell/pidl.hpp> // apidl_t

#include <boost/shared_ptr.hpp>


namespace swish {
namespace remote_folder {

/**
 * Creates an SFTP connection.
 *
 * The connection is created from the information stored in this
 * folder's PIDL, @a pidl, and the window handle to be used as the owner
 * window for any user interaction. This window handle can be NULL but (in order
 * to enforce good UI etiquette - we shouldn't attempt to interact with the user
 * if Explorer isn't expecting us to) any operation which requires user 
 * interaction should quietly fail.  
 *
 * @param hwndUserInteraction  A handle to the window which should be used
 *                             as the parent window for any user interaction.
 * @throws ATL exceptions on failure.
 */
boost::shared_ptr<swish::provider::sftp_provider> connection_from_pidl(
    const winapi::shell::pidl::apidl_t& pidl, HWND hwnd);

}} // namespace swish::remote_folder

#endif
