// Copyright 2009, 2010, 2012, 2013, 2016 Alexander Lamaison

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

#include "swish/drop_target/DropTarget.hpp" // Test subject
#include "swish/shell/shell.hpp"            // data_object_for_files

#include "test/common_boost/data_object_utils.hpp" // DataObjects on zip
#include "test/common_boost/helpers.hpp"           // BOOST_REQUIRE_OK
#include "test/common_boost/fixtures.hpp"          // ComFixture
#include "test/fixtures/local_sandbox_fixture.hpp"
#include "test/fixtures/provider_fixture.hpp"

#include <ssh/stream.hpp>
#include <ssh/filesystem.hpp>

#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include <shlobj.h>

#include <string>
#include <vector>
#include <iterator>
#include <algorithm>

using swish::drop_target::CDropTarget;
using swish::drop_target::DropActionCallback;
using swish::drop_target::copy_data_to_provider;
using swish::drop_target::Progress;
using swish::shell::data_object_for_files;

using test::ComFixture;
using test::fixtures::provider_fixture;
using test::fixtures::local_sandbox_fixture;
using test::data_object_utils::create_test_zip_file;
using test::data_object_utils::data_object_for_zipfile;

using comet::com_ptr;

using ssh::filesystem::ifstream;
using ssh::filesystem::sftp_filesystem;

using boost::filesystem::ofstream;
using boost::filesystem::path;
using boost::make_shared;
using boost::shared_ptr;
using boost::test_tools::predicate_result;

using std::string;
using std::wstring;
using std::vector;
using std::istreambuf_iterator;

using washer::shell::pidl::apidl_t;

namespace
{ // private

const string TEST_DATA = "Lorem ipsum dolor sit amet.\nbob\r\nsally";
const string LARGER_TEST_DATA =
    ";sdkfna;sldjnksj fjnweneofiun weof woenf woeunr2938y4192n34kj1458c"
    "d;ofn3498tv 3405jnv 3498thv-948rc 34f 9485hv94htc rwr98thv3948h534h4";

/**
 * The test data which will be written and read from files to
 * check correct transmission.
 */
string test_data()
{
    return TEST_DATA;
}

/**
 * Write some data to a collection of local files and return them in
 * a DataObject created by the shell.
 *
 * The files must all be in the same filesystem folder.
 */
template <typename It>
com_ptr<IDataObject> create_multifile_data_object(It begin, It end)
{
    std::for_each(begin, end, fill_file);
    return data_object_for_files(begin, end);
}

/**
 * Write some data to a local file and return it as a DataObject.
 */
com_ptr<IDataObject> create_data_object(const path& local)
{
    return create_multifile_data_object(&local, &local + 1);
}

/**
 * Fill a file with the test data.
 */
void fill_file(const path& file)
{
    ofstream stream(file, std::ios_base::binary);
    stream.write(test_data().c_str(), test_data().size());
}

/**
 * Create a new empty file at the given path.
 */
void create_empty_file(path name)
{
    ofstream file(name, std::ios_base::out | std::ios_base::trunc);
}

class ProgressStub : public Progress
{
public:
    bool user_cancelled()
    {
        return false;
    }
    void line(DWORD, const wstring&)
    {
    }
    void line_path(DWORD, const wstring&)
    {
    }
    void update(ULONGLONG, ULONGLONG)
    {
    }
    void hide()
    {
    }
    void show()
    {
    }
};

class CopyCallbackStub : public DropActionCallback
{
public:
    void site(com_ptr<IUnknown>)
    {
    }

    std::auto_ptr<Progress> progress()
    {
        return std::auto_ptr<Progress>(new ProgressStub());
    }

    bool can_overwrite(const ssh::filesystem::path&)
    {
        throw std::exception("unexpected request to confirm overwrite");
    }

