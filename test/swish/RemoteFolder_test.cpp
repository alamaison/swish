#include "stdafx.h"
#include "../CppUnitExtensions.h"

#include <HostPidl.h>
#include <RemotePidl.h>

class CRemoteFolderPreInitialize_test : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE( CRemoteFolderPreInitialize_test );
		CPPUNIT_TEST( testQueryInterface );
		CPPUNIT_TEST( testGetCLSID );
		CPPUNIT_TEST( testInitialize );
	CPPUNIT_TEST_SUITE_END();

public:
	CRemoteFolderPreInitialize_test() : m_pFolder(NULL)
	{
		HRESULT hr;

		// Start up COM
		hr = ::CoInitialize(NULL);
		CPPUNIT_ASSERT_OK(hr);

		// One-off tests

		// Store RemoteFolder CLSID
		CLSID CLSID_Folder;
		hr = ::CLSIDFromProgID(OLESTR("Swish.RemoteFolder"), &CLSID_Folder);
		CPPUNIT_ASSERT_OK(hr);

		// Check that CLSID was correctly constructed from ProgID
		LPOLESTR pszUuid = NULL;
		hr = ::StringFromCLSID( CLSID_Folder, &pszUuid );
		CString strExpectedUuid = L"{b816a83c-5022-11dc-9153-0090f5284f85}";
		CString strActualUuid = pszUuid;
		::CoTaskMemFree(pszUuid);
		CPPUNIT_ASSERT_EQUAL(
			strExpectedUuid.MakeLower(), strActualUuid.MakeLower());

		// Check that CLSID was correctly constructed from __uuidof()
		pszUuid = NULL;
		hr = ::StringFromCLSID( __uuidof(Swish::CRemoteFolder), &pszUuid );
		strExpectedUuid = L"{b816a83c-5022-11dc-9153-0090f5284f85}";
		strActualUuid = pszUuid;
		::CoTaskMemFree(pszUuid);
		CPPUNIT_ASSERT_EQUAL(
			strExpectedUuid.MakeLower(), strActualUuid.MakeLower());

		// Shut down COM
		::CoUninitialize();
	}

	void setUp()
	{
		HRESULT hr;

		// Start up COM
		hr = ::CoInitialize(NULL);
		CPPUNIT_ASSERT_OK(hr);

		// Create instance of folder using CLSID
		hr = m_spFolder.CoCreateInstance(__uuidof(Swish::CRemoteFolder));
		CPPUNIT_ASSERT_OK(hr);

		// Copy to regular interface pointer so we can test for memory 
		// leaks in tearDown()
		m_spFolder.CopyTo(&m_pFolder);
	}

	void tearDown()
	{
		m_spFolder.Release();

		if (m_pFolder) // Possible for test to fail before initialised
		{
			ULONG cRefs = m_pFolder->Release();
			CPPUNIT_ASSERT_EQUAL( (ULONG)0, cRefs );
		}
		m_pFolder = NULL;

		// Shut down COM
		::CoUninitialize();
	}

