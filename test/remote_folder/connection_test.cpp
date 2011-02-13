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
#include <comet/threading.h> // auto_coinit
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

using test::ComFixture;
using test::OpenSshFixture;
using test::CConsumerStub;

using comet::com_ptr;
using comet::bstr_t;
using comet::thread;
using comet::auto_coinit;

using boost::filesystem::path;
using boost::shared_ptr;

using std::string;
using std::wstring;
using std::vector;

namespace { // private

	/**
	 * Fixture that returns backend connections from the connection pool.
	 */
	class PoolFixture : public ComFixture, OpenSshFixture
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
			com_ptr<CConsumerStub> consumer = new CConsumerStub();
			consumer->SetKeyPaths(PrivateKeyPath(), PublicKeyPath());
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

/**
 * Test that a second GetSession() after releasing the first provider
 * returns a *different* instance.
 */
BOOST_AUTO_TEST_CASE( get_session_twice_separately )
{
	ISftpProvider* first_provider = GetSession().detach();
	BOOST_REQUIRE(first_provider);

	IUnknown* first_unk;
	first_provider->QueryInterface(&first_unk);
	if (first_provider)
		first_provider->Release();
	if (first_unk)
		first_unk->Release();

	com_ptr<ISftpProvider> second_provider = GetSession();
	BOOST_REQUIRE(second_provider);
	CheckAlive(second_provider);

	com_ptr<IUnknown> second_unk(second_provider);
	BOOST_REQUIRE(first_unk != second_unk.get());
}


#pragma region Threaded tests
BOOST_AUTO_TEST_SUITE(pool_tests_threaded)

const int THREAD_COUNT = 3;

template <typename T>
class use_session_thread : public thread
{
public:
	use_session_thread(T* fixture, COINIT ci=COINIT_MULTITHREADED)
		: thread(), m_fixture(fixture), m_coinit(ci) {}

private:
	DWORD thread_main()
	{
		try
		{
			auto_coinit coinit(m_coinit);

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
	const COINIT m_coinit;
};

typedef use_session_thread<PoolFixture> test_thread;

/**
 * Retrieve a session with different apartment than the one that created it.
 *
 * The thread should be correctly marshalled across the apartments.
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

template <typename T>
inline void do_thread_test(
	T* fixture, COINIT starting_thread_type, COINIT retrieving_thread_type)
{
	// cycle first type of thread to create session and store for
	// later clients
	test_thread creation_thread(fixture, starting_thread_type);
	creation_thread.start();
	creation_thread.wait();

	// start other type of threads which should try to retrieve same session
	vector<shared_ptr<test_thread> > threads(THREAD_COUNT);
	BOOST_FOREACH(shared_ptr<test_thread>& t, threads)
	{
		t = shared_ptr<test_thread>(
			new test_thread(fixture, retrieving_thread_type));
		t->start();
	}

	BOOST_FOREACH(shared_ptr<test_thread>& t, threads)
	{
		t->wait();
	}
}

/**
 * Retrieve a session with different apartment than the one that created it.
 *
 * In this case, the thread that creates the session is in an STA and has
 * terminated by the time other (MTA) threads try to reuse the session.
 */
BOOST_AUTO_TEST_CASE( threaded_create_sta_use_mta )
{
	do_thread_test(this, COINIT_APARTMENTTHREADED, COINIT_MULTITHREADED);
}

/**
 * Retrieve a session with different apartment than the one that created it.
 *
 * In this case, the thread that creates the session is in an MTA and has
 * terminated by the time other (STA) threads try to reuse the session.
 *
 * @todo  This test hangs.  Why?
 */
BOOST_AUTO_TEST_CASE( threaded_create_mta_use_sta )
{
	BOOST_MESSAGE("skipping threaded_create_mta_use_sta test");
	//do_thread_test(this, COINIT_MULTITHREADED, COINIT_APARTMENTTHREADED);
}

/**
 * Retrieve a session with different apartment than the one that created it.
 *
 * In this case, the thread that creates the session is in an STA and has
 * terminated by the time other (STA) threads try to reuse the session.
 *
 * @todo  This test hangs.  Why?
 */
BOOST_AUTO_TEST_CASE( threaded_create_sta_use_sta )
{
	BOOST_MESSAGE("skipping threaded_create_sta_use_sta test");
	//do_thread_test(this, COINIT_APARTMENTTHREADED, COINIT_APARTMENTTHREADED);
}

/**
 * Retrieve a session with different apartment than the one that created it.
 *
 * In this case, the thread that creates the session is in an MTA and has
 * terminated by the time other (MTA) threads try to reuse the session.
 */
BOOST_AUTO_TEST_CASE( threaded_create_mta_use_mta )
{
	do_thread_test(this, COINIT_MULTITHREADED, COINIT_MULTITHREADED);
}

BOOST_AUTO_TEST_SUITE_END()
#pragma endregion

BOOST_AUTO_TEST_SUITE_END()
