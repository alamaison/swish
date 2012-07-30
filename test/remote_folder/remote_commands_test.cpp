/**
    @file

    Unit tests command functors for remote folder.

    @if license

    Copyright (C) 2011, 2012  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "swish/remote_folder/commands/commands.hpp" // test subject
#include "swish/remote_folder/commands/NewFolder.hpp" // test subject

#include "test/common_boost/helpers.hpp" // BOOST_REQUIRE_OK
#include "test/common_boost/PidlFixture.hpp"  // PidlFixture

#include <boost/bind.hpp> // bind;
#include <boost/filesystem/path.hpp> // wpath, wdirectory_iterator
#include <boost/test/unit_test.hpp>

#include <string>

using swish::nse::IEnumUICommand;
using swish::nse::IUICommand;
using swish::remote_folder::commands::NewFolder;
using swish::remote_folder::commands::remote_folder_task_pane_tasks;

using test::PidlFixture;

using comet::com_ptr;

using boost::bind;
using boost::filesystem::wdirectory_iterator;
using boost::filesystem::wpath;

using std::wstring;

namespace { // private

    const wstring NEW_FOLDER = L"New folder";

    class NewFolderCommandFixture : public PidlFixture
    {
    public:
        NewFolder new_folder_command()
        {
            return NewFolder(
                sandbox_pidl(), bind(&NewFolderCommandFixture::Provider, this),
                bind(&NewFolderCommandFixture::Consumer, this));
        }
    };
}

template<> struct comet::comtype<IObjectWithSite>
{
    static const IID& uuid() throw() { return IID_IObjectWithSite; }
    typedef ::IUnknown base;
};

#pragma region NewFolder tests
BOOST_FIXTURE_TEST_SUITE(new_folder_tests, NewFolderCommandFixture)

/**
 * Test NewFolder command has correct properties that don't involve executing
 * the command.
 */
BOOST_AUTO_TEST_CASE( non_execution_properties )
{
    NewFolder command = new_folder_command();
    BOOST_CHECK(!command.guid().is_null());
    BOOST_CHECK(!command.title(NULL).empty());
    BOOST_CHECK(!command.tool_tip(NULL).empty());
    BOOST_CHECK(!command.icon_descriptor(NULL).empty());
    BOOST_CHECK(!command.disabled(NULL, true));
    BOOST_CHECK(!command.hidden(NULL, true));
}

/**
 * Test in empty directory that (inevitably) has no collisions.
 */
BOOST_AUTO_TEST_CASE( no_collision_empty )
{
    wpath expected = Sandbox() / NEW_FOLDER;

    NewFolder command = new_folder_command();
    command(NULL, NULL);

    BOOST_REQUIRE(is_directory(expected));

    BOOST_CHECK_EQUAL(
        distance(wdirectory_iterator(Sandbox()), wdirectory_iterator()), 1);
}

/**
 * Test in a directory that isn't empty but which doesn't have any collisions.
 */
BOOST_AUTO_TEST_CASE( no_collision )
{
    NewFileInSandbox();
    wpath expected = Sandbox() / NEW_FOLDER;

    NewFolder command = new_folder_command();
    command(NULL, NULL);

    BOOST_REQUIRE(is_directory(expected));

    BOOST_CHECK_EQUAL(
        distance(wdirectory_iterator(Sandbox()), wdirectory_iterator()), 2);
}

/**
 * Test in a directory that has existing "New folder".  Should create
  * "New folder (2)" instead.
 */
BOOST_AUTO_TEST_CASE( basic_collision )
{
    wpath collision = Sandbox() / NEW_FOLDER;
    wpath expected = Sandbox() / (NEW_FOLDER + L" (2)");

    BOOST_REQUIRE(create_directory(collision));

    NewFolder command = new_folder_command();
    command(NULL, NULL);

    BOOST_REQUIRE(is_directory(expected));
    BOOST_REQUIRE(is_directory(collision));

    BOOST_CHECK_EQUAL(
        distance(wdirectory_iterator(Sandbox()), wdirectory_iterator()), 2);
}

/**
 * Test in a directory that has existing "New folder (2)" but not "New folder".
 * We want to make sure that this doesn't prevent "New folder" being created.
 */
BOOST_AUTO_TEST_CASE( non_interfering_collision )
{
    wpath collision = Sandbox() / (NEW_FOLDER + L" (2)");
    wpath expected = Sandbox() / NEW_FOLDER;

    BOOST_REQUIRE(create_directory(collision));

    NewFolder command = new_folder_command();
    command(NULL, NULL);

    BOOST_REQUIRE(is_directory(expected));
    BOOST_REQUIRE(is_directory(collision));

    BOOST_CHECK_EQUAL(
        distance(wdirectory_iterator(Sandbox()), wdirectory_iterator()), 2);
}

/**
 * Test in a directory that has existing "New folder" and "New folder (2)".
 * Should create "New folder (3)" instead.
 */
BOOST_AUTO_TEST_CASE( multiple_collision )
{
    wpath collision1 = Sandbox() / NEW_FOLDER;
    wpath collision2 = Sandbox() / (NEW_FOLDER + L" (2)");
    wpath expected = Sandbox() / (NEW_FOLDER + L" (3)");

    BOOST_REQUIRE(create_directory(collision1));
    BOOST_REQUIRE(create_directory(collision2));

    NewFolder command = new_folder_command();
    command(NULL, NULL);

    BOOST_REQUIRE(is_directory(expected));
    BOOST_REQUIRE(is_directory(collision1));
    BOOST_REQUIRE(is_directory(collision2));

    BOOST_CHECK_EQUAL(
        distance(wdirectory_iterator(Sandbox()), wdirectory_iterator()), 3);
}

