/**
    @file

    Properties available for items in a host folder.

    @if license

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

#include <washer/shell/pidl.hpp> // cpidl_t
#include <washer/shell/property_key.hpp> // property_key

#include <comet/variant.h> // variant_t

#include <WTypes.h> // PROPERTYKEY

namespace swish {
namespace host_folder {

comet::variant_t property_from_pidl(
    const washer::shell::pidl::cpidl_t& pidl,
    const washer::shell::property_key& key);

int compare_pidls_by_property(
    const washer::shell::pidl::cpidl_t& pidl_left,
    const washer::shell::pidl::cpidl_t& pidl_right,
    const washer::shell::property_key& key);

/**
 * @name Custom properties (PKEYs) for Swish remote folder.
 *
 * Ideally, we want as few of these as possible.  If an appropriate
 * one already exists in propkey.h, that should be used instead.
 *
 * The Swish remote folder FMTID GUID which collects all the custom
 * properties together is @c {b816a851-5022-11dc-9153-0090f5284f85}.
 */
// @{
extern "C" const PROPERTYKEY PKEY_SwishHostUser;
extern "C" const PROPERTYKEY PKEY_SwishHostPort;
// @}

}} // namespace swish::host_folder

#endif
