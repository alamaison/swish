/**
    @file

    Management functions for host entries saved in the registry.

    @if license

    Copyright (C) 2009, 2011  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#ifndef SWISH_HOST_FOLDER_HOST_MANAGEMENT_HPP
#define SWISH_HOST_FOLDER_HOST_MANAGEMENT_HPP
#pragma once

#include <winapi/shell/pidl.hpp> // cpidl_t

#include <string>
#include <vector>

namespace swish {
namespace host_folder {
namespace host_management {

std::vector<winapi::shell::pidl::cpidl_t> LoadConnectionsFromRegistry();

void AddConnectionToRegistry(
    std::wstring label, std::wstring host, int port, 
    std::wstring username, std::wstring path);

void RemoveConnectionFromRegistry(std::wstring label);

bool ConnectionExists(std::wstring label);

}}} // namespace swish::host_folder::host_management

#endif
