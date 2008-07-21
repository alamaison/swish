//Libssh2Provider_Test.cpp  -   defines the class Libssh2Provider_Test

#include "stdafx.h"
#include "Libssh2Provider_test.h"

#include <ATLComTime.h>

CPPUNIT_TEST_SUITE_REGISTRATION( CLibssh2Provider_test );

void CLibssh2Provider_test::setUp()
{
	HRESULT hr;

	// Start up COM
	hr = ::CoInitialize(NULL);
	CPPUNIT_ASSERT_OK(hr);

	// Save Libssh2Provider CLSID in member variable
	CLSID CLSID_CLibssh2Provider;
	hr = ::CLSIDFromProgID(
		OLESTR("Libssh2Provider.Libssh2Provider"),
		&CLSID_CLibssh2Provider
	);
	CPPUNIT_ASSERT_OK(hr);

	// Check that CLSID was correctly constructed from ProgID
	LPOLESTR pszUuid = NULL;
	hr = ::StringFromCLSID( CLSID_CLibssh2Provider, &pszUuid );
	CPPUNIT_ASSERT_OK(hr);
	CString strExpectedUuid = _T("{b816a847-5022-11dc-9153-0090f5284f85}");
	CString strActualUuid = pszUuid;
	CPPUNIT_ASSERT_EQUAL(
		strExpectedUuid.MakeLower(),
		strActualUuid.MakeLower()
	);

	// Create instance of Libssh2 Provider using CLSID
	hr = ::CoCreateInstance(
		__uuidof(Libssh2Provider::CLibssh2Provider), NULL, CLSCTX_INPROC_SERVER,
		__uuidof(ISftpProvider), (LPVOID *)&m_pProvider);
	CPPUNIT_ASSERT_OK(hr);

	// Create mock SftpConsumer for use in Initialize()
	CreateMockSftpConsumer( &m_pCoConsumer, &m_pConsumer );
}

/**
 * Test that the class responds to IUnknown::QueryInterface correctly.
 *
 * This test will be roughly the same for *any* valid COM object except
 * one that implement IShellView as this has been chosen to test failure. The
 * cases being tested are based on those explained by Raymond Chen:
 * http://blogs.msdn.com/oldnewthing/archive/2004/03/26/96777.aspx
 */
void CLibssh2Provider_test::testQueryInterface()
{
	HRESULT hr;

	// Supports IUnknown (valid COM object)?
	IUnknown *pUnk;
	hr = m_pProvider->QueryInterface(__uuidof(IUnknown), (void **)&pUnk);
	CPPUNIT_ASSERT_OK(hr);
	pUnk->Release();

	// Supports ILibssh2Provider (valid self!)?
	ISftpProvider *pProv;
	hr = m_pProvider->QueryInterface(__uuidof(ISftpProvider), (void **)&pProv);
	CPPUNIT_ASSERT_OK(hr);
	pProv->Release();

	// Says no properly (must return NULL)
	IShellView *pShell = (IShellView *)this; // Arbitrary non-null
	hr = m_pProvider->QueryInterface(__uuidof(IShellView), (void **)&pShell);
	if (SUCCEEDED(hr))
	{
		pShell->Release();
		CPPUNIT_ASSERT(FAILED(hr));
	}
	CPPUNIT_ASSERT(pShell == NULL);
}

void CLibssh2Provider_test::testInitialize()
{
	CComBSTR bstrUser = GetUserName();
	CComBSTR bstrHost = GetHostName();

	// Choose mock behaviours
	m_pCoConsumer->SetPasswordBehaviour(CMockSftpConsumer::CustomPassword);
	m_pCoConsumer->SetCustomPassword(GetPassword());

	CPPUNIT_ASSERT_OK(
		m_pProvider->Initialize( m_pConsumer, bstrUser, bstrHost, GetPort() )
	);
}

void CLibssh2Provider_test::testGetListing()
{
	HRESULT hr;

	CComBSTR bstrUser = GetUserName();
	CComBSTR bstrHost = GetHostName();

	// Choose mock behaviours
	m_pCoConsumer->SetPasswordBehaviour(CMockSftpConsumer::CustomPassword);
	m_pCoConsumer->SetCustomPassword(GetPassword());

	CPPUNIT_ASSERT_OK(
		m_pProvider->Initialize( m_pConsumer, bstrUser, bstrHost, GetPort() )
	);

	// Fetch listing enumerator
	IEnumListing *pEnum;
	CComBSTR bstrDirectory(_T("/tmp"));
	hr = m_pProvider->GetListing(bstrDirectory, &pEnum);
	if (FAILED(hr))
		pEnum = NULL;
	CPPUNIT_ASSERT_OK(hr);

	// Check format of listing is sensible
	testListingFormat(pEnum);

	ULONG cRefs = pEnum->Release();
	CPPUNIT_ASSERT_EQUAL( (ULONG)0, cRefs );
}