    void handle_last_exception()
    {
    }
};

class ForbidOverwrite : public CopyCallbackStub
{
public:
    bool can_overwrite(const ssh::filesystem::path&)
    {
        return false;
    }
};

class AllowOverwrite : public CopyCallbackStub
{
public:
    bool can_overwrite(const ssh::filesystem::path&)
    {
        return true;
    }
};

class DropTargetFixture : public provider_fixture,
                          public ComFixture,
                          public local_sandbox_fixture
{
public:
    comet::com_ptr<IDropTarget> create_drop_target()
    {
        ssh::filesystem::path target_directory = sandbox() / L"drop-target";
        create_directory(filesystem(), target_directory);

        return new CDropTarget(Provider(), directory_pidl(target_directory),
                               make_shared<CopyCallbackStub>());
    }

    /**
     * Check if a file's contents is our test data.
     */
    predicate_result file_contents_correct(const ssh::filesystem::path& file)
    {
        ifstream stream(filesystem(), file);
        string contents = string(istreambuf_iterator<char>(stream),
                                 istreambuf_iterator<char>());

        if (contents.size() != test_data().size())
        {
            predicate_result res(false);
            res.message() << "File contents differs in length from expected ["
                          << contents.size() << " != " << test_data().size()
                          << "] [" << contents << " != " << test_data() << "]";
            return res;
        }
        else if (contents != test_data())
        {
            for (size_t i = 0; i < contents.size(); ++i)
            {
                if (contents[i] != test_data()[i])
                {
                    predicate_result res(false);
                    res.message() << "File contents differs at index " << i
                                  << " [" << contents[i]
                                  << " != " << test_data()[i] << "] ["
                                  << contents << " != " << test_data() << "]";
                    return res;
                }

                return true;
            }
        };
        return true;
    }
};
}

#pragma region SFTP folder Drop Target tests
BOOST_FIXTURE_TEST_SUITE(drop_target_tests, DropTargetFixture)

/**
 * Create an instance.
 */
BOOST_AUTO_TEST_CASE(create)
{
    com_ptr<IDropTarget> sp = create_drop_target();
    BOOST_REQUIRE(sp);
}

#pragma region DataObject copy tests
BOOST_FIXTURE_TEST_SUITE(drop_target_copy_tests, DropTargetFixture)

/**
 * Copy single regular file.
 *
 * Test our ability to handle a DataObject produced by the shell for a
 * single, regular file (real file in the filesystem).
 */
BOOST_AUTO_TEST_CASE(copy_single)
{
    path file = new_file_in_local_sandbox();

    com_ptr<IDataObject> spdo = create_data_object(file);

    ssh::filesystem::path destination = new_directory_in_sandbox();

    shared_ptr<CopyCallbackStub> cb(new CopyCallbackStub);
    copy_data_to_provider(spdo, Provider(), directory_pidl(destination), cb);

    ssh::filesystem::path expected = destination / file.filename().wstring();
    BOOST_REQUIRE(exists(filesystem(), expected));
    BOOST_CHECK_EQUAL(file_size(filesystem(), expected), test_data().size());
    BOOST_REQUIRE(file_contents_correct(expected));
}

/**
 * Copy several regular files.
 *
 * Test our ability to handle a DataObject produced by the shell for
 * more than one regular file (real files in the filesystem).
 */
BOOST_AUTO_TEST_CASE(copy_many)
{
    vector<path> locals;
    locals.push_back(new_file_in_local_sandbox());
    locals.push_back(new_file_in_local_sandbox());
    locals.push_back(new_file_in_local_sandbox());

    com_ptr<IDataObject> spdo =
        create_multifile_data_object(locals.begin(), locals.end());

    ssh::filesystem::path destination = new_directory_in_sandbox();

    shared_ptr<CopyCallbackStub> cb(new CopyCallbackStub);
    copy_data_to_provider(spdo, Provider(), directory_pidl(destination), cb);

    vector<path>::const_iterator it;
    for (it = locals.begin(); it != locals.end(); ++it)
    {
        ssh::filesystem::path expected =
            destination / (*it).filename().wstring();
        BOOST_REQUIRE(exists(filesystem(), expected));
        BOOST_REQUIRE(file_contents_correct(expected));
    }
}

