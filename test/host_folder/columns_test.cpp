/**
    @file

    Exercise host-folder columns.

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

#include <swish/host_folder/columns.hpp> // test subject

#include <swish/host_folder/properties.hpp> // PKEY_Swish*
#include <swish/shell_folder/HostPidl.h> // CHostItem

#include <winapi/shell/shell.hpp> // strret_to_string

#include <boost/test/unit_test.hpp>

#include <string>

#include <Propkey.h> // PKEY_ *

using swish::host_folder::detail_from_property_key;
using swish::host_folder::header_from_column_index;

using winapi::shell::strret_to_string;

using comet::variant_t;

using std::string;

BOOST_AUTO_TEST_SUITE(column_tests)

namespace {

	CHostItem gimme_pidl()
	{
		return CHostItem(
			L"bobuser", L"myhost", L"/home/bobuser", 25, L"My Label");
	}
}

/**
 * Get first and last header.
 */
BOOST_AUTO_TEST_CASE( headers )
{
	SHELLDETAILS sd = header_from_column_index(0);
	// this should free SHELLDETAILS
	string header = strret_to_string<char>(sd.str);

	BOOST_CHECK_EQUAL(header, "Name");

	sd = header_from_column_index(5); // update this index if columns change
	header = strret_to_string<char>(sd.str);

	BOOST_CHECK_EQUAL(header, "Type");
}

/**
 * Get one header too far.
 */
BOOST_AUTO_TEST_CASE( header_out_of_bounds )
{
	BOOST_CHECK_THROW(header_from_column_index(6), std::exception);
}

BOOST_AUTO_TEST_CASE( prop_label )
{
	SHELLDETAILS sd = detail_from_property_key(
		PKEY_ItemNameDisplay, gimme_pidl());
	string prop = strret_to_string<char>(sd.str);

	BOOST_CHECK_EQUAL(prop, "My Label");
}

BOOST_AUTO_TEST_CASE( prop_host )
{
	SHELLDETAILS sd = detail_from_property_key(
		PKEY_ComputerName, gimme_pidl());
	string prop = strret_to_string<char>(sd.str);

	BOOST_CHECK_EQUAL(prop, "myhost");
}

BOOST_AUTO_TEST_CASE( prop_user )
{
	SHELLDETAILS sd = detail_from_property_key(
		PKEY_SwishHostUser, gimme_pidl());
	string prop = strret_to_string<char>(sd.str);

	BOOST_CHECK_EQUAL(prop, "bobuser");
}

BOOST_AUTO_TEST_CASE( prop_port )
{
	SHELLDETAILS sd = detail_from_property_key(
		PKEY_SwishHostPort, gimme_pidl());
	string prop = strret_to_string<char>(sd.str);

	BOOST_CHECK_EQUAL(prop, "25");
}

BOOST_AUTO_TEST_CASE( prop_path )
{
	SHELLDETAILS sd = detail_from_property_key(
		PKEY_ItemPathDisplay, gimme_pidl());
	string prop = strret_to_string<char>(sd.str);

	BOOST_CHECK_EQUAL(prop, "/home/bobuser");
}

BOOST_AUTO_TEST_CASE( prop_type )
{
	SHELLDETAILS sd = detail_from_property_key(
		PKEY_ItemType, gimme_pidl());
	string prop = strret_to_string<char>(sd.str);

	BOOST_CHECK_EQUAL(prop, "Network Drive");
}

BOOST_AUTO_TEST_SUITE_END()