protected:
	/**
	 * Test that the class responds to IUnknown::QueryInterface correctly.
	 *
	 * This test will be roughly the same for *any* valid COM object except
	 * one that implements IHTMLDOMTextNode2 as this has been chosen to test 
	 * failure. 
	 * The cases being tested are based on those explained by Raymond Chen:
	 * http://blogs.msdn.com/oldnewthing/archive/2004/03/26/96777.aspx
	 */
	void testQueryInterface()
	{
		HRESULT hr;

		// Supports IUnknown (valid COM object)?
		IUnknown *pUnk;
		hr = m_pFolder->QueryInterface(&pUnk);
		CPPUNIT_ASSERT_OK(hr);
		pUnk->Release();

		// Supports IShellFolder2 (valid self!)?
		IShellFolder2 *pFolder;
		hr = m_pFolder->QueryInterface(&pFolder);
		CPPUNIT_ASSERT_OK(hr);
		pFolder->Release();

		// Says no properly (Very unlikely to support this - must return NULL)
		IHTMLDOMTextNode2 *pShell = (IHTMLDOMTextNode2 *)this;
		hr = m_pFolder->QueryInterface(&pShell);
		if (SUCCEEDED(hr))
		{
			pShell->Release();
			CPPUNIT_ASSERT(FAILED(hr));
		}
		CPPUNIT_ASSERT(pShell == NULL);
	}

	void testGetCLSID()
	{
		CComQIPtr<IPersist> spPersist(m_spFolder);
		CPPUNIT_ASSERT(spPersist);

		CLSID clsid;
		HRESULT hr = spPersist->GetClassID(&clsid);
		CPPUNIT_ASSERT_OK(hr);

		// Check that CLSID is correct
		LPOLESTR pszUuid = NULL;
		::StringFromCLSID(clsid, &pszUuid);
		CString strExpectedUuid = L"{b816a83c-5022-11dc-9153-0090f5284f85}";
		CString strActualUuid = pszUuid;
		::CoTaskMemFree(pszUuid);
		CPPUNIT_ASSERT_EQUAL(
			strExpectedUuid.MakeLower(), strActualUuid.MakeLower());
	}

	void testInitialize()
	{
		CComQIPtr<IPersistFolder> spPersist(m_spFolder);
		CPPUNIT_ASSERT(spPersist);

		// Get Swish PIDL + HOSTPIDL + REMOTEPIDL as RemoteFolder root
		PIDLIST_ABSOLUTE pidl = _CreateRootPidl();

		// Initialise RemoteFolder with its PIDL
		HRESULT hr = spPersist->Initialize(pidl);
		::ILFree(pidl);
		CPPUNIT_ASSERT_OK(hr);
	}

	void testGetPIDL()
	{
		HRESULT hr;

		CComQIPtr<IPersistFolder2> spPersist(m_spFolder);
		CPPUNIT_ASSERT(spPersist);

		// Get Swish PIDL + HOSTPIDL + REMOTEPIDL as RemoteFolder root
		PIDLIST_ABSOLUTE pidlRoot = _CreateRootPidl();
		// We will leak this PIDL if the tests below fail

		// Initialise RemoteFolder with its PIDL
		hr = spPersist->Initialize(pidlRoot);
		CPPUNIT_ASSERT_OK(hr);

		// Read the PIDL back - should be identical
		PIDLIST_ABSOLUTE pidl;
		hr = spPersist->GetCurFolder(&pidl);
		CPPUNIT_ASSERT_OK(hr);
		CPPUNIT_ASSERT(::ILIsEqual(pidl, pidlRoot));
	}


	/**
	 * Get root PIDL appropriate for current test.
	 */
	virtual PIDLIST_ABSOLUTE _CreateRootPidl()
	{
		return _CreateRootRemotePidl().Detach();
	}

	/**
	 * Get the PIDL which represents the HostFolder (Swish icon) in Explorer.
	 */
	CAbsolutePidl _GetSwishPidl()
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
	CAbsolutePidl _CreateRootRemotePidl()
	{
		// Create test absolute HOSTPIDL
		CAbsolutePidl pidlHost = _CreateRootHostPidl();
		
		// Create root child REMOTEPIDL
		CRemoteItem pidlRemote(
			L"dir", L"owner", L"group", true, false, 0677, 1024);

		// Concatenate to make absolute pidl to RemoteFolder root
		return CAbsolutePidl(pidlHost, pidlRemote);
	}

	/**
	 * Get an absolute PIDL that ends in a HOSTPIDL to root RemoteFolder on.
	 */
	CAbsolutePidl _CreateRootHostPidl()
	{
		// Create absolute PIDL to Swish icon
		CAbsolutePidl pidlSwish = _GetSwishPidl();
		
		// Create test child HOSTPIDL
		CHostItem pidlHost(
			L"user", L"test.example.com", 22, L"/home/user", L"Test PIDL");

		// Concatenate to make absolute pidl to RemoteFolder root
		return CAbsolutePidl(pidlSwish, pidlHost);
	}

	CComPtr<IShellFolder2> m_spFolder;
	IShellFolder2 *m_pFolder;
};


