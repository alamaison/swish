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
#include "test/common_boost/MockConsumer.hpp"
#include "test/common_boost/MockProvider.hpp"
#include "test/common_boost/SwishPidlFixture.hpp"
#include "exercise_data_object.h"

#include "swish/host_folder/host_pidl.hpp" // create_host_itemid
#include "swish/remote_folder/remote_pidl.hpp" // remote_itemid_view
#include "swish/shell_folder/SftpDataObject.h"

#include <winapi/shell/pidl.hpp> // cpidl_t, apidl_t

#include <comet/bstr.h> // bstr_t
#include <comet/ptr.h> // com_ptr

#include <boost/test/unit_test.hpp>

#include <vector>

using swish::host_folder::create_host_itemid;
using swish::remote_folder::remote_itemid_view;

using winapi::shell::pidl::apidl_t;
using winapi::shell::pidl::cpidl_t;

using comet::bstr_t;
using comet::com_ptr;

using std::string;
using std::vector;
using std::wstring;

namespace comet {

template<> struct comtype<IDataObject>
{
	static const IID& uuid() { return IID_IDataObject; }
	typedef ::IUnknown base;
};

}

namespace test {

namespace {

class TestFixture : public ComFixture, public SwishPidlFixture
{
public:
    TestFixture()
    {
        // Create mock object coclass instances
        m_pConsumer = com_ptr<ISftpConsumer>(new MockConsumer());
        m_pProvider = com_ptr<ISftpProvider>(new MockProvider());
    }

protected:

