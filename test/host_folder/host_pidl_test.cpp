/**
    @file

    Exercise host PIDL.

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

#include <swish/host_folder/host_pidl.hpp> // test subject

#include "test/common_boost/helpers.hpp" // wide-char comparison

#include <washer/shell/pidl.hpp> // apidl_t, cpidl_t
#include <washer/shell/shell.hpp> // pidl_from_parsing_name

#include <boost/test/unit_test.hpp>

#include <exception>
#include <stdexcept> // runtime_error
#include <string>

using swish::host_folder::create_host_itemid;
using swish::host_folder::host_itemid_view;
using swish::host_folder::find_host_itemid;
using swish::host_folder::url_from_host_itemid;

using washer::shell::pidl::apidl_t;
using washer::shell::pidl::cpidl_t;
using washer::shell::pidl::pidl_t;
using washer::shell::pidl_from_parsing_name;

using comet::com_ptr;

using std::exception;
using std::runtime_error;

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
}

BOOST_AUTO_TEST_SUITE( host_pidl_tests )

BOOST_AUTO_TEST_CASE( create )
{
    cpidl_t item = create_host_itemid(
        L"host.example.com", L"bobuser", L"/home/directory", 65535, L"My Label");

    BOOST_REQUIRE(host_itemid_view(item).valid());
    BOOST_CHECK_EQUAL(host_itemid_view(item).host(), L"host.example.com");
    BOOST_CHECK_EQUAL(host_itemid_view(item).user(), L"bobuser");
    BOOST_CHECK_EQUAL(host_itemid_view(item).path(), L"/home/directory");
    BOOST_CHECK_EQUAL(host_itemid_view(item).label(), L"My Label");
    BOOST_CHECK_EQUAL(host_itemid_view(item).port(), 65535);
    // repeat
    BOOST_CHECK_EQUAL(host_itemid_view(item).host(), L"host.example.com");
}

BOOST_AUTO_TEST_CASE( create_from_raw )
{
    cpidl_t managed_pidl = create_host_itemid(
        L"host.example.com", L"bobuser", L"/home/directory", 65535, L"My Label");
    PCITEMID_CHILD item = managed_pidl.get();

    BOOST_REQUIRE(host_itemid_view(item).valid());
    BOOST_CHECK_EQUAL(host_itemid_view(item).host(), L"host.example.com");
    BOOST_CHECK_EQUAL(host_itemid_view(item).user(), L"bobuser");
    BOOST_CHECK_EQUAL(host_itemid_view(item).path(), L"/home/directory");
    BOOST_CHECK_EQUAL(host_itemid_view(item).label(), L"My Label");
    BOOST_CHECK_EQUAL(host_itemid_view(item).port(), 65535);
    // repeat
    BOOST_CHECK_EQUAL(host_itemid_view(item).host(), L"host.example.com");
}

BOOST_AUTO_TEST_CASE( create_default_arg )
{
    cpidl_t item = create_host_itemid(
        L"host.example.com", L"bobuser", L"/home/directory", 65535);

    BOOST_REQUIRE(host_itemid_view(item).valid());
    BOOST_CHECK_EQUAL(host_itemid_view(item).host(), L"host.example.com");
    BOOST_CHECK_EQUAL(host_itemid_view(item).user(), L"bobuser");
    BOOST_CHECK_EQUAL(host_itemid_view(item).path(), L"/home/directory");
    BOOST_CHECK_EQUAL(host_itemid_view(item).label(), L"");
    BOOST_CHECK_EQUAL(host_itemid_view(item).port(), 65535);
    // repeat
    BOOST_CHECK_EQUAL(host_itemid_view(item).host(), L"host.example.com");
}

BOOST_AUTO_TEST_CASE( invalid_host_item )
{
    apidl_t pidl = washer::shell::special_folder_pidl(CSIDL_DRIVES);

    BOOST_CHECK(!host_itemid_view(pidl).valid());
    BOOST_CHECK_THROW(host_itemid_view(pidl).host(), exception);
    BOOST_CHECK_THROW(host_itemid_view(pidl).user(), exception);
    BOOST_CHECK_THROW(host_itemid_view(pidl).path(), exception);
    BOOST_CHECK_THROW(host_itemid_view(pidl).label(), exception);
    BOOST_CHECK_THROW(host_itemid_view(pidl).port(), exception);
    // repeat
    BOOST_CHECK_THROW(host_itemid_view(pidl).host(), exception);
}

BOOST_AUTO_TEST_CASE( find_host_item_in_pidl )
{
    apidl_t pidl = swish_pidl();
    cpidl_t item = create_host_itemid(
        L"host.example.com", L"bobuser", L"/", 65535);
    pidl += item;

    pidl_t result = *find_host_itemid(pidl);

    BOOST_REQUIRE(host_itemid_view(result).valid());
    BOOST_CHECK_EQUAL(host_itemid_view(result).host(), L"host.example.com");
    BOOST_CHECK_EQUAL(host_itemid_view(result).user(), L"bobuser");
    BOOST_CHECK_EQUAL(host_itemid_view(result).path(), L"/");
    BOOST_CHECK_EQUAL(host_itemid_view(result).label(), L"");
    BOOST_CHECK_EQUAL(host_itemid_view(result).port(), 65535);
}

BOOST_AUTO_TEST_CASE( fail_to_find_host_item_in_pidl )
{
    apidl_t pidl = swish_pidl();

    BOOST_CHECK_THROW(find_host_itemid(pidl), runtime_error);
}

BOOST_AUTO_TEST_CASE( hostitem_to_url )
{
    cpidl_t item = create_host_itemid(
        L"host.example.com", L"bobuser", L"/p", 65535);

    BOOST_CHECK_EQUAL(
        url_from_host_itemid(item, false),
        L"sftp://bobuser@host.example.com:65535//p");
}

BOOST_AUTO_TEST_CASE( hostitem_to_url_default_port )
{
    cpidl_t item = create_host_itemid(
        L"host.example.com", L"bobuser", L"/p", 22);

    BOOST_CHECK_EQUAL(
        url_from_host_itemid(item, false),
        L"sftp://bobuser@host.example.com//p");
}

BOOST_AUTO_TEST_CASE( hostitem_to_url_canonical )
{
    cpidl_t item = create_host_itemid(
        L"host.example.com", L"bobuser", L"/p", 65535);

    BOOST_CHECK_EQUAL(
        url_from_host_itemid(item, true),
        L"sftp://bobuser@host.example.com:65535//p");
}

BOOST_AUTO_TEST_CASE( hostitem_to_url_default_port_canonical )
{
    cpidl_t item = create_host_itemid(
        L"host.example.com", L"bobuser", L"/p", 22);

    BOOST_CHECK_EQUAL(
        url_from_host_itemid(item, true),
        L"sftp://bobuser@host.example.com:22//p");
}

BOOST_AUTO_TEST_SUITE_END()
