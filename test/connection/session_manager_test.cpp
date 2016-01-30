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

#include "swish/connection/session_manager.hpp" // Test subject

#include "swish/connection/authenticated_session.hpp"
#include "swish/connection/connection_spec.hpp"

#include "test/common_boost/helpers.hpp"
#include "test/fixtures/openssh_fixture.hpp"
#include "test/common_boost/ConsumerStub.hpp"

#include <comet/ptr.h> // com_ptr

#include <boost/container/vector.hpp> // move-aware vector
#include <boost/move/move.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

#include <exception>
#include <string>
#include <vector>

using swish::connection::authenticated_session;
using swish::connection::connection_spec;
using swish::connection::session_manager;
using swish::connection::session_reservation;

using test::CConsumerStub;
using test::fixtures::openssh_fixture;

using comet::com_ptr;

using boost::container::vector;
using boost::move;
using boost::mutex;
using boost::shared_ptr;
using boost::ref;
using boost::test_tools::predicate_result;
using boost::thread;

using std::exception;
using std::string;

namespace
{ // private

/**
 * Fixture that returns backend connections from the connection pool.
 */
class fixture : private openssh_fixture
{
public:
    connection_spec get_connection()
    {
        return connection_spec(whost(), wuser(), port());
    }

    com_ptr<ISftpConsumer> consumer()
    {
        com_ptr<CConsumerStub> consumer =
            new CConsumerStub(private_key_path(), public_key_path());
        return consumer;
    }
};

/**
 * Check that the given provider responds sensibly to a request.
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
}

BOOST_FIXTURE_TEST_SUITE(session_manager_tests, fixture)

BOOST_AUTO_TEST_CASE(new_reservation_are_registered_with_session_manager)
{
    connection_spec spec(get_connection());

    BOOST_CHECK(!session_manager().has_session(spec));

    session_reservation ticket =
        session_manager().reserve_session(spec, consumer(), "Testing");

    BOOST_CHECK(session_manager().has_session(spec));

    authenticated_session& session = ticket.session();

    BOOST_CHECK(session_manager().has_session(spec));

    BOOST_CHECK(alive(session));
}

BOOST_AUTO_TEST_CASE(session_outlives_reservation)
{
    connection_spec spec(get_connection());

    BOOST_CHECK(!session_manager().has_session(spec));

    session_manager().reserve_session(spec, consumer(), "Testing");

    BOOST_CHECK(session_manager().has_session(spec));
}

BOOST_AUTO_TEST_CASE(factory_reuses_existing_sessions)
{
    connection_spec spec(get_connection());

    session_reservation ticket1 =
        session_manager().reserve_session(spec, consumer(), "Testing1");

    session_reservation ticket2 =
        session_manager().reserve_session(spec, consumer(), "Testing2");

    BOOST_CHECK(&(ticket1.session()) == &(ticket2.session()));
}

namespace
{
class progress_callback : boost::noncopyable
{
public:
    progress_callback(
        vector<session_reservation> tickets = vector<session_reservation>())
        : m_releasing_started(false), m_tickets(move(tickets))
    {
    }

    template <typename Range>
    bool operator()(const Range& pending_tasks)
    {
        mutex::scoped_lock lock(m_mutex);

        m_notified_task_ranges.push_back(std::vector<string>(
            boost::begin(pending_tasks), boost::end(pending_tasks)));

        if (!m_releasing_started)
        {
            thread(&progress_callback::release_tickets, this);

            m_releasing_started = true;
        }

        return true;
    }

    std::vector<std::vector<string>> notifications()
    {
        mutex::scoped_lock lock(m_mutex);
        return m_notified_task_ranges;
    }

private:
    void release_tickets()
    {
        while (!m_tickets.empty())
        {
            m_tickets.erase(m_tickets.end() - 1);
        }
    }

    mutex m_mutex;
    bool m_releasing_started;

    // Stores the tickets we need to simulate other task gradually
    // releasing their reservations on this
    vector<session_reservation> m_tickets;

    // Stores each range of tasks we are notified of, in the
    // order we are notified of them
    std::vector<std::vector<string>> m_notified_task_ranges;
};
}

BOOST_AUTO_TEST_CASE(removing_session_really_removes_it)
{
    connection_spec spec(get_connection());

    session_manager().reserve_session(spec, consumer(), "Testing");

    BOOST_CHECK(session_manager().has_session(spec));

    progress_callback progress;
    session_manager().disconnect_session(spec, ref(progress));

    BOOST_CHECK(!session_manager().has_session(spec));

    // There should be no pending_task notifications because there were
    // no tasks with a reservation when we disconnected the session.
    // The only notification should be the empty task range indicating 'done'
    BOOST_REQUIRE_EQUAL(progress.notifications().size(), 1U);
    BOOST_CHECK_EQUAL(progress.notifications()[0].size(), 0U);
}

BOOST_AUTO_TEST_CASE(removing_session_with_pending_task)
{
    connection_spec spec(get_connection());

    vector<session_reservation> tasks;
    tasks.push_back(
        session_manager().reserve_session(spec, consumer(), "Testing"));

    progress_callback progress(move(tasks));
    session_manager().disconnect_session(spec, ref(progress));

    BOOST_CHECK(!session_manager().has_session(spec));

    // The progress should have been notified twice ...
    BOOST_REQUIRE_EQUAL(progress.notifications().size(), 2U);
    // ... first with one pending task
    BOOST_REQUIRE_EQUAL(progress.notifications()[0].size(), 1U);
    BOOST_CHECK_EQUAL(progress.notifications()[0][0], "Testing");
    // ... then to say it's done
    BOOST_CHECK_EQUAL(progress.notifications()[1].size(), 0U);
}

BOOST_AUTO_TEST_CASE(removing_session_with_multiple_pending_tasks)
{
    connection_spec spec(get_connection());

    vector<session_reservation> tasks;
    tasks.push_back(
        session_manager().reserve_session(spec, consumer(), "Testing"));
    tasks.push_back(
        session_manager().reserve_session(spec, consumer(), "Testing2"));
    tasks.push_back(
        session_manager().reserve_session(spec, consumer(), "Testing3"));

    progress_callback progress(move(tasks));
    session_manager().disconnect_session(spec, ref(progress));

    BOOST_CHECK(!session_manager().has_session(spec));

    // The progress should have been notified four times ...
    BOOST_REQUIRE_EQUAL(progress.notifications().size(), 4U);
    // ... each time with one less task
    BOOST_REQUIRE_EQUAL(progress.notifications()[0].size(), 3U);
    BOOST_CHECK_EQUAL(progress.notifications()[0][0], "Testing");
    BOOST_CHECK_EQUAL(progress.notifications()[0][1], "Testing2");
    BOOST_CHECK_EQUAL(progress.notifications()[0][2], "Testing3");
    BOOST_REQUIRE_EQUAL(progress.notifications()[1].size(), 2U);
    BOOST_CHECK_EQUAL(progress.notifications()[1][0], "Testing");
    BOOST_CHECK_EQUAL(progress.notifications()[1][1], "Testing2");
    BOOST_REQUIRE_EQUAL(progress.notifications()[2].size(), 1U);
    BOOST_CHECK_EQUAL(progress.notifications()[2][0], "Testing");
    // ... until it's done
    BOOST_CHECK_EQUAL(progress.notifications()[3].size(), 0U);
}

BOOST_AUTO_TEST_CASE(removing_session_with_colliding_task_names)
{
    connection_spec spec(get_connection());

    vector<session_reservation> tasks;
    tasks.push_back(
        session_manager().reserve_session(spec, consumer(), "Testing"));
    tasks.push_back(
        session_manager().reserve_session(spec, consumer(), "Testing"));

    progress_callback progress(move(tasks));
    session_manager().disconnect_session(spec, ref(progress));

    BOOST_CHECK(!session_manager().has_session(spec));

    // The progress should have been notified thrice ...
    BOOST_REQUIRE_EQUAL(progress.notifications().size(), 3U);
    // ... each time with one less task
    BOOST_REQUIRE_EQUAL(progress.notifications()[0].size(), 2U);
    BOOST_CHECK_EQUAL(progress.notifications()[0][0], "Testing");
    BOOST_CHECK_EQUAL(progress.notifications()[0][1], "Testing");
    BOOST_REQUIRE_EQUAL(progress.notifications()[1].size(), 1U);
    BOOST_CHECK_EQUAL(progress.notifications()[1][0], "Testing");
    // ... until it's done
    BOOST_CHECK_EQUAL(progress.notifications()[2].size(), 0U);
}

BOOST_AUTO_TEST_SUITE_END()