/**
 * Test in a directory that has existing "New folder" and "New folder (3)"
 * but not "New folder (2). Should create "New folder (2)" in the gap.
 */
BOOST_AUTO_TEST_CASE( non_contiguous_collision1 )
{
    wpath collision1 = Sandbox() / NEW_FOLDER;
    wpath collision2 = Sandbox() / (NEW_FOLDER + L" (3)");
    wpath expected = Sandbox() / (NEW_FOLDER + L" (2)");

    BOOST_REQUIRE(create_directory(collision1));
    BOOST_REQUIRE(create_directory(collision2));

    NewFolder command = new_folder_command();
    command(NULL, NULL);

    BOOST_REQUIRE(is_directory(expected));
    BOOST_REQUIRE(is_directory(collision1));
    BOOST_REQUIRE(is_directory(collision2));

    BOOST_CHECK_EQUAL(
        distance(wdirectory_iterator(Sandbox()), wdirectory_iterator()), 3);
}

/**
 * Test in a directory that has existing "New folder", "New folder (2)" and
 * "New Folder (4)" but not "New folder (3)". Should create "New folder (3)" in
 * the gap.
 */
BOOST_AUTO_TEST_CASE( non_contiguous_collision2 )
{
    wpath collision1 = Sandbox() / NEW_FOLDER;
    wpath collision2 = Sandbox() / (NEW_FOLDER + L" (2)");
    wpath collision3 = Sandbox() / (NEW_FOLDER + L" (4)");
    wpath expected = Sandbox() / (NEW_FOLDER + L" (3)");

    BOOST_REQUIRE(create_directory(collision1));
    BOOST_REQUIRE(create_directory(collision2));
    BOOST_REQUIRE(create_directory(collision3));

    NewFolder command = new_folder_command();
    command(NULL, NULL);

    BOOST_REQUIRE(is_directory(expected));
    BOOST_REQUIRE(is_directory(collision1));
    BOOST_REQUIRE(is_directory(collision2));
    BOOST_REQUIRE(is_directory(collision3));

    BOOST_CHECK_EQUAL(
        distance(wdirectory_iterator(Sandbox()), wdirectory_iterator()), 4);
}

/**
 * Test in a directory that has existing "New folder", "New folder (2)" and
 * "New folder (3) " (note the trailing space). Should create "New folder (3)"
 * as it doesn't collide.
 */
BOOST_AUTO_TEST_CASE( collision_suffix_mismatch )
{
    wpath collision1 = Sandbox() / NEW_FOLDER;
    wpath collision2 = Sandbox() / (NEW_FOLDER + L" (2)");
    wpath collision3 = Sandbox() / (NEW_FOLDER + L" (3) ");
    wpath expected = Sandbox() / (NEW_FOLDER + L" (3)");

    BOOST_REQUIRE(create_directory(collision1));
    BOOST_REQUIRE(create_directory(collision2));
    BOOST_REQUIRE(create_directory(collision3));

    NewFolder command = new_folder_command();
    command(NULL, NULL);

    BOOST_REQUIRE(is_directory(expected));
    BOOST_REQUIRE(is_directory(collision1));
    BOOST_REQUIRE(is_directory(collision2));
    BOOST_REQUIRE(is_directory(collision3));

    BOOST_CHECK_EQUAL(
        distance(wdirectory_iterator(Sandbox()), wdirectory_iterator()), 4);
}

/**
 * Test in a directory that has existing "New folder", "New folder (2)" and
 * " New folder (3)" (note the leading space). Should create "New folder (3)"
 * as it doesn't collide.
 */
BOOST_AUTO_TEST_CASE( collision_prefix_mismatch )
{
    wpath collision1 = Sandbox() / NEW_FOLDER;
    wpath collision2 = Sandbox() / (NEW_FOLDER + L" (2)");
    wpath collision3 = Sandbox() / (L" " + NEW_FOLDER + L" (3) ");
    wpath expected = Sandbox() / (NEW_FOLDER + L" (3)");

    BOOST_REQUIRE(create_directory(collision1));
    BOOST_REQUIRE(create_directory(collision2));
    BOOST_REQUIRE(create_directory(collision3));

    NewFolder command = new_folder_command();
    command(NULL, NULL);

    BOOST_REQUIRE(is_directory(expected));
    BOOST_REQUIRE(is_directory(collision1));
    BOOST_REQUIRE(is_directory(collision2));
    BOOST_REQUIRE(is_directory(collision3));

    BOOST_CHECK_EQUAL(
        distance(wdirectory_iterator(Sandbox()), wdirectory_iterator()), 4);
}

BOOST_AUTO_TEST_SUITE_END()
#pragma endregion

#pragma region Task pane tests
BOOST_FIXTURE_TEST_SUITE( new_folder_task_pane_tests, PidlFixture )

/**
 * Test that task pane items can have their OLE site set.
 */
BOOST_AUTO_TEST_CASE( task_pane_old_site )
{
    std::pair<com_ptr<IEnumUICommand>, com_ptr<IEnumUICommand> > panes =
        remote_folder_task_pane_tasks(
            NULL, sandbox_pidl(), NULL,
            bind(&NewFolderCommandFixture::Provider, this),
            bind(&NewFolderCommandFixture::Consumer, this));

    BOOST_REQUIRE(panes.first);

    com_ptr<IUICommand> new_folder;
    ULONG fetched = 0;
    HRESULT hr = panes.first->Next(1, new_folder.out(), &fetched);
    BOOST_REQUIRE_OK(hr);

    com_ptr<IObjectWithSite> object = try_cast(new_folder);
    hr = object->SetSite(NULL);
    BOOST_REQUIRE_OK(hr);
}

BOOST_AUTO_TEST_SUITE_END()
#pragma endregion
