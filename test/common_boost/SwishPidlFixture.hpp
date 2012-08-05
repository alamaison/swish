/**
    @file

    Fixture for tests that need a complete Swish PIDL.

    @if license

    Copyright (C) 2012  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#ifndef TEST_COMMON_SWISH_PIDL_FIXTURE_HPP
#define TEST_COMMON_SWISH_PIDL_FIXTURE_HPP

#include <swish/host_folder/host_pidl.hpp> // create_host_itemid
#include <swish/remote_folder/remote_pidl.hpp> // create_remote_itemid

#include <comet/datetime.h> // datetime_t

#include <winapi/shell/pidl.hpp> // apidl_t, cpidl_t
#include <winapi/shell/shell.hpp> // pidl_from_parsing_name

#include <string>

namespace test {

class SwishPidlFixture
{
public:

    /**
     * Return the PIDL to the Swish HostFolder in Explorer.
     */
    winapi::shell::pidl::apidl_t swish_pidl()
    {
        return winapi::shell::pidl_from_parsing_name(
            L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\"
            L"::{B816A83A-5022-11DC-9153-0090F5284F85}");
    }

    winapi::shell::pidl::cpidl_t create_dummy_remote_itemid(
        const std::wstring& filename, bool is_folder)
    {
        return swish::remote_folder::create_remote_itemid(
            filename, is_folder, false, L"bobuser", L"bob's group", 1001, 65535,
            040666, 18446744073709551615,
            comet::datetime_t(1970, 11, 1, 9, 15, 42, 6),
            comet::datetime_t((DATE)0));
    }

    /**
     * Get an absolute PIDL that ends in a HOSTPIDL to root RemoteFolder on.
     */
    winapi::shell::pidl::apidl_t create_dummy_root_host_pidl()
    {
        return swish_pidl() + swish::host_folder::create_host_itemid(
            L"test.example.com", L"user", L"/tmp", 22, L"Test PIDL");
    }
    
    /**
     * Get an absolute PIDL that ends in a REMOTEPIDL to root RemoteFolder on.
     */
    winapi::shell::pidl::apidl_t create_dummy_root_pidl()
    {
        return create_dummy_root_host_pidl() + create_dummy_remote_itemid(
            L"swish", true);
        // Some (older) tests rely on the name being "swish" here
    }
};

}

#endif