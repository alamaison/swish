/**
 * @file
 *
 * Tests for the CSession class.
 */

#include "pch.h"
#include "standard.h"

#include "../common/CppUnitExtensions.h"
#include "../common/TestConfig.h"

#include <Session.hpp>

#include <memory>

using namespace ATL;
using std::auto_ptr;

class CSession_test : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE( CSession_test );
		CPPUNIT_TEST( testCreateSession );
		CPPUNIT_TEST( testCreateSessionHeap );
		CPPUNIT_TEST( testConnect );
		CPPUNIT_TEST( testMultiConnect );
		CPPUNIT_TEST( testStartSftpBeforeConnect );
		CPPUNIT_TEST( testStartSftpTooEarly );
		CPPUNIT_TEST( testAutoPointer );
	CPPUNIT_TEST_SUITE_END();

public:
	void setUp()
	{
		// Start up Winsock
		WSADATA wsadata;
		CPPUNIT_ASSERT( ::WSAStartup(WINSOCK_VERSION, &wsadata) == 0 );
	}

	void tearDown()
	{
		// Shut down Winsock
		CPPUNIT_ASSERT( ::WSACleanup() == 0 );
	}

protected:
	void testCreateSession()
	{
		CSession session;
	}

	void testCreateSessionHeap()
	{
		CSession *pSession = new CSession();
		delete pSession;
	}

	/**
	 * Test that connecting succeeds.
	 */
	void testConnect()
	{
		CSession session;
		session.Connect(config.GetHost(), config.GetPort());
	}

	void testMultiConnect()
	{
		CSession *apSession = new CSession[5];
		for (int i = 0; i < 5; i++)
		{
			apSession[i].Connect(config.GetHost(), config.GetPort());
		}

		delete [] apSession;
	}

	/** 
	 * Test that trying to start SFTP channel before connecting fails.
	 */
	void testStartSftpBeforeConnect()
	{
		CSession session;
		CPPUNIT_ASSERT_THROW( session.StartSftp(), CAtlException );
	}

	/** 
	 * Test that trying to start SFTP channel before authenticating fails.
	 */
	void testStartSftpTooEarly()
	{
		CSession session;
		session.Connect(config.GetHost(), config.GetPort());
		CPPUNIT_ASSERT_THROW( session.StartSftp(), CAtlException );
	}
	
	/**
	 * Test that session behaves correctly when used with an auto_ptr.
	 */
	void testAutoPointer()
	{
		LIBSSH2_SESSION *p = NULL;

		CSession *pSession = new CSession();
		p = *pSession;
		CPPUNIT_ASSERT( p );

		auto_ptr<CSession> spSession(pSession);
		p = *spSession;
		CPPUNIT_ASSERT( p );

		spSession->Connect(config.GetHost(), config.GetPort());
		p = *spSession;
		CPPUNIT_ASSERT( p );
	}

private:
	CTestConfig config;
};

CPPUNIT_TEST_SUITE_REGISTRATION( CSession_test );