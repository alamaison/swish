#include "stdafx.h"
#include "../CppUnitExtensions.h"
#include "../MockSftpConsumer.h"
#include "../MockSftpProvider.h"
typedef CMockSftpProvider MP;
typedef CMockSftpConsumer MC;
#include "../TestConfig.h"
#include "DataObjectTests.h"

#include <SftpDataObject.h>
#include <RemotePidl.h>
#include <HostPidl.h>

class CSftpDataObject_test : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE( CSftpDataObject_test );
		CPPUNIT_TEST( testCreate );
		CPPUNIT_TEST( testCreateMulti );
		CPPUNIT_TEST( testQueryFormatsEmpty );
		CPPUNIT_TEST( testEnumFormatsEmpty );
		CPPUNIT_TEST( testQueryFormats );
		CPPUNIT_TEST( testEnumFormats );
		CPPUNIT_TEST( testQueryFormatsMulti );
		CPPUNIT_TEST( testEnumFormatsMulti );
	CPPUNIT_TEST_SUITE_END();

public:
	CSftpDataObject_test() :
		m_pDo(NULL),
		m_pCoConsumer(NULL), m_pCoProvider(NULL),
		m_pConsumer(NULL), m_pProvider(NULL) {}
	
	void setUp()
	{
		// Start up COM
		HRESULT hr = ::CoInitialize(NULL);
		CPPUNIT_ASSERT_OK(hr);

		_CreateMockSftpProvider(&m_pCoProvider, &m_pProvider);
		_CreateMockSftpConsumer(&m_pCoConsumer, &m_pConsumer);

		m_pProvider->Initialize(
			m_pConsumer, CComBSTR(config.GetUser()),
			CComBSTR(config.GetHost()), config.GetPort()
		);

		CConnection conn;
		conn.spProvider = m_pProvider;
		conn.spConsumer = m_pConsumer;
		CPPUNIT_ASSERT(conn.spProvider);
		CPPUNIT_ASSERT(conn.spConsumer);
	}

	void tearDown()
	{
		try
		{
			if (m_pDo) // Test for leaking refs to DataObject
			{
				CPPUNIT_ASSERT_ZERO(m_pDo->Release());
				m_pDo = NULL;
			}

			if (m_pCoProvider)
				m_pCoProvider->Release();
			m_pCoProvider = NULL;
			if (m_pCoConsumer)
				m_pCoConsumer->Release();
			m_pCoConsumer = NULL;

			if (m_pProvider) // Same again for mock provider
			{
				CPPUNIT_ASSERT_ZERO(m_pProvider->Release());
				m_pProvider = NULL;
			}

			if (m_pConsumer) // Same again for mock consumer
			{
				CPPUNIT_ASSERT_ZERO(m_pConsumer->Release());
				m_pConsumer = NULL;
			}
		}
		catch(...)
		{
			// Shut down COM
			::CoUninitialize();
			throw;
		}

		// Shut down COM
		::CoUninitialize();
	}

