/**
    @file

    Testing Provider authentication.

    @if license

    Copyright (C) 2008, 2009, 2010, 2012, 2013
    Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "test/common_boost/helpers.hpp"
#include "test/common_boost/MockConsumer.hpp"
#include "test/common_boost/remote_test_config.hpp"

#include "swish/connection/connection_spec.hpp"
#include "swish/provider/Provider.hpp"

#include <comet/ptr.h> // com_ptr

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

#include <exception>
#include <string> // wstring

using test::MockConsumer;
using test::remote_test_config;

using swish::connection::connection_spec;
using swish::provider::CProvider;
using swish::provider::sftp_provider;

using comet::com_ptr;

using boost::make_shared;
using boost::shared_ptr;
using boost::test_tools::predicate_result;

using std::exception;
using std::wstring;

namespace {
    
    shared_ptr<sftp_provider> create_provider()
    {
        remote_test_config config;

        return make_shared<CProvider>(
            connection_spec(
                config.GetHost(), config.GetUser(), config.GetPort()));
    }

    /**
     * Check that the given provider responds sensibly to a request given
     * a particular consumer.
     *
     * This may mean that the provider wasn't authenticated but survived
     * an attempt to make it do something (presumably) by authenticating.
     */
    predicate_result alive(
        shared_ptr<sftp_provider> provider, com_ptr<ISftpConsumer> consumer)
    {
        try
        {
            provider->listing(consumer, L"/");

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
    
    /**
     * Check that the given provider responds sensibly to a request.
     */
    predicate_result alive(shared_ptr<sftp_provider> provider)
    {
        return alive(provider, new MockConsumer());
    }
}

// These tests use the host defined in the TEST_HOST_NAME, TEST_HOST_PORT,
// TEST_USER_NAME and TEST_PASSWORD environment variables.  This is necessary
// because our usual local OpenSSH server setup used for all the other tests
// can't test passwords as OpenSSH will always use a Windows user account
// and we can't get at those passwords.  

BOOST_AUTO_TEST_SUITE( provider_network_auth_tests )

BOOST_AUTO_TEST_CASE( SimplePasswordAuthentication )
{
    // Choose mock behaviours to force only simple password authentication
    com_ptr<MockConsumer> consumer = new MockConsumer();
    consumer->set_password_behaviour(MockConsumer::CustomPassword);
    consumer->set_keyboard_interactive_behaviour(MockConsumer::AbortResponse);
    consumer->set_pubkey_behaviour(MockConsumer::AbortKeys);

    remote_test_config config;
    consumer->set_password(config.GetPassword());

    shared_ptr<sftp_provider> provider = create_provider();

    BOOST_CHECK(alive(provider, consumer));
}

BOOST_AUTO_TEST_CASE( WrongPassword )
{
    com_ptr<MockConsumer> consumer = new MockConsumer();

    consumer->set_password_behaviour(MockConsumer::WrongPassword);

    shared_ptr<sftp_provider> provider = create_provider();

    BOOST_CHECK(!alive(provider, consumer));
}

BOOST_AUTO_TEST_CASE( KeyboardInteractiveAuthentication )
{
    // Choose mock behaviours to force only kbd-interactive authentication
    com_ptr<MockConsumer> consumer = new MockConsumer();
    consumer->set_password_behaviour(MockConsumer::FailPassword);
    consumer->set_pubkey_behaviour(MockConsumer::AbortKeys);
    consumer->set_keyboard_interactive_behaviour(MockConsumer::CustomResponse);

    remote_test_config config;
    consumer->set_password(config.GetPassword());

    shared_ptr<sftp_provider> provider = create_provider();

    // This may fail if the server (which we can't control) doesn't allow
    // ki-auth
    BOOST_CHECK(alive(provider, consumer));
}

/**
 * Test to see that we can connect successfully after an aborted attempt.
 */
BOOST_AUTO_TEST_CASE( ReconnectAfterAbort )
{
    // Choose mock behaviours to simulate a user cancelling authentication
    com_ptr<MockConsumer> consumer = new MockConsumer();
    consumer->set_pubkey_behaviour(MockConsumer::AbortKeys);
    consumer->set_password_behaviour(MockConsumer::AbortPassword);
    consumer->set_keyboard_interactive_behaviour(
        MockConsumer::AbortResponse);

    shared_ptr<sftp_provider> provider = create_provider();

    // Try to fetch a listing enumerator - it should fail
    BOOST_CHECK(!alive(provider, consumer));

    // Change mock behaviours so that authentication succeeds
    consumer->set_password_max_attempts(2);
    consumer->set_password_behaviour(MockConsumer::CustomPassword);
    consumer->set_keyboard_interactive_behaviour(MockConsumer::CustomResponse);

    remote_test_config config;
    consumer->set_password(config.GetPassword());

    BOOST_CHECK(alive(provider, consumer));
}

BOOST_AUTO_TEST_SUITE_END();