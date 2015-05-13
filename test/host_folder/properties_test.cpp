/**
    @file

    Exercise host-folder properties.

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

#include <swish/host_folder/properties.hpp> // test subject
#include <swish/host_folder/host_pidl.hpp> // create_host_itemid

#include <washer/shell/property_key.hpp> // property_key

#include <boost/test/unit_test.hpp>

#include <string>

#include <Propkey.h> // PKEY_ *

using swish::host_folder::compare_pidls_by_property;
using swish::host_folder::create_host_itemid;
using swish::host_folder::property_from_pidl;

using washer::shell::pidl::cpidl_t;
using washer::shell::property_key;

using comet::variant_t;
using std::string;

BOOST_AUTO_TEST_SUITE(properties_tests)

namespace {

    cpidl_t gimme_pidl()
    {
        return create_host_itemid(
            L"myhost", L"bobuser", L"/home/bobuser", 25, L"My Label");
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
    string prop = property_from_pidl(
        gimme_pidl(), swish::host_folder::PKEY_SwishHostUser);
    BOOST_CHECK_EQUAL(prop, "bobuser");
}

BOOST_AUTO_TEST_CASE( prop_port )
{
    string prop = property_from_pidl(
        gimme_pidl(), swish::host_folder::PKEY_SwishHostPort);
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

namespace {
    
    cpidl_t comp_pidl()
    {
        return create_host_itemid(
            L"myhost", L"boxuser", L"/home/aobuser", 24, L"Your Label");
            //   ==        >               <         <     >
    }

    int compare(const property_key& key)
    {
        return compare_pidls_by_property(gimme_pidl(), comp_pidl(), key);
    }
}

BOOST_AUTO_TEST_CASE( comp_label )
{
    int res = compare(PKEY_ItemNameDisplay);
    BOOST_CHECK_LT(res, 0);
}

BOOST_AUTO_TEST_CASE( comp_host )
{
    int res = compare(PKEY_ComputerName);
    BOOST_CHECK_EQUAL(res, 0);
}

BOOST_AUTO_TEST_CASE( comp_user )
{
    int res = compare(swish::host_folder::PKEY_SwishHostUser);
    BOOST_CHECK_LT(res, 0);
}

BOOST_AUTO_TEST_CASE( comp_port )
{
    int res = compare(swish::host_folder::PKEY_SwishHostPort);
    BOOST_CHECK_GT(res, 0);
}

BOOST_AUTO_TEST_CASE( comp_path )
{
    int res = compare(PKEY_ItemPathDisplay);
    BOOST_CHECK_GT(res, 0);
}

BOOST_AUTO_TEST_CASE( comp_type )
{
    int res = compare(PKEY_ItemType);
    BOOST_CHECK_EQUAL(res, 0);
}

BOOST_AUTO_TEST_SUITE_END()