void CLibssh2Provider_test::testGetListing_WrongPassword()
{
	CComBSTR bstrUser = GetUserName();
	CComBSTR bstrHost = GetHostName();

	// Choose mock behaviours
	m_pCoConsumer->SetPasswordBehaviour(CMockSftpConsumer::WrongPassword);
	m_pCoConsumer->SetMaxPasswordAttempts(5); // Tries 5 times then gives up

	CPPUNIT_ASSERT_OK(
		m_pProvider->Initialize( m_pConsumer, bstrUser, bstrHost, GetPort() )
	);

	// Fetch listing enumerator
	IEnumListing *pEnum;
	CComBSTR bstrDirectory(_T("/tmp"));
	HRESULT hr = m_pProvider->GetListing(bstrDirectory, &pEnum);
	if (FAILED(hr))
		pEnum = NULL;
	CPPUNIT_ASSERT_FAILED(hr);
}

void CLibssh2Provider_test::tearDown()
{
	if (m_pProvider) // Possible for test to fail before m_pProvider initialised
	{
		ULONG cRefs = m_pProvider->Release();
		CPPUNIT_ASSERT_EQUAL( (ULONG)0, cRefs );
	}
	m_pProvider = NULL;

	if (m_pConsumer) // Same again for mock consumer
	{
		ULONG cRefs = m_pConsumer->Release();
		CPPUNIT_ASSERT_EQUAL( (ULONG)0, cRefs );
	}
	m_pConsumer = NULL;

	// Shut down COM
	::CoUninitialize();
}

/*----------------------------------------------------------------------------*
 * Private functions
 *----------------------------------------------------------------------------*/

/**
 * Creates a CMockSftpConsumer and returns pointers to its CComObject
 * as well as its ISftpConsumer interface.
 */
void CLibssh2Provider_test::CreateMockSftpConsumer(
	CComObject<CMockSftpConsumer> **ppCoConsumer, ISftpConsumer **ppConsumer )
	const
{
	// Create mock object coclass instance
	*ppCoConsumer = NULL;
	HRESULT hr = CComObject<CMockSftpConsumer>::CreateInstance(ppCoConsumer);
	CPPUNIT_ASSERT_OK(hr);
	CPPUNIT_ASSERT(*ppCoConsumer);

	// Get ISftpConsumer interface
	*ppConsumer = NULL;
	(*ppCoConsumer)->QueryInterface(ppConsumer);
	CPPUNIT_ASSERT(*ppConsumer);
}

/**
 * Tests that the format of the enumeration of listings is correct.
 *
 * @param pEnum The Listing enumerator to be tested.
 */
