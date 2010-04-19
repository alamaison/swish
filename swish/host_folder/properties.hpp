/**
    @file

    Properties available for items in a host folder.

    @if licence

    Copyright (C) 2010  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#ifndef SWISH_HOST_FOLDER_PROPERTIES_HPP
#define SWISH_HOST_FOLDER_PROPERTIES_HPP
#pragma once

#include <winapi/shell/pidl.hpp> // cpidl_t
#include <winapi/shell/property_key.hpp> // property_key

#include <comet/variant.h> // variant_t

#include <Propkey.h> // DEFINE_PROPERTYKEY

namespace swish {
namespace host_folder {

comet::variant_t property_from_pidl(
	const winapi::shell::pidl::cpidl_t& pidl,
	const winapi::shell::property_key& key);

}} // namespace swish::host_folder

// PKEYs for custom swish details/properties
// Swish Host FMTID GUID {b816a850-5022-11dc-9153-0090f5284f85}
DEFINE_PROPERTYKEY(PKEY_SwishHostUser, 0xb816a850, 0x5022, 0x11dc, \
                   0x91, 0x53, 0x00, 0x90, 0xf5, 0x28, 0x4f, 0x85, \
                   PID_FIRST_USABLE);
DEFINE_PROPERTYKEY(PKEY_SwishHostPort, 0xb816a850, 0x5022, 0x11dc, \
                   0x91, 0x53, 0x00, 0x90, 0xf5, 0x28, 0x4f, 0x85, \
                   PID_FIRST_USABLE + 1);

#endif