class CRemoteFolderPostInitialize_test : public CRemoteFolderPreInitialize_test
{
public:
	CRemoteFolderPostInitialize_test() : CRemoteFolderPreInitialize_test() {}

	void setUp()
	{
		__super::setUp();

		// Initialise RemoteFolder with its PIDL
		CComQIPtr<IPersistFolder> spPersist(m_spFolder);
		PIDLIST_ABSOLUTE pidl = _CreateRootPidl();
		HRESULT hr = spPersist->Initialize(pidl);
		::ILFree(pidl);
		CPPUNIT_ASSERT_OK(hr);
	}
};

/**
 * Base class of display name tests.
 */
class CRemoteFolderDisplayName_test : public CRemoteFolderPostInitialize_test
{
public:
	CRemoteFolderDisplayName_test() : CRemoteFolderPostInitialize_test() {}

	void _testName(PCWSTR pwszName, SHGDNF uFlags)
	{
		CString strActual(_GetDisplayName(uFlags));
		CString strExpected(pwszName);

		CPPUNIT_ASSERT_EQUAL(strExpected, strActual);
	}

	CString _GetDisplayName(SHGDNF uFlags)
	{
		HRESULT hr;

		// Create test PIDL
		PITEMID_CHILD pidl = _CreateTestPidl();

		STRRET strret;
		hr = m_spFolder->GetDisplayNameOf(pidl, uFlags, &strret);
		CPPUNIT_ASSERT_OK(hr);

		PWSTR pwszName;
		hr = ::StrRetToStr(&strret, pidl, &pwszName);
		CPPUNIT_ASSERT_OK(hr);
		CString strName(pwszName);
		::CoTaskMemFree(pwszName);

		if (strret.uType == STRRET_WSTR)
			::CoTaskMemFree(strret.pOleStr);

		return strName;
	}

	virtual PITEMID_CHILD _CreateTestPidl() PURE;
};

// Tests for following configuration:
//     ComputerPIDL\SwishPIDL\HOSTPIDL\REMOTEPIDL\REMOTEPIDL
// where this RemoteFolder is rooted at:
//     ComputerPIDL\SwishPIDL\HOSTPIDL\REMOTEPIDL

static const wchar_t *DN1_FRIENDLY_RELATIVE = L"TestFile";
static const wchar_t *DN1_FRIENDLY_ABSOLUTE = L"TestFile";

static const wchar_t *DN1_PARSING_RELATIVE = L"TestFile.bmp";
static const wchar_t *DN1_PARSING_ABSOLUTE = 
	L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\"
	L"::{B816A83A-5022-11DC-9153-0090F5284F85}\\"
	L"sftp://user@test.example.com:22//home/user/dir/TestFile.bmp";

static const wchar_t *DN1_ADDRESSBAR_RELATIVE = L"TestFile";
static const wchar_t *DN1_ADDRESSBAR_ABSOLUTE = 
	L"sftp://user@test.example.com//home/user/dir/TestFile";

static const wchar_t *DN1_PARSINGADDRESSBAR_RELATIVE = L"TestFile.bmp";
static const wchar_t *DN1_PARSINGADDRESSBAR_ABSOLUTE = 
	L"Computer\\Swish\\sftp://user@test.example.com:22/"
	L"/home/user/dir/TestFile.bmp";

static const wchar_t *DN1_EDITING_RELATIVE = L"TestFile.bmp";
static const wchar_t *DN1_EDITING_ABSOLUTE = L"TestFile.bmp";

