/**
    @file

    Tests the connection_spec class.

    @if license

    Copyright (C) 2013  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "swish/connection/connection_spec.hpp"  // Test subject
#include "swish/utils.hpp"

#include "test/common_boost/helpers.hpp"
#include "test/common_boost/fixtures.hpp"
#include "test/common_boost/ConsumerStub.hpp"

#include <comet/ptr.h>  // com_ptr

#include <boost/test/unit_test.hpp>
#include <boost/shared_ptr.hpp>

#include <exception>
#include <map>

using swish::provider::sftp_provider;
using swish::connection::connection_spec;
using swish::utils::Utf8StringToWideString;

using test::OpenSshFixture;
using test::CConsumerStub;

using comet::com_ptr;

using boost::shared_ptr;
using boost::test_tools::predicate_result;

using std::exception;
using std::map;

namespace {

    class ConnectionSpecFixture : public OpenSshFixture
    {
    public:

        connection_spec get_connection()
        {
            return connection_spec(
                Utf8StringToWideString(GetHost()), 
                Utf8StringToWideString(GetUser()), GetPort());
        }

        com_ptr<ISftpConsumer> Consumer()
        {
            com_ptr<CConsumerStub> consumer = new CConsumerStub(
                PrivateKeyPath(), PublicKeyPath());
            return consumer;
        }

        /**
         * Check that the given provider responds sensibly to a request.
         */
        predicate_result alive(shared_ptr<sftp_provider> provider)
        {
            try
            {
                provider->listing(Consumer(), L"/");

                predicate_result res(true);
                res.message() << "Provider seems to be alive";
                return res;
            }
            catch(const exception& e)
            {
                predicate_result res(false);
                res.message() << "Provider seems to be dead: " << e.what();
                return res;
            }
        }

    };
}


BOOST_FIXTURE_TEST_SUITE(
    connection_spec_session_create, ConnectionSpecFixture)

BOOST_AUTO_TEST_CASE( create )
{
    shared_ptr<sftp_provider> provider = get_connection().create_session();
    BOOST_CHECK(alive(provider));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(connection_spec_comparison)

BOOST_AUTO_TEST_CASE( self )
{
    connection_spec s(L"A",L"b",12);
    BOOST_CHECK(!(s < s));
}

BOOST_AUTO_TEST_CASE( equal )
{
    connection_spec s1(L"A",L"b",12);
    connection_spec s2(L"A",L"b",12);
    BOOST_CHECK(!(s1 < s2));
    BOOST_CHECK(!(s2 < s1));
}

BOOST_AUTO_TEST_CASE( less_host )
{
    connection_spec s1(L"A",L"b",12);
    connection_spec s2(L"B",L"b",12);
    BOOST_CHECK(s1 < s2);
    BOOST_CHECK(!(s2 < s1));
}

BOOST_AUTO_TEST_CASE( equal_host_less_user )
{
    connection_spec s1(L"A",L"a",12);
    connection_spec s2(L"A",L"b",12);
    BOOST_CHECK(s1 < s2);
    BOOST_CHECK(!(s2 < s1));
}

BOOST_AUTO_TEST_CASE( greater_host_less_user )
{
    connection_spec s1(L"B",L"a",12);
    connection_spec s2(L"A",L"b",12);
    BOOST_CHECK(!(s1 < s2));
    BOOST_CHECK(s2 < s1);
}

BOOST_AUTO_TEST_CASE( equal_host_equal_user_less_port )
{
    connection_spec s1(L"A",L"b",11);
    connection_spec s2(L"A",L"b",12);
    BOOST_CHECK(s1 < s2);
    BOOST_CHECK(!(s2 < s1));
}

BOOST_AUTO_TEST_CASE( equal_host_greater_user_less_port )
{
    connection_spec s1(L"A",L"c",11);
    connection_spec s2(L"A",L"b",12);
    BOOST_CHECK(!(s1 < s2));
    BOOST_CHECK(s2 < s1);
}

BOOST_AUTO_TEST_CASE( use_as_map_key_same )
{
    connection_spec s1(L"A",L"b",12);
    connection_spec s2(L"A",L"b",12);
    map<connection_spec, int> m;
    m[s1] = 3;
    m[s2] = 7;
    BOOST_CHECK_EQUAL(m.size(), 1);
    BOOST_CHECK_EQUAL(m[s1], 7);
    BOOST_CHECK_EQUAL(m[s2], 7);
}

BOOST_AUTO_TEST_CASE( use_as_map_key_different_user )
{
    connection_spec s1(L"A",L"b",12);
    connection_spec s2(L"A",L"a",12);
    map<connection_spec, int> m;
    m[s1] = 3;
    m[s2] = 7;
    BOOST_CHECK_EQUAL(m.size(), 2);
    BOOST_CHECK_EQUAL(m[s1], 3);
    BOOST_CHECK_EQUAL(m[s2], 7);
}

BOOST_AUTO_TEST_SUITE_END()

