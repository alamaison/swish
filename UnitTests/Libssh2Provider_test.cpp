//Libssh2Provider_Test.cpp  -   defines the class Libssh2Provider_Test

#include "stdafx.h"
#include "CppUnitExtensions.h"
#include "MockSftpConsumer.h"
#include "TestConfig.h"

#include <ATLComTime.h>

// Libssh2Provider CLibssh2Provider component
#import "progid:Libssh2Provider.Libssh2Provider" raw_interfaces_only, raw_native_types, auto_search

struct testFILEDATA
{
	BOOL fIsFolder;
	CString strPath;
	CString strOwner;
	CString strGroup;
	CString strAuthor;

	ULONGLONG uSize; // 64-bit allows files up to 16 Ebibytes (a lot)
	time_t dtModified;
	DWORD dwPermissions;
};

class CLibssh2Provider_test : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE( CLibssh2Provider_test );
		CPPUNIT_TEST( testQueryInterface );
		CPPUNIT_TEST( testInitialize );
		CPPUNIT_TEST( testGetListing );
		CPPUNIT_TEST( testGetListing_WrongPassword );
		CPPUNIT_TEST( testGetListingRepeatedly );
		CPPUNIT_TEST( testRename );
		CPPUNIT_TEST( testRenameNoDirectory );
		CPPUNIT_TEST( testRenameFolder );
		CPPUNIT_TEST( testRenameWithRefusedConfirmation );
		CPPUNIT_TEST( testRenameFolderWithRefusedConfirmation );
		CPPUNIT_TEST( testRenameInSubfolder );
		CPPUNIT_TEST( testRenameInNonHomeFolder );
		CPPUNIT_TEST( testRenameInNonHomeSubfolder );
	CPPUNIT_TEST_SUITE_END();