/**
 * Recursively copy a folder hierarchy.
 *
 * Our test hierarchy look like this:
 * Sandbox - file0
 *         \ file1
 *         \ empty_folder
 *         \ non_empty_folder - second_level_file
 *                            \ second_level_folder - third_level_file
 *
 * We could just make a DataObject by passing the sandbox dir to the shell
 * function but instead we pass the four items directly within it to
 * test how we handle a mix of recursive dirs and simple files.
 */
BOOST_AUTO_TEST_CASE(copy_recursively)
{
    vector<path> top_level;

    // Build top-level - these are the only items stored in the vector
    top_level.push_back(new_file_in_local_sandbox());
    top_level.push_back(new_file_in_local_sandbox());

    path empty_folder = local_sandbox() / L"empty";
    path non_empty_folder = local_sandbox() / L"non-empty";
    create_directory(empty_folder);
    create_directory(non_empty_folder);
    top_level.push_back(empty_folder);
    top_level.push_back(non_empty_folder);

    // Build lower levels

    path second_level_folder = non_empty_folder / L"second-level-folder";
    create_directory(second_level_folder);

    path second_level_file = non_empty_folder / L"second-level-file";
    create_empty_file(second_level_file);
    fill_file(second_level_file);

    path second_level_zip_file = create_test_zip_file(non_empty_folder);

    path third_level_file = second_level_folder / L"third-level-file";
    create_empty_file(third_level_file);
    fill_file(third_level_file);

    com_ptr<IDataObject> spdo =
        create_multifile_data_object(top_level.begin(), top_level.end());

    ssh::filesystem::path destination = sandbox() / L"copy-destination";
    create_directory(filesystem(), destination);

    shared_ptr<CopyCallbackStub> cb(new CopyCallbackStub);
    copy_data_to_provider(spdo, Provider(), directory_pidl(destination), cb);

    ssh::filesystem::path expected;

    expected = destination / top_level[0].filename().wstring();
    BOOST_REQUIRE(exists(filesystem(), expected));
    BOOST_REQUIRE(file_contents_correct(expected));

    expected = destination / top_level[0].filename().wstring();
    BOOST_REQUIRE(exists(filesystem(), expected));
    BOOST_REQUIRE(file_contents_correct(expected));

    expected = destination / empty_folder.filename().wstring();
    BOOST_REQUIRE(exists(filesystem(), expected));
    BOOST_REQUIRE(is_directory(filesystem(), expected));
    BOOST_REQUIRE(is_empty(filesystem(), expected));

    expected = destination / non_empty_folder.filename().wstring();
    BOOST_REQUIRE(exists(filesystem(), expected));
    BOOST_REQUIRE(is_directory(filesystem(), expected));

    expected = destination / non_empty_folder.filename().wstring() /
               second_level_file.filename().wstring();
    BOOST_REQUIRE(exists(filesystem(), expected));
    BOOST_REQUIRE(file_contents_correct(expected));

    expected = destination / non_empty_folder.filename().wstring() /
               second_level_folder.filename().wstring();
    BOOST_REQUIRE(exists(filesystem(), expected));
    BOOST_REQUIRE(is_directory(filesystem(), expected));
    BOOST_REQUIRE(!is_empty(filesystem(), expected));

    // The zip file must be copied as-is, not expanded
    expected = destination / non_empty_folder.filename().wstring() /
               second_level_zip_file.filename().wstring();
    BOOST_REQUIRE(exists(filesystem(), expected));
    BOOST_REQUIRE(is_regular_file(filesystem(), expected));
    BOOST_REQUIRE_GT(file_size(filesystem(), expected), 800);

    expected = destination / non_empty_folder.filename().wstring() /
               second_level_folder.filename().wstring() /
               third_level_file.filename().wstring();
    BOOST_REQUIRE(exists(filesystem(), expected));
    BOOST_REQUIRE(file_contents_correct(expected));
}

/**
 * Recursively copy a virtual hierarchy from a ZIP file.
 *
 * Our test hierarchy look like this:
 * Sandbox - file1.txt
 *         \ file2.txt
 *         \ empty_folder
 *         \ non_empty_folder - second_level_file
 *                            \ second_level_folder - third_level_file
 *
 * We could just make a DataObject by passing the sandbox dir to the shell
 * function but instead we pass the four items directly within it to
 * test how we handle a mix of recursive dirs and simple files.
 */