protected:

	void testCreate()
	{
		CConnection conn;
		conn.spProvider = m_pProvider;
		conn.spConsumer = m_pConsumer;
		CAbsolutePidl pidlRoot = _CreateRootRemotePidl();
		CRemoteItem pidl(
			L"testswishfile.ext", false, L"mockowner", L"mockgroup",
			false, 0677, 1024);

		CComPtr<IDataObject> spDo = 
			CSftpDataObject::Create(1, &(pidl.m_pidl), pidlRoot, conn);

		// Keep extra reference to check for leaks in tearDown()
		spDo.CopyTo(&m_pDo);

		// Test CFSTR_SHELLIDLIST (PIDL array) format
		CRemoteItemHandle pidlFolder = ::ILFindLastID(pidlRoot);
		_testShellPIDLFolder(spDo, pidlFolder.GetFilename());
		_testShellPIDL(spDo, pidl.GetFilename(), 0);

		// Test CFSTR_FILEDESCRIPTOR (FILEGROUPDESCRIPTOR) format
		_testFileDescriptor(spDo, L"testswishfile.ext", 0);

		// Test CFSTR_FILECONTENTS (IStream) format
		_testStreamContents(spDo, L"/tmp/swish/testswishfile.ext", 0);
	}

	void testCreateMulti()
	{
		CConnection conn;
		conn.spProvider = m_pProvider;
		conn.spConsumer = m_pConsumer;
		CAbsolutePidl pidlRoot = _CreateRootRemotePidl();
		CRemoteItem pidl1(
			L"testswishfile.ext", false, L"mockowner", L"mockgroup",
			false, 0677, 1024);
		CRemoteItem pidl2(
			L"testswishfile.txt", false, L"mockowner", L"mockgroup",
			false, 0677, 1024);
		CRemoteItem pidl3(
			L"testswishFile", false, L"mockowner", L"mockgroup",
			false, 0677, 1024);
		PCITEMID_CHILD aPidl[3];
		aPidl[0] = pidl1;
		aPidl[1] = pidl2;
		aPidl[2] = pidl3;

		CComPtr<IDataObject> spDo =
			CSftpDataObject::Create(3, aPidl, pidlRoot, conn);

		// Keep extra reference to check for leaks in tearDown()
		m_pDo = spDo.p;
		m_pDo->AddRef();

		// Test CFSTR_SHELLIDLIST (PIDL array) format
		CRemoteItemHandle pidlFolder = ::ILFindLastID(pidlRoot);
		_testShellPIDLFolder(spDo, pidlFolder.GetFilename());
		_testShellPIDL(spDo, pidl1.GetFilename(), 0);
		_testShellPIDL(spDo, pidl2.GetFilename(), 1);
		_testShellPIDL(spDo, pidl3.GetFilename(), 2);

		// Test CFSTR_FILEDESCRIPTOR (FILEGROUPDESCRIPTOR) format
		_testFileDescriptor(spDo, L"testswishfile.ext", 0);
		_testFileDescriptor(spDo, L"testswishfile.txt", 1);
		_testFileDescriptor(spDo, L"testswishFile", 2);

		// Test CFSTR_FILECONTENTS (IStream) format
		_testStreamContents(spDo, L"/tmp/swish/testswishfile.ext", 0);
		_testStreamContents(spDo, L"/tmp/swish/testswishfile.txt", 1);
		_testStreamContents(spDo, L"/tmp/swish/testswishFile", 2);
	}

	/**
	 * Test that QueryGetData fails for all our formats when created with
	 * empty PIDL list.
	 */
	void testQueryFormatsEmpty()
	{
		CConnection conn;
		conn.spProvider = m_pProvider;
		conn.spConsumer = m_pConsumer;

		CComPtr<IDataObject> spDo = CSftpDataObject::Create(0, NULL, NULL, conn);

		// Keep extra reference to check for leaks in tearDown()
		spDo.CopyTo(&m_pDo);

		// Perform query tests
		_testQueryFormats(spDo, true);
	}

	/**
	 * Test that none of our expected formats are in the enumerator when 
	 * created with empty PIDL list.
	 */
	void testEnumFormatsEmpty()
	{
		CConnection conn;
		conn.spProvider = m_pProvider;
		conn.spConsumer = m_pConsumer;

		CComPtr<IDataObject> spDo = CSftpDataObject::Create(0, NULL, NULL, conn);

		// Keep extra reference to check for leaks in tearDown()
		spDo.CopyTo(&m_pDo);

		// Test enumerators of both GetData() and SetData() formats
		_testBothEnumerators(spDo, true);
	}

	/**
	 * Test that QueryGetData responds successfully for all our formats.
	 */
	void testQueryFormats()
	{
		CConnection conn;
		conn.spProvider = m_pProvider;
		conn.spConsumer = m_pConsumer;
		CAbsolutePidl pidlRoot = _CreateRootRemotePidl();
		CRemoteItem pidl(
			L"testswishfile.ext", false, L"mockowner", L"mockgroup",
			false, 0677, 1024);

		CComPtr<IDataObject> spDo = 
			CSftpDataObject::Create(1, &(pidl.m_pidl), pidlRoot, conn);

		// Keep extra reference to check for leaks in tearDown()
		spDo.CopyTo(&m_pDo);

		// Perform query tests
		_testQueryFormats(spDo);
	}

	/**
	 * Test that all our expected formats are in the enumeration.
	 */
	void testEnumFormats()
	{
		CConnection conn;
		conn.spProvider = m_pProvider;
		conn.spConsumer = m_pConsumer;
		CAbsolutePidl pidlRoot = _CreateRootRemotePidl();
		CRemoteItem pidl(
			L"testswishfile.ext", false, L"mockowner", L"mockgroup",
			false, 0677, 1024);

		CComPtr<IDataObject> spDo = 
			CSftpDataObject::Create(1, &(pidl.m_pidl), pidlRoot, conn);

		// Keep extra reference to check for leaks in tearDown()
		spDo.CopyTo(&m_pDo);

		// Test enumerators of both GetData() and SetData() formats
		_testBothEnumerators(spDo);
	}

	/**
	 * Test that QueryGetData responds successfully for all our formats when
	 * initialised with multiple PIDLs.
	 */
	void testQueryFormatsMulti()
	{
		CConnection conn;
		conn.spProvider = m_pProvider;
		conn.spConsumer = m_pConsumer;
		CAbsolutePidl pidlRoot = _CreateRootRemotePidl();
		CRemoteItem pidl1(
			L"testswishfile.ext", false, L"mockowner", L"mockgroup",
			false, 0677, 1024);
		CRemoteItem pidl2(
			L"testswishfile.txt", false, L"mockowner", L"mockgroup",
			false, 0677, 1024);
		CRemoteItem pidl3(
			L"testswishFile", false, L"mockowner", L"mockgroup",
			false, 0677, 1024);
		PCITEMID_CHILD aPidl[3];
		aPidl[0] = pidl1;
		aPidl[1] = pidl2;
		aPidl[2] = pidl3;

		CComPtr<IDataObject> spDo =
			CSftpDataObject::Create(3, aPidl, pidlRoot, conn);

		// Keep extra reference to check for leaks in tearDown()
		spDo.CopyTo(&m_pDo);

		// Perform query tests
		_testQueryFormats(spDo);
	}

	/**
	 * Test that all our expected formats are in the enumeration when
	 * initialised with multiple PIDLs.
	 */
	void testEnumFormatsMulti()
	{
		CConnection conn;
		conn.spProvider = m_pProvider;
		conn.spConsumer = m_pConsumer;
		CAbsolutePidl pidlRoot = _CreateRootRemotePidl();
		CRemoteItem pidl1(
			L"testswishfile.ext", false, L"mockowner", L"mockgroup",
			false, 0677, 1024);
		CRemoteItem pidl2(
			L"testswishfile.txt", false, L"mockowner", L"mockgroup",
			false, 0677, 1024);
		CRemoteItem pidl3(
			L"testswishFile", false, L"mockowner", L"mockgroup",
			false, 0677, 1024);
		PCITEMID_CHILD aPidl[3];
		aPidl[0] = pidl1;
		aPidl[1] = pidl2;
		aPidl[2] = pidl3;

		CComPtr<IDataObject> spDo =
			CSftpDataObject::Create(3, aPidl, pidlRoot, conn);

		// Keep extra reference to check for leaks in tearDown()
		spDo.CopyTo(&m_pDo);

		// Test enumerators of both GetData() and SetData() formats
		_testBothEnumerators(spDo);
	}

