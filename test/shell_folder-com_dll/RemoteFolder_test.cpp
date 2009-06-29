/**
    @file

    Testing CRemoteFolder via the external COM interfaces.

    @if licence

    Copyright (C) 2009  Alexander Lamaison <awl03@doc.ic.ac.uk>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

    @endif
*/

#include "pidl.hpp"  // Custom PIDL functions

#include "test/common/CppUnitExtensions.h"

#include "swish/shell_folder/Swish.h" // for HostFolder UUID

#include "swish/atl.hpp"  // Common ATL setup

#include <mshtml.h>  // For IHTMLDOMTextNode2

namespace test {
namespace swish {
namespace com_dll {

using pidl::MakeHostPidl;
using pidl::MakeRemotePidl;

using ATL::CComPtr;
using ATL::CComQIPtr;
using ATL::CString;

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
		hr = ::StringFromCLSID( __uuidof(CRemoteFolder), &pszUuid );
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
		hr = m_spFolder.CoCreateInstance(__uuidof(CRemoteFolder));
		CPPUNIT_ASSERT_OK(hr);

		// Copy to regular interface pointer so we can test for memory 
		// leaks in tearDown()
		m_spFolder.CopyTo(&m_pFolder);
	}

	void tearDown()
	{
		try
		{
			m_spFolder.Release();

			if (m_pFolder) // Test for leaking refs
			{
				CPPUNIT_ASSERT_ZERO(m_pFolder->Release());
				m_pFolder = NULL;
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
		::ILFree(pidl);
		::ILFree(pidlRoot);
	}


	/**
	 * Get root PIDL appropriate for current test.
	 */
	virtual PIDLIST_ABSOLUTE _CreateRootPidl()
	{
		return _CreateRootRemotePidl();
	}

	/**
	 * Get the PIDL which represents the HostFolder (Swish icon) in Explorer.
	 */
	PIDLIST_ABSOLUTE _GetSwishPidl()
	{
		HRESULT hr;
		PIDLIST_ABSOLUTE pidl;
		CComPtr<IShellFolder> spDesktop;
		hr = ::SHGetDesktopFolder(&spDesktop);
		CPPUNIT_ASSERT_OK(hr);
		hr = spDesktop->ParseDisplayName(NULL, NULL, 
			L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\"
			L"::{B816A83A-5022-11DC-9153-0090F5284F85}", NULL, 
			reinterpret_cast<PIDLIST_RELATIVE*>(&pidl), NULL);
		CPPUNIT_ASSERT_OK(hr);

		return pidl;
	}

	/**
	 * Get an absolute PIDL that ends in a REMOTEPIDL to root RemoteFolder on.
	 */
	PIDLIST_ABSOLUTE _CreateRootRemotePidl()
	{
		// Create test absolute HOSTPIDL
		PIDLIST_ABSOLUTE pidlHost = _CreateRootHostPidl();
		
		// Create root child REMOTEPIDL
		PITEMID_CHILD pidlRemote = MakeRemotePidl(
			L"dir", true, L"owner", L"group", 1001, 1002, false, 0677, 1024);

		// Concatenate to make absolute pidl to RemoteFolder root
		PIDLIST_ABSOLUTE pidl = ::ILCombine(pidlHost, pidlRemote);
		::ILFree(pidlRemote);
		::ILFree(pidlHost);
		return pidl;
	}

	/**
	 * Get an absolute PIDL that ends in a HOSTPIDL to root RemoteFolder on.
	 */
	PIDLIST_ABSOLUTE _CreateRootHostPidl()
	{
		// Create absolute PIDL to Swish icon
		PIDLIST_ABSOLUTE pidlSwish = _GetSwishPidl();
		
		// Create test child HOSTPIDL
		PITEMID_CHILD pidlHost = MakeHostPidl(
			L"user", L"test.example.com", L"/home/user", 22, L"Test PIDL");

		// Concatenate to make absolute pidl to RemoteFolder root
		PIDLIST_ABSOLUTE pidl = ::ILCombine(pidlSwish, pidlHost);
		::ILFree(pidlSwish);
		::ILFree(pidlHost);
		return pidl;
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
		return MakeRemotePidl(
			L"TestFile.bmp", false, L"me", L"us", 1001, 1002, false, 
			0677, 511, NULL);
	}
};

// Tests for following configuration:
//     ComputerPIDL\SwishPIDL\HOSTPIDL\REMOTEPIDL
// where this RemoteFolder is rooted at:
//     ComputerPIDL\SwishPIDL\HOSTPIDL

static const wchar_t *DN2_FRIENDLY_RELATIVE = L"TestDirectory.ext";
static const wchar_t *DN2_FRIENDLY_ABSOLUTE = L"TestDirectory.ext";

static const wchar_t *DN2_PARSING_RELATIVE = L"TestDirectory.ext";
static const wchar_t *DN2_PARSING_ABSOLUTE = 
	L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\"
	L"::{B816A83A-5022-11DC-9153-0090F5284F85}\\"
	L"sftp://user@test.example.com:22//home/user/TestDirectory.ext";

static const wchar_t *DN2_ADDRESSBAR_RELATIVE = L"TestDirectory.ext";
static const wchar_t *DN2_ADDRESSBAR_ABSOLUTE = 
	L"sftp://user@test.example.com//home/user/TestDirectory.ext";

static const wchar_t *DN2_PARSINGADDRESSBAR_RELATIVE = L"TestDirectory.ext";
static const wchar_t *DN2_PARSINGADDRESSBAR_ABSOLUTE = 
	L"Computer\\Swish\\sftp://user@test.example.com:22/"
	L"/home/user/TestDirectory.ext";

static const wchar_t *DN2_EDITING_RELATIVE = L"TestDirectory.ext";
static const wchar_t *DN2_EDITING_ABSOLUTE = L"TestDirectory.ext";

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
		return MakeRemotePidl(
			L"TestDirectory.ext", true, L"me", L"us", 1001, 1002, false, 
			0677, 511, NULL);
	}

	/**
	 * Get root PIDL appropriate for current test.
	 */
	virtual PIDLIST_ABSOLUTE _CreateRootPidl()
	{
		return _CreateRootHostPidl();
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( CRemoteFolderPreInitialize_test );
CPPUNIT_TEST_SUITE_REGISTRATION( CRemoteFolderDisplayName1_test );
CPPUNIT_TEST_SUITE_REGISTRATION( CRemoteFolderDisplayName2_test );

}}} // namespace test::swish::com_dll