BOOST_AUTO_TEST_CASE(copy_virtual_hierarchy_recursively)
{
    com_ptr<IDataObject> spdo =
        data_object_for_zipfile(create_test_zip_file(local_sandbox()));

    ssh::filesystem::path destination = sandbox() / L"copy-destination";
    create_directory(filesystem(), destination);

    shared_ptr<CopyCallbackStub> cb(new CopyCallbackStub);
    copy_data_to_provider(spdo, Provider(), directory_pidl(destination), cb);

    ssh::filesystem::path expected;

    expected = destination / L"file1.txt";
    BOOST_REQUIRE(exists(filesystem(), expected));

    expected = destination / L"file2.txt";
    BOOST_REQUIRE(exists(filesystem(), expected));

    expected = destination / L"empty";
    BOOST_REQUIRE(exists(filesystem(), expected));
    BOOST_REQUIRE(is_directory(filesystem(), expected));
    BOOST_REQUIRE(is_empty(filesystem(), expected));

    expected = destination / L"non-empty";
    BOOST_REQUIRE(exists(filesystem(), expected));
    BOOST_REQUIRE(is_directory(filesystem(), expected));

    expected = destination / L"non-empty" / L"second-level-file";
    BOOST_REQUIRE(exists(filesystem(), expected));

    expected = destination / L"non-empty" / L"second-level-folder";
    BOOST_REQUIRE(exists(filesystem(), expected));
    BOOST_REQUIRE(is_directory(filesystem(), expected));
    BOOST_REQUIRE(!is_empty(filesystem(), expected));

    expected = destination / L"non-empty" / L"second-level-folder" /
               L"third-level-file";
    BOOST_REQUIRE(exists(filesystem(), expected));
}

/**
 * Overwrite an existing file.
 *
 * Must ask the user to confirm.  This test and the test after together
 * ensure that the user's response makes a difference to the outcome and
 * thereby proves that the user was asked.
 */
BOOST_AUTO_TEST_CASE(copy_overwrite_yes)
{
    path file = new_file_in_local_sandbox();
    com_ptr<IDataObject> spdo = create_data_object(file);

    ssh::filesystem::path destination = sandbox() / L"copy-destination";
    ssh::filesystem::path obstruction = destination / file.filename().wstring();

    create_directory(filesystem(), destination);
    ssh::filesystem::ofstream(filesystem(), obstruction).close();

    BOOST_CHECK(exists(filesystem(), obstruction));
    BOOST_CHECK(!file_contents_correct(obstruction));

    shared_ptr<AllowOverwrite> cb(new AllowOverwrite);
    copy_data_to_provider(spdo, Provider(), directory_pidl(destination), cb);

    BOOST_CHECK(exists(filesystem(), obstruction));
    BOOST_CHECK(file_contents_correct(obstruction));
}

/**
 * Deny permission to overwrite an existing file.
 */
BOOST_AUTO_TEST_CASE(copy_overwrite_no)
{
    path file = new_file_in_local_sandbox();
    com_ptr<IDataObject> spdo = create_data_object(file);

    ssh::filesystem::path destination = sandbox() / L"copy-destination";
    ssh::filesystem::path obstruction = destination / file.filename().wstring();

    create_directory(filesystem(), destination);
    ssh::filesystem::ofstream(filesystem(), obstruction).close(); // empty

    BOOST_CHECK(exists(filesystem(), obstruction));
    BOOST_CHECK(!file_contents_correct(obstruction));

    shared_ptr<ForbidOverwrite> cb(new ForbidOverwrite);
    copy_data_to_provider(spdo, Provider(), directory_pidl(destination), cb);

    BOOST_CHECK(exists(filesystem(), obstruction));
    BOOST_CHECK_EQUAL(file_size(filesystem(), obstruction), 0); // still empty
}

/**
 * Overwrite a larger file.
 *
 * Tests that we truncate the large file before writing.  Otherwise the
 * final
 * file would be corrupt.
 */
