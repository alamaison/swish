/**
    @file

    Exercise remote-folder columns.

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

#include <swish/remote_folder/columns.hpp> // test subject, Column
#include <swish/remote_folder/remote_pidl.hpp> // create_remote_itemid

#include <test/common_boost/helpers.hpp> // wide-string output

#include <winapi/shell/format.hpp> // format_date_time

#include <comet/datetime.h> // datetime_t

#include <boost/test/unit_test.hpp>

#include <string>

using swish::remote_folder::Column;
using swish::remote_folder::create_remote_itemid;

using winapi::shell::format_date_time;
using winapi::shell::pidl::cpidl_t;

using comet::datetime_t;

using std::wstring;

BOOST_AUTO_TEST_SUITE(column_tests)

namespace {

	cpidl_t gimme_pidl()
	{
		return create_remote_itemid(
			L"some filename.txt", false, false, L"bobowner", L"mygroup",
			578, 1001, 0100666, 1024, datetime_t(2010, 1, 1, 12, 30, 17, 42),
			datetime_t(2010, 1, 1, 0, 0, 5, 7));
	}

	wstring header(size_t index)
	{
		Column col(index);
		return col.header();
	}

	wstring detail(size_t index)
	{
		Column col(index);
		return col.detail(gimme_pidl());
	}
}


BOOST_AUTO_TEST_CASE( label )
{
	BOOST_CHECK_EQUAL(header(0), L"Name");
	BOOST_CHECK_EQUAL(detail(0), L"some filename.txt");
}

BOOST_AUTO_TEST_CASE( size )
{
	BOOST_CHECK_EQUAL(header(1), L"Size");
	BOOST_CHECK_EQUAL(detail(1), L"1 KB");
}

BOOST_AUTO_TEST_CASE( type )
{
	BOOST_CHECK_EQUAL(header(2), L"Type");
	BOOST_CHECK_EQUAL(detail(2), L"Text Document");
}

BOOST_AUTO_TEST_CASE( modified )
{
	BOOST_CHECK_EQUAL(header(3), L"Date Modified");
	BOOST_CHECK_EQUAL(
		detail(3),
		format_date_time<wchar_t>(datetime_t(2010, 1, 1, 12, 30, 17, 42)));
}

BOOST_AUTO_TEST_CASE( accessed )
{
	BOOST_CHECK_EQUAL(header(4), L"Date Accessed");
	BOOST_CHECK_EQUAL(
		detail(4),
		format_date_time<wchar_t>(datetime_t(2010, 1, 1, 0, 0, 5, 7)));
}

BOOST_AUTO_TEST_CASE( perms )
{
	BOOST_CHECK_EQUAL(header(5), L"Permissions");
	BOOST_CHECK_EQUAL(detail(5), L"-rw-rw-rw-");
}

BOOST_AUTO_TEST_CASE( owner )
{
	BOOST_CHECK_EQUAL(header(6), L"Owner");
	BOOST_CHECK_EQUAL(detail(6), L"bobowner");
}

BOOST_AUTO_TEST_CASE( group )
{
	BOOST_CHECK_EQUAL(header(7), L"Group");
	BOOST_CHECK_EQUAL(detail(7), L"mygroup");
}

BOOST_AUTO_TEST_CASE( ownerid )
{
	BOOST_CHECK_EQUAL(header(8), L"Owner ID");
	BOOST_CHECK_EQUAL(detail(8), L"578");
}

BOOST_AUTO_TEST_CASE( groupid )
{
	BOOST_CHECK_EQUAL(header(9), L"Group ID");
	BOOST_CHECK_EQUAL(detail(9), L"1001");
}

/**
 * Get one header too far.
 */
BOOST_AUTO_TEST_CASE( out_of_bounds )
{
	BOOST_CHECK_THROW(header(10), std::exception);
}

BOOST_AUTO_TEST_SUITE_END()
