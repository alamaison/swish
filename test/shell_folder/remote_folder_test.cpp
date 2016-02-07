// Copyright 2011, 2016 Alexander Lamaison

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "swish/shell_folder/RemoteFolder.h" // test subject

#include "swish/remote_folder/remote_pidl.hpp" // remote_itemid_view

#include "test/common_boost/helpers.hpp"  // BOOST_REQUIRE_OK
#include "test/common_boost/fixtures.hpp" // ComFixture
#include "test/fixtures/provider_fixture.hpp"

#include <washer/shell/pidl.hpp>  // apidl_t
#include <washer/shell/shell.hpp> // strret_to_string

#include <comet/datetime.h>      // datetime_t
#include <comet/enum_iterator.h> // enum_iterator
#include <comet/error.h>         // com_error
#include <comet/ptr.h>           // com_ptr

#include <boost/bind.hpp>         // bind
#include <boost/lexical_cast.hpp> // lexical_cast
#include <boost/test/unit_test.hpp>

#include <algorithm> // find_if

using test::fixtures::provider_fixture;

using swish::remote_folder::remote_itemid_view;
using swish::utils::Utf8StringToWideString;

using washer::shell::pidl::apidl_t;
using washer::shell::pidl::cpidl_t;
using washer::shell::strret_to_string;

using comet::com_error;
using comet::com_ptr;
using comet::datetime_t;
using comet::enum_iterator;

using ssh::filesystem::path;

using boost::lexical_cast;
using boost::test_tools::predicate_result;

using std::find_if;
using std::wstring;

namespace comet
{

template <>
struct comtype<IEnumIDList>
{
    static const IID& uuid() throw()
    {
        return IID_IEnumIDList;
    }
    typedef IUnknown base;
};

template <>
struct enumerated_type_of<IEnumIDList>
{
    typedef PITEMID_CHILD is;
};

/**
 * Copy-policy for use by enumerators of child PIDLs.
 */
template <>
struct impl::type_policy<PITEMID_CHILD>
{
    static void init(PITEMID_CHILD& t, const cpidl_t& s)
    {
        s.copy_to(t);
    }

    static void clear(PITEMID_CHILD& t)
    {
        ::ILFree(t);
    }
};
}

namespace
{ // private

class RemoteFolderFixture : public provider_fixture, public test::ComFixture
{
private:
    com_ptr<IShellFolder> m_folder;

public:
    RemoteFolderFixture()
        : m_folder(CRemoteFolder::Create(
              sandbox_pidl().get(),
              boost::bind(&RemoteFolderFixture::consumer_factory, this, _1)))
    {
    }

    com_ptr<IShellFolder> folder() const
    {
        return m_folder;
    }

    comet::com_ptr<ISftpConsumer> consumer_factory(HWND)
    {
        return Consumer();
    }
};

void test_enum(com_ptr<IEnumIDList> pidls, SHCONTF flags)
{
    PITEMID_CHILD pidl;
    ULONG fetched;
    HRESULT hr = pidls->Next(1, &pidl, &fetched);
    BOOST_REQUIRE_OK(hr);
    BOOST_CHECK_EQUAL(fetched, 1U);

    do
    {
        remote_itemid_view itemid(pidl);

        // Check REMOTEPIDLness
        BOOST_REQUIRE(itemid.valid());

        // Check filename
        BOOST_CHECK_GT(itemid.filename().size(), 0U);
        if (!(flags & SHCONTF_INCLUDEHIDDEN))
            BOOST_CHECK_NE(itemid.filename(), L".");

        // Check folderness
        if (!(flags & SHCONTF_FOLDERS))
            BOOST_CHECK(!itemid.is_folder());
        if (!(flags & SHCONTF_NONFOLDERS))
            BOOST_CHECK(itemid.is_folder());

        // Check group and owner exist
        BOOST_CHECK_GT(itemid.owner().size(), 0U);
        BOOST_CHECK_GT(itemid.group().size(), 0U);

        // Check date validity
        BOOST_CHECK(itemid.date_modified().good());

        hr = pidls->Next(1, &pidl, &fetched);
    } while (hr == S_OK);

    BOOST_CHECK_EQUAL(hr, S_FALSE);
    BOOST_CHECK_EQUAL(fetched, 0U);
}

void test_enum(ATL::CComPtr<IEnumIDList> pidls, SHCONTF flags)
{
    test_enum(com_ptr<IEnumIDList>(pidls.p), flags);
}
}

BOOST_FIXTURE_TEST_SUITE(remote_folder_tests, RemoteFolderFixture)

/**
 * When a remote directory is empty, the remote folder's enumerator must
 * be empty.
 */