class CRemoteFolderDisplayName1_test : public CRemoteFolderDisplayName_test
{
	CPPUNIT_TEST_SUITE( CRemoteFolderDisplayName1_test );
		CPPUNIT_TEST( testDisplayNormal );
		CPPUNIT_TEST( testDisplayInFolder );
		CPPUNIT_TEST( testParsingNormal );
		CPPUNIT_TEST( testParsingInFolder );
		CPPUNIT_TEST( testAddressbarNormal );
		CPPUNIT_TEST( testAddressbarInFolder );
		CPPUNIT_TEST( testEditingNormal );
		CPPUNIT_TEST( testEditingInFolder );
		CPPUNIT_TEST( testParsingAddressbarNormal );
		CPPUNIT_TEST( testParsingAddressbarInFolder );
	CPPUNIT_TEST_SUITE_END();

public:
	CRemoteFolderDisplayName1_test() : CRemoteFolderDisplayName_test() {}

protected:

	void testDisplayNormal()
	{
		_testName(DN1_FRIENDLY_ABSOLUTE, SHGDN_NORMAL);
	}

	void testDisplayInFolder()
	{
		_testName(DN1_FRIENDLY_RELATIVE, SHGDN_INFOLDER);
	}

	void testParsingNormal()
	{
		_testName(DN1_PARSING_ABSOLUTE, SHGDN_FORPARSING);
	}

	void testParsingInFolder()
	{
		_testName(DN1_PARSING_RELATIVE, SHGDN_INFOLDER | SHGDN_FORPARSING);
	}

	void testAddressbarNormal()
	{
		_testName(DN1_ADDRESSBAR_ABSOLUTE, SHGDN_FORADDRESSBAR);
	}

	void testAddressbarInFolder()
	{
		_testName(
			DN1_ADDRESSBAR_RELATIVE, SHGDN_INFOLDER | SHGDN_FORADDRESSBAR);
	}

	void testEditingNormal()
	{
		_testName(DN1_EDITING_ABSOLUTE, SHGDN_FOREDITING);
	}

	void testEditingInFolder()
	{
		_testName(DN1_EDITING_RELATIVE, SHGDN_INFOLDER | SHGDN_FOREDITING);
	}

	void testParsingAddressbarNormal()
	{
		_testName(
			DN1_PARSINGADDRESSBAR_ABSOLUTE, 
			SHGDN_FORADDRESSBAR | SHGDN_FORPARSING);
	}

	void testParsingAddressbarInFolder()
	{
		_testName(
			DN1_PARSINGADDRESSBAR_RELATIVE, 
			SHGDN_INFOLDER | SHGDN_FORADDRESSBAR | SHGDN_FORPARSING);
	}

private:

	PITEMID_CHILD _CreateTestPidl()
	{
		// Create test REMOTEPIDL
		CRemoteItem pidl(
			L"TestFile.bmp", L"me", L"us", false, false, 0677, 511, NULL);
		return pidl.Detach();
	}
};

// Tests for following configuration:
//     ComputerPIDL\SwishPIDL\HOSTPIDL\REMOTEPIDL
// where this RemoteFolder is rooted at:
//     ComputerPIDL\SwishPIDL\HOSTPIDL

static const wchar_t *DN2_FRIENDLY_RELATIVE = L"TestDirectory";
static const wchar_t *DN2_FRIENDLY_ABSOLUTE = L"TestDirectory";

static const wchar_t *DN2_PARSING_RELATIVE = L"TestDirectory";
static const wchar_t *DN2_PARSING_ABSOLUTE = 
	L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\"
	L"::{B816A83A-5022-11DC-9153-0090F5284F85}\\"
	L"sftp://user@test.example.com:22//home/user/TestDirectory";

static const wchar_t *DN2_ADDRESSBAR_RELATIVE = L"TestDirectory";
static const wchar_t *DN2_ADDRESSBAR_ABSOLUTE = 
	L"sftp://user@test.example.com//home/user/TestDirectory";

