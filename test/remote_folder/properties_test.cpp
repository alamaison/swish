/**
    @file

    Exercise remote-folder properties.

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

#include <swish/remote_folder/properties.hpp> // test subject
#include <swish/remote_folder/remote_pidl.hpp> // create_remote_itemid

#include <washer/shell/property_key.hpp> // property_key

#include <comet/datetime.h> // datetime_t

#include <boost/test/unit_test.hpp>

#include <string>

#include <Propkey.h> // PKEY_ *

using swish::remote_folder::compare_pidls_by_property;
using swish::remote_folder::create_remote_itemid;
using swish::remote_folder::property_from_pidl;

using washer::shell::pidl::cpidl_t;
using washer::shell::property_key;

using comet::variant_t;
using comet::datetime_t;

using std::string;

BOOST_AUTO_TEST_SUITE(properties_tests)

namespace {

    cpidl_t gimme_pidl()
    {
        return create_remote_itemid(
            L"some filename.txt", false, false, L"bobowner", L"mygroup",
            578, 1001, 0100666, 1024, datetime_t(2010, 1, 1, 12, 30, 17, 42),
            datetime_t(2010, 1, 1, 0, 0, 5, 7));
    }
}

BOOST_AUTO_TEST_CASE( prop_label )
{
    string prop = property_from_pidl(gimme_pidl(), PKEY_ItemNameDisplay);
    BOOST_CHECK_EQUAL(prop, "some filename.txt");
}

BOOST_AUTO_TEST_CASE( prop_owner )
{
    string prop = property_from_pidl(gimme_pidl(), PKEY_FileOwner);
    BOOST_CHECK_EQUAL(prop, "bobowner");
}

BOOST_AUTO_TEST_CASE( prop_group )
{
    string prop = property_from_pidl(
        gimme_pidl(), swish::remote_folder::PKEY_Group);
    BOOST_CHECK_EQUAL(prop, "mygroup");
}

BOOST_AUTO_TEST_CASE( prop_ownerid )
{
    int prop = property_from_pidl(
        gimme_pidl(), swish::remote_folder::PKEY_OwnerId);
    BOOST_CHECK_EQUAL(prop, 578);
}

BOOST_AUTO_TEST_CASE( prop_groupid )
{
    int prop = property_from_pidl(
        gimme_pidl(), swish::remote_folder::PKEY_GroupId);
    BOOST_CHECK_EQUAL(prop, 1001);
}

BOOST_AUTO_TEST_CASE( prop_perms )
{
    string prop = property_from_pidl(
        gimme_pidl(), swish::remote_folder::PKEY_Permissions);
    BOOST_CHECK_EQUAL(prop, "-rw-rw-rw-");
}

BOOST_AUTO_TEST_CASE( prop_size )
{
    ULONGLONG prop =
        property_from_pidl(gimme_pidl(), PKEY_Size).as_ulonglong();
    BOOST_CHECK_EQUAL(prop, 1024U);
}

BOOST_AUTO_TEST_CASE( prop_modified )
{
    datetime_t prop = property_from_pidl(gimme_pidl(), PKEY_DateModified);
    BOOST_CHECK_EQUAL(prop, datetime_t(2010, 1, 1, 12, 30, 17, 42));
}

BOOST_AUTO_TEST_CASE( prop_accessed )
{
    datetime_t prop = property_from_pidl(gimme_pidl(), PKEY_DateAccessed);
    BOOST_CHECK_EQUAL(prop, datetime_t(2010, 1, 1, 0, 0, 5, 7));
}

BOOST_AUTO_TEST_CASE( prop_type )
{
    string prop = property_from_pidl(gimme_pidl(), PKEY_ItemTypeText);
    BOOST_CHECK_EQUAL(prop, "Text Document");
}

namespace {
    
    cpidl_t comp_pidl()
    {
        return create_remote_itemid(
            L"sane filename.txt", false, false, L"booowner", L"mygroup",0, 1001,
            // <                                   >          ==        <  ==
            0100666, 1023,
            //  ==   <
            datetime_t(2010, 1, 1, 12, 30, 17, 43), // >
            datetime_t(2010, 1, 1, 0, 0, 5, 7)); // ==
    }

    int compare(const property_key& key)
    {
        return compare_pidls_by_property(gimme_pidl(), comp_pidl(), key);
    }
}

BOOST_AUTO_TEST_CASE( comp_label )
{
    int res = compare(PKEY_ItemNameDisplay);
    BOOST_CHECK_GT(res, 0);
}

BOOST_AUTO_TEST_CASE( comp_owner )
{
    int res = compare(PKEY_FileOwner);
    BOOST_CHECK_LT(res, 0);
}

BOOST_AUTO_TEST_CASE( comp_group )
{
    int res = compare(swish::remote_folder::PKEY_Group);
    BOOST_CHECK_EQUAL(res, 0);
}

BOOST_AUTO_TEST_CASE( comp_ownerid )
{
    int res = compare(swish::remote_folder::PKEY_OwnerId);
    BOOST_CHECK_GT(res, 0);
}

BOOST_AUTO_TEST_CASE( comp_groupid )
{
    int res = compare(swish::remote_folder::PKEY_GroupId);
    BOOST_CHECK_EQUAL(res, 0);
}

BOOST_AUTO_TEST_CASE( comp_perms )
{
    int res = compare(swish::remote_folder::PKEY_Permissions);
    BOOST_CHECK_EQUAL(res, 0);
}

BOOST_AUTO_TEST_CASE( comp_size )
{
    int res = compare(PKEY_Size);
    BOOST_CHECK_GT(res, 0);
}

BOOST_AUTO_TEST_CASE( comp_modified )
{
    int res = compare(PKEY_DateModified);
    BOOST_CHECK_LT(res, 0);
}

BOOST_AUTO_TEST_CASE( comp_accessed )
{
    int res = compare(PKEY_DateAccessed);
    BOOST_CHECK_EQUAL(res, 0);
}

BOOST_AUTO_TEST_CASE( comp_type )
{
    int res = compare(PKEY_ItemTypeText);
    BOOST_CHECK_EQUAL(res, 0);
}

BOOST_AUTO_TEST_SUITE_END()