BOOST_AUTO_TEST_CASE(enum_empty)
{
    SHCONTF flags =
        SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN;

    com_ptr<IEnumIDList> listing;
    HRESULT hr = folder()->EnumObjects(NULL, flags, listing.out());
    BOOST_REQUIRE_OK(hr);

    ULONG fetched = 1;
    PITEMID_CHILD pidl;
    BOOST_CHECK_EQUAL(listing->Next(1, &pidl, &fetched), S_FALSE);
    BOOST_CHECK_EQUAL(fetched, 0U);
}

/**
 * Requesting everything should return folder and dotted files as well.
 */
BOOST_AUTO_TEST_CASE(enum_everything)
{
    path file1 = new_file_in_sandbox();
    path file2 = new_file_in_sandbox();
    path folder1 = sandbox() / L"folder1";
    create_directory(filesystem(), folder1);
    path folder2 = sandbox() / L"folder2";
    create_directory(filesystem(), folder2);

    SHCONTF flags =
        SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN;

    com_ptr<IEnumIDList> listing;
    HRESULT hr = folder()->EnumObjects(NULL, flags, listing.out());
    BOOST_REQUIRE_OK(hr);

    test_enum(listing, flags);
}

namespace
{

bool pidl_matches_filename(PCUITEMID_CHILD remote_pidl, wstring name)
{
    remote_itemid_view item(remote_pidl);
    return item.filename() == name;
}

cpidl_t pidl_for_file(com_ptr<IShellFolder> folder, wstring name)
{
    SHCONTF flags =
        SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN;

    com_ptr<IEnumIDList> listing;
    HRESULT hr = folder->EnumObjects(NULL, flags, listing.out());
    BOOST_REQUIRE_OK(hr);

    enum_iterator<IEnumIDList> pos = std::find_if(
        enum_iterator<IEnumIDList>(listing), enum_iterator<IEnumIDList>(),
        bind(pidl_matches_filename, _1, name));
    BOOST_REQUIRE_MESSAGE(pos != enum_iterator<IEnumIDList>(),
                          "PIDL not found");

    return *pos;
}

predicate_result display_name_matches(com_ptr<IShellFolder> folder,
                                      SHGDNF flags, const path& filename,
                                      const wstring& expected_display_name)
{
    cpidl_t pidl = pidl_for_file(folder, filename.wstring());

    STRRET strret;
    HRESULT hr = folder->GetDisplayNameOf(pidl.get(), flags, &strret);
    BOOST_REQUIRE_OK(hr);

    wstring display_name = strret_to_string<wchar_t>(strret);
    if (display_name != expected_display_name)
    {
        predicate_result res(false);
        res.message() << L"Display name for '" << filename << L"' unexpected: ["
                      << display_name << L" != " << expected_display_name
                      << L"]";
        return res;
    }

    return true;
}
}

/**
 * Request the display name for a file.
 *
 * This is the name of the file in a form suitable displaying to the user
 * anywhere in Windows and therefore may need disambiguation information
 * included.  For example 'filename on host' rather than just 'filename'.
 *
 * The result may or may not include the extension depending on the user's
 * settings, so we accept either as a successful result.
 *
 * Currently we don't support disambiguation information in Swish.
 *
 * This name does not have to be parseable.
 */
BOOST_AUTO_TEST_CASE(display_name_file)
{
    path file = new_file_in_sandbox(L"testfile.txt");

    SHGDNF flags = SHGDN_NORMAL;
    wstring expected_with = L"testfile.txt";
    wstring expected_without = L"testfile";

    BOOST_CHECK(
        display_name_matches(folder(), flags, file.filename(), expected_with) ||
        display_name_matches(folder(), flags, file.filename(),
                             expected_without));
}

/**
 * Request the display name for a Unix 'hidden' file.
 *
 * On Unix files are considered to be hidden if they start with a full-stop.
 * We adhere to this convention and should not treat an initial dot dot as part
 * of the extension.
 *
 * The result may or may not include the extension depending on the user's
 * settings, so we accept either as a successful result.
 */
BOOST_AUTO_TEST_CASE(display_name_hidden_file)
{
    path file1 = new_file_in_sandbox(L".hidden");
    path file2 = new_file_in_sandbox(L".testfile.txt");

    SHGDNF flags = SHGDN_NORMAL;
    wstring expected1 = L".hidden";
    wstring expected2_with = L".testfile.txt";
    wstring expected2_without = L".testfile";

    BOOST_CHECK(
        display_name_matches(folder(), flags, file1.filename(), expected1));
    BOOST_CHECK(display_name_matches(folder(), flags, file2.filename(),
                                     expected2_with) ||
                display_name_matches(folder(), flags, file2.filename(),
                                     expected2_without));
}

/**
 * Request the editing name for a file as though it were being edited elsewhere
 * than within its parent folder view.
 * I'm not sure how this situation would work but I don't think it matters for
 * us so we just return the usual editing name.
 */
