#include "test/common/CppUnitExtensions.h"
#include "test/common/MockSftpConsumer.h"
#include "test/common/MockSftpProvider.h"
typedef CMockSftpProvider MP;
typedef CMockSftpConsumer MC;
#include "test/common/TestConfig.h"
#include "DataObjectTests.h"

#include "swish/host_folder/host_pidl.hpp" // create_host_itemid
#include "swish/remote_folder/remote_pidl.hpp" // create_remote_itemid,
                                               // remote_itemid_view
#include "swish/shell_folder/DataObject.h"

using swish::host_folder::create_host_itemid;
using swish::remote_folder::create_remote_itemid;
using swish::remote_folder::remote_itemid_view;

using ATL::CComObject;
using ATL::CComPtr;
using ATL::CComBSTR;

/**
 * Tests for our generic shell DataObject wrapper.  This class only creates
 * CFSTR_SHELLIDLIST formats (and some misc private shell ones) on its own.
 * However, it will store other format when they are set using SetData() and
 * will return them in GetData() and well as acknowleging their presence
 * in QueryGetData() and in the IEnumFORMATETC.
 *
 * Creation of other formats is left to the CSftpDataObject subclass.
 *
 * These tests verify this behaviour.
 */
class CDataObject_test : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( CDataObject_test );
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
            CComBSTR(config.GetUser()),
            CComBSTR(config.GetHost()), config.GetPort()
        );
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
        CAbsolutePidl pidlRoot = _CreateRootRemotePidl();
        cpidl_t pidl = create_remote_itemid(
            L"testswishfile.ext", false, false, L"mockowner", L"mockgroup",
            1001, 1002, 0677, 1024);

        CComPtr<IDataObject> spDo = 
            CDataObject::Create(1, pidl.get(), pidlRoot);

        // Keep extra reference to check for leaks in tearDown()
        spDo.CopyTo(&m_pDo);

        // Test CFSTR_SHELLIDLIST (PIDL array) format
        remote_itemid_view pidlFolder(::ILFindLastID(pidlRoot));
        _testShellPIDLFolder(spDo, pidlFolder.filename());
        _testShellPIDL(spDo, remote_itemid_view(pidl).filename(), 0);

        // Test CFSTR_FILEDESCRIPTOR (FILEGROUPDESCRIPTOR) format
        CPPUNIT_ASSERT_THROW_MESSAGE(
            "CDataObject should not produce a CFSTR_FILEDESCRIPTOR format",
            _testFileDescriptor(spDo, L"testswishfile.ext", 0),
            CppUnit::Exception);

        // Test CFSTR_FILECONTENTS (IStream) format
        CPPUNIT_ASSERT_THROW_MESSAGE(
            "CDataObject should not produce a CFSTR_FILECONTENTS format",
            _testStreamContents(spDo, L"/tmp/swish/testswishfile.ext", 0),
            CppUnit::Exception);
    }

    void testCreateMulti()
    {
        CAbsolutePidl pidlRoot = _CreateRootRemotePidl();
        cpidl_t pidl1 = create_remote_itemid(
            L"testswishfile.ext", false, false, L"mockowner", L"mockgroup",
            1001, 1002, 0677, 1024);
        cpidl_t pidl2 = create_remote_itemid(
            L"testswishfile.txt", false, false, L"mockowner", L"mockgroup",
            1001, 1002, 0677, 1024);
        cpidl_t pidl3 = create_remote_itemid(
            L"testswishFile", false, false, L"mockowner", L"mockgroup",
            1001, 1002, 0677, 1024);
        PCITEMID_CHILD aPidl[3];
        aPidl[0] = pidl1.get();
        aPidl[1] = pidl2.get();
        aPidl[2] = pidl3.get();

        CComPtr<IDataObject> spDo =
            CDataObject::Create(3, aPidl, pidlRoot);

        // Keep extra reference to check for leaks in tearDown()
        m_pDo = spDo.p;
        m_pDo->AddRef();

        // Test CFSTR_SHELLIDLIST (PIDL array) format
        remote_itemid_view pidlFolder(::ILFindLastID(pidlRoot));
        _testShellPIDLFolder(spDo, pidlFolder.filename());
        _testShellPIDL(spDo, remote_itemid_view(pidl1).filename(), 0);
        _testShellPIDL(spDo, remote_itemid_view(pidl2).filename(), 1);
        _testShellPIDL(spDo, remote_itemid_view(pidl3).filename(), 2);
    }

    /**
     * Test that QueryGetData fails for all our formats when created with
     * empty PIDL list.
     */
    void testQueryFormatsEmpty()
    {
        CComPtr<IDataObject> spDo = CDataObject::Create(0, NULL, NULL);

        // Keep extra reference to check for leaks in tearDown()
        spDo.CopyTo(&m_pDo);

        // Test that QueryGetData() responds negatively for all our formats
        _testQueryFormats(spDo, true);
    }

    /**
     * Test that none of our expected formats are in the enumerator when 
     * created with empty PIDL list.
     */
    void testEnumFormatsEmpty()
    {
        CComPtr<IDataObject> spDo = CDataObject::Create(0, NULL, NULL);

        // Keep extra reference to check for leaks in tearDown()
        spDo.CopyTo(&m_pDo);

        // Test that enumerators of both GetData() and SetData()
        // formats fail to enumerate any of our formats
        _testBothEnumerators(spDo, true);
    }

    /**
     * Test that QueryGetData responds successfully for all our formats.
     */
    void testQueryFormats()
    {
        CAbsolutePidl pidlRoot = _CreateRootRemotePidl();
        cpidl_t pidl = create_remote_itemid(
            L"testswishfile.ext", false, false, L"mockowner", L"mockgroup",
            1001, 1002, 0677, 1024);

        CComPtr<IDataObject> spDo = 
            CDataObject::Create(1, pidl.get(), pidlRoot);

        // Keep extra reference to check for leaks in tearDown()
        spDo.CopyTo(&m_pDo);

        // Perform query tests
        _testCDataObjectQueryFormats(spDo);
    }

    /**
     * Test that all our expected formats are in the enumeration.
     */
    void testEnumFormats()
    {
        CAbsolutePidl pidlRoot = _CreateRootRemotePidl();
        cpidl_t pidl = create_remote_itemid(
            L"testswishfile.ext", false, false, L"mockowner", L"mockgroup",
            1001, 1002, 0677, 1024);

        CComPtr<IDataObject> spDo = 
            CDataObject::Create(1, pidl.get(), pidlRoot);

        // Keep extra reference to check for leaks in tearDown()
        spDo.CopyTo(&m_pDo);

        // Test enumerators of both GetData() and SetData() formats
        _testBothCDataObjectEnumerators(spDo);
    }

    /**
     * Test that QueryGetData responds successfully for all our formats when
     * initialised with multiple PIDLs.
     */
    void testQueryFormatsMulti()
    {
        CAbsolutePidl pidlRoot = _CreateRootRemotePidl();
        cpidl_t pidl1 = create_remote_itemid(
            L"testswishfile.ext", false, false, L"mockowner", L"mockgroup",
            1001, 1002, 0677, 1024);
        cpidl_t pidl2 = create_remote_itemid(
            L"testswishfile.txt", false, false, L"mockowner", L"mockgroup",
            1001, 1002, 0677, 1024);
        cpidl_t pidl3 = create_remote_itemid(
            L"testswishFile", false, false, L"mockowner", L"mockgroup",
            1001, 1002, 0677, 1024);
        PCITEMID_CHILD aPidl[3];
        aPidl[0] = pidl1.get();
        aPidl[1] = pidl2.get();
        aPidl[2] = pidl3.get();

        CComPtr<IDataObject> spDo = CDataObject::Create(3, aPidl, pidlRoot);

        // Keep extra reference to check for leaks in tearDown()
        spDo.CopyTo(&m_pDo);

        // Perform query tests
        _testCDataObjectQueryFormats(spDo);
    }

    /**
     * Test that all our expected formats are in the enumeration when
     * initialised with multiple PIDLs.
     */
    void testEnumFormatsMulti()
    {
        CAbsolutePidl pidlRoot = _CreateRootRemotePidl();
        cpidl_t pidl1 = create_remote_itemid(
            L"testswishfile.ext", false, false, L"mockowner", L"mockgroup",
            1001, 1002, 0677, 1024);
        cpidl_t pidl2 = create_remote_itemid(
            L"testswishfile.txt", false, false, L"mockowner", L"mockgroup",
            1001, 1002, 0677, 1024);
        cpidl_t pidl3 = create_remote_itemid(
            L"testswishFile", false, false, L"mockowner", L"mockgroup",
            1001, 1002, 0677, 1024);
        PCITEMID_CHILD aPidl[3];
        aPidl[0] = pidl1.get();
        aPidl[1] = pidl2.get();
        aPidl[2] = pidl3.get();

        CComPtr<IDataObject> spDo = CDataObject::Create(3, aPidl, pidlRoot);

        // Keep extra reference to check for leaks in tearDown()
        spDo.CopyTo(&m_pDo);

        // Test enumerators of both GetData() and SetData() formats
        _testBothCDataObjectEnumerators(spDo);
    }

