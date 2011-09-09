/**
 * @file
 *
 * Tests for the CSession class.
 */

#include "test/common_boost/fixtures.hpp" // WinsockFixture
#include "test/common_boost/remote_test_config.hpp" // remote_test_config

#include "swish/provider/Session.hpp"

#include <memory>

using test::remote_test_config;
using test::WinsockFixture;

using std::auto_ptr;

BOOST_FIXTURE_TEST_SUITE( CSession_tests, WinsockFixture )

BOOST_AUTO_TEST_CASE( create_session )
{
	CSession session;
}

BOOST_AUTO_TEST_CASE( create_session_on_heap )
{
	CSession *pSession = new CSession();
	delete pSession;
}

/**
 * Test that connecting succeeds.
 */
BOOST_AUTO_TEST_CASE( connect )
{
	CSession session;
	remote_test_config config;
	session.Connect(config.GetHost().c_str(), config.GetPort());
}

BOOST_AUTO_TEST_CASE( multiple_connections )
{
	remote_test_config config;

	CSession *sessions = new CSession[5];
	for (int i = 0; i < 5; i++)
	{
		sessions[i].Connect(config.GetHost().c_str(), config.GetPort());
	}

	delete [] sessions;
}

/** 
 * Test that trying to start SFTP channel before connecting fails.
 */
BOOST_AUTO_TEST_CASE( start_sftp_before_connecting )
{
	CSession session;
	BOOST_CHECK_THROW(session.StartSftp(), comet::com_error);
}

/** 
 * Test that trying to start SFTP channel before authenticating fails.
 */
BOOST_AUTO_TEST_CASE( start_sftp_too_early )
{
	CSession session;
	remote_test_config config;
	session.Connect(config.GetHost().c_str(), config.GetPort());
	BOOST_CHECK_THROW(session.StartSftp(), comet::com_error);
}

/**
 * Test that session behaves correctly when used with an auto_ptr.
 */
BOOST_AUTO_TEST_CASE( auto_ptr_to_session )
{
	remote_test_config config;
	LIBSSH2_SESSION *p = NULL;

	CSession *pSession = new CSession();
	p = *pSession;
	BOOST_CHECK(p);

	auto_ptr<CSession> spSession(pSession);
	p = *spSession;
	BOOST_CHECK(p);

	spSession->Connect(config.GetHost().c_str(), config.GetPort());
	p = *spSession;
	BOOST_CHECK(p);
}

BOOST_AUTO_TEST_SUITE_END()
