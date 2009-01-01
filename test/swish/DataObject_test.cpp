#include "stdafx.h"
#include "../CppUnitExtensions.h"
#include "../MockSftpConsumer.h"
#include "../MockSftpProvider.h"
typedef CMockSftpProvider MP;
typedef CMockSftpConsumer MC;
#include "../TestConfig.h"

#include <DataObject.h>
#include <RemotePidl.h>
#include <HostPidl.h>

class CDataObject_test : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE( CDataObject_test );
		CPPUNIT_TEST( testCreate );
		CPPUNIT_TEST( testCreateMulti );
	CPPUNIT_TEST_SUITE_END();

public:
	CDataObject_test() :
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
			L"testswishfile.ext", L"mockowner", L"mockgroup",
			false, false, 0677, 1024);

		CComPtr<IDataObject> spDo = 
			CDataObject::Create(conn, pidlRoot, 1, &(pidl.m_pidl));

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
			L"testswishfile.ext", L"mockowner", L"mockgroup",
			false, false, 0677, 1024);

		CRemoteItem pidl2(
			L"testswishfile.txt", L"mockowner", L"mockgroup",
			false, false, 0677, 1024);

		CRemoteItem pidl3(
			L"testswishFile", L"mockowner", L"mockgroup",
			false, false, 0677, 1024);

		PCUITEMID_CHILD aPidl[3];
		aPidl[0] = pidl1;
		aPidl[1] = pidl2;
		aPidl[2] = pidl3;

		CComPtr<IDataObject> spDo =
			CDataObject::Create(conn, pidlRoot, 3, aPidl);

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

private:

	IDataObject *m_pDo;
	CComObject<CMockSftpConsumer> *m_pCoConsumer;
	ISftpConsumer *m_pConsumer;
	CComObject<CMockSftpProvider> *m_pCoProvider;
	ISftpProvider *m_pProvider;

	CTestConfig config;


#define GetPIDLFolder(pida) \
	(PCIDLIST_ABSOLUTE)(((LPBYTE)pida)+(pida)->aoffset[0])
#define GetPIDLItem(pida, i) \
	(PCIDLIST_RELATIVE)(((LPBYTE)pida)+(pida)->aoffset[i+1])

	/**
	 * Test that Shell PIDL from DataObject represent the expected file.
	 */
	static void _testShellPIDL(
		IDataObject *pDo, CString strExpected, UINT iFile)
	{
		FORMATETC fetc = {
			(CLIPFORMAT)::RegisterClipboardFormat(CFSTR_SHELLIDLIST),
			NULL,
			DVASPECT_CONTENT,
			-1,
			TYMED_HGLOBAL
		};

		STGMEDIUM stg;
		HRESULT hr = pDo->GetData(&fetc, &stg);
		CPPUNIT_ASSERT_OK(hr);
		
		CPPUNIT_ASSERT(stg.hGlobal);
		CIDA *pida = (CIDA *)::GlobalLock(stg.hGlobal);
		CPPUNIT_ASSERT(pida);

		CRemoteItemListHandle pidlActual = GetPIDLItem(pida, iFile);
		CPPUNIT_ASSERT_EQUAL(strExpected, pidlActual.GetFilename());

		::GlobalUnlock(stg.hGlobal);
		pida = NULL;
		::ReleaseStgMedium(&stg);
	}

	/**
	 * Test that Shell PIDL from DataObject represent the common root folder.
	 */
	static void _testShellPIDLFolder(IDataObject *pDo, CString strExpected)
	{
		FORMATETC fetc = {
			(CLIPFORMAT)::RegisterClipboardFormat(CFSTR_SHELLIDLIST),
			NULL,
			DVASPECT_CONTENT,
			-1,
			TYMED_HGLOBAL
		};

		STGMEDIUM stg;
		HRESULT hr = pDo->GetData(&fetc, &stg);
		CPPUNIT_ASSERT_OK(hr);
		
		CPPUNIT_ASSERT(stg.hGlobal);
		CIDA *pida = (CIDA *)::GlobalLock(stg.hGlobal);
		CPPUNIT_ASSERT(pida);

		CRemoteItemHandle pidlActual = ::ILFindLastID(GetPIDLFolder(pida));
		CPPUNIT_ASSERT_EQUAL(strExpected, pidlActual.GetFilename());

		::GlobalUnlock(stg.hGlobal);
		pida = NULL;
		::ReleaseStgMedium(&stg);
	}

	/**
	 * Test that the the FILEGROUPDESCRIPTOR and ith FILEDESCRIPTOR
	 * match expected values.
	 */
	static void _testFileDescriptor(
		IDataObject *pDo, CString strExpected, UINT iFile)
	{
		FORMATETC fetc = {
			(CLIPFORMAT)::RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR),
			NULL,
			DVASPECT_CONTENT,
			-1,
			TYMED_HGLOBAL
		};

		STGMEDIUM stg;
		HRESULT hr = pDo->GetData(&fetc, &stg);
		CPPUNIT_ASSERT_OK(hr);
		
		CPPUNIT_ASSERT(stg.hGlobal);
		FILEGROUPDESCRIPTOR *fgd = 
			(FILEGROUPDESCRIPTOR *)::GlobalLock(stg.hGlobal);
		CPPUNIT_ASSERT(fgd);

		CPPUNIT_ASSERT(iFile < fgd->cItems);
		CPPUNIT_ASSERT(fgd->fgd);
		CString strActual(fgd->fgd[iFile].cFileName);
		CPPUNIT_ASSERT_EQUAL(strExpected, strActual);

		::GlobalUnlock(stg.hGlobal);
		fgd = NULL;
		::ReleaseStgMedium(&stg);
	}

	/**
	 * Test that the contents of the DummyStream matches what is expected.
	 */
	static void _testStreamContents(
		IDataObject *pDo, CString strExpected, UINT iFile)
	{
		FORMATETC fetc = {
			(CLIPFORMAT)::RegisterClipboardFormat(CFSTR_FILECONTENTS),
			NULL,
			DVASPECT_CONTENT,
			iFile,
			TYMED_ISTREAM
		};

		STGMEDIUM stg;
		HRESULT hr = pDo->GetData(&fetc, &stg);
		CPPUNIT_ASSERT_OK(hr);
		
		CPPUNIT_ASSERT(stg.pstm);

		char szBuf[MAX_PATH];
		::ZeroMemory(szBuf, ARRAYSIZE(szBuf));
		ULONG cbRead = 0;
		hr = stg.pstm->Read(szBuf, ARRAYSIZE(szBuf), &cbRead);
		CPPUNIT_ASSERT_OK(hr);

		CString strActual(szBuf);
		CPPUNIT_ASSERT_EQUAL(strExpected, strActual);

		::ReleaseStgMedium(&stg);
	}

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
			L"swish", L"owner", L"group", true, false, 0677, 1024);

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
			L"user", L"test.example.com", 22, L"/tmp", L"Test PIDL");

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

CPPUNIT_TEST_SUITE_REGISTRATION( CDataObject_test );