public:
	CLibssh2Provider_test() : 
	    m_pProvider(NULL), m_pConsumer(NULL),
		m_bstrHomeDir(CComBSTR("/home/")+config.GetUser()+CComBSTR("/"))
	{
		HRESULT hr;

		// Start up COM
		hr = ::CoInitialize(NULL);
		CPPUNIT_ASSERT_OK(hr);

		// One-off tests

		// Store Libssh2Provider CLSID
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
		::CoTaskMemFree(pszUuid);

		// Shut down COM
		::CoUninitialize();
	}

	void setUp()
	{
		HRESULT hr;

		// Start up COM
		hr = ::CoInitialize(NULL);
		CPPUNIT_ASSERT_OK(hr);

		// Create instance of Libssh2 Provider using CLSID
		hr = ::CoCreateInstance(
			__uuidof(Libssh2Provider::CLibssh2Provider), NULL,
			CLSCTX_INPROC_SERVER,
			__uuidof(Swish::ISftpProvider), (LPVOID *)&m_pProvider);
		CPPUNIT_ASSERT_OK(hr);

		// Create mock SftpConsumer for use in Initialize()
		_CreateMockSftpConsumer( &m_pCoConsumer, &m_pConsumer );
	}

	void tearDown()
	{
		if (m_pProvider) // Possible for test to fail before initialised
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

protected:

	/**
	 * Test that the class responds to IUnknown::QueryInterface correctly.
	 *
	 * This test will be roughly the same for *any* valid COM object except
	 * one that implement IShellView as this has been chosen to test failure. 
	 * The cases being tested are based on those explained by Raymond Chen:
	 * http://blogs.msdn.com/oldnewthing/archive/2004/03/26/96777.aspx
	 */
	void testQueryInterface()
	{
		HRESULT hr;

		// Supports IUnknown (valid COM object)?
		IUnknown *pUnk;
		hr = m_pProvider->QueryInterface(&pUnk);
		CPPUNIT_ASSERT_OK(hr);
		pUnk->Release();

		// Supports ILibssh2Provider (valid self!)?
		Swish::ISftpProvider *pProv;
		hr = m_pProvider->QueryInterface(&pProv);
		CPPUNIT_ASSERT_OK(hr);
		pProv->Release();

		// Says no properly (Very unlikely to support this - must return NULL)
		IHTMLDOMTextNode2 *pShell = (IHTMLDOMTextNode2 *)this;
		hr = m_pProvider->QueryInterface(&pShell);
		if (SUCCEEDED(hr))
		{
			pShell->Release();
			CPPUNIT_ASSERT(FAILED(hr));
		}
		CPPUNIT_ASSERT(pShell == NULL);
	}

	void testInitialize()
	{
		CComBSTR bstrUser = config.GetUser();
		CComBSTR bstrHost = config.GetHost();

		// Choose mock behaviours
		m_pCoConsumer->SetPasswordBehaviour(CMockSftpConsumer::CustomPassword);
		m_pCoConsumer->SetCustomPassword(config.GetPassword());

		// Test with invalid port values
	#pragma warning (push)
	#pragma warning (disable: 4245) // unsigned signed mismatch
		CPPUNIT_ASSERT_EQUAL(
			E_INVALIDARG,
			m_pProvider->Initialize(m_pConsumer, bstrUser, bstrHost, -1)
		);
		CPPUNIT_ASSERT_EQUAL(
			E_INVALIDARG,
			m_pProvider->Initialize(m_pConsumer, bstrUser, bstrHost, 65536)
		);
	#pragma warning (pop)

		// Run real test
		CPPUNIT_ASSERT_OK(
			m_pProvider->Initialize(
				m_pConsumer, bstrUser, bstrHost, config.GetPort()));
	}

	void testGetListing()
	{
		HRESULT hr;

		CComBSTR bstrUser = config.GetUser();
		CComBSTR bstrHost = config.GetHost();

		// Choose mock behaviours
		m_pCoConsumer->SetPasswordBehaviour(CMockSftpConsumer::CustomPassword);
		m_pCoConsumer->SetCustomPassword(config.GetPassword());

		CPPUNIT_ASSERT_OK(
			m_pProvider->Initialize(
				m_pConsumer, bstrUser, bstrHost, config.GetPort()));

		// Fetch listing enumerator
		Swish::IEnumListing *pEnum;
		CComBSTR bstrDirectory(_T("/tmp"));
		hr = m_pProvider->GetListing(bstrDirectory, &pEnum);
		if (FAILED(hr))
			pEnum = NULL;
		CPPUNIT_ASSERT_OK(hr);

		// Check format of listing is sensible
		_TestListingFormat(pEnum);

		ULONG cRefs = pEnum->Release();
		CPPUNIT_ASSERT_EQUAL( (ULONG)0, cRefs );
	}

	void testGetListing_WrongPassword()
	{
		CComBSTR bstrUser = config.GetUser();
		CComBSTR bstrHost = config.GetHost();

		// Choose mock behaviours
		m_pCoConsumer->SetPasswordBehaviour(CMockSftpConsumer::WrongPassword);
		m_pCoConsumer->SetMaxPasswordAttempts(5); // Tries 5 times then gives up

		CPPUNIT_ASSERT_OK(
			m_pProvider->Initialize(
				m_pConsumer, bstrUser, bstrHost, config.GetPort()));

		// Fetch listing enumerator
		Swish::IEnumListing *pEnum;
		CComBSTR bstrDirectory(_T("/tmp"));
		HRESULT hr = m_pProvider->GetListing(bstrDirectory, &pEnum);
		if (FAILED(hr))
			pEnum = NULL;
		CPPUNIT_ASSERT_FAILED(hr);
	}

	void testGetListingRepeatedly()
	{
		HRESULT hr;

		CComBSTR bstrUser = config.GetUser();
		CComBSTR bstrHost = config.GetHost();

		// Choose mock behaviours
		m_pCoConsumer->SetPasswordBehaviour(CMockSftpConsumer::CustomPassword);
		m_pCoConsumer->SetCustomPassword(config.GetPassword());

		CPPUNIT_ASSERT_OK(
			m_pProvider->Initialize(
				m_pConsumer, bstrUser, bstrHost, config.GetPort()));

		// Fetch 5 listing enumerators
		Swish::IEnumListing *apEnum[5];
		CComBSTR bstrDirectory(_T("/tmp"));
		for (int i = 0; i < 5; i++)
		{
			hr = m_pProvider->GetListing(bstrDirectory, &apEnum[i]);
			if (FAILED(hr))
				apEnum[i] = NULL;
			CPPUNIT_ASSERT_OK(hr);
		}

		// Release 5 listing enumerators
		for (int i = 4; i >= 0; i--)
		{
			ULONG cRefs = apEnum[i]->Release();
			CPPUNIT_ASSERT_EQUAL( (ULONG)0, cRefs );
		}
	}

	void testRename()
	{
		HRESULT hr;

		CComBSTR bstrUser = config.GetUser();
		CComBSTR bstrHost = config.GetHost();

		// Choose mock behaviours
		m_pCoConsumer->SetPasswordBehaviour(CMockSftpConsumer::CustomPassword);
		m_pCoConsumer->SetCustomPassword(config.GetPassword());

		CPPUNIT_ASSERT_OK(
			m_pProvider->Initialize(
				m_pConsumer, bstrUser, bstrHost, config.GetPort()));

		CComBSTR bstrSubject(_HomeDir(L"swishRenameTestFile"));
		CComBSTR bstrTarget(_HomeDir(L"swishRenameTestFilePassed"));

		// Check that our required test subject file exists
		_CheckFileExists(bstrSubject);

		// Test renaming file
		VARIANT_BOOL fWasOverwritten = VARIANT_FALSE;
		hr = m_pProvider->Rename(bstrSubject, bstrTarget, &fWasOverwritten);
		CPPUNIT_ASSERT_OK(hr);
		CPPUNIT_ASSERT(fWasOverwritten == VARIANT_FALSE);

		// Test renaming file back
		hr = m_pProvider->Rename(bstrTarget, bstrSubject, &fWasOverwritten);
		CPPUNIT_ASSERT_OK(hr);
		CPPUNIT_ASSERT(fWasOverwritten == VARIANT_FALSE);
	}

	/**
	 * We are not checking that the file exists beforehand so the libssh2 has
	 * no way to know which directory we intended.  If this passes then it is
	 * defaulting to home directory.
	 */
	void testRenameNoDirectory()
	{
		HRESULT hr;

		CComBSTR bstrUser = config.GetUser();
		CComBSTR bstrHost = config.GetHost();

		// Choose mock behaviours
		m_pCoConsumer->SetPasswordBehaviour(CMockSftpConsumer::CustomPassword);
		m_pCoConsumer->SetCustomPassword(config.GetPassword());

		CPPUNIT_ASSERT_OK(
			m_pProvider->Initialize(
				m_pConsumer, bstrUser, bstrHost, config.GetPort()));

		CComBSTR bstrSubject(_HomeDir(L"swishRenameTestFile"));
		CComBSTR bstrTarget(_HomeDir(L"swishRenameTestFilePassed"));

		// Test renaming file
		VARIANT_BOOL fWasOverwritten = VARIANT_FALSE;
		hr = m_pProvider->Rename(bstrSubject, bstrTarget, &fWasOverwritten);
		CPPUNIT_ASSERT_OK(hr);
		CPPUNIT_ASSERT(fWasOverwritten == VARIANT_FALSE);

		// Test renaming file back
		hr = m_pProvider->Rename(bstrTarget, bstrSubject, &fWasOverwritten);
		CPPUNIT_ASSERT_OK(hr);
		CPPUNIT_ASSERT(fWasOverwritten == VARIANT_FALSE);
	}


	void testRenameFolder()
	{
		HRESULT hr;

		CComBSTR bstrUser = config.GetUser();
		CComBSTR bstrHost = config.GetHost();

		// Choose mock behaviours
		m_pCoConsumer->SetPasswordBehaviour(CMockSftpConsumer::CustomPassword);
		m_pCoConsumer->SetCustomPassword(config.GetPassword());

		CPPUNIT_ASSERT_OK(
			m_pProvider->Initialize(
				m_pConsumer, bstrUser, bstrHost, config.GetPort()));

		CComBSTR bstrSubject(_HomeDir(L"swishRenameTestFolder/"));
		CComBSTR bstrTarget(_HomeDir(L"swishRenameTestFolderPassed/"));

		// Check that our required test subject directory exists
		_CheckFileExists(bstrSubject);

		// Test renaming directory
		VARIANT_BOOL fWasOverwritten = VARIANT_FALSE;
		hr = m_pProvider->Rename(bstrSubject, bstrTarget, &fWasOverwritten);
		CPPUNIT_ASSERT_OK(hr);
		CPPUNIT_ASSERT(fWasOverwritten == VARIANT_FALSE);

		// Test renaming directory back
		hr = m_pProvider->Rename(bstrTarget, bstrSubject, &fWasOverwritten);
		CPPUNIT_ASSERT_OK(hr);
		CPPUNIT_ASSERT(fWasOverwritten == VARIANT_FALSE);
	}

	void testRenameWithRefusedConfirmation()
	{
		HRESULT hr;

		CComBSTR bstrUser = config.GetUser();
		CComBSTR bstrHost = config.GetHost();

		// Choose mock behaviours
		m_pCoConsumer->SetPasswordBehaviour(CMockSftpConsumer::CustomPassword);
		m_pCoConsumer->SetCustomPassword(config.GetPassword());
		m_pCoConsumer->SetConfirmOverwriteBehaviour(
			CMockSftpConsumer::PreventOverwrite);

		CPPUNIT_ASSERT_OK(
			m_pProvider->Initialize(
				m_pConsumer, bstrUser, bstrHost, config.GetPort()));

		CComBSTR bstrSubject(_HomeDir(L"swishRenameTestFile"));
		CComBSTR bstrTarget(_HomeDir(L"swishRenameTestFileObstruction"));

		// Check that our required test subject files exist
		_CheckFileExists(bstrSubject);
		_CheckFileExists(bstrTarget);

		// Test renaming file
		VARIANT_BOOL fWasOverwritten = VARIANT_FALSE;
		hr = m_pProvider->Rename(bstrSubject, bstrTarget, &fWasOverwritten);
		CPPUNIT_ASSERT_FAILED(hr);
		CPPUNIT_ASSERT(fWasOverwritten == VARIANT_FALSE);

		// Check that both files still exist
		_CheckFileExists(bstrSubject);
		_CheckFileExists(bstrTarget);
	}

	void testRenameFolderWithRefusedConfirmation()
	{
		HRESULT hr;

		CComBSTR bstrUser = config.GetUser();
		CComBSTR bstrHost = config.GetHost();

		// Choose mock behaviours
		m_pCoConsumer->SetPasswordBehaviour(CMockSftpConsumer::CustomPassword);
		m_pCoConsumer->SetCustomPassword(config.GetPassword());
		m_pCoConsumer->SetConfirmOverwriteBehaviour(
			CMockSftpConsumer::PreventOverwrite);

		CPPUNIT_ASSERT_OK(
			m_pProvider->Initialize(
				m_pConsumer, bstrUser, bstrHost, config.GetPort()));

		CComBSTR bstrSubject(_HomeDir(L"swishRenameTestFolder/"));
		CComBSTR bstrTarget(_HomeDir(L"swishRenameTestFolderObstruction/"));

		// Check that our required test subject directories exist
		_CheckFileExists(bstrSubject);
		_CheckFileExists(bstrTarget);

		// Test renaming directory
		VARIANT_BOOL fWasOverwritten = VARIANT_FALSE;
		hr = m_pProvider->Rename(bstrSubject, bstrTarget, &fWasOverwritten);
		CPPUNIT_ASSERT_FAILED(hr);
		CPPUNIT_ASSERT(fWasOverwritten == VARIANT_FALSE);

		// Check that both directories still exist
		_CheckFileExists(bstrSubject);
		_CheckFileExists(bstrTarget);
	}

	void testRenameInSubfolder()
	{
		HRESULT hr;

		CComBSTR bstrUser = config.GetUser();
		CComBSTR bstrHost = config.GetHost();

		// Choose mock behaviours
		m_pCoConsumer->SetPasswordBehaviour(CMockSftpConsumer::CustomPassword);
		m_pCoConsumer->SetCustomPassword(config.GetPassword());

		CPPUNIT_ASSERT_OK(
			m_pProvider->Initialize(
				m_pConsumer, bstrUser, bstrHost, config.GetPort()));

		CComBSTR bstrSubject(_HomeDir(L"swishSubfolder/subRenameTestFile"));
		CComBSTR bstrTarget(_HomeDir(L"swishSubfolder/subRenameFilePassed"));

		// Check that our required test subject file exists
		_CheckFileExists(bstrSubject);

		// Test renaming file
		VARIANT_BOOL fWasOverwritten = VARIANT_FALSE;
		hr = m_pProvider->Rename(bstrSubject, bstrTarget, &fWasOverwritten);
		CPPUNIT_ASSERT_OK(hr);
		CPPUNIT_ASSERT(fWasOverwritten == VARIANT_FALSE);

		// Test renaming file back
		hr = m_pProvider->Rename(bstrTarget, bstrSubject, &fWasOverwritten);
		CPPUNIT_ASSERT_OK(hr);
		CPPUNIT_ASSERT(fWasOverwritten == VARIANT_FALSE);
	}

	void testRenameInNonHomeFolder()
	{
		HRESULT hr;

		CComBSTR bstrUser = config.GetUser();
		CComBSTR bstrHost = config.GetHost();

		// Choose mock behaviours
		m_pCoConsumer->SetPasswordBehaviour(CMockSftpConsumer::CustomPassword);
		m_pCoConsumer->SetCustomPassword(config.GetPassword());

		CPPUNIT_ASSERT_OK(
			m_pProvider->Initialize(
				m_pConsumer, bstrUser, bstrHost, config.GetPort()));

		CComBSTR bstrSubject(L"/tmp/swishNonHomeTestFile");
		CComBSTR bstrTarget(L"/tmp/swishNonHomeFilePassed");

		// Check that our required test subject file exists
		_CheckFileExists(bstrSubject);

		// Test renaming file
		VARIANT_BOOL fWasOverwritten = VARIANT_FALSE;
		hr = m_pProvider->Rename(bstrSubject, bstrTarget, &fWasOverwritten);
		CPPUNIT_ASSERT_OK(hr);
		CPPUNIT_ASSERT(fWasOverwritten == VARIANT_FALSE);

		// Test renaming file back
		hr = m_pProvider->Rename(bstrTarget, bstrSubject, &fWasOverwritten);
		CPPUNIT_ASSERT_OK(hr);
		CPPUNIT_ASSERT(fWasOverwritten == VARIANT_FALSE);
	}

	void testRenameInNonHomeSubfolder()
	{
		HRESULT hr;

		CComBSTR bstrUser = config.GetUser();
		CComBSTR bstrHost = config.GetHost();

		// Choose mock behaviours
		m_pCoConsumer->SetPasswordBehaviour(CMockSftpConsumer::CustomPassword);
		m_pCoConsumer->SetCustomPassword(config.GetPassword());

		CPPUNIT_ASSERT_OK(
			m_pProvider->Initialize(
				m_pConsumer, bstrUser, bstrHost, config.GetPort()));

		CComBSTR bstrSubject(L"/tmp/swishNonHomeFolder/subNonHomeTestFile");
		CComBSTR bstrTarget(L"/tmp/swishNonHomeFolder/subNonHomeFilePassed");

		// Check that our required test subject file exists
		_CheckFileExists(bstrSubject);

		// Test renaming file
		VARIANT_BOOL fWasOverwritten = VARIANT_FALSE;
		hr = m_pProvider->Rename(bstrSubject, bstrTarget, &fWasOverwritten);
		CPPUNIT_ASSERT_OK(hr);
		CPPUNIT_ASSERT(fWasOverwritten == VARIANT_FALSE);

		// Test renaming file back
		hr = m_pProvider->Rename(bstrTarget, bstrSubject, &fWasOverwritten);
		CPPUNIT_ASSERT_OK(hr);
		CPPUNIT_ASSERT(fWasOverwritten == VARIANT_FALSE);
	}

private:
	CComObject<CMockSftpConsumer> *m_pCoConsumer;
	Swish::ISftpConsumer *m_pConsumer;
	Swish::ISftpProvider *m_pProvider;
	CTestConfig config;
	const CComBSTR m_bstrHomeDir;

	/**
	 * Tests that the format of the enumeration of listings is correct.
	 *
	 * @param pEnum The Listing enumerator to be tested.
	 */
	void _TestListingFormat(__in Swish::IEnumListing *pEnum) const
	{
		// Check format of listing is sensible
		CPPUNIT_ASSERT_OK( pEnum->Reset() );
		Swish::Listing lt;
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
			fd.uSize = lt.uSize;
			fd.dtModified = (time_t) COleDateTime(lt.dateModified);

			CString strOwner2 = lt.bstrOwner;
			CPPUNIT_ASSERT( !strFilename.IsEmpty() );

			CPPUNIT_ASSERT( lt.uPermissions > 0 );
			CPPUNIT_ASSERT( lt.cHardLinks > 0 );
			CPPUNIT_ASSERT( lt.uSize >= 0 );
			CPPUNIT_ASSERT( !strOwner.IsEmpty() );
			CPPUNIT_ASSERT( !strGroup.IsEmpty() );

			CPPUNIT_ASSERT( lt.dateModified );
			COleDateTime dateModified( lt.dateModified );
			// Check year
			CPPUNIT_ASSERT( dateModified.GetYear() >= 1604 );
			CPPUNIT_ASSERT( 
				dateModified.GetYear() <= 
				COleDateTime::GetCurrentTime().GetYear()
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
			CPPUNIT_ASSERT_EQUAL(COleDateTime::valid, dateModified.GetStatus());

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

	void _CheckFileExists(__in PCTSTR pszFilePath)
	{
		HRESULT hr;
		CString strFilePath(pszFilePath);

		// Strip trailing slash
		strFilePath.TrimRight(_T('/'));
		
		// Find directory portion of path
		int iLastSep = strFilePath.ReverseFind(_T('/'));
		CString strDirectory = strFilePath.Left(iLastSep+1);
		int cFilenameLen = strFilePath.GetLength() - (iLastSep+1);
		CString strFilename = strFilePath.Right(cFilenameLen);

		// Fetch listing enumerator
		Swish::IEnumListing *pEnum;
		hr = m_pProvider->GetListing(CComBSTR(strDirectory), &pEnum);
		if (FAILED(hr))
			pEnum = NULL;
		CPPUNIT_ASSERT_OK(hr);

		// Search for file
		Swish::Listing lt;
		bool fFoundSubjectFile = false;
		hr = pEnum->Next(1, &lt, NULL);
		CPPUNIT_ASSERT_OK(hr);
		while (hr == S_OK)
		{
			if (strFilename == CComBSTR(lt.bstrFilename))
			{
				fFoundSubjectFile = true;
				break;
			}

			hr = pEnum->Next(1, &lt, NULL);
		}
		ULONG cRefs = pEnum->Release();
		CPPUNIT_ASSERT_EQUAL( (ULONG)0, cRefs );
		char szMessage[300];
		_snprintf_s(szMessage, 300, MAX_PATH,
			"Rename test subject missing: %s", CW2A(strFilePath));
		CPPUNIT_ASSERT_MESSAGE( szMessage, fFoundSubjectFile );
	}

	/**
	 * Returns path as subpath of home directory in a BSTR.
	 */
	CComBSTR _HomeDir(CString strPath) const
	{
		CComBSTR bstrFullPath(m_bstrHomeDir);
		bstrFullPath += strPath;
		return bstrFullPath;
	}

	/**
	 * Creates a CMockSftpConsumer and returns pointers to its CComObject
	 * as well as its ISftpConsumer interface.
	 */
	void _CreateMockSftpConsumer(
		__out CComObject<CMockSftpConsumer> **ppCoConsumer,
		__out Swish::ISftpConsumer **ppConsumer
	) const
	{
		HRESULT hr;

		// Create mock object coclass instance
		*ppCoConsumer = NULL;
		hr = CComObject<CMockSftpConsumer>::CreateInstance(ppCoConsumer);
		CPPUNIT_ASSERT_OK(hr);
		CPPUNIT_ASSERT(*ppCoConsumer);

		// Get ISftpConsumer interface
		*ppConsumer = NULL;
		(*ppCoConsumer)->QueryInterface(ppConsumer);
		CPPUNIT_ASSERT(*ppConsumer);
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( CLibssh2Provider_test );
