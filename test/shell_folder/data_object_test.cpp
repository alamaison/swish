/**
    @file

    Testing DataObject implementation.

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

#include "test/common_boost/fixtures.hpp"
#include "test/common_boost/helpers.hpp"
#include "test/common_boost/SwishPidlFixture.hpp"
#include "exercise_data_object.h"

#include "swish/host_folder/host_pidl.hpp" // create_host_itemid
#include "swish/remote_folder/remote_pidl.hpp" // remote_itemid_view
#include "swish/shell_folder/DataObject.h"

#include <washer/shell/pidl.hpp>

#include <comet/bstr.h> // bstr_t
#include <comet/ptr.h> // com_ptr

#include <boost/test/unit_test.hpp>

#include <vector>

using swish::host_folder::create_host_itemid;
using swish::remote_folder::remote_itemid_view;

using washer::shell::pidl::apidl_t;
using washer::shell::pidl::cpidl_t;

using comet::bstr_t;
using comet::com_ptr;

namespace comet {

template<> struct comtype<IDataObject>
{
	static const IID& uuid() { return IID_IDataObject; }
	typedef ::IUnknown base;
};

}

namespace test {

namespace {

/**
 * Test enumerator for the presence of CFSTR_SHELLIDLIST but the absence of
 * CFSTR_FILEDESCRIPTOR and CFSTR_FILECONTENTS.
 *
 * Format-limited version of _testEnumerator() in DataObjectTests.h.
 */
void _testCDataObjectEnumerator(com_ptr<IEnumFORMATETC> pEnum)
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
    BOOST_CHECK(fFoundShellIdList);

    // Test CFSTR_FILEDESCRIPTOR (FILEGROUPDESCRIPTOR) format absent
    BOOST_CHECK(!fFoundDescriptor);

    // Test CFSTR_FILECONTENTS (IStream) format absent
    BOOST_CHECK(!fFoundContents);
}

/**
 * Test the GetData() enumerator for the presence of CFSTR_SHELLIDLIST
 * but the absence of CFSTR_FILEDESCRIPTOR and CFSTR_FILECONTENTS.
 *
 * Test the SetData() enumerator for the presence of all three formats.
 *
 * Format-limited version of _testBothEnumerators() in DataObjectTests.h.
 */
void _testBothCDataObjectEnumerators(com_ptr<IDataObject> data_object)
{
    HRESULT hr;

    // Test enumerator of GetData() formats
    com_ptr<IEnumFORMATETC> spEnumGet;
    hr = data_object->EnumFormatEtc(DATADIR_GET, spEnumGet.out());
    BOOST_REQUIRE_OK(hr);
    _testCDataObjectEnumerator(spEnumGet);

    // Test enumerator of SetData() formats
    com_ptr<IEnumFORMATETC> spEnumSet;
    hr = data_object->EnumFormatEtc(DATADIR_SET, spEnumSet.out());
    BOOST_REQUIRE_OK(hr);
    _testCDataObjectEnumerator(spEnumSet);
}

/**
 * Test QueryGetData() enumerator for the presence of CFSTR_SHELLIDLIST
 * but the absence of CFSTR_FILEDESCRIPTOR and CFSTR_FILECONTENTS.
 *
 * Format-limited version of _testQueryFormats() in DataObjectTests.h.
 */
void _testCDataObjectQueryFormats(com_ptr<IDataObject> data_object)
{
    // Test CFSTR_SHELLIDLIST (PIDL array) format succeeds
    CFormatEtc fetcShellIdList(CFSTR_SHELLIDLIST);
    BOOST_REQUIRE_OK(data_object->QueryGetData(&fetcShellIdList));

    // Test CFSTR_FILEDESCRIPTOR (FILEGROUPDESCRIPTOR) format fails
    CFormatEtc fetcDescriptor(CFSTR_FILEDESCRIPTOR);
    BOOST_CHECK(data_object->QueryGetData(&fetcDescriptor) == S_FALSE);

    // Test CFSTR_FILECONTENTS (IStream) format fails
    CFormatEtc fetcContents(CFSTR_FILECONTENTS);
    BOOST_CHECK(data_object->QueryGetData(&fetcContents) == S_FALSE);
}

class TestFixture : public SwishPidlFixture, public ComFixture
{
};

}

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
BOOST_FIXTURE_TEST_SUITE(data_object_tests, TestFixture)

BOOST_AUTO_TEST_CASE( Create )
{
    apidl_t root = create_dummy_root_pidl();
    cpidl_t pidl = create_dummy_remote_itemid(L"testswishfile.ext", false);

    PCITEMID_CHILD pidl_array[] = { pidl.get() };

    com_ptr<IDataObject> data_object =
        new CDataObject(1, pidl_array, root.get());

    // Test CFSTR_SHELLIDLIST (PIDL array) format
    cpidl_t root_child = root.last_item();
    remote_itemid_view folder(root_child);
    _testShellPIDLFolder(data_object, folder.filename());
    _testShellPIDL(data_object, remote_itemid_view(pidl).filename(), 0);

    // Test CFSTR_FILEDESCRIPTOR (FILEGROUPDESCRIPTOR) format
    // CDataObject should not produce a CFSTR_FILEDESCRIPTOR format
    BOOST_CHECK_THROW(
        _testFileDescriptor(data_object, L"testswishfile.ext", 0),
        std::exception);

    // Test CFSTR_FILECONTENTS (IStream) format
    // CDataObject should not produce a CFSTR_FILECONTENTS format
    BOOST_CHECK_THROW(
        _testStreamContents(data_object, L"/tmp/swish/testswishfile.ext", 0),
        std::exception);
}

