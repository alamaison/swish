#include "stdafx.h"
#include "../CppUnitExtensions.h"
#include "../MockSftpConsumer.h"
#include "../MockSftpProvider.h"
typedef CMockSftpProvider MP;
typedef CMockSftpConsumer MC;
#include "../TestConfig.h"
#include "DataObjectTests.h"

#include <ATLComTime.h>

// Redefine the 'private' keyword to inject a friend declaration for this 
// test class directly into the target class's header
class CSftpDirectory_test;
#undef private
#define private \
	friend class CSftpDirectory_test; private
#include <SftpDirectory.h>
#undef private

#include <Connection.h>
#include <HostPidl.h>

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
		CPPUNIT_TEST( testGetEnumEmpty );
		CPPUNIT_TEST( testIEnumIDListSurvival );
		CPPUNIT_TEST( testRename );
		CPPUNIT_TEST( testRenameInSubfolder );
		CPPUNIT_TEST( testRenameWithConfirmation );
		CPPUNIT_TEST( testRenameWithConfirmationForbidden );
		CPPUNIT_TEST( testRenameWithErrorReported );
		CPPUNIT_TEST( testRenameFail );
		CPPUNIT_TEST( testCreateDataObjectFile );
		CPPUNIT_TEST( testCreateDataObjectFileMulti );
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

		m_pidlTestHost = CHostItemAbsolute(
			L"testuser", L"testhost", L"/tmp", 22);
	}

	void tearDown()
	{
		m_pidlTestHost.Delete();

		if (m_pDirectory)
			delete m_pDirectory;
		m_pDirectory = NULL;

		if (m_pCoProvider)
			m_pCoProvider->Release();
		m_pCoProvider = NULL;
		if (m_pCoConsumer)
			m_pCoConsumer->Release();
		m_pCoConsumer = NULL;

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
			CSftpDirectory directory(m_pidlTestHost, conn);
		}

		// Test heap creation
		m_pDirectory = new CSftpDirectory(m_pidlTestHost, conn);
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

	void testGetEnumEmpty()
	{
		SHCONTF grfFlags = 
			SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN;

		CConnection conn;
		conn.spProvider = m_pProvider;
		conn.spConsumer = m_pConsumer;

		CSftpDirectory directory(m_pidlTestHost, conn);

		// Set mock behaviour
		m_pCoProvider->SetListingBehaviour(MP::EmptyListing);

		IEnumIDList *pEnum = NULL;
		try
		{
			// Get listing
			pEnum = directory.GetEnum( grfFlags ).Detach();
			CPPUNIT_ASSERT(pEnum);

			// Test
			PITEMID_CHILD pidl;
			ULONG cFetched;
			CPPUNIT_ASSERT(pEnum->Next(1, &pidl, &cFetched) == S_FALSE);
			CPPUNIT_ASSERT_EQUAL((ULONG)0, cFetched);
		}
		catch(...)
		{
			if (pEnum)
				pEnum->Release();
			throw;
		}

		ULONG cRefs = pEnum->Release();
		CPPUNIT_ASSERT_EQUAL( (ULONG)0, cRefs );
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

		m_pDirectory = new CSftpDirectory(m_pidlTestHost, conn);

		SHCONTF grfFlags = 
			SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN;
		IEnumIDList *pEnum = m_pDirectory->GetEnum( grfFlags ).Detach();
		CPPUNIT_ASSERT(pEnum);

		delete m_pDirectory;
		m_pDirectory = NULL;

		_testEnumIDList( pEnum, grfFlags );

		ULONG cRefs = pEnum->Release();
		CPPUNIT_ASSERT_EQUAL( (ULONG)0, cRefs );
	}

	void testRename()
	{
		CConnection conn;
		conn.spProvider = m_pProvider;
		conn.spConsumer = m_pConsumer;

		// Set mock behaviour
		m_pCoProvider->SetRenameBehaviour(MP::RenameOK);

		// Create
		m_pDirectory = new CSftpDirectory(m_pidlTestHost, conn);

		// PIDL of old file.  Would normally come from GetEnum()
		CRemoteItem pidl(L"testtmpfile");

		// Test
		m_pDirectory->Rename(pidl, L"renamed");
	}

	void testRenameInSubfolder()
	{
		CConnection conn;
		conn.spProvider = m_pProvider;
		conn.spConsumer = m_pConsumer;

		// Set mock behaviour
		m_pCoProvider->SetRenameBehaviour(MP::RenameOK);

		// Create
		m_pDirectory = new CSftpDirectory(
			CHostItemAbsolute(
				L"testuser", L"testhost", L"/tmp/swish", 22), conn);

		// PIDL of old file.  Would normally come from GetEnum()
		CRemoteItem pidl(L"testswishfile");

		// Test
		m_pDirectory->Rename(pidl, L"renamed");
	}

	void testRenameWithConfirmation()
	{
		CConnection conn;
		conn.spProvider = m_pProvider;
		conn.spConsumer = m_pConsumer;

		// Set mock behaviour
		m_pCoProvider->SetRenameBehaviour(MP::ConfirmOverwrite);

		// Create
		m_pDirectory = new CSftpDirectory(m_pidlTestHost, conn);

		// PIDL of old file.  Would normally come from GetEnum()
		CRemoteItem pidl(L"testtmpfile");

		// Test that OnConfirmOverwrite is being called by forcing exception
		m_pCoConsumer->SetConfirmOverwriteBehaviour(MC::ThrowOverwrite);
		CPPUNIT_ASSERT_ASSERTION_FAIL_MESSAGE(
			"Rename failed to confirm overwrite",
			m_pDirectory->Rename(pidl, L"renamed")
		);

		// Test again but with proper behaviour
		m_pCoConsumer->SetConfirmOverwriteBehaviour(MC::AllowOverwrite);
		m_pDirectory->Rename(pidl, L"renamed");
	}

	void testRenameWithConfirmationForbidden()
	{
		CConnection conn;
		conn.spProvider = m_pProvider;
		conn.spConsumer = m_pConsumer;

		// Set mock behaviour
		m_pCoProvider->SetRenameBehaviour(MP::ConfirmOverwrite);

		// Create
		m_pDirectory = new CSftpDirectory(m_pidlTestHost, conn);

		// PIDL of old file.  Would normally come from GetEnum()
		CRemoteItem pidl(L"testtmpfile");

		// Test
		m_pCoConsumer->SetConfirmOverwriteBehaviour(MC::PreventOverwrite);
		CPPUNIT_ASSERT_THROW_MESSAGE(
			"Rename() failed to throw an exception despite overwrite "
			"confirmation being rejected",
			m_pDirectory->Rename(pidl, L"renamed"),
			CAtlException
		);

		// Change consumer behaviour and retest
		m_pCoConsumer->SetConfirmOverwriteBehaviour(MC::PreventOverwriteSFalse);
		CPPUNIT_ASSERT_THROW_MESSAGE(
			"Rename() failed to throw an exception despite overwrite "
			"confirmation being rejected with S_FALSE",
			m_pDirectory->Rename(pidl, L"renamed"),
			CAtlException
		);
	}

	void testRenameWithErrorReported()
	{
		CConnection conn;
		conn.spProvider = m_pProvider;
		conn.spConsumer = m_pConsumer;

		// Set mock behaviour
		m_pCoProvider->SetRenameBehaviour(MP::ReportError);

		// Create
		m_pDirectory = new CSftpDirectory(m_pidlTestHost, conn);

		// PIDL of old file.  Would normally come from GetEnum()
		CRemoteItem pidl(L"testtmpfile");

		// Test that OnReportError is being called by forcing exception
		m_pCoConsumer->SetReportErrorBehaviour(MC::ThrowReport);
		CPPUNIT_ASSERT_ASSERTION_FAIL_MESSAGE(
			"Rename failed to report error to mock user",
			m_pDirectory->Rename(pidl, L"renamed")
		);

		// Test properly
		m_pCoConsumer->SetReportErrorBehaviour(MC::ErrorOK);
		CPPUNIT_ASSERT_THROW_MESSAGE(
			"Rename() failed to throw an exception despite forced error",
			m_pDirectory->Rename(pidl, L"renamed"),
			CAtlException
		);
	}

	void testRenameFail()
	{
		CConnection conn;
		conn.spProvider = m_pProvider;
		conn.spConsumer = m_pConsumer;

		// Create
		m_pDirectory = new CSftpDirectory(m_pidlTestHost, conn);

		// PIDL of old file.  Would normally come from GetEnum()
		CRemoteItem pidl(L"testtmpfile");

		// Test E_ABORT failure
		m_pCoProvider->SetRenameBehaviour(MP::AbortRename);
		CPPUNIT_ASSERT_THROW_MESSAGE(
			"Rename() failed to throw an exception despite forced E_ABORT",
			m_pDirectory->Rename(pidl, L"renamed"),
			CAtlException
		);

		// Test E_FAIL failure
		m_pCoProvider->SetRenameBehaviour(MP::FailRename);
		CPPUNIT_ASSERT_THROW_MESSAGE(
			"Rename() failed to throw an exception despite forced E_FAIL",
			m_pDirectory->Rename(pidl, L"renamed"),
			CAtlException
		);
	}

	void testCreateDataObjectFile()
	{
		CConnection conn;
		conn.spProvider = m_pProvider;
		conn.spConsumer = m_pConsumer;

		// Create
		m_pDirectory = new CSftpDirectory(m_pidlTestHost, conn);

		// PIDL of a file.  Would normally come from GetEnum()
		CRemoteItem pidl(L"testtmpfile");

		// Test DataObject creator
		CComPtr<IDataObject> spDo = 
			m_pDirectory->CreateDataObjectFor(1, &(pidl.m_pidl));

		// Test CFSTR_SHELLIDLIST (PIDL array) format
		_testShellPIDLFolder(spDo, L"/tmp");
		_testShellPIDL(spDo, pidl.GetFilename(), 0);

		// Test CFSTR_FILEDESCRIPTOR (FILEGROUPDESCRIPTOR) format
		_testFileDescriptor(spDo, L"testtmpfile", 0);

		// Test CFSTR_FILECONTENTS (IStream) format
		_testStreamContents(spDo, L"/tmp/testtmpfile", 0);
	}

	void testCreateDataObjectFileMulti()
	{
		CConnection conn;
		conn.spProvider = m_pProvider;
		conn.spConsumer = m_pConsumer;

		// Create
		m_pDirectory = new CSftpDirectory(m_pidlTestHost, conn);

		// PIDLs of files.  Would normally come from GetEnum()
		CRemoteItem pidl1(
			L"testtmpfile.ext", false, L"mockowner", L"mockgroup",
			false, 0677, 1024);
		CRemoteItem pidl2(
			L"testtmpfile.txt", false, L"mockowner", L"mockgroup",
			false, 0677, 1024);
		CRemoteItem pidl3(
			L"testtmpfile", false, L"mockowner", L"mockgroup",
			false, 0677, 1024);
		PCUITEMID_CHILD aPidl[3];
		aPidl[0] = pidl1;
		aPidl[1] = pidl2;
		aPidl[2] = pidl3;

		// Test DataObject creator
		CComPtr<IDataObject> spDo = 
			m_pDirectory->CreateDataObjectFor(3, aPidl);

		// Test CFSTR_SHELLIDLIST (PIDL array) format
		_testShellPIDLFolder(spDo, L"/tmp");
		_testShellPIDL(spDo, pidl1.GetFilename(), 0);
		_testShellPIDL(spDo, pidl2.GetFilename(), 1);
		_testShellPIDL(spDo, pidl3.GetFilename(), 2);

		// Test CFSTR_FILEDESCRIPTOR (FILEGROUPDESCRIPTOR) format
		_testFileDescriptor(spDo, pidl1.GetFilename(), 0);
		_testFileDescriptor(spDo, pidl2.GetFilename(), 1);
		_testFileDescriptor(spDo, pidl3.GetFilename(), 2);

		// Test CFSTR_FILECONTENTS (IStream) format
		_testStreamContents(spDo, L"/tmp/testtmpfile.ext", 0);
		_testStreamContents(spDo, L"/tmp/testtmpfile.txt", 1);
		_testStreamContents(spDo, L"/tmp/testtmpfile", 2);
	}


private:
	CSftpDirectory *m_pDirectory;

	CComObject<CMockSftpConsumer> *m_pCoConsumer;
	ISftpConsumer *m_pConsumer;
	CComObject<CMockSftpProvider> *m_pCoProvider;
	ISftpProvider *m_pProvider;

	CTestConfig config;

	CHostItemAbsolute m_pidlTestHost;

	void _testGetEnum( __in SHCONTF grfFlags )
	{
		CConnection conn;
		conn.spProvider = m_pProvider;
		conn.spConsumer = m_pConsumer;

		CSftpDirectory directory(m_pidlTestHost, conn);

		IEnumIDList *pEnum = directory.GetEnum( grfFlags ).Detach();
		CPPUNIT_ASSERT(pEnum);

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
			RemoteItemId *pidlFile = reinterpret_cast<RemoteItemId *>(pidl);

			// Check REMOTEPIDLness
			CPPUNIT_ASSERT_EQUAL(sizeof RemoteItemId, (size_t)pidlFile->cb);
			CPPUNIT_ASSERT_EQUAL(
				RemoteItemId::FINGERPRINT, pidlFile->dwFingerprint);

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