BOOST_AUTO_TEST_CASE(editing_name_file)
{
    path file = new_file_in_sandbox(L"testfile.txt");

    SHGDNF flags = SHGDN_NORMAL | SHGDN_FOREDITING;
    wstring expected = L"testfile.txt";

    BOOST_CHECK(
        display_name_matches(folder(), flags, file.filename(), expected));
}

/**
 * Request the name for a file as though it were shown in the address bar
 * somewhere that isn't necessarily the parent folder.
 */
BOOST_AUTO_TEST_CASE(address_bar_name_file)
{
    BOOST_WARN_MESSAGE(false,
                       "skipping - testing full address bar requires "
                       "registration and knowledge of the parent host folder");
    return; // Leaving code here in case we find a way round this

    path file = new_file_in_sandbox(L"testfile.txt");

    SHGDNF flags = SHGDN_NORMAL | SHGDN_FORADDRESSBAR;
    wstring expected = L"sftp://" + wuser() + L"@" + whost() + L":" +
                       lexical_cast<wstring>(port()) + L"/" + file.wstring();

    BOOST_CHECK(
        display_name_matches(folder(), flags, file.filename(), expected));
}

/**
 * Check the display name for a file as it should be shown in a listing
 * of its containing folder.
 * In particular, this doesn't need disambiguation information that relates
 * to the folder it is in as this name is only used within the parent folder.
 *
 * The result may or may not include the extension depending on the user's
 * settings, so we accept either as a successful result.
 *
 * This name does not have to be parseable.
 */
BOOST_AUTO_TEST_CASE(in_folder_display_name_file)
{
    path file = new_file_in_sandbox(L"testfile.txt");

    SHGDNF flags = SHGDN_INFOLDER;
    wstring expected_with = L"testfile.txt";
    wstring expected_without = L"testfile";

    BOOST_CHECK(
        display_name_matches(folder(), flags, file.filename(), expected_with) ||
        display_name_matches(folder(), flags, file.filename(),
                             expected_without));
}

/**
 * Check the display name for a file of unregistered type as it should be
 * shown in a listing of its containing folder.
 * In particular, this doesn't need disambiguation information that relates
 * to the folder it is in as this name is only used within the parent folder.
 *
 * This test differs from in_folder_display_name_file in that the file
 * extension is of an unregistered type.  These should always show the
 * extension.
 *
 * This name does not have to be parseable.
 */
BOOST_AUTO_TEST_CASE(in_folder_display_name_unknown_file)
{
    // May fail if .xyz is actually a registered type
    path file = new_file_in_sandbox(L"testfile.xyz");

    SHGDNF flags = SHGDN_INFOLDER;
    wstring expected = L"testfile.xyz";

    BOOST_CHECK(
        display_name_matches(folder(), flags, file.filename(), expected));
}

/**
 * Check the parsing name of a file relative to its containing folder.
 * In other words, return the name of the file in such a form that it can
 * be uniquely identified given that we know the folder it is in.
 * Effectively, this means return the filename with its extension but any
 * decorative text that isn't part of its real name should be removed.
 *
 * Our files over SFTP don't have any decorative text but we do have to deal
 * with the extension.
 *
 * The FORPARSING flag forces the file extension to be included, regardless
 * of any user setting.
 */
BOOST_AUTO_TEST_CASE(in_folder_parsing_name_file)
{
    path file = new_file_in_sandbox(L"testfile.txt");

    SHGDNF flags = SHGDN_INFOLDER | SHGDN_FORPARSING;
    wstring expected = L"testfile.txt";

    BOOST_CHECK(
        display_name_matches(folder(), flags, file.filename(), expected));
}

/**
 * Request the editing name for a file as though it were being renamed in-place.
 * Normally in Windows this is different from the in-folder parsing name in that
 * it wouldn't include the extension but we tweak this slightly so that
 * renaming a file shows the extension even if that isn't the default user
 * setting.
 */
BOOST_AUTO_TEST_CASE(in_folder_editing_name_file)
{
    path file = new_file_in_sandbox(L"testfile.txt");

    SHGDNF flags = SHGDN_INFOLDER | SHGDN_FOREDITING;
    wstring expected = L"testfile.txt";

    BOOST_CHECK(
        display_name_matches(folder(), flags, file.filename(), expected));
}

// NORMAL + FORPARSING = ABSOLUTE
//
// ... or so it would seem

/**
 * Request the absolute name of a file as shown in the address bar.
 *
 * This should be a 'pretty' version of the name rather than the
 * truly parseable version that includes GUIDs etc.
 */
