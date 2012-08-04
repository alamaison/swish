/**
    @file

    Tests for the SFTP directory listing helper functions.

    @if license

    Copyright (C) 2009, 2010, 2011  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "swish/atl.hpp"

#include "swish/remote_folder/connection.hpp"  // Test subject
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

#include <string>
#include <vector>
#include <algorithm>

using swish::remote_folder::CPool;
using swish::utils::Utf8StringToWideString;

using test::OpenSshFixture;
using test::CConsumerStub;

using comet::com_ptr;
using comet::bstr_t;
using comet::thread;

using boost::filesystem::path;
using boost::shared_ptr;

using std::string;
using std::wstring;
using std::vector;

namespace { // private

    /**
     * Fixture that returns backend connections from the connection pool.
     */
    class PoolFixture : public OpenSshFixture
    {
    public:
        com_ptr<ISftpProvider> GetSession()
        {
            CPool pool;
            return pool.GetSession(
                Utf8StringToWideString(GetHost()).c_str(), 
                Utf8StringToWideString(GetUser()).c_str(), GetPort(),
                NULL).get();
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
        void CheckAlive(const com_ptr<ISftpProvider>& provider)
        {
            BOOST_REQUIRE(provider);

            com_ptr<IEnumListing> listing;
            HRESULT hr = provider->GetListing(
                Consumer().in(), bstr_t(L"/home").in(), listing.out());
            BOOST_CHECK(SUCCEEDED(hr));
        }
    };
}

BOOST_FIXTURE_TEST_SUITE(pool_tests, PoolFixture)

/**
 * Test a single call to GetSession().
 */
BOOST_AUTO_TEST_CASE( session )
{
    com_ptr<ISftpProvider> provider = GetSession();
    CheckAlive(provider);
}

/**
 * Test that a second call to GetSession() returns the same instance.
 */
BOOST_AUTO_TEST_CASE( twice )
{
    com_ptr<ISftpProvider> first_provider = GetSession();
    CheckAlive(first_provider);

    com_ptr<ISftpProvider> second_provider = GetSession();
    CheckAlive(second_provider);

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
                com_ptr<ISftpProvider> first_provider = 
                    m_fixture->GetSession();
                m_fixture->CheckAlive(first_provider);

                com_ptr<ISftpProvider> second_provider = 
                    m_fixture->GetSession();
                m_fixture->CheckAlive(second_provider);

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