BOOST_AUTO_TEST_CASE(copy_overwrite_larger)
{
    path target = new_file_in_local_sandbox();
    com_ptr<IDataObject> spdo = create_data_object(target);

    ssh::filesystem::path destination = sandbox() / L"copy-destination";
    ssh::filesystem::path obstruction =
        destination / target.filename().wstring();

    // make sure that the destination file already exists and is larger
    // that what we're about to copy to it
    create_directory(filesystem(), destination);
    ssh::filesystem::ofstream stream(filesystem(), obstruction);
    stream << LARGER_TEST_DATA;
    stream.close();

    BOOST_REQUIRE(exists(filesystem(), obstruction));
    BOOST_REQUIRE(!file_contents_correct(obstruction));

    shared_ptr<AllowOverwrite> cb(new AllowOverwrite);
    copy_data_to_provider(spdo, Provider(), directory_pidl(destination), cb);

    BOOST_REQUIRE(exists(filesystem(), obstruction));
    BOOST_REQUIRE(file_contents_correct(obstruction));
}

BOOST_AUTO_TEST_SUITE_END()
#pragma endregion

#pragma region Drag - n - Drop behaviour tests
BOOST_FIXTURE_TEST_SUITE(drop_target_dnd_tests, DropTargetFixture)

/**
 * Drag enter.
 * Simulate the user dragging a file onto our folder with the left
 * mouse button.  The 'shell' is requesting either a copy or a link at our
 * discretion.  The folder drop target should respond S_OK and specify
 * that the effect of the operation is a copy.
 */
BOOST_AUTO_TEST_CASE(drag_enter)
{
    path file = new_file_in_local_sandbox();
    com_ptr<IDataObject> spdo = create_data_object(file);
    com_ptr<IDropTarget> spdt = create_drop_target();

    POINTL pt = {0, 0};
    DWORD dwEffect = DROPEFFECT_COPY | DROPEFFECT_LINK;
    BOOST_REQUIRE_OK(spdt->DragEnter(spdo.in(), MK_LBUTTON, pt, &dwEffect));
    BOOST_REQUIRE_EQUAL(dwEffect, static_cast<DWORD>(DROPEFFECT_COPY));
}

/**
 * Drag enter.
 * Simulate the user dragging a file onto our folder but requesting an
 * effect, link, that we don't support.  The folder drop target should
 * respond S_OK but set the effect to DROPEFFECT_NONE to indicate the drop
 * wasn't possible.
 */
BOOST_AUTO_TEST_CASE(drag_enter_bad_effect)
{
    path file = new_file_in_local_sandbox();
    com_ptr<IDataObject> spdo = create_data_object(file);
    com_ptr<IDropTarget> spdt = create_drop_target();

    POINTL pt = {0, 0};
    DWORD dwEffect = DROPEFFECT_LINK;
    BOOST_REQUIRE_OK(spdt->DragEnter(spdo.in(), MK_LBUTTON, pt, &dwEffect));
    BOOST_REQUIRE_EQUAL(dwEffect, static_cast<DWORD>(DROPEFFECT_NONE));
}

/**
 * Drag over.
 * Simulate the situation where a user drags a file over our folder and
 * changes
 * which operation they want as they do so.  In other words, on DragEnter
 * they
 * chose a link which we cannot perform but as they continue the drag
 * (DragOver)
 * they chang their request to a copy which we can do.
 *
 * The folder drop target should respond S_OK and specify that the effect of
 * the operation is a copy.
 */
BOOST_AUTO_TEST_CASE(drag_over)
{
    path file = new_file_in_local_sandbox();
    com_ptr<IDataObject> spdo = create_data_object(file);
    com_ptr<IDropTarget> spdt = create_drop_target();

    POINTL pt = {0, 0};

    // Do enter with link which should be declined (DROPEFFECT_NONE)
    DWORD dwEffect = DROPEFFECT_LINK;
    BOOST_REQUIRE_OK(spdt->DragEnter(spdo.in(), MK_LBUTTON, pt, &dwEffect));
    BOOST_REQUIRE_EQUAL(dwEffect, static_cast<DWORD>(DROPEFFECT_NONE));

    // Change request to copy which should be accepted
    dwEffect = DROPEFFECT_COPY;
    BOOST_REQUIRE_OK(spdt->DragOver(MK_LBUTTON, pt, &dwEffect));
    BOOST_REQUIRE_EQUAL(dwEffect, static_cast<DWORD>(DROPEFFECT_COPY));
}