    com_ptr<ISftpConsumer> m_pConsumer;
    com_ptr<ISftpProvider> m_pProvider;
};

}

// HACK:
// A lot of these tests rely on SwishPidlFixture creating a host PIDL with
// path `/tmp` and a remote root PIDL with path `swish`.

BOOST_FIXTURE_TEST_SUITE( sftp_data_object_tests, TestFixture )

BOOST_AUTO_TEST_CASE( Create )
{
    apidl_t root = create_dummy_root_pidl();
    cpidl_t pidl = create_dummy_remote_itemid(L"testswishfile.ext", false);

    PCUITEMID_CHILD pidl_array[] = { pidl.get() };

    com_ptr<IDataObject> data_object = 
        new CSftpDataObject(
            1, pidl_array, root.get(), m_pProvider, m_pConsumer);

    // Test CFSTR_SHELLIDLIST (PIDL array) format
    cpidl_t root_child = root.last_item();
    remote_itemid_view folder(root_child);
    _testShellPIDLFolder(data_object, folder.filename());
    _testShellPIDL(data_object, remote_itemid_view(pidl).filename(), 0);

    // Test CFSTR_FILEDESCRIPTOR (FILEGROUPDESCRIPTOR) format
    _testFileDescriptor(data_object, L"testswishfile.ext", 0);

    // Test CFSTR_FILECONTENTS (IStream) format
    _testStreamContents(data_object, L"/tmp/swish/testswishfile.ext", 0);
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
        new CSftpDataObject(3, aPidl, root.get(), m_pProvider, m_pConsumer);

    // Test CFSTR_SHELLIDLIST (PIDL array) format
    cpidl_t root_child = root.last_item();
    remote_itemid_view folder(root_child);
    _testShellPIDLFolder(data_object, folder.filename());
    _testShellPIDL(data_object, remote_itemid_view(pidl1).filename(), 0);
    _testShellPIDL(data_object, remote_itemid_view(pidl2).filename(), 1);
    _testShellPIDL(data_object, remote_itemid_view(pidl3).filename(), 2);

    // Test CFSTR_FILEDESCRIPTOR (FILEGROUPDESCRIPTOR) format
    _testFileDescriptor(data_object, L"testswishfile.ext", 0);
    _testFileDescriptor(data_object, L"testswishfile.txt", 1);
    _testFileDescriptor(data_object, L"testswishFile", 2);

    // Test CFSTR_FILECONTENTS (IStream) format
    _testStreamContents(data_object, L"/tmp/swish/testswishfile.ext", 0);
    _testStreamContents(data_object, L"/tmp/swish/testswishfile.txt", 1);
    _testStreamContents(data_object, L"/tmp/swish/testswishFile", 2);
}

/**
 * Test that QueryGetData fails for all our formats when created with
 * empty PIDL list.
 */
BOOST_AUTO_TEST_CASE( QueryFormatsEmpty )
{
    com_ptr<IDataObject> data_object = new CSftpDataObject(
        0, NULL, NULL, m_pProvider, m_pConsumer);

    // Perform query tests
    _testQueryFormats(data_object, true);
}

/**
 * Test that none of our expected formats are in the enumerator when 
 * created with empty PIDL list.
 */
BOOST_AUTO_TEST_CASE( EnumFormatsEmpty )
{
    com_ptr<IDataObject> data_object = new CSftpDataObject(
        0, NULL, NULL, m_pProvider, m_pConsumer);

    // Test enumerators of both GetData() and SetData() formats
    _testBothEnumerators(data_object, true);
}

/**
 * Test that QueryGetData responds successfully for all our formats.
 */
BOOST_AUTO_TEST_CASE( QueryFormats )
{
    apidl_t root = create_dummy_root_pidl();
    cpidl_t pidl = create_dummy_remote_itemid(L"testswishfile.ext", false);

    PCUITEMID_CHILD pidl_array[] = { pidl.get() };

    com_ptr<IDataObject> data_object = 
        new CSftpDataObject(
            1, pidl_array, root.get(), m_pProvider, m_pConsumer);

    // Perform query tests
    _testQueryFormats(data_object);
}

/**
 * Test that all our expected formats are in the enumeration.
 */
BOOST_AUTO_TEST_CASE( EnumFormats )
{
    apidl_t root = create_dummy_root_pidl();
    cpidl_t pidl = create_dummy_remote_itemid(L"testswishfile.ext", false);

    PCUITEMID_CHILD pidl_array[] = { pidl.get() };

    com_ptr<IDataObject> data_object = 
        new CSftpDataObject(
            1, pidl_array, root.get(), m_pProvider, m_pConsumer);

    // Test enumerators of both GetData() and SetData() formats
    _testBothEnumerators(data_object);
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

    com_ptr<IDataObject> data_object =
        new CSftpDataObject(
            3, aPidl, root.get(), m_pProvider, m_pConsumer);

    // Perform query tests
    _testQueryFormats(data_object);
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

    com_ptr<IDataObject> data_object =
        new CSftpDataObject(
            3, aPidl, root.get(), m_pProvider, m_pConsumer);

    // Test enumerators of both GetData() and SetData() formats
    _testBothEnumerators(data_object);
}

BOOST_AUTO_TEST_CASE( FullDirectoryTree )
{
    // This has to start at / rather than /tmp
    apidl_t root = swish_pidl() + create_host_itemid(
        L"test.example.com", L"user", L"/", 22, L"Test PIDL");

    cpidl_t pidl = create_dummy_remote_itemid(L"tmp", true);

    PCUITEMID_CHILD pidl_array[] = { pidl.get() };

    com_ptr<IDataObject> data_object =
        new CSftpDataObject(
            1, pidl_array, root.get(), m_pProvider, m_pConsumer);

    // Test CFSTR_SHELLIDLIST (PIDL array) format.
    _testShellPIDLFolder(data_object, L"/");
    _testShellPIDLCount(data_object, 1);
    _testShellPIDL(data_object, L"tmp", 0);

    // Build list of paths in entire expected hierarchy

    // HACK:
    // These paths depend on the paths generated in the MockProvider. Any
    // slight change there kills this test
    vector<wstring> testfiles;
    testfiles.push_back(L"tmp");
    testfiles.push_back(L"tmp/.qtmp");
    testfiles.push_back(L"tmp/.testtmphiddenfile");
    testfiles.push_back(L"tmp/.testtmphiddenfolder");
    testfiles.push_back(L"tmp/another linktmpfolder");
    testfiles.push_back(L"tmp/linktmpfolder");
    testfiles.push_back(L"tmp/ptmp");
    testfiles.push_back(L"tmp/swish");
    testfiles.push_back(L"tmp/swish/.qswish");
    testfiles.push_back(L"tmp/swish/.testswishhiddenfile");
    testfiles.push_back(L"tmp/swish/.testswishhiddenfolder");
    testfiles.push_back(L"tmp/swish/another linkswishfolder");
    testfiles.push_back(L"tmp/swish/linkswishfolder");
    testfiles.push_back(L"tmp/swish/pswish");
    testfiles.push_back(L"tmp/swish/testswishfile");
    testfiles.push_back(L"tmp/swish/testswishFile");
    testfiles.push_back(
        L"tmp/swish/testswishfile with \"quotes\" and spaces");
    testfiles.push_back(L"tmp/swish/testswishfile with spaces");
    testfiles.push_back(L"tmp/swish/testswishfile..");
    testfiles.push_back(L"tmp/swish/testswishfile.ext");
    testfiles.push_back(L"tmp/swish/testswishfile.ext.txt");
    testfiles.push_back(L"tmp/swish/testswishfile.txt");
    testfiles.push_back(L"tmp/swish/Testswishfolder");
    testfiles.push_back(L"tmp/swish/testswishfolder with spaces");
    testfiles.push_back(L"tmp/swish/testswishfolder.bmp");
    testfiles.push_back(L"tmp/swish/testswishfolder.ext");
    testfiles.push_back(L"tmp/swish/this_link_is_broken_swish");
    testfiles.push_back(L"tmp/testtmpfile");
    testfiles.push_back(L"tmp/testtmpFile");
    testfiles.push_back(L"tmp/testtmpfile with \"quotes\" and spaces");
    testfiles.push_back(L"tmp/testtmpfile with spaces");
    testfiles.push_back(L"tmp/testtmpfile..");
    testfiles.push_back(L"tmp/testtmpfile.ext");
    testfiles.push_back(L"tmp/testtmpfile.ext.txt");
    testfiles.push_back(L"tmp/testtmpfile.txt");
    testfiles.push_back(L"tmp/Testtmpfolder");
    testfiles.push_back(L"tmp/testtmpfolder with spaces");
    testfiles.push_back(L"tmp/testtmpfolder.bmp");
    testfiles.push_back(L"tmp/testtmpfolder.ext");
    testfiles.push_back(L"tmp/this_link_is_broken_tmp");

    // Test CFSTR_FILEDESCRIPTOR (FILEGROUPDESCRIPTOR) format.  The 
    // descriptor should include every item in the entire hierarchy 
    // generated by CMockSftpProvider.
    for (UINT i = 0; i < testfiles.size(); ++i)
    {
        _testFileDescriptor(data_object, testfiles[i].c_str(), i);
    }

    // Test CFSTR_FILECONTENTS (IStream) format.  The dummy streams should
    // contain the absolute path to the file as a string
    for (UINT i = 0; i < testfiles.size(); ++i)
    {
        _testStreamContents(data_object, wstring(L"/") + testfiles[i], i);
    }
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace test
