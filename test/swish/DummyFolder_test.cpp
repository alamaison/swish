#include "stdafx.h"
#include "../CppUnitExtensions.h"

#include <pshpack1.h>
struct DummyItemId
{
	USHORT cb;
	DWORD dwFingerprint;
	int level;

	static const DWORD FINGERPRINT = 0x624a0fe5;
};
#include <poppack.h>

class CDummyFolderPreInitialize_test : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE( CDummyFolderPreInitialize_test );
		CPPUNIT_TEST( testQueryInterface );
		CPPUNIT_TEST( testGetCLSID );
		CPPUNIT_TEST( testInitialize );
	CPPUNIT_TEST_SUITE_END();

public:
	CDummyFolderPreInitialize_test() : m_pFolder(NULL)
	{
		HRESULT hr;

		// Start up COM
		hr = ::CoInitialize(NULL);
		CPPUNIT_ASSERT_OK(hr);

		// One-off tests

		// Store RemoteFolder CLSID
		CLSID CLSID_Folder;
		hr = ::CLSIDFromProgID(OLESTR("Swish.DummyFolder"), &CLSID_Folder);
		CPPUNIT_ASSERT_OK(hr);

		// Check that CLSID was correctly constructed from ProgID
		LPOLESTR pszUuid = NULL;
		hr = ::StringFromCLSID( CLSID_Folder, &pszUuid );
		CString strExpectedUuid = L"{708F09A0-FED0-46E8-9C56-55B7AA6AD1B2}";
		CString strActualUuid = pszUuid;
		::CoTaskMemFree(pszUuid);
		CPPUNIT_ASSERT_EQUAL(
			strExpectedUuid.MakeLower(), strActualUuid.MakeLower());

		// Check that CLSID was correctly constructed from __uuidof()
		pszUuid = NULL;
		hr = ::StringFromCLSID( __uuidof(Swish::CDummyFolder), &pszUuid );
		strExpectedUuid = L"{708F09A0-FED0-46E8-9C56-55B7AA6AD1B2}";
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

		IClassFactory *pFactory = NULL;
		hr = ::CoGetClassObject(
			__uuidof(Swish::CDummyFolder), CLSCTX_INPROC_SERVER, NULL,
			IID_PPV_ARGS(&pFactory));
		CPPUNIT_ASSERT_OK(hr);

		IShellFolder *pFolder = NULL;
		hr = pFactory->CreateInstance(NULL, IID_PPV_ARGS(&pFolder));
		CPPUNIT_ASSERT_OK(hr);

		pFolder->Release();
		pFactory->Release();

		// Create instance of folder using CLSID
		hr = m_spFolder.CoCreateInstance(__uuidof(Swish::CDummyFolder));
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

		// Supports IShellFolder (valid self!)?
		IShellFolder *pFolder;
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
		CString strExpectedUuid = L"{708F09A0-FED0-46E8-9C56-55B7AA6AD1B2}";
		CString strActualUuid = pszUuid;
		::CoTaskMemFree(pszUuid);
		CPPUNIT_ASSERT_EQUAL(
			strExpectedUuid.MakeLower(), strActualUuid.MakeLower());
	}

	void testInitialize()
	{
		CComQIPtr<IPersistFolder> spPersist(m_spFolder);
		CPPUNIT_ASSERT(spPersist);

		// Get PIDL of folder in My Computer as root pidl
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

		// Get PIDL of folder in My Computer as root pidl
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
		return _GetDummySwishPidl();
	}

	/**
	 * Get the PIDL which represents the folder in My Computer.
	 */
	PIDLIST_ABSOLUTE _GetDummySwishPidl()
	{
		HRESULT hr;
		PIDLIST_ABSOLUTE pidl;
		CComPtr<IShellFolder> spDesktop;
		hr = ::SHGetDesktopFolder(&spDesktop);
		CPPUNIT_ASSERT_OK(hr);
		hr = spDesktop->ParseDisplayName(NULL, NULL, 
			L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\"
			L"::{708F09A0-FED0-46E8-9C56-55B7AA6AD1B2}", NULL, 
			reinterpret_cast<PIDLIST_RELATIVE *>(&pidl), NULL);
		CPPUNIT_ASSERT_OK(hr);

		return pidl;
	}

	CComPtr<IShellFolder> m_spFolder;
	IShellFolder *m_pFolder;
};

