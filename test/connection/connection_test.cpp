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

#include "swish/connection/connection.hpp"  // Test subject
#include "swish/utils.hpp"

#include "test/common_boost/helpers.hpp"
#include "test/common_boost/fixtures.hpp"
#include "test/common_boost/ConsumerStub.hpp"

#include <comet/ptr.h>  // com_ptr
#include <comet/bstr.h> // bstr_t
#include <comet/util.h> // thread

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/shared_ptr.hpp>  // shared_ptr
#include <boost/foreach.hpp>  // BOOST_FOREACH
#include <boost/exception/diagnostic_information.hpp> // diagnostic_information

#include <algorithm>
#include <exception>
#include <string>
#include <vector>

using swish::provider::sftp_provider;
using swish::connection::connection_spec;
using swish::utils::Utf8StringToWideString;

using test::OpenSshFixture;
using test::CConsumerStub;

using comet::com_ptr;
using comet::bstr_t;
using comet::thread;

using boost::filesystem::path;
using boost::shared_ptr;
using boost::test_tools::predicate_result;

using std::exception;
using std::string;
using std::vector;
using std::wstring;

namespace { // private

    /**
     * Fixture that returns backend connections from the connection pool.
     */
    class PoolFixture : public OpenSshFixture
    {
    public:
        shared_ptr<sftp_provider> GetSession()
        {
            return connection_spec(
                Utf8StringToWideString(GetHost()), 
                Utf8StringToWideString(GetUser()), GetPort()).pooled_session();
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

BOOST_FIXTURE_TEST_SUITE(pool_tests, PoolFixture)

/**
 * Test a single call to GetSession().
 */
BOOST_AUTO_TEST_CASE( session )
{
    shared_ptr<sftp_provider> provider = GetSession();
    BOOST_CHECK(alive(provider));
}

/**
 * Test that a second call to GetSession() returns the same instance.
 */
BOOST_AUTO_TEST_CASE( twice )
{
    shared_ptr<sftp_provider> first_provider = GetSession();
    BOOST_CHECK(alive(first_provider));

    shared_ptr<sftp_provider> second_provider = GetSession();
    BOOST_CHECK(alive(second_provider));

    BOOST_REQUIRE(second_provider == first_provider);
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
                shared_ptr<sftp_provider> first_provider = 
                    m_fixture->GetSession();
                BOOST_CHECK(m_fixture->alive(first_provider));

                shared_ptr<sftp_provider> second_provider = 
                    m_fixture->GetSession();
                BOOST_CHECK(m_fixture->alive(second_provider));

                BOOST_REQUIRE(second_provider == first_provider);
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
 * Retrieve an prod a session from many threads.
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

BOOST_AUTO_TEST_SUITE_END()
