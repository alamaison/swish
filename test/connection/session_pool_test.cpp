// Copyright 2009, 2010, 2011, 2013, 2014, 2016 Alexander Lamaison

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

#include "swish/connection/session_pool.hpp" // Test subject
#include "swish/connection/connection_spec.hpp"

#include "test/common_boost/ConsumerStub.hpp"
#include "test/common_boost/helpers.hpp"
#include "test/fixtures/openssh_fixture.hpp"

#include <comet/ptr.h>  // com_ptr
#include <comet/util.h> // thread

#include <boost/test/unit_test.hpp>
#include <boost/shared_ptr.hpp>                       // shared_ptr
#include <boost/foreach.hpp>                          // BOOST_FOREACH
#include <boost/exception/diagnostic_information.hpp> // diagnostic_information
#include <boost/exception_ptr.hpp>

#include <algorithm>
#include <exception>
#include <vector>

using swish::connection::authenticated_session;
using swish::connection::connection_spec;
using swish::connection::session_pool;

using test::CConsumerStub;
using test::fixtures::openssh_fixture;

using comet::com_ptr;
using comet::thread;

using boost::exception_ptr;
using boost::shared_ptr;
using boost::test_tools::predicate_result;

using std::exception;
using std::vector;

namespace
{ // private

class fixture : public openssh_fixture
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

    /**
     * Check that the given session responds sensibly to a request.
     */
    predicate_result alive(authenticated_session& session)
    {
        try
        {
            session.get_sftp_filesystem().directory_iterator("/");

            predicate_result res(true);
            res.message() << "Provider seems to be alive";
            return res;
        }
        catch (const exception& e)
        {
            predicate_result res(false);
            res.message() << "Provider seems to be dead: " << e.what();
            return res;
        }
    }
};
}

BOOST_FIXTURE_TEST_SUITE(pool_tests, fixture)

/**
 * Test the situation where the specified connection is not already in the
 * pool.
 *
 * Ensures a connection specification can create a session and that the
 * pool reports session status correctly.
 */
BOOST_AUTO_TEST_CASE(new_session)
{
    connection_spec spec(get_connection());

    BOOST_CHECK(!session_pool().has_session(spec));

    authenticated_session& session =
        session_pool().pooled_session(spec, consumer());

    BOOST_CHECK(session_pool().has_session(spec));

    BOOST_CHECK(alive(session));
}

/**
 * Test that creating a session does not affect the status of unrelated
 * connections.
 */
BOOST_AUTO_TEST_CASE(unrelated_unaffected_by_creation)
{
    connection_spec unrelated_spec(L"Unrelated", L"Spec", 123);

    BOOST_CHECK(!session_pool().has_session(unrelated_spec));

    authenticated_session& session =
        session_pool().pooled_session(get_connection(), consumer());

    BOOST_CHECK(!session_pool().has_session(unrelated_spec));
}

/**
 * Test that the pool reuses existing sessions.
 */
BOOST_AUTO_TEST_CASE(existing_session)
{
    connection_spec spec(get_connection());

    authenticated_session& first_session =
        session_pool().pooled_session(spec, consumer());

    authenticated_session& second_session =
        session_pool().pooled_session(spec, consumer());

    BOOST_CHECK(&second_session == &first_session);

    BOOST_CHECK(alive(second_session));

    BOOST_CHECK(session_pool().has_session(spec));
}

const int THREAD_COUNT = 30;

template <typename T>
class use_session_thread : public thread
{
public:
    use_session_thread(T* fixture, exception_ptr& error)
        : thread(), m_fixture(fixture), m_error(error)
    {
    }

private:
    DWORD thread_main()
    {
        try
        {
            {
                connection_spec spec(m_fixture->get_connection());

                // This first call may or may not return running depending on
                // whether it is on the first thread scheduled, so we don't test
                // its value, just that it succeeds.
                session_pool().has_session(spec);

                authenticated_session& first_session =
                    session_pool().pooled_session(spec, m_fixture->consumer());

                // However, by this point it *must* be running
                if (!session_pool().has_session(spec))
                    BOOST_THROW_EXCEPTION(
                        std::exception("Test failed: no session"));

                if (!m_fixture->alive(first_session))
                    BOOST_THROW_EXCEPTION(
                        std::exception("Test failed: first session is dead"));

                authenticated_session& second_session =
                    session_pool().pooled_session(spec, m_fixture->consumer());

                if (!session_pool().has_session(spec))
                    BOOST_THROW_EXCEPTION(
                        std::exception("Test failed: no session"));

                if (!m_fixture->alive(second_session))
                    BOOST_THROW_EXCEPTION(
                        std::exception("Test failed: second session is dead"));

                if (&second_session != &first_session)
                    BOOST_THROW_EXCEPTION(
                        std::exception("Test failed: session was not reused"));
            }
        }
        catch (...)
        {
            // Boost.Test is not threadsafe so we can't report the error
            // directly when it happens.  Instead pass it out via exception_ptr
            // to let the test find it.
            m_error = boost::current_exception();
        }

        return 1;
    }

    exception_ptr& m_error;
    T* m_fixture;
};

typedef use_session_thread<fixture> test_thread;

/**
 * Retrieve and prod a session from many threads.
 */
BOOST_AUTO_TEST_CASE(threaded)
{
    vector<shared_ptr<test_thread>> threads(THREAD_COUNT);
    vector<exception_ptr> errors(THREAD_COUNT);

    for (size_t i = 0; i < threads.size(); ++i)
    {
        threads[i] = shared_ptr<test_thread>(new test_thread(this, errors[i]));
        threads[i]->start();
    }

    for (size_t i = 0; i < threads.size(); ++i)
    {
        threads[i]->wait();
    }

    // Must process errors after finished waiting for all threads, otherwise
    // remaining threads will try to write to errors vector after it has been
    // destroyed
    for (size_t i = 0; i < errors.size(); ++i)
    {
        if (errors[i])
        {
            boost::rethrow_exception(errors[i]);
        }
    }
}

BOOST_AUTO_TEST_CASE(remove_session)
{
    connection_spec spec(get_connection());

    authenticated_session& session =
        session_pool().pooled_session(spec, consumer());

    session_pool().remove_session(spec);

    BOOST_CHECK(!session_pool().has_session(spec));
}

/**
 * Test that sessions in the pool survive server restarts
 * (modulo re-authentication).
 *
 * By 'survive', we mean the pool is able to serve a usable session
 * with the same specification, not that the actual session instance has
 * to be the same (value-semantics and all that jazz).
 */
BOOST_AUTO_TEST_CASE(sessions_across_server_restart)
{
    connection_spec spec(get_connection());

    session_pool().pooled_session(spec, consumer());

    BOOST_CHECK(session_pool().has_session(spec));

    restart_server();

    BOOST_CHECK(alive(session_pool().pooled_session(spec, consumer())));
}

BOOST_AUTO_TEST_SUITE_END()