class CDummyFolderPostInitialize_test : public CDummyFolderPreInitialize_test
{
public:
	CDummyFolderPostInitialize_test() : CDummyFolderPreInitialize_test() {}

	void setUp()
	{
		__super::setUp();

		// Initialise folder with its PIDL
		CComQIPtr<IPersistFolder> spPersist(m_spFolder);
		PIDLIST_ABSOLUTE pidl = _CreateRootPidl();
		HRESULT hr = spPersist->Initialize(pidl);
		::ILFree(pidl);
		CPPUNIT_ASSERT_OK(hr);
	}
};

class CDummyFolderEnum_test : public CDummyFolderPostInitialize_test
{
	CPPUNIT_TEST_SUITE( CDummyFolderEnum_test );
		CPPUNIT_TEST( testEnumFolders );
	CPPUNIT_TEST_SUITE_END();

public:
	CDummyFolderEnum_test() : CDummyFolderPostInitialize_test() {}

protected:
	void testEnumFolders()
	{
		HRESULT hr;

		// Create IEnumIDList
		CComPtr<IEnumIDList> spEnum;
		hr = m_spFolder->EnumObjects(NULL, SHCONTF_FOLDERS, &spEnum);
		CPPUNIT_ASSERT_OK(hr);

		// Getting first item (there should only be one)
		PITEMID_CHILD pidl;
		ULONG celtFetched;
		hr = spEnum->Next(1, &pidl, &celtFetched);
		CPPUNIT_ASSERT_OK(hr);
		CPPUNIT_ASSERT(pidl);

		// Test its contents

		// Check level
		const DummyItemId *pitemid = reinterpret_cast<const DummyItemId *>(pidl);
		CPPUNIT_ASSERT_EQUAL(0, pitemid->level);

		// Check folderness
		SFGAOF rgfInOut = SFGAO_FOLDER | SFGAO_HASSUBFOLDER;
		hr = m_spFolder->GetAttributesOf(1, &pidl, &rgfInOut);
		CPPUNIT_ASSERT_OK(hr);
		CPPUNIT_ASSERT_EQUAL(SFGAO_FOLDER | SFGAO_HASSUBFOLDER, rgfInOut);
		::ILFree(pidl);

		// Try to get next item.  This should fail with S_FALSE
		hr = spEnum->Next(1, &pidl, &celtFetched);
		CPPUNIT_ASSERT(hr == S_FALSE);
		//::ILFree(pidl); Do not free - fetch failed
	}
};

class CDummyFolderView_test : public CDummyFolderPostInitialize_test
{
	CPPUNIT_TEST_SUITE( CDummyFolderView_test );
		CPPUNIT_TEST( testCreateDefView );
	CPPUNIT_TEST_SUITE_END();

public:
	CDummyFolderView_test() : CDummyFolderPostInitialize_test() {}

protected:
	void testCreateDefView()
	{
		HRESULT hr;

		CComPtr<IShellView> spView;
		hr = m_spFolder->CreateViewObject(NULL, IID_PPV_ARGS(&spView));
		CPPUNIT_ASSERT_OK(hr);
	}
};

class CDummyFolderSubfolders_test : public CDummyFolderPostInitialize_test
{
	CPPUNIT_TEST_SUITE( CDummyFolderSubfolders_test );
		CPPUNIT_TEST( testBindToChildFolder );
		CPPUNIT_TEST( testBindToManyFolders );
		CPPUNIT_TEST( testBindToFarawayFolder );
	CPPUNIT_TEST_SUITE_END();

public:
	CDummyFolderSubfolders_test() : CDummyFolderPostInitialize_test() {}

protected:
	void testBindToChildFolder()
	{
		// Enumerate and test top folder
		CComPtr<IShellFolder> spSubfolder = _enumFolderAndReturnSubfolder(
			m_spFolder, 0);

		// Enumerate and test subfolder
		spSubfolder = _enumFolderAndReturnSubfolder(spSubfolder, 1);
	}

