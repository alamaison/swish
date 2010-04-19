/**
    @file

    Exercise host-folder properties.

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

#include <swish/host_folder/properties.hpp> // test subject

#include <swish/shell_folder/HostPidl.h> // CHostItem

#include <boost/test/unit_test.hpp>

#include <string>

#include <Propkey.h> // PKEY_ *

using swish::host_folder::property_from_pidl;

using comet::variant_t;
using std::string;

BOOST_AUTO_TEST_SUITE(properties_tests)

namespace {

	CHostItem gimme_pidl()
	{
		return CHostItem(
			L"bobuser", L"myhost", L"/home/bobuser", 25, L"My Label");
	}
}

BOOST_AUTO_TEST_CASE( prop_label )
{
	string prop = property_from_pidl(gimme_pidl(), PKEY_ItemNameDisplay);
	BOOST_CHECK_EQUAL(prop, "My Label");
}

BOOST_AUTO_TEST_CASE( prop_host )
{
	string prop = property_from_pidl(gimme_pidl(), PKEY_ComputerName);
	BOOST_CHECK_EQUAL(prop, "myhost");
}

BOOST_AUTO_TEST_CASE( prop_user )
{
	string prop = property_from_pidl(gimme_pidl(), PKEY_SwishHostUser);
	BOOST_CHECK_EQUAL(prop, "bobuser");
}

BOOST_AUTO_TEST_CASE( prop_port )
{
	string prop = property_from_pidl(gimme_pidl(), PKEY_SwishHostPort);
	BOOST_CHECK_EQUAL(prop, "25");
}

BOOST_AUTO_TEST_CASE( prop_path )
{
	string prop = property_from_pidl(gimme_pidl(), PKEY_ItemPathDisplay);
	BOOST_CHECK_EQUAL(prop, "/home/bobuser");
}

BOOST_AUTO_TEST_CASE( prop_type )
{
	string prop = property_from_pidl(gimme_pidl(), PKEY_ItemType);
	BOOST_CHECK_EQUAL(prop, "Network Drive");
}

BOOST_AUTO_TEST_SUITE_END()
