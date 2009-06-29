#include "standard.h"

#include "TestConfig.h"
#include <cppunit/extensions/HelperMacros.h>

using namespace ATL;

CTestConfig::CTestConfig()
{
	// Host name
	if(!m_strHost.GetEnvironmentVariable(_T("TEST_HOST_NAME")))
		CPPUNIT_FAIL("Please set TEST_HOST_NAME environment variable");
	CPPUNIT_ASSERT(!m_strHost.IsEmpty());
	CPPUNIT_ASSERT(m_strHost.GetLength() > 2);
	CPPUNIT_ASSERT(m_strHost.GetLength() < 255);

	// User name
	if(!m_strUser.GetEnvironmentVariable(_T("TEST_USER_NAME")))
		CPPUNIT_FAIL("Please set TEST_USER_NAME environment variable");
	CPPUNIT_ASSERT(!m_strUser.IsEmpty());
	CPPUNIT_ASSERT(m_strUser.GetLength() > 2);
	CPPUNIT_ASSERT(m_strUser.GetLength() < 64);

	// Port number
	CString strPort;
	if(!strPort.GetEnvironmentVariable(_T("TEST_HOST_PORT")))
	{
		m_uPort = 22; // Default SSH port
	}
	else
	{
		int m_uPort = ::StrToInt(strPort);
		CPPUNIT_ASSERT(m_uPort >= 0);
		CPPUNIT_ASSERT(m_uPort <= 65535);
	}

	// Password
	if(!m_strPassword.GetEnvironmentVariable(_T("TEST_PASSWORD")))
		CPPUNIT_FAIL("Please set TEST_PASSWORD environment variable");
	CPPUNIT_ASSERT(!m_strPassword.IsEmpty());
}

CTestConfig::~CTestConfig()
{
}

/**
 * Get the host name of the machine to connect to for remote testing.
 *
 * The host name is retrieved from the TEST_HOST_NAME environment variable.
 * If this variable is not set, a CPPUNIT exception is thrown.
 * 
 * In order to be useful, the host name should exist and the machine 
 * should be accessible via SSH.
 * 
 * @pre the host name should have at least 3 characters
 * @pre the host name should have less than 255 characters
 *
 * @return the host name
 */
CString CTestConfig::GetHost() const
{
	return m_strHost;
}

/**
 * Get the user name of the SSH account to connect to on the remote machine.
 *
 * The user name is retrieved from the TEST_USER_NAME environment variable.
 * If this variable is not set, a CPPUNIT exception is thrown.
 * 
 * In order to be useful, the user name should correspond to a valid SSH
 * account on the testing machine.
 * 
 * @pre the user name should have at least 3 characters
 * @pre the user name should have less than 64 characters
 *
 * @return the user name
 */
CString CTestConfig::GetUser() const
{

	return m_strUser;
}

/**
 * Get the port to connect to on the remote testing machine.
 *
 * The port is retrieved from the TEST_HOST_PORT environment variable.
 * If this variable is not set, the default SSH port 22 is returned.
 * 
 * In order to be useful, the machine should be accessible via SSH on
 * this port.
 * 
 * @post the port should be between 0 and 65535 inclusive
 *
 * @return the port number
 */
USHORT CTestConfig::GetPort() const
{
	return static_cast<USHORT>(m_uPort);
}

/**
 * Get the password to use to connect to the SSH account on the remote machine.
 *
 * The password is retrieved from the TEST_PASSWORD environment variable.
 * If this variable is not set, a CPPUNIT exception is thrown.
 * 
 * In order to be useful, the password should be valid fot the SSH
 * account on the testing machine.
 * 
 * @return the password
 */
CString CTestConfig::GetPassword() const
{
	return m_strPassword;
}