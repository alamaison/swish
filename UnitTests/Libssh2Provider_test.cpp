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

#define CHECK_PATH_EXISTS(path) _CheckPathExists(path, CPPUNIT_SOURCELINE())
#define CHECK_PATH_NOT_EXISTS(path) \
	_CheckPathNotExists(path, CPPUNIT_SOURCELINE())

class CLibssh2Provider_test : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE( CLibssh2Provider_test );
		CPPUNIT_TEST( testQueryInterface );
		CPPUNIT_TEST( testInitialize );
		CPPUNIT_TEST( testGetListing );
		CPPUNIT_TEST( testGetListing_WrongPassword );
		CPPUNIT_TEST( testGetListingRepeatedly );
		CPPUNIT_TEST( testGetListingIndependence );
		CPPUNIT_TEST( testRename );
		CPPUNIT_TEST( testRenameNoDirectory );
		CPPUNIT_TEST( testRenameFolder );
		CPPUNIT_TEST( testRenameWithRefusedConfirmation );
		CPPUNIT_TEST( testRenameFolderWithRefusedConfirmation );
		CPPUNIT_TEST( testRenameInNonHomeFolder );
		CPPUNIT_TEST( testRenameInNonHomeSubfolder );
		CPPUNIT_TEST( testCreateAndDelete );
		CPPUNIT_TEST( testCreateAndDeleteEmptyDirectory );
		CPPUNIT_TEST( testCreateAndDeleteDirectoryRecursive );
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
		_StandardSetup();

		HRESULT hr;

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
		_StandardSetup();

		HRESULT hr;

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

	void testGetListingIndependence()
	{
		_StandardSetup();

		HRESULT hr;

		// Put some files in the test area
		CComBSTR bstrDirectory(_TestArea());
		CComBSTR bstrOne(_TestArea(L"GetListingIndependence1"));
		CComBSTR bstrTwo(_TestArea(L"GetListingIndependence2"));
		CComBSTR bstrThree(_TestArea(L"GetListingIndependence3"));
		CPPUNIT_ASSERT_OK(m_pProvider->CreateNewFile(bstrOne));
		CPPUNIT_ASSERT_OK(m_pProvider->CreateNewFile(bstrTwo));
		CPPUNIT_ASSERT_OK(m_pProvider->CreateNewFile(bstrThree));

		// Fetch first listing enumerator
		Swish::IEnumListing *pEnumBefore;
		hr = m_pProvider->GetListing(bstrDirectory, &pEnumBefore);
		CPPUNIT_ASSERT_OK(hr);

		// Delete one of the files
		CPPUNIT_ASSERT_OK(m_pProvider->Delete(bstrTwo));

		// Fetch second listing enumerator
		Swish::IEnumListing *pEnumAfter;
		hr = m_pProvider->GetListing(bstrDirectory, &pEnumAfter);
		CPPUNIT_ASSERT_OK(hr);

		// The first listing should still show the file. The second should not.
		CPPUNIT_ASSERT(_FileExistsInListing(
			L"GetListingIndependence1", pEnumBefore));
		CPPUNIT_ASSERT(_FileExistsInListing(
			L"GetListingIndependence2", pEnumBefore));
		CPPUNIT_ASSERT(_FileExistsInListing(
			L"GetListingIndependence3", pEnumBefore));
		CPPUNIT_ASSERT(_FileExistsInListing(
			L"GetListingIndependence1", pEnumAfter));
		CPPUNIT_ASSERT(!_FileExistsInListing(
			L"GetListingIndependence2", pEnumAfter));
		CPPUNIT_ASSERT(_FileExistsInListing(
			L"GetListingIndependence3", pEnumAfter));

		// Cleanup
		CPPUNIT_ASSERT_OK(m_pProvider->Delete(bstrOne));
		CPPUNIT_ASSERT_OK(m_pProvider->Delete(bstrThree));
		
		ULONG cRefs = pEnumBefore->Release();
		CPPUNIT_ASSERT_EQUAL( (ULONG)0, cRefs );

		cRefs = pEnumAfter->Release();
		CPPUNIT_ASSERT_EQUAL( (ULONG)0, cRefs );
	}

	void testRename()
	{
		_StandardSetup();

		HRESULT hr;

		CComBSTR bstrSubject(_TestArea(L"Rename"));
		CComBSTR bstrTarget(_TestArea(L"Rename_Passed"));

		// Create our test subject and check existence
		CPPUNIT_ASSERT_OK(m_pProvider->CreateNewFile(bstrSubject));
		CHECK_PATH_EXISTS(bstrSubject);
		CHECK_PATH_NOT_EXISTS(bstrTarget);

		// Test renaming file
		VARIANT_BOOL fWasOverwritten = VARIANT_FALSE;
		hr = m_pProvider->Rename(bstrSubject, bstrTarget, &fWasOverwritten);
		CPPUNIT_ASSERT_OK(hr);
		CPPUNIT_ASSERT(fWasOverwritten == VARIANT_FALSE);

		// Test renaming file back
		hr = m_pProvider->Rename(bstrTarget, bstrSubject, &fWasOverwritten);
		CPPUNIT_ASSERT_OK(hr);
		CPPUNIT_ASSERT(fWasOverwritten == VARIANT_FALSE);

		// Check that the target does not still exist
		CHECK_PATH_NOT_EXISTS(bstrTarget);

		// Cleanup
		CPPUNIT_ASSERT_OK(m_pProvider->Delete(bstrSubject));
	}

	/**
	 * We are not checking that the file exists beforehand so the libssh2 has
	 * no way to know which directory we intended.  If this passes then it is
	 * defaulting to home directory.
	 */
	void testRenameNoDirectory()
	{
		_StandardSetup();

		HRESULT hr;

		CComBSTR bstrSubject(L"RenameNoDirectory");
		CComBSTR bstrTarget(L"RenameNoDirectory_Passed");
		CPPUNIT_ASSERT_OK(m_pProvider->CreateNewFile(bstrSubject));

		// Test renaming file
		VARIANT_BOOL fWasOverwritten = VARIANT_FALSE;
		hr = m_pProvider->Rename(bstrSubject, bstrTarget, &fWasOverwritten);
		CPPUNIT_ASSERT_OK(hr);
		CPPUNIT_ASSERT(fWasOverwritten == VARIANT_FALSE);

		// Test renaming file back
		hr = m_pProvider->Rename(bstrTarget, bstrSubject, &fWasOverwritten);
		CPPUNIT_ASSERT_OK(hr);
		CPPUNIT_ASSERT(fWasOverwritten == VARIANT_FALSE);

		// Cleanup
		CPPUNIT_ASSERT_OK(m_pProvider->Delete(bstrSubject));
	}


	void testRenameFolder()
	{
		_StandardSetup();

		HRESULT hr;

		CComBSTR bstrSubject(_TestArea(L"RenameFolder/"));
		CComBSTR bstrTarget(_TestArea(L"RenameFolder_Passed/"));

		// Create our test subject and check existence
		CPPUNIT_ASSERT_OK(m_pProvider->CreateNewDirectory(bstrSubject));
		CHECK_PATH_EXISTS(bstrSubject);
		CHECK_PATH_NOT_EXISTS(bstrTarget);

		// Test renaming directory
		VARIANT_BOOL fWasOverwritten = VARIANT_FALSE;
		hr = m_pProvider->Rename(bstrSubject, bstrTarget, &fWasOverwritten);
		CPPUNIT_ASSERT_OK(hr);
		CPPUNIT_ASSERT(fWasOverwritten == VARIANT_FALSE);

		// Test renaming directory back
		hr = m_pProvider->Rename(bstrTarget, bstrSubject, &fWasOverwritten);
		CPPUNIT_ASSERT_OK(hr);
		CPPUNIT_ASSERT(fWasOverwritten == VARIANT_FALSE);

		// Check that the target does not still exist
		CHECK_PATH_NOT_EXISTS(bstrTarget);

		// Cleanup
		CPPUNIT_ASSERT_OK(m_pProvider->DeleteDirectory(bstrSubject));
		CHECK_PATH_NOT_EXISTS(bstrSubject);
	}

	void testRenameWithRefusedConfirmation()
	{
		_StandardSetup();

		HRESULT hr;

		// Choose mock behaviour
		m_pCoConsumer->SetConfirmOverwriteBehaviour(
			CMockSftpConsumer::PreventOverwrite);

		CComBSTR bstrSubject(_TestArea(L"RenameWithRefusedConfirmation"));
		CComBSTR bstrTarget(
			_TestArea(L"RenameWithRefusedConfirmation_Obstruction"));

		// Create our test subjects and check existence
		CPPUNIT_ASSERT_OK(m_pProvider->CreateNewFile(bstrSubject));
		CPPUNIT_ASSERT_OK(m_pProvider->CreateNewFile(bstrTarget));
		CHECK_PATH_EXISTS(bstrSubject);
		CHECK_PATH_EXISTS(bstrTarget);

		// Test renaming file
		VARIANT_BOOL fWasOverwritten = VARIANT_FALSE;
		hr = m_pProvider->Rename(bstrSubject, bstrTarget, &fWasOverwritten);
		CPPUNIT_ASSERT_FAILED(hr);
		CPPUNIT_ASSERT(fWasOverwritten == VARIANT_FALSE);

		// Check that both files still exist
		CHECK_PATH_EXISTS(bstrSubject);
		CHECK_PATH_EXISTS(bstrTarget);

		// Cleanup
		CPPUNIT_ASSERT_OK(m_pProvider->Delete(bstrSubject));
		CPPUNIT_ASSERT_OK(m_pProvider->Delete(bstrTarget));
		CHECK_PATH_NOT_EXISTS(bstrSubject);
		CHECK_PATH_NOT_EXISTS(bstrTarget);
	}

	void testRenameFolderWithRefusedConfirmation()
	{
		_StandardSetup();

		HRESULT hr;

		// Choose mock behaviour
		m_pCoConsumer->SetConfirmOverwriteBehaviour(
			CMockSftpConsumer::PreventOverwrite);

		CComBSTR bstrSubject(
			_TestArea(L"RenameFolderWithRefusedConfirmation/"));
		CComBSTR bstrTarget(
			_TestArea(L"RenameFolderWithRefusedConfirmation_Obstruction/"));

		// Create our test subjects and check existence
		CPPUNIT_ASSERT_OK(m_pProvider->CreateNewDirectory(bstrSubject));
		CPPUNIT_ASSERT_OK(m_pProvider->CreateNewDirectory(bstrTarget));
		CHECK_PATH_EXISTS(bstrSubject);
		CHECK_PATH_EXISTS(bstrTarget);

		// Test renaming directory
		VARIANT_BOOL fWasOverwritten = VARIANT_FALSE;
		hr = m_pProvider->Rename(bstrSubject, bstrTarget, &fWasOverwritten);
		CPPUNIT_ASSERT_FAILED(hr);
		CPPUNIT_ASSERT(fWasOverwritten == VARIANT_FALSE);

		// Check that both directories still exist
		CHECK_PATH_EXISTS(bstrSubject);
		CHECK_PATH_EXISTS(bstrTarget);

		// Cleanup
		CPPUNIT_ASSERT_OK(m_pProvider->DeleteDirectory(bstrSubject));
		CPPUNIT_ASSERT_OK(m_pProvider->DeleteDirectory(bstrTarget));
		CHECK_PATH_NOT_EXISTS(bstrSubject);
		CHECK_PATH_NOT_EXISTS(bstrTarget);
	}

	void testRenameInNonHomeFolder()
	{
		_StandardSetup();

		HRESULT hr;

		CComBSTR bstrSubject(L"/tmp/swishRenameInNonHomeFolder");
		CComBSTR bstrTarget(L"/tmp/swishRenameInNonHomeFolder_Passed");

		// Create our test subjects and check existence
		CPPUNIT_ASSERT_OK(m_pProvider->CreateNewFile(bstrSubject));
		CHECK_PATH_EXISTS(bstrSubject);
		CHECK_PATH_NOT_EXISTS(bstrTarget);

		// Test renaming file
		VARIANT_BOOL fWasOverwritten = VARIANT_FALSE;
		hr = m_pProvider->Rename(bstrSubject, bstrTarget, &fWasOverwritten);
		CPPUNIT_ASSERT_OK(hr);
		CPPUNIT_ASSERT(fWasOverwritten == VARIANT_FALSE);

		// Test renaming file back
		hr = m_pProvider->Rename(bstrTarget, bstrSubject, &fWasOverwritten);
		CPPUNIT_ASSERT_OK(hr);
		CPPUNIT_ASSERT(fWasOverwritten == VARIANT_FALSE);

		// Check that the target does not still exist
		CHECK_PATH_NOT_EXISTS(bstrTarget);

		// Cleanup
		CPPUNIT_ASSERT_OK(m_pProvider->Delete(bstrSubject));
		CHECK_PATH_NOT_EXISTS(bstrSubject);
		CHECK_PATH_NOT_EXISTS(bstrTarget);
	}

	void testRenameInNonHomeSubfolder()
	{
		_StandardSetup();

		HRESULT hr;

		CComBSTR bstrFolder(L"/tmp/swishSubfolder/");
		CComBSTR bstrSubject(
			L"/tmp/swishSubfolder/RenameInNonHomeSubfolder");
		CComBSTR bstrTarget(
			L"/tmp/swishSubfolder/RenameInNonHomeSubfolder_Passed");

		// Create our test subjects and check existence
		CPPUNIT_ASSERT_OK(m_pProvider->CreateNewDirectory(bstrFolder));
		CPPUNIT_ASSERT_OK(m_pProvider->CreateNewFile(bstrSubject));
		CHECK_PATH_EXISTS(bstrSubject);
		CHECK_PATH_NOT_EXISTS(bstrTarget);

		// Test renaming file
		VARIANT_BOOL fWasOverwritten = VARIANT_FALSE;
		hr = m_pProvider->Rename(bstrSubject, bstrTarget, &fWasOverwritten);
		CPPUNIT_ASSERT_OK(hr);
		CPPUNIT_ASSERT(fWasOverwritten == VARIANT_FALSE);

		// Test renaming file back
		hr = m_pProvider->Rename(bstrTarget, bstrSubject, &fWasOverwritten);
		CPPUNIT_ASSERT_OK(hr);
		CPPUNIT_ASSERT(fWasOverwritten == VARIANT_FALSE);

		// Check that the target does not still exist
		CHECK_PATH_NOT_EXISTS(bstrTarget);
		
		// Cleanup
		CPPUNIT_ASSERT_OK(m_pProvider->DeleteDirectory(bstrFolder));
		CHECK_PATH_NOT_EXISTS(bstrFolder);
	}

	void testCreateAndDelete()
	{
		_StandardSetup();

		HRESULT hr;

		CComBSTR bstrSubject(_TestArea(L"CreateAndDelete"));

		// Check that the file does not already exist
		CHECK_PATH_NOT_EXISTS(bstrSubject);

		// Test creating file
		hr = m_pProvider->CreateNewFile(bstrSubject);
		CPPUNIT_ASSERT_OK(hr);

		// Test deleting file
		hr = m_pProvider->Delete(bstrSubject);
		CPPUNIT_ASSERT_OK(hr);

		// Check that the file does not still exist
		CHECK_PATH_NOT_EXISTS(bstrSubject);
	}

	void testCreateAndDeleteEmptyDirectory()
	{
		_StandardSetup();

		HRESULT hr;

		CComBSTR bstrSubject(_TestArea(L"CreateAndDeleteEmptyDirectory"));

		// Check that the directory does not already exist
		CHECK_PATH_NOT_EXISTS(bstrSubject);

		// Test creating directory
		hr = m_pProvider->CreateNewDirectory(bstrSubject);
		CPPUNIT_ASSERT_OK(hr);

		// Test deleting directory
		hr = m_pProvider->DeleteDirectory(bstrSubject);
		CPPUNIT_ASSERT_OK(hr);

		// Check that the directory does not still exist
		CHECK_PATH_NOT_EXISTS(bstrSubject);
	}

	void testCreateAndDeleteDirectoryRecursive()
	{
		_StandardSetup();

		HRESULT hr;

		CComBSTR bstrDir(_TestArea(L"CreateAndDeleteDirectory"));
		CComBSTR bstrFile(_TestArea(L"CreateAndDeleteDirectory/Recursive"));

		// Check that subjects do not already exist
		CHECK_PATH_NOT_EXISTS(bstrDir);
		CHECK_PATH_NOT_EXISTS(bstrFile);

		// Create directory
		hr = m_pProvider->CreateNewDirectory(bstrDir);
		CPPUNIT_ASSERT_OK(hr);

		// Add file to directory
		hr = m_pProvider->CreateNewFile(bstrFile);
		CPPUNIT_ASSERT_OK(hr);

		// Test deleting directory
		hr = m_pProvider->DeleteDirectory(bstrDir);
		CPPUNIT_ASSERT_OK(hr);

		// Check that the directory does not still exist
		CHECK_PATH_NOT_EXISTS(bstrDir);
	}


