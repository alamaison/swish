/**
    @file

    Exercise code that operates over complete Swish PIDLs.

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

#include <swish/remote_folder/swish_pidl.hpp> // test subject

#include <swish/host_folder/host_pidl.hpp> // create_host_itemid
#include <swish/remote_folder/remote_pidl.hpp> // create_remote_itemid

#include "test/common_boost/helpers.hpp" // wide-char comparison

#include <comet/datetime.h> // datetime_t

#include <winapi/shell/pidl.hpp> // apidl_t, cpidl_t
#include <winapi/shell/shell.hpp> // pidl_from_parsing_name

#include <boost/test/unit_test.hpp>

#include <string>

using swish::host_folder::create_host_itemid;
using swish::remote_folder::absolute_path_from_swish_pidl;
using swish::remote_folder::create_remote_itemid;

using winapi::shell::pidl::apidl_t;
using winapi::shell::pidl::cpidl_t;
using winapi::shell::pidl_from_parsing_name;

using comet::datetime_t;
using std::wstring;

namespace {

    /**
     * Return the PIDL to the Swish HostFolder in Explorer.
     */
    apidl_t swish_pidl()
    {
        return pidl_from_parsing_name(
            L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\"
            L"::{B816A83A-5022-11DC-9153-0090F5284F85}");
    }

    cpidl_t test_remote_itemid(const wstring& filename, bool is_folder)
    {
        return create_remote_itemid(
            filename, is_folder, false, L"bobuser", L"bob's group", 1001, 65535,
            040666, 18446744073709551615, datetime_t(1970, 11, 1, 9, 15, 42, 6),
            datetime_t((DATE)0));
    }
}

BOOST_AUTO_TEST_SUITE( swish_pidl_tests )

/**
 * Test that a Swish PIDL ending in just a host itemid results in the
 * correct path.
 *
 * @TODO: test with remote itemids as well.
 */
BOOST_AUTO_TEST_CASE( pidl_to_absolute_path_host_item_only )
{
    apidl_t pidl = swish_pidl() + create_host_itemid(
        L"host.example.com", L"bobuser", L"/p/q", 22);

    BOOST_CHECK_EQUAL(
        absolute_path_from_swish_pidl(pidl), L"/p/q");
}

/**
 * Test path extraction for a Swish PIDL containing a remote item id.
 */
BOOST_AUTO_TEST_CASE( pidl_to_absolute_path )
{
    apidl_t pidl = swish_pidl() + create_host_itemid(
        L"host.example.com", L"bobuser", L"/p/q", 22);
    pidl += test_remote_itemid(L"foo", false);

    BOOST_CHECK_EQUAL(
        absolute_path_from_swish_pidl(pidl), L"/p/q/foo");
}

/**
 * Test path extraction for a Swish PIDL containing two remote item ids.
 */
BOOST_AUTO_TEST_CASE( pidl_to_absolute_path_multiple_remote_items )
{
    apidl_t pidl = swish_pidl() + create_host_itemid(
        L"host.example.com", L"bobuser", L"/p/q", 22);
    pidl += test_remote_itemid(L"foo", true);
    pidl += test_remote_itemid(L".bob", false);

    BOOST_CHECK_EQUAL(
        absolute_path_from_swish_pidl(pidl), L"/p/q/foo/.bob");
}

BOOST_AUTO_TEST_SUITE_END()
