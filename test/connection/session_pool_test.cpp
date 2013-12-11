/**
    @file

    Tests for the pool of SFTP connections.

    @if license

    Copyright (C) 2009, 2010, 2011, 2013
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

#include "swish/connection/session_pool.hpp" // Test subject
#include "swish/connection/connection_spec.hpp"
#include "swish/utils.hpp"

#include "test/common_boost/helpers.hpp"
#include "test/common_boost/fixtures.hpp"
#include "test/common_boost/ConsumerStub.hpp"

#include <comet/ptr.h>  // com_ptr
#include <comet/util.h> // thread

#include <boost/test/unit_test.hpp>
#include <boost/shared_ptr.hpp>  // shared_ptr
#include <boost/foreach.hpp>  // BOOST_FOREACH
#include <boost/exception/diagnostic_information.hpp> // diagnostic_information

#include <algorithm>
#include <exception>
#include <vector>


using swish::connection::authenticated_session;
using swish::connection::connection_spec;
using swish::connection::session_pool;
using swish::utils::Utf8StringToWideString;

using test::CConsumerStub;
using test::OpenSshFixture;

using comet::com_ptr;
using comet::bstr_t;
using comet::thread;

using boost::shared_ptr;
using boost::test_tools::predicate_result;

using std::exception;
using std::vector;

namespace { // private

    /**
     * Fixture that returns backend connections from the connection pool.
     */
    class PoolFixture : public OpenSshFixture
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
         * Check that the given session responds sensibly to a request.
         */
        predicate_result alive(shared_ptr<authenticated_session> session)
        {
            try
            {
                session->get_sftp_filesystem().directory_iterator("/");

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

BOOST_FIXTURE_TEST_SUITE(pool_tests, PoolFixture)

/**
 * Test the situation where the specified connection is not already in the
 * pool.
 *
 * Ensures a connection specification can create a session and that the
 * pool reports session status correctly.
 */
BOOST_AUTO_TEST_CASE( new_session )
{
    connection_spec spec(get_connection());

    BOOST_CHECK(!session_pool().has_session(spec));

    shared_ptr<authenticated_session> session =
        session_pool().pooled_session(spec, Consumer());

    BOOST_CHECK(session_pool().has_session(spec));

    BOOST_CHECK(alive(session));
}

/**
 * Test that creating a session does not affect the status of unrelated
 * connections.
 */
BOOST_AUTO_TEST_CASE( unrelated_unaffected_by_creation )
{
    connection_spec unrelated_spec(L"Unrelated", L"Spec", 123);

    BOOST_CHECK(!session_pool().has_session(unrelated_spec));

    shared_ptr<authenticated_session> session =
        session_pool().pooled_session(get_connection(), Consumer());

    BOOST_CHECK(!session_pool().has_session(unrelated_spec));
}

/**
 * Test that the pool reuses existing sessions.
 */
BOOST_AUTO_TEST_CASE( existing_session )
{
    connection_spec spec(get_connection());

    shared_ptr<authenticated_session> first_session =
        session_pool().pooled_session(spec, Consumer());

    shared_ptr<authenticated_session> second_session = 
        session_pool().pooled_session(spec, Consumer());

    BOOST_CHECK(second_session == first_session);

    BOOST_CHECK(alive(second_session));

    BOOST_CHECK(session_pool().has_session(spec));
}

const int THREAD_COUNT = 30;

template <typename T>
class use_session_thread : public thread
{
public:
    use_session_thread(T* fixture) : thread(), m_fixture(fixture) {}

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

                shared_ptr<authenticated_session> first_session =
                    session_pool().pooled_session(spec, m_fixture->Consumer());

                // However, by this point it *must* be running
                BOOST_CHECK(session_pool().has_session(spec));

                BOOST_CHECK(m_fixture->alive(first_session));

                shared_ptr<authenticated_session> second_session = 
                    session_pool().pooled_session(spec, m_fixture->Consumer());

                BOOST_CHECK(session_pool().has_session(spec));

                BOOST_CHECK(m_fixture->alive(second_session));

                BOOST_CHECK(second_session == first_session);
            }
        }
        catch (const std::exception& e)
        {
            BOOST_MESSAGE(boost::diagnostic_information(e));
        }

        return 1;
    }

    T* m_fixture;
};

typedef use_session_thread<PoolFixture> test_thread;

/**
 * Retrieve and prod a session from many threads.
 */
BOOST_AUTO_TEST_CASE( threaded )
{
    vector<shared_ptr<test_thread> > threads(THREAD_COUNT);

    BOOST_FOREACH(shared_ptr<test_thread>& thread, threads)
    {
        thread = shared_ptr<test_thread>(new test_thread(this));
        thread->start();
    }

    BOOST_FOREACH(shared_ptr<test_thread>& thread, threads)
    {
        thread->wait();
    }
}

BOOST_AUTO_TEST_CASE( remove_session )
{
    connection_spec spec(get_connection());

    shared_ptr<authenticated_session> session =
        session_pool().pooled_session(spec, Consumer());

    session_pool().remove_session(spec);

    BOOST_CHECK(!session_pool().has_session(spec));

    // Even though we removed the session from the pool, existing
    // references should still be alive
    BOOST_CHECK(alive(session));
}

BOOST_AUTO_TEST_SUITE_END()

