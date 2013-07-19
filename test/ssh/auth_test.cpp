/**
    @file

    Tests for session authentication.

    @if license

    Copyright (C) 2010, 2012, 2013  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

    In addition, as a special exception, the the copyright holders give you
    permission to combine this program with free software programs or the 
    OpenSSL project's "OpenSSL" library (or with modified versions of it, 
    with unchanged license). You may copy and distribute such a system 
    following the terms of the GNU GPL for this program and the licenses 
    of the other code concerned. The GNU General Public License gives 
    permission to release a modified version without this exception; this 
    exception also makes it possible to release a modified version which 
    carries forward this exception.

    @endif
*/

#include "session_fixture.hpp" // session_fixture

#include <ssh/session.hpp> // test subject

#include <boost/test/unit_test.hpp>

#include <exception>
#include <string>

using ssh::ssh_error;
using ssh::session;
using ssh::agent::agent_identities;
using ssh::agent::identity;

using test::ssh::session_fixture;

using std::exception;
using std::string;

BOOST_FIXTURE_TEST_SUITE(auth_tests, session_fixture)

/**
 * New sessions must not be authenticated.
 *
 * Assumes the server doesn't support authentication method 'none'.
 */
BOOST_AUTO_TEST_CASE( intial_state )
{
    session s = test_session();

    BOOST_CHECK(!s.authenticated());
}

/**
 * Try password authentication.
 *
 * This will fail as we can't set a password on our fixture server.
 *
 * @todo  Find a way to test the success case with the fixture server.
 */
BOOST_AUTO_TEST_CASE( password_fail )
{
    session s = test_session();

    BOOST_CHECK(!s.authenticate_by_password(user(), "dummy password"));
    BOOST_CHECK(!s.authenticated());
}

/**
 * Try pubkey authentication with public key that should fail.
 */
BOOST_AUTO_TEST_CASE( pubkey_wrong_public )
{
    session s = test_session();

    BOOST_CHECK_THROW(
        s.authenticate_by_key(
            user(), wrong_public_key_path(), private_key_path(), ""),
        ssh_error);
    BOOST_CHECK(!s.authenticated());
}

/**
 * Try pubkey authentication with private key that should fail.
 */
BOOST_AUTO_TEST_CASE( pubkey_wrong_private )
{
    session s = test_session();

    BOOST_CHECK_THROW(
        s.authenticate_by_key(
            user(), public_key_path(), wrong_private_key_path(), ""),
        ssh_error);
    BOOST_CHECK(!s.authenticated());
}

/**
 * Try pubkey authentication with both keys (but matching pair!) that
 * should fail.
 */
BOOST_AUTO_TEST_CASE( pubkey_wrong_pair )
{
    session s = test_session();

    BOOST_CHECK_THROW(
        s.authenticate_by_key(
            user(), wrong_public_key_path(), wrong_private_key_path(), ""),
        ssh_error);
    BOOST_CHECK(!s.authenticated());
}

/**
 * Try pubkey authentication with a key public that can't be parsed.
 */
BOOST_AUTO_TEST_CASE( pubkey_invalid_public )
{
    session s = test_session();

    BOOST_CHECK_THROW(
        s.authenticate_by_key(
            user(), private_key_path(), private_key_path(), ""), ssh_error);
    BOOST_CHECK(!s.authenticated());
}

/**
 * Try pubkey authentication with a key private that can't be parsed.
 */
BOOST_AUTO_TEST_CASE( pubkey_invalid_private )
{
    session s = test_session();

    BOOST_CHECK_THROW(
        s.authenticate_by_key(
            user(), public_key_path(), public_key_path(), ""), ssh_error);
    BOOST_CHECK(!s.authenticated());
}

/**
 * Pubkey authentication with correct keys.
 */
BOOST_AUTO_TEST_CASE( pubkey )
{
    session s = test_session();

    BOOST_CHECK(!s.authenticated());
    s.authenticate_by_key(user(), public_key_path(), private_key_path(), "");
    BOOST_CHECK(s.authenticated());
}

/**
 * Request connection to agent.  Allowed to fail but not catastrophically.
 */
BOOST_AUTO_TEST_CASE( agent )
{
    session s = test_session();

    BOOST_CHECK(!s.authenticated());

    try
    {
        agent_identities identities = s.agent_identities();

        BOOST_FOREACH(identity i, identities)
        {
            try
            {
                i.authenticate(user());
                BOOST_CHECK(s.authenticated());
                return;
            }
            catch(const exception&) {}

            BOOST_CHECK(!s.authenticated());
        }
    }
    catch (exception&) { /* agent not running - failure ok */ }
}

/**
 * Agent copy behaviour.
 */
BOOST_AUTO_TEST_CASE( agent_copy )
{
    session s = test_session();

    BOOST_CHECK(!s.authenticated());

    try
    {
        agent_identities identities = s.agent_identities();
        agent_identities identities2 = identities;

        BOOST_FOREACH(identity i, identities)
        {
        }
        BOOST_FOREACH(identity i, identities2)
        {
        }
    }
    catch (exception&) { /* agent not running - failure ok */ }
}

/**
 * Agent idempotence - creating the object more than once should be ok.
 */
BOOST_AUTO_TEST_CASE( agent_idempotence )
{
    session s = test_session();

    BOOST_CHECK(!s.authenticated());

    try
    {
        agent_identities identities = s.agent_identities();
        agent_identities identities2 = s.agent_identities();

        BOOST_FOREACH(identity i, identities)
        {
        }
        BOOST_FOREACH(identity i, identities2)
        {
        }
    } 
    catch (exception&) { /* agent not running - failure ok */ }
}

BOOST_AUTO_TEST_SUITE_END();