private:

    IDataObject *m_pDo;
    CComObject<CMockSftpConsumer> *m_pCoConsumer;
    ISftpConsumer *m_pConsumer;
    CComObject<CMockSftpProvider> *m_pCoProvider;
    ISftpProvider *m_pProvider;

    CTestConfig config;

    /**
     * Test enumerator for the presence of CFSTR_SHELLIDLIST but the absence of
     * CFSTR_FILEDESCRIPTOR and CFSTR_FILECONTENTS.
     *
     * Format-limited version of _testEnumerator() in DataObjectTests.h.
     */
    static void _testCDataObjectEnumerator(IEnumFORMATETC *pEnum)
    {
        CLIPFORMAT cfShellIdList = static_cast<CLIPFORMAT>(
            ::RegisterClipboardFormat(CFSTR_SHELLIDLIST));
        CLIPFORMAT cfDescriptor = static_cast<CLIPFORMAT>(
            ::RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR));
        CLIPFORMAT cfContents = static_cast<CLIPFORMAT>(
            ::RegisterClipboardFormat(CFSTR_FILECONTENTS));

        bool fFoundShellIdList = false;
        bool fFoundDescriptor = false;
        bool fFoundContents = false;

        HRESULT hr;
        do {
            FORMATETC fetc;
            hr = pEnum->Next(1, &fetc, NULL);
            if (hr == S_OK)
            {
                if (fetc.cfFormat == cfShellIdList)
                    fFoundShellIdList = true;
                else if (fetc.cfFormat == cfDescriptor)
                    fFoundDescriptor = true;
                else if (fetc.cfFormat == cfContents)
                    fFoundContents = true;
            }
        } while (hr == S_OK);

        // Test CFSTR_SHELLIDLIST (PIDL array) format present
        CPPUNIT_ASSERT(fFoundShellIdList);

        // Test CFSTR_FILEDESCRIPTOR (FILEGROUPDESCRIPTOR) format absent
        CPPUNIT_ASSERT(!fFoundDescriptor);

        // Test CFSTR_FILECONTENTS (IStream) format absent
        CPPUNIT_ASSERT(!fFoundContents);
    }

    /**
     * Test the GetData() enumerator for the presence of CFSTR_SHELLIDLIST
     * but the absence of CFSTR_FILEDESCRIPTOR and CFSTR_FILECONTENTS.
     *
     * Test the SetData() enumerator for the presence of all three formats.
     *
     * Format-limited version of _testBothEnumerators() in DataObjectTests.h.
     */
    static void _testBothCDataObjectEnumerators(IDataObject *pDo)
    {
        HRESULT hr;

        // Test enumerator of GetData() formats
        CComPtr<IEnumFORMATETC> spEnumGet;
        hr = pDo->EnumFormatEtc(DATADIR_GET, &spEnumGet);
        CPPUNIT_ASSERT_OK(hr);
        _testCDataObjectEnumerator(spEnumGet);

        // Test enumerator of SetData() formats
        CComPtr<IEnumFORMATETC> spEnumSet;
        hr = pDo->EnumFormatEtc(DATADIR_SET, &spEnumSet);
        CPPUNIT_ASSERT_OK(hr);
        _testCDataObjectEnumerator(spEnumSet);
    }

    /**
     * Test QueryGetData() enumerator for the presence of CFSTR_SHELLIDLIST
     * but the absence of CFSTR_FILEDESCRIPTOR and CFSTR_FILECONTENTS.
     *
     * Format-limited version of _testQueryFormats() in DataObjectTests.h.
     */
    static void _testCDataObjectQueryFormats(IDataObject *pDo)
    {
        // Test CFSTR_SHELLIDLIST (PIDL array) format succeeds
        CFormatEtc fetcShellIdList(CFSTR_SHELLIDLIST);
        CPPUNIT_ASSERT_OK(pDo->QueryGetData(&fetcShellIdList));

        // Test CFSTR_FILEDESCRIPTOR (FILEGROUPDESCRIPTOR) format fails
        CFormatEtc fetcDescriptor(CFSTR_FILEDESCRIPTOR);
        CPPUNIT_ASSERT(pDo->QueryGetData(&fetcDescriptor) == S_FALSE);

        // Test CFSTR_FILECONTENTS (IStream) format fails
        CFormatEtc fetcContents(CFSTR_FILECONTENTS);
        CPPUNIT_ASSERT(pDo->QueryGetData(&fetcContents) == S_FALSE);
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
        cpidl_t pidlRemote = create_remote_itemid(
            L"swish", true, false, L"owner", L"group", 1001, 1002, 0677, 1024);

        // Concatenate to make absolute pidl to RemoteFolder root
        return CAbsolutePidl(pidlHost, pidlRemote.get());
    }

    /**
     * Get an absolute PIDL that ends in a HOSTPIDL to root RemoteFolder on.
     */
    static CAbsolutePidl _CreateRootHostPidl()
    {
        // Create absolute PIDL to Swish icon
        CAbsolutePidl pidlSwish = _GetSwishPidl();

        // Concatenate to make absolute pidl to RemoteFolder root
        return CAbsolutePidl(
            pidlSwish, create_host_itemid(
                L"test.example.com", L"user", L"/tmp", 22, L"Test PIDL").get());
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