void CLibssh2Provider_test::testListingFormat(IEnumListing *pEnum) const
{
	// Check format of listing is sensible
	CPPUNIT_ASSERT_OK( pEnum->Reset() );
	Listing lt;
	HRESULT hr = pEnum->Next(1, &lt, NULL);
	CPPUNIT_ASSERT_OK(hr);
	while (hr == S_OK)
	{
		CString strFilename(lt.bstrFilename),
				strOwner(lt.bstrOwner),
				strGroup(lt.bstrGroup);

		testFILEDATA fd;
		//::ZeroMemory(&fd, sizeof(fd));
		fd.strPath = lt.bstrFilename;
		fd.strOwner = lt.bstrOwner;
		fd.strGroup = lt.bstrGroup;
		fd.dwPermissions = lt.uPermissions;
		fd.uSize = lt.cSize;
		fd.dtModified = (time_t) COleDateTime(lt.dateModified);

		CString strOwner2 = lt.bstrOwner;
		CPPUNIT_ASSERT( !strFilename.IsEmpty() );

		CPPUNIT_ASSERT( lt.uPermissions > 0 );
		CPPUNIT_ASSERT( lt.cHardLinks > 0 );
		CPPUNIT_ASSERT( lt.cSize >= 0 );
		CPPUNIT_ASSERT( !strOwner.IsEmpty() );
		CPPUNIT_ASSERT( !strGroup.IsEmpty() );

		CPPUNIT_ASSERT( lt.dateModified );
		COleDateTime dateModified( lt.dateModified );
		// Check year
		CPPUNIT_ASSERT( dateModified.GetYear() >= 1604 );
		CPPUNIT_ASSERT( 
			dateModified.GetYear() <= COleDateTime::GetCurrentTime().GetYear()
		);
		// Check month
		CPPUNIT_ASSERT( dateModified.GetMonth() > 0 );
		CPPUNIT_ASSERT( dateModified.GetMonth() <= 12 );
		// Check date
		CPPUNIT_ASSERT( dateModified.GetDay() > 0 );
		CPPUNIT_ASSERT( dateModified.GetDay() <= 31 );
		// Check hour
		CPPUNIT_ASSERT( dateModified.GetHour() >= 0 );
		CPPUNIT_ASSERT( dateModified.GetHour() <= 23 );
		// Check minute
		CPPUNIT_ASSERT( dateModified.GetMinute() >= 0 );
		CPPUNIT_ASSERT( dateModified.GetMinute() <= 59 );
		// Check second
		CPPUNIT_ASSERT( dateModified.GetSecond() >= 0 );
		CPPUNIT_ASSERT( dateModified.GetSecond() <= 59 );
		// Check overall validity
		CPPUNIT_ASSERT_EQUAL( COleDateTime::valid, dateModified.GetStatus() );

		// TODO: test numerical permissions using old swish C 
		//       permissions functions here
		//CPPUNIT_ASSERT(
		//	strPermissions[0] == _T('d') ||
		//	strPermissions[0] == _T('b') ||
		//	strPermissions[0] == _T('c') ||
		//	strPermissions[0] == _T('l') ||
		//	strPermissions[0] == _T('p') ||
		//	strPermissions[0] == _T('s') ||
		//	strPermissions[0] == _T('-'));

		hr = pEnum->Next(1, &lt, NULL);
	}
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
CString CLibssh2Provider_test::GetHostName() const
{
	CString strHostName;
	/* static CString strHostName;

	if (strHostName.IsEmpty()) // Might be cached in static variable
	{*/
		if(!strHostName.GetEnvironmentVariable(_T("TEST_HOST_NAME")))
			CPPUNIT_FAIL("Please set TEST_HOST_NAME environment variable");
	//}

	CPPUNIT_ASSERT(!strHostName.IsEmpty());
	CPPUNIT_ASSERT(strHostName.GetLength() > 2);
	CPPUNIT_ASSERT(strHostName.GetLength() < 255);
	
	return strHostName;
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
CString CLibssh2Provider_test::GetUserName() const
{
	CString strUser;
	/*static CString strUser;

	if (strUser.IsEmpty()) // Might be cached in static variable
	{*/
		if(!strUser.GetEnvironmentVariable(_T("TEST_USER_NAME")))
			CPPUNIT_FAIL("Please set TEST_USER_NAME environment variable");
	//}

	CPPUNIT_ASSERT(!strUser.IsEmpty());
	CPPUNIT_ASSERT(strUser.GetLength() > 2);
	CPPUNIT_ASSERT(strUser.GetLength() < 64);
	
	return strUser;
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
USHORT CLibssh2Provider_test::GetPort() const
{
	/* static */ CString strPort;

	//if (strPort.IsEmpty())
	//{
		if(!strPort.GetEnvironmentVariable(_T("TEST_HOST_PORT")))
			return 22; // Default SSH port
	//}
	CPPUNIT_ASSERT(!strPort.IsEmpty());

	int uPort = ::StrToInt(strPort);
	CPPUNIT_ASSERT(uPort >= 0);
	CPPUNIT_ASSERT(uPort <= 65535);

	return (USHORT)uPort;
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
CString CLibssh2Provider_test::GetPassword() const
{
	static CString strPassword;

	if (strPassword.IsEmpty()) // Might be cached in static variable
	{
		if(!strPassword.GetEnvironmentVariable(_T("TEST_PASSWORD")))
			CPPUNIT_FAIL("Please set TEST_PASSWORD environment variable");
	}

	CPPUNIT_ASSERT(!strPassword.IsEmpty());
	
	return strPassword;
}