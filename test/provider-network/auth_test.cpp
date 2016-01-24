// Copyright 2008, 2009, 2010, 2012, 2013, 2016 Alexander Lamaison

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

#include "test/openssh_fixture/openssh_fixture.hpp"
#include "test/common_boost/helpers.hpp"
#include "test/common_boost/MockConsumer.hpp"

#include "swish/connection/authenticated_session.hpp"
#include "swish/connection/connection_spec.hpp"

#include <comet/error.h> // com_error
#include <comet/ptr.h>   // com_ptr

#include <boost/system/system_error.hpp>
#include <boost/test/unit_test.hpp>

#include <exception>

using test::openssh_fixture;
using test::MockConsumer;

using swish::connection::authenticated_session;
using swish::connection::connection_spec;

using comet::com_error;
using comet::com_ptr;

using boost::system::system_error;
using boost::test_tools::predicate_result;

using std::exception;

namespace
{

/**
 * Check that the given session responds sensibly to a request.
 */
predicate_result alive(authenticated_session& session)
{
    try
    {
        session.get_sftp_filesystem().directory_iterator("/");

        predicate_result res(true);
        res.message() << "Session seems to be alive";
        return res;
    }
    catch (const exception& e)
    {
        predicate_result res(false);
        res.message() << "Session seems to be dead: " << e.what();
        return res;
    }
}

bool is_e_abort(com_error e)
{
    return e.hr() == E_ABORT;
}

class fixture : public openssh_fixture
{
public:
    swish::connection::connection_spec as_connection_spec() const
    {
        return swish::connection::connection_spec(whost(), wuser(), port());
    }
};
}

BOOST_FIXTURE_TEST_SUITE(network_auth_tests, fixture)

// This test needs kb-int to be disabled on the server, otherwise it will be
// requested first and either succeed, which means password-auth doesn't get
// tested, or fail, which aborts the whole process.
/*
BOOST_AUTO_TEST_CASE(SimplePasswordAuthentication)
{
    // Choose mock behaviours to force only simple password authentication
    com_ptr<MockConsumer> consumer = new MockConsumer();
    consumer->set_password_behaviour(MockConsumer::CustomPassword);
    consumer->set_keyboard_interactive_behaviour(MockConsumer::AbortResponse);
    consumer->set_pubkey_behaviour(MockConsumer::AbortKeys);
    consumer->set_password(wpassword());

    // Fails if keyboard-int supported on the server as that gets preference
    // and replies with user-aborted
    authenticated_session session =
        as_connection_spec().create_session(consumer);

    BOOST_CHECK(alive(session));
}
*/

BOOST_AUTO_TEST_CASE(KeyboardInteractiveAuthentication)
{
    // Choose mock behaviours to force only kbd-interactive authentication
    com_ptr<MockConsumer> consumer = new MockConsumer();
    consumer->set_password_behaviour(MockConsumer::FailPassword);
    consumer->set_pubkey_behaviour(MockConsumer::AbortKeys);
    consumer->set_keyboard_interactive_behaviour(MockConsumer::CustomResponse);
    consumer->set_password(wpassword());

    // This may fail if the server (which we can't control) doesn't allow
    // ki-auth
    authenticated_session session =
        as_connection_spec().create_session(consumer);

    BOOST_CHECK(alive(session));
}

BOOST_AUTO_TEST_CASE(WrongPasswordOrResponse)
{
    com_ptr<MockConsumer> consumer = new MockConsumer();

    consumer->set_pubkey_behaviour(MockConsumer::AbortKeys);
    // We don't know which of password or kb-int (or both) is set up on the
    // server so we have to prime both to return the wrong password else
    // we may get E_ABORT for the kb-interactive response
    consumer->set_keyboard_interactive_behaviour(MockConsumer::WrongResponse);
    consumer->set_password_behaviour(MockConsumer::WrongPassword);

    // FIXME: Any exception will do.  We don't have fine enough control over the
    // mock to test this properly.
    BOOST_CHECK_THROW(as_connection_spec().create_session(consumer), exception);
}

BOOST_AUTO_TEST_CASE(UserAborted)
{
    com_ptr<MockConsumer> consumer = new MockConsumer();

    consumer->set_keyboard_interactive_behaviour(MockConsumer::AbortResponse);
    consumer->set_password_behaviour(MockConsumer::AbortPassword);
    consumer->set_pubkey_behaviour(MockConsumer::AbortKeys);

    BOOST_CHECK_EXCEPTION(as_connection_spec().create_session(consumer),
                          com_error, is_e_abort);
}

/**
 * Test to see that we can connect successfully after an aborted attempt.
 */
BOOST_AUTO_TEST_CASE(ReconnectAfterAbort)
{
    // Choose mock behaviours to simulate a user cancelling authentication
    com_ptr<MockConsumer> consumer = new MockConsumer();
    consumer->set_pubkey_behaviour(MockConsumer::AbortKeys);
    consumer->set_password_behaviour(MockConsumer::AbortPassword);
    consumer->set_keyboard_interactive_behaviour(MockConsumer::AbortResponse);

    BOOST_CHECK_EXCEPTION(as_connection_spec().create_session(consumer),
                          com_error, is_e_abort);

    // Change mock behaviours so that authentication succeeds
    consumer->set_password_max_attempts(2);
    consumer->set_keyboard_interactive_max_attempts(2);
    consumer->set_password_behaviour(MockConsumer::CustomPassword);
    consumer->set_keyboard_interactive_behaviour(MockConsumer::CustomResponse);
    consumer->set_password(wpassword());

    authenticated_session session =
        as_connection_spec().create_session(consumer);

    BOOST_CHECK(alive(session));
}

BOOST_AUTO_TEST_SUITE_END();