	void testBindToManyFolders()
	{
		// Enumerate and test top folder
		CComPtr<IShellFolder> spSubfolder = _enumFolderAndReturnSubfolder(
			m_spFolder, 0);

		// Enumerate and test first-level subfolder
		spSubfolder = _enumFolderAndReturnSubfolder(spSubfolder, 1);

		// Enumerate and test second-level subfolder
		spSubfolder = _enumFolderAndReturnSubfolder(spSubfolder, 2);

		// Enumerate and test third-level subfolder
		_enumFolderAndReturnSubfolder(spSubfolder, 3);
	}

	void testBindToFarawayFolder()
	{
		HRESULT hr;
		CComPtr<IEnumIDList> spEnum;
		PIDLIST_RELATIVE pidl;
		CComPtr<IShellFolder> spSubfolder;

		// Get a PIDL to a folder in the 7th level of subfolders
		pidl = _walkDownFolders(m_spFolder, 7);
		CPPUNIT_ASSERT(pidl);

		// Bind to its folder using the top level folder
		hr = m_spFolder->BindToObject(pidl, NULL, IID_PPV_ARGS(&spSubfolder));
		::ILFree(pidl);

		_enumFolderAndReturnSubfolder(spSubfolder, 8);
	}

private:

	/**
	 * Walk down the levels of subfolder creating a relative PIDL to the @a max level.
	 */
	static PIDLIST_RELATIVE _walkDownFolders(IShellFolder *pFolder, int max, int i=0)
	{
		HRESULT hr;
		CComPtr<IEnumIDList> spEnum;
		PITEMID_CHILD pidlChild;
		CComPtr<IShellFolder> spSubfolder;

		// Enumerate folder
		hr = pFolder->EnumObjects(NULL, SHCONTF_FOLDERS, &spEnum);
		CPPUNIT_ASSERT_OK(hr);
		hr = spEnum->Next(1, &pidlChild, NULL);
		CPPUNIT_ASSERT_OK(hr);

		if (i < max)
		{
			// Get subfolder from first PIDL
			hr = pFolder->BindToObject(pidlChild, NULL, IID_PPV_ARGS(&spSubfolder));
			CPPUNIT_ASSERT_OK(hr);
			
			PIDLIST_RELATIVE pidl = ::ILCombine(
				reinterpret_cast<PIDLIST_ABSOLUTE>(pidlChild),
				_walkDownFolders(spSubfolder, max, i+1));

			::ILFree(pidlChild);

			return pidl;
		}
		else
		{
			return static_cast<PIDLIST_RELATIVE>(pidlChild);
		}
	}

	static CComPtr<IShellFolder> _enumFolderAndReturnSubfolder(
		IShellFolder *pFolder, int nExpectedLevel)
	{
		HRESULT hr;
		CComPtr<IEnumIDList> spEnum;
		PITEMID_CHILD pidl;
		ULONG celtFetched;
		CComPtr<IShellFolder> spSubfolder;

		// Enumerate folder
		hr = pFolder->EnumObjects(NULL, SHCONTF_FOLDERS, &spEnum);
		hr = spEnum->Next(1, &pidl, &celtFetched);
		CPPUNIT_ASSERT_OK(hr);
		CPPUNIT_ASSERT(pidl);

		// Test its first item (should only have one)

		// Check level
		const DummyItemId *pitemid = reinterpret_cast<const DummyItemId *>(pidl);
		CPPUNIT_ASSERT_EQUAL(nExpectedLevel, pitemid->level);

		// Check folderness
		SFGAOF rgfInOut = SFGAO_FOLDER | SFGAO_HASSUBFOLDER;
		hr = pFolder->GetAttributesOf(1, &pidl, &rgfInOut);
		CPPUNIT_ASSERT_OK(hr);
		CPPUNIT_ASSERT_EQUAL(SFGAO_FOLDER | SFGAO_HASSUBFOLDER, rgfInOut);


		// Get subfolder from first PIDL
		hr = pFolder->BindToObject(pidl, NULL, IID_PPV_ARGS(&spSubfolder));
		::ILFree(pidl);
		CPPUNIT_ASSERT_OK(hr);
		CPPUNIT_ASSERT(spSubfolder);

		return spSubfolder;
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( CDummyFolderPreInitialize_test );
CPPUNIT_TEST_SUITE_REGISTRATION( CDummyFolderEnum_test );
CPPUNIT_TEST_SUITE_REGISTRATION( CDummyFolderView_test );
CPPUNIT_TEST_SUITE_REGISTRATION( CDummyFolderSubfolders_test );