BOOST_AUTO_TEST_CASE(absolute_address_bar_parsing_name_file)
{
    BOOST_WARN_MESSAGE(false,
                       "skipping - testing absolute parsing name requires "
                       "registration and knowledge of the parent");
    return; // Leaving code here in case we find a way round this

    path file = new_file_in_sandbox(L"testfile.txt");

    SHGDNF flags = SHGDN_NORMAL | SHGDN_FORADDRESSBAR | SHGDN_FORPARSING;
    wstring expected = L"Computer\\Swish\\sftp://" + wuser() + L"@" + whost() +
                       L":" + lexical_cast<wstring>(port()) + L"/" +
                       file.wstring();

    BOOST_CHECK(
        display_name_matches(folder(), flags, file.filename(), expected));
}

/**
 * Request the absolute parsing name for a file.
 *
 * It must be possible to pass this to the @b desktop folder's ParseDisplayName
 * and get back a pidl for this item.
 */
BOOST_AUTO_TEST_CASE(absolute_parsing_name_file)
{
    BOOST_WARN_MESSAGE(false,
                       "skipping - testing absolute parsing name requires "
                       "registration and knowledge of the parent");
    return; // Leaving code here in case we find a way round this

    path file = new_file_in_sandbox(L"testfile.txt");

    SHGDNF flags = SHGDN_NORMAL | SHGDN_FORPARSING;
    wstring expected = L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\"
                       L"::{B816A83A-5022-11DC-9153-0090F5284F85}\\sftp://" +
                       wuser() + L"@" + whost() + L":" +
                       lexical_cast<wstring>(port()) + L"/" + file.wstring();

    BOOST_CHECK(
        display_name_matches(folder(), flags, file.filename(), expected));
}

/**
 * Request the display name for a folder.
 *
 * This is the name of the file in a form suitable displaying to the user
 * anywhere in Windows and therefore may need disambiguation information
 * included.  For example 'folder on host' rather than just 'folder'.
 *
 * Currently we don't support disambiguation information in Swish.
 *
 * This name does not have to be parseable.
 */
BOOST_AUTO_TEST_CASE(display_name_folder)
{
    path directory = sandbox() / L"testfolder";
    create_directory(filesystem(), directory);

    SHGDNF flags = SHGDN_NORMAL;
    wstring expected = L"testfolder";

    BOOST_CHECK(
        display_name_matches(folder(), flags, directory.filename(), expected));
}

/**
 * Request the display name for a folder within its parent folder view.
 *
 * This name does not have to be parseable.
 */
BOOST_AUTO_TEST_CASE(in_folder_name_folder)
{
    path directory = sandbox() / L"testfolder";
    create_directory(filesystem(), directory);

    SHGDNF flags = SHGDN_INFOLDER;
    wstring expected = L"testfolder";

    BOOST_CHECK(
        display_name_matches(folder(), flags, directory.filename(), expected));
}

/**
 * Request the display name for a folder that looks like it has an extension.
 *
 * Dots in a folder don't really indicate an extension so we should return
 * the whole thing.
 */
BOOST_AUTO_TEST_CASE(display_name_folder_with_extension)
{
    path directory = sandbox() / L"testfolder.txt";
    create_directory(filesystem(), directory);

    SHGDNF flags = SHGDN_NORMAL;
    wstring expected = L"testfolder.txt";

    BOOST_CHECK(
        display_name_matches(folder(), flags, directory.filename(), expected));
}

/**
 * Request the display name for a folder that looks like it has an extension
 * in a form for use within its parent folder view.
 *
 * Dots in a folder don't really indicate an extension so we should return
 * the whole thing.
 */
BOOST_AUTO_TEST_CASE(in_folder_name_folder_with_extension)
{
    path directory = sandbox() / L"testfolder.txt";
    create_directory(filesystem(), directory);

    SHGDNF flags = SHGDN_INFOLDER;
    wstring expected = L"testfolder.txt";

    BOOST_CHECK(
        display_name_matches(folder(), flags, directory.filename(), expected));
}

/**
 * Request the display name for a Unix 'hidden' directory.
 *
 * On Unix files are considered to be hidden if they start with a full-stop.
 * Although we shouldn't treat any part of a folder name as an extension, we
 * test the initial-dot case here specially just to make sure.
 */
BOOST_AUTO_TEST_CASE(display_name_hidden_folder)
{
    path dir1 = sandbox() / L".hidden";
    create_directory(filesystem(), dir1);
    path dir2 = sandbox() / L".testfolder.txt";
    create_directory(filesystem(), dir2);

    SHGDNF flags = SHGDN_NORMAL;
    wstring expected1 = L".hidden";
    wstring expected2 = L".testfolder.txt";

    BOOST_CHECK(
        display_name_matches(folder(), flags, dir1.filename(), expected1));
    BOOST_CHECK(
        display_name_matches(folder(), flags, dir2.filename(), expected2));
}

BOOST_AUTO_TEST_SUITE_END()
