/**
    @file

    Miscellaneous tests for the Swish DataObject.

    @if license

    Copyright (C) 2012  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#ifndef TEST_SHELL_FOLDER_EXERCISE_DATA_OBJECT_HPP
#define TEST_SHELL_FOLDER_EXERCISE_DATA_OBJECT_HPP

#include "swish/host_folder/host_pidl.hpp" // host_itemid_view
#include "swish/remote_folder/remote_pidl.hpp" // path_from_remote_pidl
                                               // remote_itemid_view
#include "swish/shell_folder/DataObject.h"

#include <comet/error.h> // com_error_from_interface
#include <comet/ptr.h> // com_ptr

#include <boost/numeric/conversion/cast.hpp> // numeric_cast
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <algorithm> // replace
#include <string>
#include <vector>

/**
 * Test that Shell PIDL from DataObject holds the expected number of PIDLs.
 */
static void _testShellPIDLCount(
    comet::com_ptr<IDataObject> data_object, UINT nExpected)
{
    FORMATETC fetc = {
        (CLIPFORMAT)::RegisterClipboardFormat(CFSTR_SHELLIDLIST),
        NULL,
        DVASPECT_CONTENT,
        -1,
        TYMED_HGLOBAL
    };

    STGMEDIUM stg;
    HRESULT hr = data_object->GetData(&fetc, &stg);
    BOOST_REQUIRE_OK(hr);
    
    BOOST_CHECK(stg.hGlobal);
    CIDA *pida = (CIDA *)::GlobalLock(stg.hGlobal);
    BOOST_CHECK(pida);

    UINT nActual = pida->cidl;
    BOOST_CHECK_EQUAL(nExpected, nActual);

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
    comet::com_ptr<IDataObject> data_object, std::wstring expected, UINT iFile)
{
    FORMATETC fetc = {
        (CLIPFORMAT)::RegisterClipboardFormat(CFSTR_SHELLIDLIST),
        NULL,
        DVASPECT_CONTENT,
        -1,
        TYMED_HGLOBAL
    };

    STGMEDIUM stg;
    HRESULT hr = data_object->GetData(&fetc, &stg);
    BOOST_REQUIRE_OK(hr);
    
    BOOST_CHECK(stg.hGlobal);
    CIDA *pida = (CIDA *)::GlobalLock(stg.hGlobal);
    BOOST_CHECK(pida);

    BOOST_CHECK_EQUAL(
        expected,
        swish::remote_folder::path_from_remote_pidl(
            GetPIDLItem(pida, iFile)).string().c_str());

    ::GlobalUnlock(stg.hGlobal);
    pida = NULL;
    ::ReleaseStgMedium(&stg);
}

/**
 * Test that Shell PIDL from DataObject represents the common root folder.
 *
 * The PIDL may be a RemoteItemId, in which case @p expected should be the
 * name of the directory (e.g "tmp"), but it may also be an HostItemId in which 
 * case the path (e.g. "/tmp") that is expected to be found in that item
 * should be passed.
 */
static void _testShellPIDLFolder(
    comet::com_ptr<IDataObject> data_object, std::wstring expected)
{
    FORMATETC fetc = {
        (CLIPFORMAT)::RegisterClipboardFormat(CFSTR_SHELLIDLIST),
        NULL,
        DVASPECT_CONTENT,
        -1,
        TYMED_HGLOBAL
    };

    STGMEDIUM stg;
    HRESULT hr = data_object->GetData(&fetc, &stg);
    BOOST_REQUIRE_OK(hr);
    
    BOOST_CHECK(stg.hGlobal);
    CIDA *pida = (CIDA *)::GlobalLock(stg.hGlobal);
    BOOST_CHECK(pida);

    // Test folder PIDL which may be a RemoteItemId or a HostItemId
    washer::shell::pidl::cpidl_t actual = ::ILFindLastID(GetPIDLFolder(pida));
    if (swish::remote_folder::remote_itemid_view(actual).valid())
    {
        BOOST_CHECK_EQUAL(
            expected,
            swish::remote_folder::remote_itemid_view(actual).filename());
    }
    else if (swish::host_folder::host_itemid_view(actual).valid())
    {
        swish::host_folder::host_itemid_view itemid(actual);
        boost::filesystem::wpath actual_path = itemid.path();
        BOOST_CHECK_EQUAL(expected, actual_path.string());
    }
    else
    {
        BOOST_FAIL("Invalid PIDL");
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
    comet::com_ptr<IDataObject> data_object, std::wstring expected, UINT iFile)
{
    std::replace(expected.begin(), expected.end(), L'/', L'\\');

    FORMATETC fetc = {
        (CLIPFORMAT)::RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR),
        NULL,
        DVASPECT_CONTENT,
        -1,
        TYMED_HGLOBAL
    };

    STGMEDIUM stg = STGMEDIUM();
    stg.tymed = fetc.tymed;
    HRESULT hr = data_object->GetData(&fetc, &stg);
    if (FAILED(hr))
        BOOST_THROW_EXCEPTION(comet::com_error_from_interface(data_object, hr));
    
    BOOST_CHECK(stg.hGlobal);
    FILEGROUPDESCRIPTOR *fgd = 
        (FILEGROUPDESCRIPTOR *)::GlobalLock(stg.hGlobal);
    BOOST_CHECK(fgd);

    BOOST_CHECK(iFile < fgd->cItems);
    BOOST_CHECK(fgd->fgd);
    std::wstring actual(fgd->fgd[iFile].cFileName);
    BOOST_CHECK_EQUAL(expected, actual);

    ::GlobalUnlock(stg.hGlobal);
    fgd = NULL;
    ::ReleaseStgMedium(&stg);
}

