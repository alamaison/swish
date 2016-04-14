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

#include "swish/connection/running_session.hpp" // Test subject

#include "test/fixtures/openssh_fixture.hpp"

#include <boost/move/move.hpp>
#include <boost/test/unit_test.hpp>

#include <memory>
#include <stdexcept> // runtime_error
#include <string>
#include <vector>

using test::fixtures::openssh_fixture;

using swish::connection::running_session;

using boost::move;

using std::make_shared;
using std::shared_ptr;
using std::runtime_error;
using std::vector;
using std::wstring;

BOOST_FIXTURE_TEST_SUITE(running_session_tests, openssh_fixture)

BOOST_AUTO_TEST_CASE(connecting_with_correct_host_and_port_succeeds)
{
    running_session session(whost(), port());
    BOOST_CHECK(!session.is_dead());
}

BOOST_AUTO_TEST_CASE(connection_failure_throws_error)
{
    BOOST_CHECK_THROW(running_session(L"nonsense.invalid", 65535),
                      runtime_error);
}

BOOST_AUTO_TEST_CASE(multiple_connections_do_not_interfere)
{
    vector<shared_ptr<running_session>> sessions;
    for (int i = 0; i < 5; i++)
    {
        sessions.push_back(make_shared<running_session>(whost(), port()));
    }

    for (int i = 0; i < 5; i++)
    {
        BOOST_CHECK(!sessions.at(i)->is_dead());
    }
}

namespace
{

running_session move_create(const wstring& host, unsigned int port)
{
    running_session session(host, port);
    return move(session);
}
}

BOOST_AUTO_TEST_CASE(session_survives_move_construction)
{
    running_session session = move_create(whost(), port());
    BOOST_CHECK(!session.is_dead());
}

BOOST_AUTO_TEST_CASE(session_survives_move_assignment)
{
    running_session session1(whost(), port());
    running_session session2(whost(), port());

    session1 = move(session2);

    BOOST_CHECK(!session1.is_dead());
}

BOOST_AUTO_TEST_SUITE_END()
