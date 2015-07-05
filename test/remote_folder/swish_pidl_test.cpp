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

#include "test/common_boost/helpers.hpp" // wide-char comparison
#include "test/common_boost/SwishPidlFixture.hpp"


#include <washer/shell/pidl.hpp> // apidl_t

#include <boost/test/unit_test.hpp>

using swish::host_folder::create_host_itemid;
using swish::remote_folder::absolute_path_from_swish_pidl;

using washer::shell::pidl::apidl_t;


BOOST_FIXTURE_TEST_SUITE( swish_pidl_tests, test::SwishPidlFixture )

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
    pidl += create_dummy_remote_itemid(L"foo", false);

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
    pidl += create_dummy_remote_itemid(L"foo", true);
    pidl += create_dummy_remote_itemid(L".bob", false);

    BOOST_CHECK_EQUAL(
        absolute_path_from_swish_pidl(pidl), L"/p/q/foo/.bob");
}

BOOST_AUTO_TEST_SUITE_END()