/**
 * Test that the contents of the DummyStream matches what is expected.
 */
static void _testStreamContents(
    comet::com_ptr<IDataObject> data_object, std::wstring expected, UINT iFile)
{
    FORMATETC fetc = {
        (CLIPFORMAT)::RegisterClipboardFormat(CFSTR_FILECONTENTS),
        NULL,
        DVASPECT_CONTENT,
        iFile,
        TYMED_ISTREAM
    };

    STGMEDIUM stg;
    HRESULT hr = data_object->GetData(&fetc, &stg);
    if (FAILED(hr))
        BOOST_THROW_EXCEPTION(comet::com_error_from_interface(data_object, hr));
    
    BOOST_CHECK(stg.pstm);

    std::vector<wchar_t> buffer(MAX_PATH);
    ULONG cbRead = 0;
    hr = stg.pstm->Read(
        &buffer[0], boost::numeric_cast<ULONG>(buffer.size()), &cbRead);
    BOOST_REQUIRE_OK(hr);

    std::wstring actual(&buffer[0]);
    BOOST_CHECK_EQUAL(expected, actual);

    ::ReleaseStgMedium(&stg);
}

/**
 * Test for success (or failure) when querying the presence of
 * our expected formats.
 */
static void _testQueryFormats(
    comet::com_ptr<IDataObject> data_object, bool fFailTest=false)
{
    HRESULT hr;

    // Test CFSTR_SHELLIDLIST (PIDL array) format
    if (!fFailTest) // Vista includes this format even for empty PIDL array
    {
        CFormatEtc fetcShellIdList(CFSTR_SHELLIDLIST);
        hr = data_object->QueryGetData(&fetcShellIdList);
        BOOST_REQUIRE_OK(hr);
    }

    // Test CFSTR_FILEDESCRIPTOR (FILEGROUPDESCRIPTOR) format
    CFormatEtc fetcDescriptor(CFSTR_FILEDESCRIPTOR);
    hr = data_object->QueryGetData(&fetcDescriptor);
    BOOST_CHECK(hr == ((fFailTest) ? S_FALSE : S_OK));

    // Test CFSTR_FILECONTENTS (IStream) 

    // Since Windows 7 (or maybe Vista) we must get TYMED_ISTREAM right here.
    // Previously if you prodded with a TYMED_ISTREAM but checked with
    // TYMED_GLOBAL it still worked.  Not any more
    CFormatEtc fetcContents(CFSTR_FILECONTENTS, TYMED_ISTREAM);
    hr = data_object->QueryGetData(&fetcContents);
    BOOST_CHECK(hr == ((fFailTest) ? S_FALSE : S_OK));
}

/**
 * Test enumerator for the presence (or absence) of our expected formats.
 */
static void _testEnumerator(
    comet::com_ptr<IEnumFORMATETC> pEnum, bool fFailTest=false)
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
        BOOST_CHECK(fFoundShellIdList);

    // Test CFSTR_FILEDESCRIPTOR (FILEGROUPDESCRIPTOR) format
    BOOST_CHECK((fFailTest) ? !fFoundDescriptor : fFoundDescriptor);

    // Test CFSTR_FILECONTENTS (IStream) 
    BOOST_CHECK((fFailTest) ? !fFoundContents : fFoundContents);
}

/**
 * Perform our enumerator tests for both SetData() and GetData() enums.
 */
static void _testBothEnumerators(
    comet::com_ptr<IDataObject> data_object, bool fFailTest=false)
{
    HRESULT hr;

    // Test enumerator of GetData() formats
    comet::com_ptr<IEnumFORMATETC> enum_get;
    hr = data_object->EnumFormatEtc(DATADIR_GET, enum_get.out());
    BOOST_REQUIRE_OK(hr);
    _testEnumerator(enum_get, fFailTest);

    // Test enumerator of SetData() formats
    comet::com_ptr<IEnumFORMATETC> enum_set;
    hr = data_object->EnumFormatEtc(DATADIR_SET, enum_set.out());
    BOOST_REQUIRE_OK(hr);
    _testEnumerator(enum_set, fFailTest);
}

#endif