private:
	CComObject<CMockSftpConsumer> *m_pCoConsumer;
	Swish::ISftpConsumer *m_pConsumer;
	Swish::ISftpProvider *m_pProvider;
	CTestConfig config;
	const CComBSTR m_bstrHomeDir;

	/**
	 * Performs a typical test setup.
	 *
	 * The mock consumer is set to authenticate using the correct password
	 * and throw an exception on all other callbacks to it.  This setup is 
	 * suitable for any tests that simply to test functionality rather than
	 * testing the process of authentication itself.  If the test expects
	 * the provider to callback to the consumer, these behaviours can be added
	 * after this method is called.
	 */
	void _StandardSetup()
	{
		HRESULT hr;

		CComBSTR bstrUser = config.GetUser();
		CComBSTR bstrHost = config.GetHost();

		// Standard mock behaviours
		m_pCoConsumer->SetPasswordBehaviour(CMockSftpConsumer::CustomPassword);
		m_pCoConsumer->SetCustomPassword(config.GetPassword());

		hr = m_pProvider->Initialize(
				m_pConsumer, bstrUser, bstrHost, config.GetPort());
		CPPUNIT_ASSERT_OK(hr);

		// Create test area (not used by all tests)
		if (!_FileExists(CString(_TestArea())))
		{
			CPPUNIT_ASSERT_OK(m_pProvider->CreateNewDirectory(_TestArea()));
		}
	}

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

	bool _FileExistsInListing(
		__in CComBSTR bstrFilename, __in CComPtr<Swish::IEnumListing> spEnum)
	{
		HRESULT hr;

		// Search for file
		Swish::Listing lt;
		hr = spEnum->Reset();
		CPPUNIT_ASSERT_OK(hr);
		hr = spEnum->Next(1, &lt, NULL);
		CPPUNIT_ASSERT_OK(hr);
		while (hr == S_OK)
		{
			if (bstrFilename == lt.bstrFilename)
				return true;

			hr = spEnum->Next(1, &lt, NULL);
		}

		return false;
	}

	bool _FileExists(__in PCWSTR pszFilePath)
	{
		HRESULT hr;
		CString strFilePath(pszFilePath);

		// Strip trailing slash
		strFilePath.TrimRight(L'/');
		
		// Find directory portion of path
		int iLastSep = strFilePath.ReverseFind(L'/');
		CString strDirectory = strFilePath.Left(iLastSep+1);
		int cFilenameLen = strFilePath.GetLength() - (iLastSep+1);
		CString strFilename = strFilePath.Right(cFilenameLen);

		// Fetch listing enumerator
		CComPtr<Swish::IEnumListing> spEnum;
		hr = m_pProvider->GetListing(CComBSTR(strDirectory), &spEnum);
		if (FAILED(hr))
			return false;

		return _FileExistsInListing(CComBSTR(strFilename), spEnum);
	}

	void _CheckPathExists(
		__in PCWSTR pszFilePath, __in CPPUNIT_NS::SourceLine sourceLine )
	{
		char szMessage[MAX_PATH + 30];
		_snprintf_s(szMessage, sizeof szMessage, MAX_PATH,
			"Expected file not found: %s", CW2A(pszFilePath));

		CPPUNIT_NS::Asserter::failIf( !(_FileExists(pszFilePath)),
			CPPUNIT_NS::Message( "assertion failed", szMessage), sourceLine);
	}

	void _CheckPathNotExists(
		__in PCWSTR pszFilePath, __in CPPUNIT_NS::SourceLine sourceLine )
	{
		char szMessage[MAX_PATH + 30];
		_snprintf_s(szMessage, sizeof szMessage, MAX_PATH,
			"Unexpected file found: %s", CW2A(pszFilePath));

		CPPUNIT_NS::Asserter::failIf( (_FileExists(pszFilePath)),
			CPPUNIT_NS::Message( "assertion failed", szMessage), sourceLine);
	}

	/**
	 * Returns path as subpath of home directory in a BSTR.
	 */
	CComBSTR _HomeDir(CString strPath = L"") const
	{
		CComBSTR bstrFullPath(m_bstrHomeDir);
		bstrFullPath += strPath;
		return bstrFullPath;
	}
	
	/**
	 * Returns path as subpath of the test area directory in a BSTR.
	 */
	CComBSTR _TestArea(CString strPath = L"") const
	{
		CComBSTR bstrFullPath(_HomeDir(L"testArea"));
		bstrFullPath += L"/";
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