BOOST_AUTO_TEST_CASE( CreateMulti )
{
    apidl_t root = create_dummy_root_pidl();
    cpidl_t pidl1 = create_dummy_remote_itemid(L"testswishfile.ext", false);
    cpidl_t pidl2 = create_dummy_remote_itemid(L"testswishfile.txt", false);
    cpidl_t pidl3 = create_dummy_remote_itemid(L"testswishFile", false);

    PCITEMID_CHILD aPidl[3];
    aPidl[0] = pidl1.get();
    aPidl[1] = pidl2.get();
    aPidl[2] = pidl3.get();

    com_ptr<IDataObject> data_object =
        new CDataObject(3, aPidl, root.get());

    // Test CFSTR_SHELLIDLIST (PIDL array) format
    cpidl_t root_child = root.last_item();
    remote_itemid_view folder(root_child);
    _testShellPIDLFolder(data_object, folder.filename());
    _testShellPIDL(data_object, remote_itemid_view(pidl1).filename(), 0);
    _testShellPIDL(data_object, remote_itemid_view(pidl2).filename(), 1);
    _testShellPIDL(data_object, remote_itemid_view(pidl3).filename(), 2);
}

/**
 * Test that QueryGetData fails for all our formats when created with
 * empty PIDL list.
 */
BOOST_AUTO_TEST_CASE( QueryFormatsEmpty )
{
    com_ptr<IDataObject> data_object = new CDataObject(0, NULL, NULL);

    // Test that QueryGetData() responds negatively for all our formats
    _testQueryFormats(data_object, true);
}

/**
 * Test that none of our expected formats are in the enumerator when 
 * created with empty PIDL list.
 */
BOOST_AUTO_TEST_CASE( EnumFormatsEmpty )
{
    com_ptr<IDataObject> data_object = new CDataObject(0, NULL, NULL);

    // Test that enumerators of both GetData() and SetData()
    // formats fail to enumerate any of our formats
    _testBothEnumerators(data_object, true);
}

/**
 * Test that QueryGetData responds successfully for all our formats.
 */
BOOST_AUTO_TEST_CASE( QueryFormats )
{
    apidl_t root = create_dummy_root_pidl();
    cpidl_t pidl = create_dummy_remote_itemid(L"testswishfile.ext", false);

    PCITEMID_CHILD pidl_array[] = { pidl.get() };

    com_ptr<IDataObject> data_object = 
        new CDataObject(1, pidl_array, root.get());

    // Perform query tests
    _testCDataObjectQueryFormats(data_object);
}

/**
 * Test that all our expected formats are in the enumeration.
 */
BOOST_AUTO_TEST_CASE( EnumFormats )
{
    apidl_t root = create_dummy_root_pidl();
    cpidl_t pidl = create_dummy_remote_itemid(L"testswishfile.ext", false);

    PCITEMID_CHILD pidl_array[] = { pidl.get() };

    com_ptr<IDataObject> data_object = 
        new CDataObject(1, pidl_array, root.get());

    // Test enumerators of both GetData() and SetData() formats
    _testBothCDataObjectEnumerators(data_object);
}

/**
 * Test that QueryGetData responds successfully for all our formats when
 * initialised with multiple PIDLs.
 */
BOOST_AUTO_TEST_CASE( QueryFormatsMulti )
{
    apidl_t root = create_dummy_root_pidl();
    cpidl_t pidl1 = create_dummy_remote_itemid(L"testswishfile.ext", false);
    cpidl_t pidl2 = create_dummy_remote_itemid(L"testswishfile.txt", false);
    cpidl_t pidl3 = create_dummy_remote_itemid(L"testswishFile", false);
    PCITEMID_CHILD aPidl[3];
    aPidl[0] = pidl1.get();
    aPidl[1] = pidl2.get();
    aPidl[2] = pidl3.get();

    com_ptr<IDataObject> data_object = new CDataObject(3, aPidl, root.get());

    // Perform query tests
    _testCDataObjectQueryFormats(data_object);
}

/**
 * Test that all our expected formats are in the enumeration when
 * initialised with multiple PIDLs.
 */
BOOST_AUTO_TEST_CASE( EnumFormatsMulti )
{
    apidl_t root = create_dummy_root_pidl();
    cpidl_t pidl1 = create_dummy_remote_itemid(L"testswishfile.ext", false);
    cpidl_t pidl2 = create_dummy_remote_itemid(L"testswishfile.txt", false);
    cpidl_t pidl3 = create_dummy_remote_itemid(L"testswishFile", false);

    PCITEMID_CHILD aPidl[3];
    aPidl[0] = pidl1.get();
    aPidl[1] = pidl2.get();
    aPidl[2] = pidl3.get();

    com_ptr<IDataObject> data_object = new CDataObject(3, aPidl, root.get());

    // Test enumerators of both GetData() and SetData() formats
    _testBothCDataObjectEnumerators(data_object);
}

} // namespace test

BOOST_AUTO_TEST_SUITE_END()