private:

	IDataObject *m_pDo;
	CComObject<CMockSftpConsumer> *m_pCoConsumer;
	ISftpConsumer *m_pConsumer;
	CComObject<CMockSftpProvider> *m_pCoProvider;
	ISftpProvider *m_pProvider;

	CTestConfig config;

	/**
	 * Get the PIDL which represents the HostFolder (Swish icon) in Explorer.
	 */
	static CAbsolutePidl _GetSwishPidl()
	{
		HRESULT hr;
		CAbsolutePidl pidl;
		CComPtr<IShellFolder> spDesktop;
		hr = ::SHGetDesktopFolder(&spDesktop);
		CPPUNIT_ASSERT_OK(hr);
		hr = spDesktop->ParseDisplayName(NULL, NULL, 
			L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\"
			L"::{B816A83A-5022-11DC-9153-0090F5284F85}", NULL, 
			reinterpret_cast<PIDLIST_RELATIVE *>(&pidl), NULL);
		CPPUNIT_ASSERT_OK(hr);

		return pidl;
	}

	/**
	 * Get an absolute PIDL that ends in a REMOTEPIDL to root RemoteFolder on.
	 */
	static CAbsolutePidl _CreateRootRemotePidl()
	{
		// Create test absolute HOSTPIDL
		CAbsolutePidl pidlHost = _CreateRootHostPidl();
		
		// Create root child REMOTEPIDL
		CRemoteItem pidlRemote(
			L"swish", true, L"owner", L"group", false, 0677, 1024);

		// Concatenate to make absolute pidl to RemoteFolder root
		return CAbsolutePidl(pidlHost, pidlRemote);
	}

	/**
	 * Get an absolute PIDL that ends in a HOSTPIDL to root RemoteFolder on.
	 */
	static CAbsolutePidl _CreateRootHostPidl()
	{
		// Create absolute PIDL to Swish icon
		CAbsolutePidl pidlSwish = _GetSwishPidl();
		
		// Create test child HOSTPIDL
		CHostItem pidlHost(
			L"user", L"test.example.com", L"/tmp", 22, L"Test PIDL");

		// Concatenate to make absolute pidl to RemoteFolder root
		return CAbsolutePidl(pidlSwish, pidlHost);
	}

	/**
	 * Creates a CMockSftpConsumer and returns pointers to its CComObject
	 * as well as its ISftpConsumer interface.
	 */
	static void _CreateMockSftpConsumer(
		__out CComObject<CMockSftpConsumer> **ppCoConsumer,
		__out ISftpConsumer **ppConsumer
	)
	{
		// Create mock object coclass instance
		*ppCoConsumer = NULL;
		HRESULT hr;
		hr = CComObject<CMockSftpConsumer>::CreateInstance(ppCoConsumer);
		CPPUNIT_ASSERT_OK(hr);
		CPPUNIT_ASSERT(*ppCoConsumer);
		(*ppCoConsumer)->AddRef();

		// Get ISftpConsumer interface
		*ppConsumer = NULL;
		(*ppCoConsumer)->QueryInterface(ppConsumer);
		CPPUNIT_ASSERT(*ppConsumer);
	}

	/**
	 * Creates a CMockSftpProvider and returns pointers to its CComObject
	 * as well as its ISftpProvider interface.
	 */
	static void _CreateMockSftpProvider(
		__out CComObject<CMockSftpProvider> **ppCoProvider,
		__out ISftpProvider **ppProvider
	)
	{
		// Create mock object coclass instance
		*ppCoProvider = NULL;
		HRESULT hr;
		hr = CComObject<CMockSftpProvider>::CreateInstance(ppCoProvider);
		CPPUNIT_ASSERT_OK(hr);
		CPPUNIT_ASSERT(*ppCoProvider);
		(*ppCoProvider)->AddRef();

		// Get ISftpConsumer interface
		*ppProvider = NULL;
		(*ppCoProvider)->QueryInterface(ppProvider);
		CPPUNIT_ASSERT(*ppProvider);
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( CSftpDataObject_test );
