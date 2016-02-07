// Copyright 2013, 2016 Alexander Lamaison

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "swish/connection/connection_spec.hpp" // Test subject

#include "test/common_boost/helpers.hpp"

#include <boost/test/unit_test.hpp>

#include <map>

using swish::connection::connection_spec;

using std::map;

BOOST_AUTO_TEST_SUITE(connection_spec_comparison)

BOOST_AUTO_TEST_CASE(self)
{
    connection_spec s(L"A", L"b", 12);
    BOOST_CHECK(!(s < s));
}

BOOST_AUTO_TEST_CASE(equal)
{
    connection_spec s1(L"A", L"b", 12);
    connection_spec s2(L"A", L"b", 12);
    BOOST_CHECK(!(s1 < s2));
    BOOST_CHECK(!(s2 < s1));
}

BOOST_AUTO_TEST_CASE(less_host)
{
    connection_spec s1(L"A", L"b", 12);
    connection_spec s2(L"B", L"b", 12);
    BOOST_CHECK(s1 < s2);
    BOOST_CHECK(!(s2 < s1));
}

BOOST_AUTO_TEST_CASE(equal_host_less_user)
{
    connection_spec s1(L"A", L"a", 12);
    connection_spec s2(L"A", L"b", 12);
    BOOST_CHECK(s1 < s2);
    BOOST_CHECK(!(s2 < s1));
}

BOOST_AUTO_TEST_CASE(greater_host_less_user)
{
    connection_spec s1(L"B", L"a", 12);
    connection_spec s2(L"A", L"b", 12);
    BOOST_CHECK(!(s1 < s2));
    BOOST_CHECK(s2 < s1);
}

BOOST_AUTO_TEST_CASE(equal_host_equal_user_less_port)
{
    connection_spec s1(L"A", L"b", 11);
    connection_spec s2(L"A", L"b", 12);
    BOOST_CHECK(s1 < s2);
    BOOST_CHECK(!(s2 < s1));
}

BOOST_AUTO_TEST_CASE(equal_host_greater_user_less_port)
{
    connection_spec s1(L"A", L"c", 11);
    connection_spec s2(L"A", L"b", 12);
    BOOST_CHECK(!(s1 < s2));
    BOOST_CHECK(s2 < s1);
}

BOOST_AUTO_TEST_CASE(use_as_map_key_same)
{
    connection_spec s1(L"A", L"b", 12);
    connection_spec s2(L"A", L"b", 12);
    map<connection_spec, int> m;
    m[s1] = 3;
    m[s2] = 7;
    BOOST_CHECK_EQUAL(m.size(), 1);
    BOOST_CHECK_EQUAL(m[s1], 7);
    BOOST_CHECK_EQUAL(m[s2], 7);
}

BOOST_AUTO_TEST_CASE(use_as_map_key_different_user)
{
    connection_spec s1(L"A", L"b", 12);
    connection_spec s2(L"A", L"a", 12);
    map<connection_spec, int> m;
    m[s1] = 3;
    m[s2] = 7;
    BOOST_CHECK_EQUAL(m.size(), 2);
    BOOST_CHECK_EQUAL(m[s1], 3);
    BOOST_CHECK_EQUAL(m[s2], 7);
}

BOOST_AUTO_TEST_SUITE_END()
