/**
    @file

    Exercise host-folder columns.

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

#include <swish/host_folder/columns.hpp> // test subject, Column
#include <swish/host_folder/host_pidl.hpp> // create_host_itemid

#include <test/common_boost/helpers.hpp> // wide-string output

#include <boost/test/unit_test.hpp>

#include <string>

using swish::host_folder::Column;
using swish::host_folder::create_host_itemid;

using winapi::shell::pidl::cpidl_t;

using std::wstring;

BOOST_AUTO_TEST_SUITE(column_tests)

namespace {

	cpidl_t gimme_pidl()
	{
		return create_host_itemid(
			L"myhost", L"bobuser", L"/home/bobuser", 25, L"My Label");
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
	BOOST_CHECK_EQUAL(detail(0), L"My Label");
}

BOOST_AUTO_TEST_CASE( host )
{
	BOOST_CHECK_EQUAL(header(1), L"Host");
	BOOST_CHECK_EQUAL(detail(1), L"myhost");
}

BOOST_AUTO_TEST_CASE( user )
{
	BOOST_CHECK_EQUAL(header(2), L"Username");
	BOOST_CHECK_EQUAL(detail(2), L"bobuser");
}

BOOST_AUTO_TEST_CASE( port )
{
	BOOST_CHECK_EQUAL(header(3), L"Port");
	BOOST_CHECK_EQUAL(detail(3), L"25");
}

BOOST_AUTO_TEST_CASE( path )
{
	BOOST_CHECK_EQUAL(header(4), L"Remote path");
	BOOST_CHECK_EQUAL(detail(4), L"/home/bobuser");
}

BOOST_AUTO_TEST_CASE( type )
{
	BOOST_CHECK_EQUAL(header(5), L"Type");
	BOOST_CHECK_EQUAL(detail(5), L"Network Drive");
}

/**
 * Get one header too far.
 */
BOOST_AUTO_TEST_CASE( out_of_bounds )
{
	BOOST_CHECK_THROW(header(6), std::exception);
}

BOOST_AUTO_TEST_SUITE_END()
