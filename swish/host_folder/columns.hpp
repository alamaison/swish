/**
    @file

    Host folder detail columns.

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

#ifndef SWISH_HOST_FOLDER_COLUMNS_HPP
#define SWISH_HOST_FOLDER_COLUMNS_HPP
#pragma once

#include <winapi/shell/pidl.hpp> // cpidl_t
#include <winapi/shell/property_key.hpp> // property_key

#include <comet/variant.h> // variant_t

namespace swish {
namespace host_folder {

const winapi::shell::property_key& property_key_from_column_index(
	size_t index);

SHELLDETAILS header_from_column_index(size_t index);

SHCOLSTATEF column_state_from_column_index(size_t index);

SHELLDETAILS detail_from_property_key(
	const winapi::shell::property_key& key,
	const winapi::shell::pidl::cpidl_t& pidl);

}} // namespace swish::host_folder

#endif