static const wchar_t *DN2_PARSINGADDRESSBAR_RELATIVE = L"TestDirectory";
static const wchar_t *DN2_PARSINGADDRESSBAR_ABSOLUTE = 
	L"Computer\\Swish\\sftp://user@test.example.com:22/"
	L"/home/user/TestDirectory";

static const wchar_t *DN2_EDITING_RELATIVE = L"TestDirectory";
static const wchar_t *DN2_EDITING_ABSOLUTE = L"TestDirectory";

class CRemoteFolderDisplayName2_test : public CRemoteFolderDisplayName_test
{
	CPPUNIT_TEST_SUITE( CRemoteFolderDisplayName2_test );
		CPPUNIT_TEST( testDisplayNormal );
		CPPUNIT_TEST( testDisplayInFolder );
		CPPUNIT_TEST( testParsingNormal );
		CPPUNIT_TEST( testParsingInFolder );
		CPPUNIT_TEST( testAddressbarNormal );
		CPPUNIT_TEST( testAddressbarInFolder );
		CPPUNIT_TEST( testEditingNormal );
		CPPUNIT_TEST( testEditingInFolder );
		CPPUNIT_TEST( testParsingAddressbarNormal );
		CPPUNIT_TEST( testParsingAddressbarInFolder );
	CPPUNIT_TEST_SUITE_END();

public:
	CRemoteFolderDisplayName2_test() : CRemoteFolderDisplayName_test() {}

protected:

	void testDisplayNormal()
	{
		_testName(DN2_FRIENDLY_ABSOLUTE, SHGDN_NORMAL);
	}

	void testDisplayInFolder()
	{
		_testName(DN2_FRIENDLY_RELATIVE, SHGDN_INFOLDER);
	}

	void testParsingNormal()
	{
		_testName(DN2_PARSING_ABSOLUTE, SHGDN_FORPARSING);
	}

	void testParsingInFolder()
	{
		_testName(DN2_PARSING_RELATIVE, SHGDN_INFOLDER | SHGDN_FORPARSING);
	}

	void testAddressbarNormal()
	{
		_testName(DN2_ADDRESSBAR_ABSOLUTE, SHGDN_FORADDRESSBAR);
	}

	void testAddressbarInFolder()
	{
		_testName(
			DN2_ADDRESSBAR_RELATIVE, SHGDN_INFOLDER | SHGDN_FORADDRESSBAR);
	}

	void testEditingNormal()
	{
		_testName(DN2_EDITING_ABSOLUTE, SHGDN_FOREDITING);
	}

	void testEditingInFolder()
	{
		_testName(DN2_EDITING_RELATIVE, SHGDN_INFOLDER | SHGDN_FOREDITING);
	}

	void testParsingAddressbarNormal()
	{
		_testName(
			DN2_PARSINGADDRESSBAR_ABSOLUTE, 
			SHGDN_FORADDRESSBAR | SHGDN_FORPARSING);
	}

	void testParsingAddressbarInFolder()
	{
		_testName(
			DN2_PARSINGADDRESSBAR_RELATIVE, 
			SHGDN_INFOLDER | SHGDN_FORADDRESSBAR | SHGDN_FORPARSING);
	}

private:

	PITEMID_CHILD _CreateTestPidl()
	{
		// Create test REMOTEPIDL
		CRemoteItem pidl(
			L"TestFile.bmp", L"me", L"us", false, false, 0677, 511, NULL);
		return pidl.Detach();
	}

	/**
	 * Get root PIDL appropriate for current test.
	 */
	virtual PIDLIST_ABSOLUTE _CreateRootPidl()
	{
		return _CreateRootHostPidl().Detach();
	}
};
CPPUNIT_TEST_SUITE_REGISTRATION( CRemoteFolderPreInitialize_test );
CPPUNIT_TEST_SUITE_REGISTRATION( CRemoteFolderDisplayName1_test );
CPPUNIT_TEST_SUITE_REGISTRATION( CRemoteFolderDisplayName2_test );
