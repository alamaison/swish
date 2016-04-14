// Copyright 2013, 2014, 2016 Alexander Lamaison

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

#include "swish/connection/authenticated_session.hpp" // Test subject

#include "test/common_boost/ConsumerStub.hpp"
#include "test/fixtures/openssh_fixture.hpp"

#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/thread/thread.hpp> // this_thread

#include <memory>
#include <string>
#include <vector>

using test::CConsumerStub;
using test::fixtures::openssh_fixture;

using swish::connection::authenticated_session;

using boost::test_tools::predicate_result;

using std::make_shared;
using std::shared_ptr;
using std::vector;
using std::wstring;

BOOST_FIXTURE_TEST_SUITE(authenticated_session_tests, openssh_fixture)

namespace
{

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
        res.message() << "SFTP not working; unable to access root directory";
        return res;
    }
}
}

/**
 * Test that connecting succeeds.
 */
BOOST_AUTO_TEST_CASE(connect)
{
    authenticated_session session(
        whost(), port(), wuser(),
        new CConsumerStub(private_key_path(), public_key_path()));
    BOOST_CHECK(!session.is_dead());
    BOOST_CHECK(sftp_is_alive(session));
}

BOOST_AUTO_TEST_CASE(multiple_connections)
{
    vector<shared_ptr<authenticated_session>> sessions;
    for (int i = 0; i < 5; i++)
    {
        sessions.push_back(make_shared<authenticated_session>(
            whost(), port(), wuser(),
            new CConsumerStub(private_key_path(), public_key_path())));
    }

    for (int i = 0; i < 5; i++)
    {
        BOOST_CHECK(!sessions.at(i)->is_dead());
        BOOST_CHECK(sftp_is_alive(*sessions.at(i)));
    }
}

/**
 * Test that session reports its death.
 */
BOOST_AUTO_TEST_CASE(server_death)
{
    authenticated_session session(
        whost(), port(), wuser(),
        new CConsumerStub(private_key_path(), public_key_path()));

    BOOST_CHECK(sftp_is_alive(session));

    stop_server();

    boost::this_thread::sleep(boost::posix_time::milliseconds(2000));

    BOOST_CHECK(session.is_dead());
    BOOST_CHECK(!sftp_is_alive(session));
}

/**
 * Test that session reports its death if server is restarted.
 */
BOOST_AUTO_TEST_CASE(server_restart)
{
    authenticated_session session(
        whost(), port(), wuser(),
        new CConsumerStub(private_key_path(), public_key_path()));

    BOOST_CHECK(sftp_is_alive(session));

    restart_server();

    boost::this_thread::sleep(boost::posix_time::milliseconds(2000));

    BOOST_CHECK(session.is_dead());
    BOOST_CHECK(!sftp_is_alive(session));
}

BOOST_AUTO_TEST_CASE(move_contruct)
{
    authenticated_session session(std::move(authenticated_session(
        whost(), port(), wuser(),
        new CConsumerStub(private_key_path(), public_key_path()))));
    BOOST_CHECK(!session.is_dead());
    BOOST_CHECK(sftp_is_alive(session));
}

BOOST_AUTO_TEST_CASE(move_assign)
{
    authenticated_session session1(
        whost(), port(), wuser(),
        new CConsumerStub(private_key_path(), public_key_path()));
    authenticated_session session2(
        whost(), port(), wuser(),
        new CConsumerStub(private_key_path(), public_key_path()));

    session1 = std::move(session2);

    BOOST_CHECK(!session1.is_dead());
    BOOST_CHECK(sftp_is_alive(session1));
}

BOOST_AUTO_TEST_SUITE_END()
