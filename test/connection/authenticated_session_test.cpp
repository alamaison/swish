/**
    @file

    Tests for the authenticated_session class.

    @if license

    Copyright (C) 2013, 2014  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "swish/connection/authenticated_session.hpp" // Test subject
#include "swish/utils.hpp" // Utf8StringToWideString

#include "test/common_boost/ConsumerStub.hpp"
#include "test/common_boost/fixtures.hpp" // OpenSshFixture

#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <boost/make_shared.hpp>
#include <boost/move/move.hpp>
#include <boost/thread/thread.hpp> // this_thread
#include <boost/shared_ptr.hpp>

#include <string>
#include <vector>

using test::CConsumerStub;
using test::OpenSshFixture;

using swish::connection::authenticated_session;
using swish::utils::Utf8StringToWideString;

using boost::make_shared;
using boost::move;
using boost::shared_ptr;
using boost::test_tools::predicate_result;

using std::vector;
using std::wstring;

BOOST_FIXTURE_TEST_SUITE( authenticated_session_tests, OpenSshFixture )

namespace {

    predicate_result sftp_is_alive(authenticated_session& session)
    {
        try
        {
            session.get_sftp_filesystem().directory_iterator("/");
            return true;
        }
        catch (...)
        {
            predicate_result res(false);
            res.message() <<
                "SFTP not working; unable to access root directory";
            return res;
        }
    }

}

/**
 * Test that connecting succeeds.
 */
BOOST_AUTO_TEST_CASE( connect )
{
    authenticated_session session(
        Utf8StringToWideString(GetHost()), GetPort(),
        Utf8StringToWideString(GetUser()),
        new CConsumerStub(PrivateKeyPath(), PublicKeyPath()));
    BOOST_CHECK(!session.is_dead());
    BOOST_CHECK(sftp_is_alive(session));
}

BOOST_AUTO_TEST_CASE( multiple_connections )
{
    vector<shared_ptr<authenticated_session>> sessions;
    for (int i = 0; i < 5; i++)
    {
        sessions.push_back(
            make_shared<authenticated_session>(
                    Utf8StringToWideString(GetHost()), GetPort(),
                    Utf8StringToWideString(GetUser()),
                    new CConsumerStub(PrivateKeyPath(), PublicKeyPath())));
    }

    for (int i = 0; i < 5; i++)
    {
        BOOST_CHECK(!sessions.at(i)->is_dead());
        BOOST_CHECK(sftp_is_alive(*sessions.at(i)));
    }
}

/**
 * Test that session reports its death if server is restarted.
 */
BOOST_AUTO_TEST_CASE( server_restart )
{
    authenticated_session session(
        Utf8StringToWideString(GetHost()), GetPort(),
        Utf8StringToWideString(GetUser()),
        new CConsumerStub(PrivateKeyPath(), PublicKeyPath()));

    BOOST_CHECK(sftp_is_alive(session));

    restart_server();

    boost::this_thread::sleep(boost::posix_time::milliseconds(2000));

    BOOST_CHECK(session.is_dead());
    BOOST_CHECK(!sftp_is_alive(session));
}

namespace {

    authenticated_session move_create(
        const wstring& host, unsigned int port, const wstring& user,
        ISftpConsumer* consumer)
    {
        authenticated_session session(host, port, user, consumer);
        return move(session);
    }
}

BOOST_AUTO_TEST_CASE( move_contruct )
{
    authenticated_session session = move_create(
        Utf8StringToWideString(GetHost()), GetPort(),
        Utf8StringToWideString(GetUser()),
        new CConsumerStub(PrivateKeyPath(), PublicKeyPath()));
    BOOST_CHECK(!session.is_dead());
    BOOST_CHECK(sftp_is_alive(session));
}

BOOST_AUTO_TEST_CASE( move_assign )
{
    authenticated_session session1(
        Utf8StringToWideString(GetHost()), GetPort(),
        Utf8StringToWideString(GetUser()),
        new CConsumerStub(PrivateKeyPath(), PublicKeyPath()));
    authenticated_session session2(
        Utf8StringToWideString(GetHost()), GetPort(),
        Utf8StringToWideString(GetUser()),
        new CConsumerStub(PrivateKeyPath(), PublicKeyPath()));

    session1 = move(session2);

    BOOST_CHECK(!session1.is_dead());
    BOOST_CHECK(sftp_is_alive(session1));
}

BOOST_AUTO_TEST_SUITE_END()
