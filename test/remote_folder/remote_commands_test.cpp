// Copyright 2011, 2012, 2013, 2015, 2016 Alexander Lamaison

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

#include "swish/remote_folder/commands/commands.hpp"  // test subject
#include "swish/remote_folder/commands/NewFolder.hpp" // test subject

#include "test/common_boost/helpers.hpp" // BOOST_REQUIRE_OK
#include "test/fixtures/provider_fixture.hpp"

#include <ssh/filesystem.hpp>
#include <ssh/filesystem/path.hpp>

#include <boost/bind.hpp> // bind;
#include <boost/test/unit_test.hpp>

#include <string>

using swish::nse::IEnumUICommand;
using swish::nse::IUICommand;
using swish::nse::command_site;
using swish::remote_folder::commands::NewFolder;
using swish::remote_folder::commands::remote_folder_task_pane_tasks;

using test::fixtures::provider_fixture;

using comet::com_ptr;

using ssh::filesystem::directory_iterator;
using ssh::filesystem::path;

using boost::bind;

using std::distance;
using std::wstring;

namespace
{ // private

const wstring NEW_FOLDER = L"New folder";

class NewFolderCommandFixture : public provider_fixture
{
public:
    NewFolder new_folder_command()
    {
        return NewFolder(sandbox_pidl(),
                         bind(&NewFolderCommandFixture::Provider, this),
                         bind(&NewFolderCommandFixture::Consumer, this));
    }
};
}

template <>
struct comet::comtype<IObjectWithSite>
{
    static const IID& uuid() throw()
    {
        return IID_IObjectWithSite;
    }
    typedef ::IUnknown base;
};

#pragma region NewFolder tests
BOOST_FIXTURE_TEST_SUITE(new_folder_tests, NewFolderCommandFixture)

/**
 * Test NewFolder command has correct properties that don't involve executing
 * the command.
 */
BOOST_AUTO_TEST_CASE(non_execution_properties)
{
    NewFolder command = new_folder_command();
    BOOST_CHECK(!command.guid().is_null());
    BOOST_CHECK(!command.title(NULL).empty());
    BOOST_CHECK(!command.tool_tip(NULL).empty());
    BOOST_CHECK(!command.icon_descriptor(NULL).empty());
    BOOST_CHECK(command.state(NULL, true) ==
                NewFolder::presentation_state::enabled);
}

/**
 * Test in empty directory that (inevitably) has no collisions.
 */
BOOST_AUTO_TEST_CASE(no_collision_empty)
{
    path expected = sandbox() / NEW_FOLDER;

    NewFolder command = new_folder_command();
    command(NULL, command_site(), NULL);

    BOOST_REQUIRE(is_directory(filesystem(), expected));

    BOOST_CHECK_EQUAL(distance(filesystem().directory_iterator(sandbox()),
                               filesystem().directory_iterator()),
                      1);
}

/**
 * Test in a directory that isn't empty but which doesn't have any collisions.
 */
BOOST_AUTO_TEST_CASE(no_collision)
{
    new_file_in_sandbox();
    path expected = sandbox() / NEW_FOLDER;

    NewFolder command = new_folder_command();
    command(NULL, command_site(), NULL);

    BOOST_REQUIRE(is_directory(filesystem(), expected));

    BOOST_CHECK_EQUAL(distance(filesystem().directory_iterator(sandbox()),
                               filesystem().directory_iterator()),
                      2);
}

/**
 * Test in a directory that has existing "New folder".  Should create
  * "New folder (2)" instead.
 */
BOOST_AUTO_TEST_CASE(basic_collision)
{
    path collision = sandbox() / NEW_FOLDER;
    path expected = sandbox() / (NEW_FOLDER + L" (2)");

    BOOST_REQUIRE(create_directory(filesystem(), collision));

    NewFolder command = new_folder_command();
    command(NULL, command_site(), NULL);

    BOOST_REQUIRE(is_directory(filesystem(), expected));
    BOOST_REQUIRE(is_directory(filesystem(), collision));

    BOOST_CHECK_EQUAL(distance(filesystem().directory_iterator(sandbox()),
                               filesystem().directory_iterator()),
                      2);
}

/**
 * Test in a directory that has existing "New folder (2)" but not "New folder".
 * We want to make sure that this doesn't prevent "New folder" being created.
 */
BOOST_AUTO_TEST_CASE(non_interfering_collision)
{
    path collision = sandbox() / (NEW_FOLDER + L" (2)");
    path expected = sandbox() / NEW_FOLDER;

    BOOST_REQUIRE(create_directory(filesystem(), collision));

    NewFolder command = new_folder_command();
    command(NULL, command_site(), NULL);

    BOOST_REQUIRE(is_directory(filesystem(), expected));
    BOOST_REQUIRE(is_directory(filesystem(), collision));

    BOOST_CHECK_EQUAL(distance(filesystem().directory_iterator(sandbox()),
                               filesystem().directory_iterator()),
                      2);
}

/**
 * Test in a directory that has existing "New folder" and "New folder (2)".
 * Should create "New folder (3)" instead.
 */
BOOST_AUTO_TEST_CASE(multiple_collision)
{
    path collision1 = sandbox() / NEW_FOLDER;
    path collision2 = sandbox() / (NEW_FOLDER + L" (2)");
    path expected = sandbox() / (NEW_FOLDER + L" (3)");

    BOOST_REQUIRE(create_directory(filesystem(), collision1));
    BOOST_REQUIRE(create_directory(filesystem(), collision2));

    NewFolder command = new_folder_command();
    command(NULL, command_site(), NULL);

    BOOST_REQUIRE(is_directory(filesystem(), expected));
    BOOST_REQUIRE(is_directory(filesystem(), collision1));
    BOOST_REQUIRE(is_directory(filesystem(), collision2));

    BOOST_CHECK_EQUAL(distance(filesystem().directory_iterator(sandbox()),
                               filesystem().directory_iterator()),
                      3);
}

