/**
    @file

    Exercise remote PIDL.

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

#include <swish/remote_folder/remote_pidl.hpp> // test subject

#include "test/common_boost/helpers.hpp" // wide-char comparison

#include <comet/datetime.h> // datetime_t

#include <washer/shell/pidl.hpp> // apidl_t, cpidl_t
#include <washer/shell/shell.hpp> // pidl_from_parsing_name

#include <boost/test/unit_test.hpp>

#include <exception>
#include <stdexcept> // runtime_error
#include <string>

using swish::remote_folder::create_remote_itemid;
using swish::remote_folder::path_from_remote_pidl;
using swish::remote_folder::remote_itemid_view;

using washer::shell::pidl::apidl_t;
using washer::shell::pidl::cpidl_t;
using washer::shell::pidl::pidl_t;
using washer::shell::pidl_from_parsing_name;

using comet::com_ptr;
using comet::datetime_t;

using std::exception;
using std::runtime_error;
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

BOOST_AUTO_TEST_SUITE( remote_pidl_tests )

BOOST_AUTO_TEST_CASE( create_for_file )
{
    cpidl_t item = test_remote_itemid(L"testfile.txt", false);

    BOOST_REQUIRE(remote_itemid_view(item).valid());
    BOOST_CHECK_EQUAL(remote_itemid_view(item).filename(), L"testfile.txt");
    BOOST_CHECK(!remote_itemid_view(item).is_folder());
    BOOST_CHECK(!remote_itemid_view(item).is_link());
    BOOST_CHECK_EQUAL(remote_itemid_view(item).owner(), L"bobuser");
    BOOST_CHECK_EQUAL(remote_itemid_view(item).group(), L"bob's group");
    BOOST_CHECK_EQUAL(remote_itemid_view(item).owner_id(), 1001U);
    BOOST_CHECK_EQUAL(remote_itemid_view(item).group_id(), 65535U);
    BOOST_CHECK_EQUAL(remote_itemid_view(item).permissions(), 040666U);
    BOOST_CHECK_EQUAL(remote_itemid_view(item).size(), 18446744073709551615U);
    BOOST_CHECK_EQUAL(
        remote_itemid_view(item).date_modified(),
        datetime_t(1970, 11, 1, 9, 15, 42, 6));
    BOOST_CHECK_EQUAL(
        remote_itemid_view(item).date_accessed(), datetime_t());
    // repeat
    BOOST_CHECK_EQUAL(remote_itemid_view(item).filename(), L"testfile.txt");
}

BOOST_AUTO_TEST_CASE( create_for_file_from_raw )
{
    cpidl_t managed_pidl = test_remote_itemid(L"testfile.txt", false);
    PCITEMID_CHILD item = managed_pidl.get();

    BOOST_REQUIRE(remote_itemid_view(item).valid());
    BOOST_CHECK_EQUAL(remote_itemid_view(item).filename(), L"testfile.txt");
    BOOST_CHECK(!remote_itemid_view(item).is_folder());
    BOOST_CHECK(!remote_itemid_view(item).is_link());
    BOOST_CHECK_EQUAL(remote_itemid_view(item).owner(), L"bobuser");
    BOOST_CHECK_EQUAL(remote_itemid_view(item).group(), L"bob's group");
    BOOST_CHECK_EQUAL(remote_itemid_view(item).owner_id(), 1001U);
    BOOST_CHECK_EQUAL(remote_itemid_view(item).group_id(), 65535U);
    BOOST_CHECK_EQUAL(remote_itemid_view(item).permissions(), 040666U);
    BOOST_CHECK_EQUAL(remote_itemid_view(item).size(), 18446744073709551615U);
    BOOST_CHECK_EQUAL(
        remote_itemid_view(item).date_modified(),
        datetime_t(1970, 11, 1, 9, 15, 42, 6));
    BOOST_CHECK_EQUAL(
        remote_itemid_view(item).date_accessed(), datetime_t());
    // repeat
    BOOST_CHECK_EQUAL(remote_itemid_view(item).filename(), L"testfile.txt");
}

BOOST_AUTO_TEST_CASE( create_for_folder )
{
    cpidl_t item = test_remote_itemid(L"testfolder.txt", true);

    BOOST_REQUIRE(remote_itemid_view(item).valid());
    BOOST_CHECK_EQUAL(remote_itemid_view(item).filename(), L"testfolder.txt");
    BOOST_CHECK(remote_itemid_view(item).is_folder());
    BOOST_CHECK(!remote_itemid_view(item).is_link());
    BOOST_CHECK_EQUAL(remote_itemid_view(item).owner(), L"bobuser");
    BOOST_CHECK_EQUAL(remote_itemid_view(item).group(), L"bob's group");
    BOOST_CHECK_EQUAL(remote_itemid_view(item).owner_id(), 1001U);
    BOOST_CHECK_EQUAL(remote_itemid_view(item).group_id(), 65535U);
    BOOST_CHECK_EQUAL(remote_itemid_view(item).permissions(), 040666U);
    BOOST_CHECK_EQUAL(remote_itemid_view(item).size(), 18446744073709551615U);
    BOOST_CHECK_EQUAL(
        remote_itemid_view(item).date_modified(),
        datetime_t(1970, 11, 1, 9, 15, 42, 6));
    BOOST_CHECK_EQUAL(
        remote_itemid_view(item).date_accessed(), datetime_t());
    // repeat
    BOOST_CHECK_EQUAL(remote_itemid_view(item).filename(), L"testfolder.txt");
}

BOOST_AUTO_TEST_CASE( invalid_remote_item )
{
    apidl_t pidl = washer::shell::special_folder_pidl(CSIDL_DRIVES);

    BOOST_CHECK(!remote_itemid_view(pidl).valid());
    BOOST_CHECK_THROW(remote_itemid_view(pidl).filename(), exception);
    BOOST_CHECK_THROW(remote_itemid_view(pidl).is_folder(), exception);
    BOOST_CHECK_THROW(remote_itemid_view(pidl).is_link(), exception);
    BOOST_CHECK_THROW(remote_itemid_view(pidl).owner(), exception);
    BOOST_CHECK_THROW(remote_itemid_view(pidl).group(), exception);
    BOOST_CHECK_THROW(remote_itemid_view(pidl).owner_id(), exception);
    BOOST_CHECK_THROW(remote_itemid_view(pidl).group_id(), exception);
    BOOST_CHECK_THROW(remote_itemid_view(pidl).permissions(), exception);
    BOOST_CHECK_THROW(remote_itemid_view(pidl).size(), exception);
    BOOST_CHECK_THROW(remote_itemid_view(pidl).date_modified(), exception);
    BOOST_CHECK_THROW(remote_itemid_view(pidl).date_accessed(), exception);
    // repeat
    BOOST_CHECK_THROW(remote_itemid_view(pidl).filename(), exception);
}

BOOST_AUTO_TEST_CASE( build_path_from_remote_pidl )
{
    pidl_t pidl =
        test_remote_itemid(L"foo", true) + test_remote_itemid(L"bar", false) +
        test_remote_itemid(L"biscuit.txt", false);

    BOOST_CHECK_EQUAL(
        path_from_remote_pidl(pidl), "foo/bar/biscuit.txt");
}

BOOST_AUTO_TEST_CASE( build_path_from_remote_pidl_renders_expected_string )
{
    // The path may compare equal to the expected string without rendering
    // itself as the same string.  For example, the slashes might be backslashes
    // instead of forward slashes.  This causes problems down the line.
    pidl_t pidl =
        test_remote_itemid(L"foo", true) + test_remote_itemid(L"bar", false) +
        test_remote_itemid(L"biscuit.txt", false);

    BOOST_CHECK_EQUAL(
        path_from_remote_pidl(pidl).string(), "foo/bar/biscuit.txt");
}

BOOST_AUTO_TEST_CASE( build_path_from_remote_pidl_single )
{
    cpidl_t pidl = test_remote_itemid(L"foo", true);

    BOOST_CHECK_EQUAL(path_from_remote_pidl(pidl), L"foo");
}

BOOST_AUTO_TEST_CASE( build_path_from_remote_pidl_root )
{
    cpidl_t pidl = test_remote_itemid(L"/", true);

    BOOST_CHECK_EQUAL(path_from_remote_pidl(pidl), L"/");
}

BOOST_AUTO_TEST_SUITE_END()
