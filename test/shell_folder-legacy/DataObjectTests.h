// Miscellanious tests for the Swish DataObject

#include "test/common/CppUnitExtensions.h"

#include "swish/host_folder/host_pidl.hpp" // host_itemid_view
#include "swish/remote_folder/remote_pidl.hpp" // path_from_remote_pidl
                                               // remote_itemid_view
#include "swish/shell_folder/DataObject.h"

/**
 * Test that Shell PIDL from DataObject holds the expected number of PIDLs.
 */
static void _testShellPIDLCount(IDataObject *pDo, UINT nExpected)
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

    UINT nActual = pida->cidl;
    CPPUNIT_ASSERT_EQUAL(nExpected, nActual);

    ::GlobalUnlock(stg.hGlobal);
    pida = NULL;
    ::ReleaseStgMedium(&stg);
}

#define GetPIDLFolder(pida) \
    (PCIDLIST_ABSOLUTE)(((LPBYTE)pida)+(pida)->aoffset[0])
#define GetPIDLItem(pida, i) \
    (PCIDLIST_RELATIVE)(((LPBYTE)pida)+(pida)->aoffset[i+1])

/**
 * Test that Shell PIDL from DataObject represent the expected file.
 */
static void _testShellPIDL(
    IDataObject *pDo, ATL::CString strExpected, UINT iFile)
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

    CPPUNIT_ASSERT_EQUAL(
        strExpected,
        swish::remote_folder::path_from_remote_pidl(
            GetPIDLItem(pida, iFile)).c_str());

    ::GlobalUnlock(stg.hGlobal);
    pida = NULL;
    ::ReleaseStgMedium(&stg);
}

/**
 * Test that Shell PIDL from DataObject represents the common root folder.
 *
 * The PIDL may be a RemoteItemId, in which case @p strExpected should be the
 * name of the directory (e.g "tmp"), but it may also be an HostItemId in which 
 * case the path (e.g. "/tmp") that is expected to be found in that item
 * should be passed.
 */
static void _testShellPIDLFolder(IDataObject *pDo, ATL::CString strExpected)
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

    // Test folder PIDL which may be a RemoteItemId or a HostItemId
    CChildPidl pidlActual = ::ILFindLastID(GetPIDLFolder(pida));
    if (swish::remote_folder::remote_itemid_view(pidlActual.m_pidl).is_folder())
    {
        CPPUNIT_ASSERT_EQUAL(
            strExpected,
            swish::remote_folder::remote_itemid_view(
                pidlActual.m_pidl).filename().c_str());
    }
    else
    {
        swish::host_folder::host_itemid_view itemid(pidlActual.m_pidl);
        CPPUNIT_ASSERT_EQUAL(strExpected, itemid.path().string().c_str());
    }
    ::GlobalUnlock(stg.hGlobal);
    pida = NULL;
    ::ReleaseStgMedium(&stg);
}

/**
 * Test that the the FILEGROUPDESCRIPTOR and ith FILEDESCRIPTOR
 * match expected values.
 * File descriptors should use Windows path separators so we replace
 * forward slashes with back slashes in expected string.
 */
static void _testFileDescriptor(
    IDataObject *pDo, ATL::CString strExpected, UINT iFile)
{
    strExpected.Replace(L"/", L"\\");

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
    ATL::CString strActual(fgd->fgd[iFile].cFileName);
    CPPUNIT_ASSERT_EQUAL(strExpected, strActual);

    ::GlobalUnlock(stg.hGlobal);
    fgd = NULL;
    ::ReleaseStgMedium(&stg);
}

/**
 * Test that the contents of the DummyStream matches what is expected.
 */
static void _testStreamContents(
    IDataObject *pDo, ATL::CString strExpected, UINT iFile)
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

    ATL::CString strActual(szBuf);
    CPPUNIT_ASSERT_EQUAL(strExpected, strActual);

    ::ReleaseStgMedium(&stg);
}

/**
 * Test for success (or failure) when querying the presence of
 * our expected formats.
 */
static void _testQueryFormats(IDataObject *pDo, bool fFailTest=false)
{
    HRESULT hr;

    // Test CFSTR_SHELLIDLIST (PIDL array) format
    if (!fFailTest) // Vista includes this format even for empty PIDL array
    {
        CFormatEtc fetcShellIdList(CFSTR_SHELLIDLIST);
        hr = pDo->QueryGetData(&fetcShellIdList);
        CPPUNIT_ASSERT_OK(hr);
    }

    // Test CFSTR_FILEDESCRIPTOR (FILEGROUPDESCRIPTOR) format
    CFormatEtc fetcDescriptor(CFSTR_FILEDESCRIPTOR);
    hr = pDo->QueryGetData(&fetcDescriptor);
    CPPUNIT_ASSERT(hr == ((fFailTest) ? S_FALSE : S_OK));

    // Test CFSTR_FILECONTENTS (IStream) 
    CFormatEtc fetcContents(CFSTR_FILECONTENTS);
    hr = pDo->QueryGetData(&fetcContents);
    CPPUNIT_ASSERT(hr == ((fFailTest) ? S_FALSE : S_OK));
}

/**
 * Test enumerator for the presence (or absence) of our expected formats.
 */
static void _testEnumerator(IEnumFORMATETC *pEnum, bool fFailTest=false)
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

    // Test CFSTR_SHELLIDLIST (PIDL array) format
    if (!fFailTest) // Vista includes this format even for empty PIDL array
        CPPUNIT_ASSERT(fFoundShellIdList);

    // Test CFSTR_FILEDESCRIPTOR (FILEGROUPDESCRIPTOR) format
    CPPUNIT_ASSERT((fFailTest) ? !fFoundDescriptor : fFoundDescriptor);

    // Test CFSTR_FILECONTENTS (IStream) 
    CPPUNIT_ASSERT((fFailTest) ? !fFoundContents : fFoundContents);
}

/**
 * Perform our enumerator tests for both SetData() and GetData() enums.
 */
static void _testBothEnumerators(IDataObject *pDo, bool fFailTest=false)
{
    HRESULT hr;

    // Test enumerator of GetData() formats
    ATL::CComPtr<IEnumFORMATETC> spEnumGet;
    hr = pDo->EnumFormatEtc(DATADIR_GET, &spEnumGet);
    CPPUNIT_ASSERT_OK(hr);
    _testEnumerator(spEnumGet, fFailTest);

    // Test enumerator of SetData() formats
    ATL::CComPtr<IEnumFORMATETC> spEnumSet;
    hr = pDo->EnumFormatEtc(DATADIR_SET, &spEnumSet);
    CPPUNIT_ASSERT_OK(hr);
    _testEnumerator(spEnumSet, fFailTest);
}