/**
 * Test in a directory that has existing "New folder" and "New folder (3)"
 * but not "New folder (2). Should create "New folder (2)" in the gap.
 */
BOOST_AUTO_TEST_CASE(non_contiguous_collision1)
{
    path collision1 = sandbox() / NEW_FOLDER;
    path collision2 = sandbox() / (NEW_FOLDER + L" (3)");
    path expected = sandbox() / (NEW_FOLDER + L" (2)");

    BOOST_REQUIRE(create_directory(filesystem(), collision1));
    BOOST_REQUIRE(create_directory(filesystem(), collision2));

    NewFolder command = new_folder_command();
    command(NULL, command_site(), NULL);

    BOOST_REQUIRE(is_directory(filesystem(), expected));
    BOOST_REQUIRE(is_directory(filesystem(), collision1));
    BOOST_REQUIRE(is_directory(filesystem(), collision2));

    BOOST_CHECK_EQUAL(distance(filesystem().directory_iterator(sandbox()),
                               filesystem().directory_iterator()),
                      3);
}

/**
 * Test in a directory that has existing "New folder", "New folder (2)" and
 * "New Folder (4)" but not "New folder (3)". Should create "New folder (3)" in
 * the gap.
 */
BOOST_AUTO_TEST_CASE(non_contiguous_collision2)
{
    path collision1 = sandbox() / NEW_FOLDER;
    path collision2 = sandbox() / (NEW_FOLDER + L" (2)");
    path collision3 = sandbox() / (NEW_FOLDER + L" (4)");
    path expected = sandbox() / (NEW_FOLDER + L" (3)");

    BOOST_REQUIRE(create_directory(filesystem(), collision1));
    BOOST_REQUIRE(create_directory(filesystem(), collision2));
    BOOST_REQUIRE(create_directory(filesystem(), collision3));

    NewFolder command = new_folder_command();
    command(NULL, command_site(), NULL);

    BOOST_REQUIRE(is_directory(filesystem(), expected));
    BOOST_REQUIRE(is_directory(filesystem(), collision1));
    BOOST_REQUIRE(is_directory(filesystem(), collision2));
    BOOST_REQUIRE(is_directory(filesystem(), collision3));

    BOOST_CHECK_EQUAL(distance(filesystem().directory_iterator(sandbox()),
                               filesystem().directory_iterator()),
                      4);
}

/**
 * Test in a directory that has existing "New folder", "New folder (2)" and
 * "New folder (3) " (note the trailing space). Should create "New folder (3)"
 * as it doesn't collide.
 */
BOOST_AUTO_TEST_CASE(collision_suffix_mismatch)
{
    path collision1 = sandbox() / NEW_FOLDER;
    path collision2 = sandbox() / (NEW_FOLDER + L" (2)");
    path collision3 = sandbox() / (NEW_FOLDER + L" (3) ");
    path expected = sandbox() / (NEW_FOLDER + L" (3)");

    BOOST_REQUIRE(create_directory(filesystem(), collision1));
    BOOST_REQUIRE(create_directory(filesystem(), collision2));
    BOOST_REQUIRE(create_directory(filesystem(), collision3));

    NewFolder command = new_folder_command();
    command(NULL, command_site(), NULL);

    BOOST_REQUIRE(is_directory(filesystem(), expected));
    BOOST_REQUIRE(is_directory(filesystem(), collision1));
    BOOST_REQUIRE(is_directory(filesystem(), collision2));
    BOOST_REQUIRE(is_directory(filesystem(), collision3));

    BOOST_CHECK_EQUAL(distance(filesystem().directory_iterator(sandbox()),
                               filesystem().directory_iterator()),
                      4);
}

/**
 * Test in a directory that has existing "New folder", "New folder (2)" and
 * " New folder (3)" (note the leading space). Should create "New folder (3)"
 * as it doesn't collide.
 */
BOOST_AUTO_TEST_CASE(collision_prefix_mismatch)
{
    path collision1 = sandbox() / NEW_FOLDER;
    path collision2 = sandbox() / (NEW_FOLDER + L" (2)");
    path collision3 = sandbox() / (L" " + NEW_FOLDER + L" (3) ");
    path expected = sandbox() / (NEW_FOLDER + L" (3)");

    BOOST_REQUIRE(create_directory(filesystem(), collision1));
    BOOST_REQUIRE(create_directory(filesystem(), collision2));
    BOOST_REQUIRE(create_directory(filesystem(), collision3));

    NewFolder command = new_folder_command();
    command(NULL, command_site(), NULL);

    BOOST_REQUIRE(is_directory(filesystem(), expected));
    BOOST_REQUIRE(is_directory(filesystem(), collision1));
    BOOST_REQUIRE(is_directory(filesystem(), collision2));
    BOOST_REQUIRE(is_directory(filesystem(), collision3));

    BOOST_CHECK_EQUAL(distance(filesystem().directory_iterator(sandbox()),
                               filesystem().directory_iterator()),
                      4);
}

BOOST_AUTO_TEST_SUITE_END()
#pragma endregion

#pragma region Task pane tests
BOOST_FIXTURE_TEST_SUITE(new_folder_task_pane_tests, provider_fixture)

/**
 * Test that task pane items can have their OLE site set.
 */
BOOST_AUTO_TEST_CASE(task_pane_old_site)
{
    std::pair<com_ptr<IEnumUICommand>, com_ptr<IEnumUICommand>> panes =
        remote_folder_task_pane_tasks(
            sandbox_pidl(), NULL,
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
