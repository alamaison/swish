#include "stdafx.h"
#include "CppUnitExtensions.h"
#include "MockSftpConsumer.h"
#include "MockSftpProvider.h"
#include "TestConfig.h"

#include <ATLComTime.h>

// Redefine the 'private' keyword to inject a friend declaration for this 
// test class directly into the target class's header
class CSftpDirectory_test;
#undef private
#define private \
	friend class CSftpDirectory_test; private
#include "../SftpDirectory.h"
#undef private

#include "../Connection.h"

class CSftpDirectory_test : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE( CSftpDirectory_test );
		CPPUNIT_TEST( testCSftpDirectory );
		CPPUNIT_TEST( testGetEnumAll );
		CPPUNIT_TEST( testGetEnumOnlyFiles );
		CPPUNIT_TEST( testGetEnumOnlyFolders );
		CPPUNIT_TEST( testGetEnumNoHidden );
		CPPUNIT_TEST( testGetEnumOnlyFilesNoHidden );
		CPPUNIT_TEST( testGetEnumOnlyFoldersNoHidden );
		CPPUNIT_TEST( testIEnumIDListSurvival );
		CPPUNIT_TEST( testFetch );
		CPPUNIT_TEST( testRename );
	CPPUNIT_TEST_SUITE_END();

public:
	CSftpDirectory_test() :
		m_pDirectory(NULL),
		m_pCoConsumer(NULL), m_pCoProvider(NULL),
		m_pConsumer(NULL), m_pProvider(NULL) {}
	
	void setUp()
	{
		// Start up COM
		HRESULT hr = ::CoInitialize(NULL);
		CPPUNIT_ASSERT_OK(hr);

		CreateMockSftpProvider(&m_pCoProvider, &m_pProvider);
		CreateMockSftpConsumer(&m_pCoConsumer, &m_pConsumer);

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
		if (m_pCoProvider)
			m_pCoProvider->Release();
		if (m_pCoConsumer)
			m_pCoConsumer->Release();

		if (m_pProvider)
		{ // Possible for test to fail before m_pProvider initialised
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

protected:

	void testCSftpDirectory()
	{
		CConnection conn;
		conn.spProvider = m_pProvider;
		conn.spConsumer = m_pConsumer;

		// Test stack creation
		{
			CSftpDirectory directory(conn, CComBSTR("/tmp"));
		}

		// Test heap creation
		m_pDirectory = new CSftpDirectory(conn, CComBSTR("/tmp"));
		delete m_pDirectory;
	}

	void testGetEnumAll()
	{
		SHCONTF grfFlags = 
			SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN;
		_testGetEnum( grfFlags );
	}

	void testGetEnumOnlyFolders()
	{
		SHCONTF grfFlags = SHCONTF_FOLDERS | SHCONTF_INCLUDEHIDDEN;
		_testGetEnum( grfFlags );
	}

	void testGetEnumOnlyFiles()
	{
		SHCONTF grfFlags = SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN;
		_testGetEnum( grfFlags );
	}

	void testGetEnumNoHidden()
	{
		SHCONTF grfFlags = SHCONTF_FOLDERS | SHCONTF_NONFOLDERS ;
		_testGetEnum( grfFlags );
	}

	void testGetEnumOnlyFoldersNoHidden()
	{
		SHCONTF grfFlags = SHCONTF_FOLDERS;
		_testGetEnum( grfFlags );
	}

	void testGetEnumOnlyFilesNoHidden()
	{
		SHCONTF grfFlags = SHCONTF_NONFOLDERS;
		_testGetEnum( grfFlags );
	}

	/**
	 * Tests that the IEnumIDList collection outlives the destruction of the
	 * SftpDirectory
	 */
	void testIEnumIDListSurvival()
	{
		CConnection conn;
		conn.spProvider = m_pProvider;
		conn.spConsumer = m_pConsumer;

		m_pDirectory = new CSftpDirectory(conn, CComBSTR("/tmp"));

		IEnumIDList *pEnum;
		SHCONTF grfFlags = 
			SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN;
		HRESULT hr = m_pDirectory->GetEnum( &pEnum, grfFlags );
		CPPUNIT_ASSERT_OK(hr);

		delete m_pDirectory;

		_testEnumIDList( pEnum, grfFlags );

		ULONG cRefs = pEnum->Release();
		CPPUNIT_ASSERT_EQUAL( (ULONG)0, cRefs );
	}

	void testRename()
	{

	}

	void testFetch()
	{
		//HRESULT hr;

		//hr = m_pDirectory->_Fetch(
		//	CComBSTR("/tmp"),
		//	SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN
		//);
		//CPPUNIT_ASSERT_OK(hr);
	}
private:
	CSftpDirectory *m_pDirectory;

	CComObject<CMockSftpConsumer> *m_pCoConsumer;
	ISftpConsumer *m_pConsumer;
	CComObject<CMockSftpProvider> *m_pCoProvider;
	ISftpProvider *m_pProvider;

	CTestConfig config;


	void _testGetEnum( __in SHCONTF grfFlags )
	{
		CConnection conn;
		conn.spProvider = m_pProvider;
		conn.spConsumer = m_pConsumer;

		CSftpDirectory directory(conn, CComBSTR("/tmp"));

		IEnumIDList *pEnum;
		HRESULT hr = directory.GetEnum( &pEnum, grfFlags );
		CPPUNIT_ASSERT_OK(hr);

		_testEnumIDList( pEnum, grfFlags );

		ULONG cRefs = pEnum->Release();
		CPPUNIT_ASSERT_EQUAL( (ULONG)0, cRefs );
	}

	void _testEnumIDList( __in IEnumIDList *pEnum, __in SHCONTF grfFlags )
	{
		pEnum->AddRef();

		HRESULT hr;
		PITEMID_CHILD pidl;
		ULONG cFetched;
		hr = pEnum->Next(1, &pidl, &cFetched);
		CPPUNIT_ASSERT_OK(hr);
		CPPUNIT_ASSERT_EQUAL((ULONG)1, cFetched);

		do {
			PREMOTEPIDL pidlFile = reinterpret_cast<PREMOTEPIDL>(pidl);

			// Check REMOTEPIDLness
			CPPUNIT_ASSERT_EQUAL(sizeof REMOTEPIDL, (size_t)pidlFile->cb);
			CPPUNIT_ASSERT_EQUAL(
				REMOTEPIDL_FINGERPRINT, pidlFile->dwFingerprint);

			// Check filename
			CPPUNIT_ASSERT( !CString(pidlFile->wszFilename).IsEmpty() );
			if (!(grfFlags & SHCONTF_INCLUDEHIDDEN))
				CPPUNIT_ASSERT( pidlFile->wszFilename[0] != L'.' );

			// Check folderness
			if (!(grfFlags & SHCONTF_FOLDERS))
				CPPUNIT_ASSERT( !pidlFile->fIsFolder );
			if (!(grfFlags & SHCONTF_NONFOLDERS))
				CPPUNIT_ASSERT( pidlFile->fIsFolder );

			// Check group and owner exist
			CPPUNIT_ASSERT( !CString(pidlFile->wszGroup).IsEmpty() );
			CPPUNIT_ASSERT( !CString(pidlFile->wszOwner).IsEmpty() );

			// Check date validity
			CPPUNIT_ASSERT_EQUAL( COleDateTime::valid, 
				COleDateTime(pidlFile->dateModified).GetStatus() );
			
			hr = pEnum->Next(1, &pidl, &cFetched);
		} while (hr == S_OK);
		CPPUNIT_ASSERT( hr == S_FALSE );
		CPPUNIT_ASSERT_EQUAL((ULONG)0, cFetched);

		pEnum->Release();
	}

	/**
	 * Creates a CMockSftpConsumer and returns pointers to its CComObject
	 * as well as its ISftpConsumer interface.
	 */
	static void CreateMockSftpConsumer(
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
	static void CreateMockSftpProvider(
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

CPPUNIT_TEST_SUITE_REGISTRATION( CSftpDirectory_test );
