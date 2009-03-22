/**
 * @file
 *
 * Tests for the CSessionFactory class.
 */

#include "pch.h"
#include "standard.h"

#include "../common/CppUnitExtensions.h"
#include "../common/TestConfig.h"
#include "../common/MockSftpConsumer.h"

#include <SessionFactory.hpp>

#include <libssh2.h>
#include <libssh2_sftp.h>

#include <memory>

using namespace ATL;
using std::auto_ptr;

#pragma warning (push)
#pragma warning (disable: 4267) // ssize_t to unsigned int

class CSessionFactory_test : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE( CSessionFactory_test );
		CPPUNIT_TEST( testCreateSession );
		CPPUNIT_TEST( testCreateSessionFail );
		CPPUNIT_TEST( testCreateSessionAbort );
	CPPUNIT_TEST_SUITE_END();

public:
	void setUp()
	{
		// Start up COM
		HRESULT hr = ::CoInitialize(NULL);
		CPPUNIT_ASSERT_OK(hr);

		// Start up Winsock
		WSADATA wsadata;
		CPPUNIT_ASSERT( ::WSAStartup(WINSOCK_VERSION, &wsadata) == 0 );

		// Create mock consumer
		_CreateMockSftpConsumer(&m_spCoConsumer, &m_spConsumer);
	}

	void tearDown()
	{
		// Destroy mock consumer
		m_spCoConsumer = NULL;
		m_spConsumer = NULL;

		// Shut down Winsock
		CPPUNIT_ASSERT( ::WSACleanup() == 0 );
		
		// Shut down COM
		::CoUninitialize();
	}

protected:

	/**
	 * Can the factory create a session object that has been properly
	 * authenticated and has a working SFTP channel?
	 */
	void testCreateSession()
	{
		// Set mock to login successfully
		m_spCoConsumer->SetKeyboardInteractiveBehaviour(
			CMockSftpConsumer::CustomResponse);
		m_spCoConsumer->SetPasswordBehaviour(
			CMockSftpConsumer::CustomPassword);
		m_spCoConsumer->SetCustomPassword(config.GetPassword());

		// Create a session using the factory and mock consumer
		auto_ptr<CSession> spSession = CSessionFactory::CreateSftpSession(
			config.GetHost(), config.GetPort(), config.GetUser(), m_spConsumer);

		// Verify that we are authenticated
		CPPUNIT_ASSERT( libssh2_userauth_authenticated(*spSession) != 0 );

		// Try to use SFTP by statting the current directory
		LIBSSH2_SFTP_ATTRIBUTES attrs;
		::ZeroMemory(&attrs, sizeof attrs);
		CPPUNIT_ASSERT( libssh2_sftp_stat(*spSession, ".", &attrs) == 0 );
	}

	/**
	 * Is authentication failure dealt with properly (exception thrown)?
	 */
	void testCreateSessionFail()
	{
		// Set mock to provider wrong password to server
		m_spCoConsumer->SetKeyboardInteractiveBehaviour(
			CMockSftpConsumer::WrongResponse);
		m_spCoConsumer->SetPasswordBehaviour(
			CMockSftpConsumer::WrongPassword);

		// Create a session using the factory and mock consumer
		// This should throw an exception indicating that the operation failed
		CPPUNIT_ASSERT_THROW_MESSAGE(
			"The factory didn't throw an exception when it should have",
			auto_ptr<CSession> spSession = CSessionFactory::CreateSftpSession(
				config.GetHost(), config.GetPort(), config.GetUser(),
				m_spConsumer),
			CAtlException
		);
	}

	/**
	 * Is an abort by the user handled correctly?
	 */
	void testCreateSessionAbort()
	{
		// Set mock to abort the operation as if the user had cancelled
		m_spCoConsumer->SetKeyboardInteractiveBehaviour(
			CMockSftpConsumer::AbortResponse);
		m_spCoConsumer->SetPasswordBehaviour(
			CMockSftpConsumer::AbortPassword);

		// Create a session using the factory and mock consumer
		// This should throw an E_ABORT CAtlException exception indicating that 
		// the operation was cancelled by the user
		try
		{
			auto_ptr<CSession> spSession = CSessionFactory::CreateSftpSession(
				config.GetHost(), config.GetPort(), config.GetUser(),
				m_spConsumer);
		}
		catch(CAtlException e)
		{
			CPPUNIT_ASSERT(e == E_ABORT);
			return;
		}

		CPPUNIT_FAIL("The factory didn't throw exception when it should have");
	}

private:
	CTestConfig config;

	CComPtr<CComObject<CMockSftpConsumer> > m_spCoConsumer;
	CComPtr<ISftpConsumer> m_spConsumer;

	/**
	 * Creates a CMockSftpConsumer and returns pointers to its CComObject
	 * as well as its ISftpConsumer interface. Both have been AddReffed.
	 */
	static void _CreateMockSftpConsumer(
		__out CComObject<CMockSftpConsumer> **ppCoConsumer,
		__out ISftpConsumer **ppConsumer
	)
	{
		HRESULT hr;

		// Create mock object coclass instance
		*ppCoConsumer = NULL;
		hr = CComObject<CMockSftpConsumer>::CreateInstance(ppCoConsumer);
		CPPUNIT_ASSERT_OK(hr);
		CPPUNIT_ASSERT(*ppCoConsumer);
		(*ppCoConsumer)->AddRef();

		// Get ISftpConsumer interface
		*ppConsumer = NULL;
		(*ppCoConsumer)->QueryInterface(ppConsumer);
		CPPUNIT_ASSERT(*ppConsumer);
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( CSessionFactory_test );

#pragma warning (pop)