/**
 * Drag leave.
 * Simulate an aborted drag-drop loop where the user drags a file onto our
 * folder, moves it around, and then leaves without dropping.
 *
 * The folder drop target should respond S_OK and any subsequent
 * DragOver calls should be declined.
 */
BOOST_AUTO_TEST_CASE(drag_leave)
{
    path file = new_file_in_local_sandbox();
    com_ptr<IDataObject> spdo = create_data_object(file);
    com_ptr<IDropTarget> spdt = create_drop_target();

    POINTL pt = {0, 0};

    // Do enter with copy which sould be accepted
    DWORD dwEffect = DROPEFFECT_COPY;
    BOOST_REQUIRE_OK(spdt->DragEnter(spdo.in(), MK_LBUTTON, pt, &dwEffect));
    BOOST_REQUIRE_EQUAL(dwEffect, static_cast<DWORD>(DROPEFFECT_COPY));

    // Continue drag
    BOOST_REQUIRE_OK(spdt->DragOver(MK_LBUTTON, pt, &dwEffect));
    BOOST_REQUIRE_EQUAL(dwEffect, static_cast<DWORD>(DROPEFFECT_COPY));

    // Finish drag without dropping
    BOOST_REQUIRE_OK(spdt->DragLeave());

    // Decline any further queries until next DragEnter()
    BOOST_REQUIRE_OK(spdt->DragOver(MK_LBUTTON, pt, &dwEffect));
    BOOST_REQUIRE_EQUAL(dwEffect, static_cast<DWORD>(DROPEFFECT_NONE));
}

/**
 * Drag and drop.
 * Simulate a complete drag-drop loop where the user drags a file onto our
 * folder, moves it around, and then drops it.
 *
 * The folder drop target should copy the contents of the DataObject to the
 * remote end, respond S_OK and any subsequent DragOver calls should be
 * declined until a new drag-and-drop loop is started with DragEnter().
 */
BOOST_AUTO_TEST_CASE(drop)
{
    path file = new_file_in_local_sandbox();

    com_ptr<IDataObject> spdo = create_data_object(file);
    com_ptr<IDropTarget> spdt = create_drop_target();

    POINTL pt = {0, 0};

    // Do enter with copy which should be accepted
    DWORD dwEffect = DROPEFFECT_COPY;
    BOOST_REQUIRE_OK(spdt->DragEnter(spdo.in(), MK_LBUTTON, pt, &dwEffect));
    BOOST_REQUIRE_EQUAL(dwEffect, static_cast<DWORD>(DROPEFFECT_COPY));

    // Continue drag
    BOOST_REQUIRE_OK(spdt->DragOver(MK_LBUTTON, pt, &dwEffect));
    BOOST_REQUIRE_EQUAL(dwEffect, static_cast<DWORD>(DROPEFFECT_COPY));

    // Drop onto DropTarget
    BOOST_REQUIRE_OK(spdt->Drop(spdo.in(), MK_LBUTTON, pt, &dwEffect));
    BOOST_REQUIRE_EQUAL(dwEffect, static_cast<DWORD>(DROPEFFECT_COPY));

    // Decline any further queries until next DragEnter()
    BOOST_REQUIRE_OK(spdt->DragOver(MK_LBUTTON, pt, &dwEffect));
    BOOST_REQUIRE_EQUAL(dwEffect, static_cast<DWORD>(DROPEFFECT_NONE));

    ssh::filesystem::path expected =
        sandbox() / L"drop-target" / file.filename().wstring();
    BOOST_CHECK(exists(filesystem(), expected));
    BOOST_CHECK(file_contents_correct(expected));
}

BOOST_AUTO_TEST_SUITE_END()
#pragma endregion

BOOST_AUTO_TEST_SUITE_END()
#pragma endregion
