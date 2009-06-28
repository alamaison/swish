/**
    @file

    Testing CHostFolder via the external COM interfaces.

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

#include "pch.h"

#include "pidl.hpp"  // Custom PIDL functions

#include "test/common/CppUnitExtensions.h"

#include "swish/shell_folder/Swish.h" // for HostFolder UUID

#include "swish/atl.hpp"  // Common ATL setup

#include <mshtml.h>  // For IHTMLDOMTextNode2

namespace test {
namespace swish {
namespace com_dll {

using pidl::MakeHostPidl;

using ATL::CComPtr;
using ATL::CComQIPtr;
using ATL::CString;

class CHostFolderPreInitialize_test : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE( CHostFolderPreInitialize_test );
		CPPUNIT_TEST( testQueryInterface );
		CPPUNIT_TEST( testGetCLSID );
		CPPUNIT_TEST( testInitialize );
	CPPUNIT_TEST_SUITE_END();

public:
	CHostFolderPreInitialize_test() : m_pFolder(NULL)
	{
		HRESULT hr;

		// Start up COM
		hr = ::CoInitialize(NULL);
		CPPUNIT_ASSERT_OK(hr);

		// One-off tests

		// Store HostFolder CLSID
		CLSID CLSID_CHostFolder;
		hr = ::CLSIDFromProgID(OLESTR("Swish.HostFolder"), &CLSID_CHostFolder);
		CPPUNIT_ASSERT_OK(hr);

		// Check that CLSID was correctly constructed from ProgID
		LPOLESTR pszUuid = NULL;
		hr = ::StringFromCLSID( CLSID_CHostFolder, &pszUuid );
		CString strExpectedUuid = L"{b816a83a-5022-11dc-9153-0090f5284f85}";
		CString strActualUuid = pszUuid;
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
		hr = m_spFolder.CoCreateInstance(__uuidof(CHostFolder));
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
	 * one that implementa IHTMLDOMTextNode2 as this has been chosen to test 
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
		CString strExpectedUuid = L"{b816a83a-5022-11dc-9153-0090f5284f85}";
		CString strActualUuid = pszUuid;
		::CoTaskMemFree(pszUuid);
		CPPUNIT_ASSERT_EQUAL(
			strExpectedUuid.MakeLower(), strActualUuid.MakeLower());
	}

	void testInitialize()
	{
		CComQIPtr<IPersistFolder> spPersist(m_spFolder);
		CPPUNIT_ASSERT(spPersist);

		// Get Swish PIDL as HostFolder root
		PIDLIST_ABSOLUTE pidlSwish = _GetSwishPidl();

		// Initialise HostFolder with its PIDL
		HRESULT hr = spPersist->Initialize(pidlSwish);
		::ILFree(pidlSwish);
		CPPUNIT_ASSERT_OK(hr);
	}

	void testGetPIDL()
	{
		HRESULT hr;

		CComQIPtr<IPersistFolder2> spPersist(m_spFolder);
		CPPUNIT_ASSERT(spPersist);

		// Get Swish PIDL as HostFolder root
		PIDLIST_ABSOLUTE pidlSwish = _GetSwishPidl();
		// We will leak this PIDL if the tests below fail

		// Initialise HostFolder with its PIDL
		hr = spPersist->Initialize(pidlSwish);
		CPPUNIT_ASSERT_OK(hr);

		// Read the PIDL back - should be identical
		PIDLIST_ABSOLUTE pidl;
		hr = spPersist->GetCurFolder(&pidl);
		CPPUNIT_ASSERT_OK(hr);
		CPPUNIT_ASSERT(::ILIsEqual(pidl, pidlSwish));
		::ILFree(pidl);
		::ILFree(pidlSwish);
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

	CComPtr<IShellFolder2> m_spFolder;
	IShellFolder2 *m_pFolder;
};


class CHostFolderPostInitialize_test : public CHostFolderPreInitialize_test
{
public:
	CHostFolderPostInitialize_test() : CHostFolderPreInitialize_test() {}

	void setUp()
	{
		__super::setUp();

		// Initialise HostFolder with its PIDL
		CComQIPtr<IPersistFolder> spPersist(m_spFolder);
		PIDLIST_ABSOLUTE pidlSwish = _GetSwishPidl();
		HRESULT hr = spPersist->Initialize(pidlSwish);
		::ILFree(pidlSwish);
		CPPUNIT_ASSERT_OK(hr);
	}
};

// Tests for following configuration:
//     ComputerPIDL\SwishPIDL\HOSTPIDL
// where this RemoteFolder is rooted at:
//     ComputerPIDL\SwishPIDL

static const wchar_t *DN_FRIENDLY_RELATIVE = L"Test PIDL";
static const wchar_t *DN_FRIENDLY_ABSOLUTE = 
	L"sftp://user@test.example.com//home/user/dir";

static const wchar_t *DN_PARSING_RELATIVE = 
	L"sftp://user@test.example.com:22//home/user/dir";
static const wchar_t *DN_PARSING_ABSOLUTE = 
	L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\"
	L"::{B816A83A-5022-11DC-9153-0090F5284F85}\\"
	L"sftp://user@test.example.com:22//home/user/dir";

static const wchar_t *DN_ADDRESSBAR_RELATIVE = 
	L"sftp://user@test.example.com//home/user/dir";
static const wchar_t *DN_ADDRESSBAR_ABSOLUTE = 
	L"sftp://user@test.example.com//home/user/dir";

static const wchar_t *DN_PARSINGADDRESSBAR_RELATIVE = 
	L"sftp://user@test.example.com:22//home/user/dir";
static const wchar_t *DN_PARSINGADDRESSBAR_ABSOLUTE = 
	L"Computer\\Swish\\sftp://user@test.example.com:22//home/user/dir";

static const wchar_t *DN_EDITING_RELATIVE = L"Test PIDL";
static const wchar_t *DN_EDITING_ABSOLUTE = L"Test PIDL";

class CHostFolderDisplayName_test : public CHostFolderPostInitialize_test
{
	CPPUNIT_TEST_SUITE( CHostFolderDisplayName_test );
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
		CPPUNIT_TEST( testParseDisplayName );
	CPPUNIT_TEST_SUITE_END();

public:
	CHostFolderDisplayName_test() : CHostFolderPostInitialize_test() {}

protected:

	void testDisplayNormal()
	{
		_testName(DN_FRIENDLY_ABSOLUTE, SHGDN_NORMAL);
	}

	void testDisplayInFolder()
	{
		_testName(DN_FRIENDLY_RELATIVE, SHGDN_INFOLDER);
	}

	void testParsingNormal()
	{
		_testName(DN_PARSING_ABSOLUTE, SHGDN_FORPARSING);
	}

	void testParsingInFolder()
	{
		_testName(DN_PARSING_RELATIVE, SHGDN_INFOLDER | SHGDN_FORPARSING);
	}

	void testAddressbarNormal()
	{
		_testName(DN_ADDRESSBAR_ABSOLUTE, SHGDN_FORADDRESSBAR);
	}

	void testAddressbarInFolder()
	{
		_testName(
			DN_ADDRESSBAR_RELATIVE, SHGDN_INFOLDER | SHGDN_FORADDRESSBAR);
	}

	void testEditingNormal()
	{
		_testName(DN_EDITING_ABSOLUTE, SHGDN_FOREDITING);
	}

	void testEditingInFolder()
	{
		_testName(DN_EDITING_RELATIVE, SHGDN_INFOLDER | SHGDN_FOREDITING);
	}

	void testParsingAddressbarNormal()
	{
		_testName(
			DN_PARSINGADDRESSBAR_ABSOLUTE, 
			SHGDN_FORADDRESSBAR | SHGDN_FORPARSING);
	}

	void testParsingAddressbarInFolder()
	{
		_testName(
			DN_PARSINGADDRESSBAR_RELATIVE, 
			SHGDN_INFOLDER | SHGDN_FORADDRESSBAR | SHGDN_FORPARSING);
	}

	void testParseDisplayName()
	{
		HRESULT hr;

		PIDLIST_RELATIVE pidl = NULL;
		wchar_t wszDisplayName[MAX_PATH];
		wcscpy_s(wszDisplayName, ARRAYSIZE(wszDisplayName), DN_PARSING_RELATIVE);
		hr = m_spFolder->ParseDisplayName(
			NULL, NULL, wszDisplayName, NULL, &pidl, NULL);
		CPPUNIT_ASSERT_OK(hr);

		// Check return against PIDL
		PITEMID_CHILD pidlTest = _CreateTestPidl();
		BOOL fPidlsMatch = ::ILIsEqual(
			reinterpret_cast<PIDLIST_ABSOLUTE>(pidl),
			reinterpret_cast<PIDLIST_ABSOLUTE>(pidlTest));
		::ILFree(pidl);
		::ILFree(pidlTest);
		CPPUNIT_ASSERT(fPidlsMatch);
	}


private:

	void _testName(PCWSTR pwszName, SHGDNF uFlags)
	{
		CString strActual(_GetDisplayName(uFlags));
		CString strExpected(pwszName);

		CPPUNIT_ASSERT_EQUAL(strExpected, strActual);
	}

	PITEMID_CHILD _CreateTestPidl()
	{
		// Create test HOSTPIDL
		return MakeHostPidl(
			L"user", L"test.example.com", L"/home/user/dir", 22, L"Test PIDL");
	}

	CString _GetDisplayName(SHGDNF uFlags)
	{
		HRESULT hr;

		// Create test HOSTPIDL
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
};

CPPUNIT_TEST_SUITE_REGISTRATION( CHostFolderPreInitialize_test );
CPPUNIT_TEST_SUITE_REGISTRATION( CHostFolderDisplayName_test );

}}} // namespace test::swish::com_dll