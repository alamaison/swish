// Copyright 2010, 2013, 2016 Alexander Lamaison

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

#include "session_fixture.hpp"
#include "sftp_fixture.hpp"

#include <ssh/filesystem.hpp> // test subject
#include <ssh/stream.hpp>

#include <boost/bind.hpp>    // bind
#include <boost/cstdint.hpp> // uintmax_t
#include <boost/foreach.hpp> // BOOST_FOREACH
#include <boost/move/move.hpp>
#include <boost/system/system_error.hpp>
#include <boost/test/predicate_result.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/thread/future.hpp>
#include <boost/thread/thread.hpp>
#include <boost/uuid/uuid_generators.hpp> // random_generator
#include <boost/uuid/uuid_io.hpp>         // to_string

#include <algorithm> // find, sort, transform
#include <string>
#include <utility>
#include <vector>

using ssh::filesystem::directory_iterator;
using ssh::filesystem::file_attributes;
using ssh::filesystem::file_status;
using ssh::filesystem::file_type;
using ssh::filesystem::ofstream;
using ssh::filesystem::overwrite_behaviour;
using ssh::filesystem::path;
using ssh::filesystem::perms;
using ssh::filesystem::sftp_file;
using ssh::filesystem::sftp_filesystem;
using ssh::session;

using boost::bind;
using boost::move;
using boost::packaged_task;
using boost::system::system_error;
using boost::test_tools::predicate_result;
using boost::thread;
using boost::uintmax_t;
using boost::uuids::random_generator;

using test::ssh::session_fixture;
using test::ssh::sftp_fixture;

using std::auto_ptr;
using std::find;
using std::make_pair;
using std::pair;
using std::string;
using std::vector;

namespace
{

predicate_result directory_is_empty(sftp_filesystem& fs,
                                    const ::ssh::filesystem::path& p)
{
    predicate_result result(true);

    directory_iterator it = fs.directory_iterator(p);

    size_t entry_count = 0;
    while (it != fs.directory_iterator())
    {
        if (it->path().filename() != "." && it->path().filename() != "..")
        {
            ++entry_count;
        }

        ++it;
    }

    if (entry_count != 0)
    {
        result = false;

        result.message() << "Directory is not empty; contains " << entry_count
                         << " entries";
    }

    return result;
}
}

namespace
{

class filesystem_fixture : public sftp_fixture
{
public:
    // The following functions return the link and target path as a pair.  Both
    // paths are relative to the sandbox, regardless of whether the symlink was
    // created with a relative or absolute path

    pair<path, path> create_relative_symlink_in_sandbox()
    {
        path link = sandbox() / "link";
        path target = new_file_in_sandbox().filename();
        create_symlink(link, target);
        return make_pair(link.filename(), target.filename());
    }

    pair<path, path> create_absolute_symlink_in_sandbox()
    {
        path link = sandbox() / "link";
        path target = absolute_sandbox() / new_file_in_sandbox().filename();
        create_symlink(link, target);
        return make_pair(link.filename(), target.filename());
    }

    pair<path, path> create_broken_symlink_in_sandbox()
    {
        path link = sandbox() / "link";
        path target = "i don't exist";
        create_symlink(link, target);
        return make_pair(link.filename(), target.filename());
    }
};
}

// Tests assume an authenticated session and established SFTP filesystem
BOOST_FIXTURE_TEST_SUITE(channel_running_tests, filesystem_fixture)

/**
 * List an empty directory.
 *
 * Will contain . and ..
 */
BOOST_AUTO_TEST_CASE(empty_dir)
{
    sftp_filesystem& c = filesystem();

    BOOST_CHECK(directory_is_empty(c, sandbox()));
}

/**
 * List a directory that doesn't exist.  Must throw.
 */
BOOST_AUTO_TEST_CASE(missing_dir)
{
    sftp_filesystem& c = filesystem();
    BOOST_CHECK_THROW(c.directory_iterator("/i/dont/exist"), system_error);
}

BOOST_AUTO_TEST_CASE(swap_filesystems)
{
    sftp_filesystem& fs1 = filesystem();
    sftp_filesystem fs2 = test_session().connect_to_filesystem();

    boost::swap(fs1, fs2);

    BOOST_CHECK(directory_is_empty(fs1, sandbox()));
    BOOST_CHECK(directory_is_empty(fs2, sandbox()));
}

BOOST_AUTO_TEST_CASE(move_construct)
{
    sftp_filesystem& c = filesystem();
    sftp_filesystem d(move(c));

    BOOST_CHECK(directory_is_empty(d, sandbox()));
}

BOOST_AUTO_TEST_CASE(move_assign)
{
    sftp_filesystem& c = filesystem();
    sftp_filesystem d(test_session().connect_to_filesystem());

    d = move(c);

    BOOST_CHECK(directory_is_empty(d, sandbox()));
}

namespace
{
string filename_getter(const sftp_file& directory_entry)
{
    return directory_entry.path().filename().native();
}
}

BOOST_AUTO_TEST_CASE(dir_with_one_file)
{
    path test_file = new_file_in_sandbox();

    vector<string> files;
    transform(filesystem().directory_iterator(sandbox()),
              filesystem().directory_iterator(), back_inserter(files),
              filename_getter);
    sort(files.begin(), files.end());

    vector<string> expected;
    expected.push_back(".");
    expected.push_back("..");
    expected.push_back(test_file.filename().native());
    BOOST_CHECK_EQUAL_COLLECTIONS(files.begin(), files.end(), expected.begin(),
                                  expected.end());
}

BOOST_AUTO_TEST_CASE(dir_with_multiple_files)
{
    path test_file1 = new_file_in_sandbox();
    path test_file2 = new_file_in_sandbox();

    vector<sftp_file> files(filesystem().directory_iterator(sandbox()),
                            filesystem().directory_iterator());
    sort(files.begin(), files.end());

    vector<sftp_file>::const_iterator it = files.begin();

    BOOST_CHECK_EQUAL(it->path().filename(), ".");
    BOOST_CHECK_GT(it->long_entry().size(), 0U);

    it++;

    BOOST_CHECK_EQUAL((*it).path().filename(), "..");

    it++;

    BOOST_CHECK(it->path().filename() ==
                    path(test_file1.filename().wstring()) ||
                it->path().filename() == path(test_file2.filename().wstring()));
    it++;

    BOOST_CHECK(it->path().filename() ==
                    path(test_file1.filename().wstring()) ||
                it->path().filename() == path(test_file2.filename().wstring()));

    it++;
}

BOOST_AUTO_TEST_CASE(move_construct_iterator)
{
    path test_file1 = new_file_in_sandbox();
    path test_file2 = new_file_in_sandbox();

    directory_iterator it = filesystem().directory_iterator(sandbox());
    it++;
    it++;

    string path_before_move = it->path();

    directory_iterator itm(move(it));

    BOOST_CHECK_EQUAL(itm->path(), path_before_move);

    itm++;

    BOOST_CHECK_NE(itm->path(), path_before_move);

    itm++;

    BOOST_CHECK(itm == filesystem().directory_iterator());
}

BOOST_AUTO_TEST_CASE(can_create_relative_symlink)
{
    path link = create_relative_symlink_in_sandbox().first;
    BOOST_CHECK(exists(filesystem(), sandbox() / link));
    BOOST_CHECK_EQUAL(find_file_in_sandbox(link).attributes().type(),
                      file_attributes::symbolic_link);
}

BOOST_AUTO_TEST_CASE(can_create_absolute_symlink)
{
    path link = create_relative_symlink_in_sandbox().first;
    BOOST_CHECK(exists(filesystem(), sandbox() / link));
    BOOST_CHECK_EQUAL(find_file_in_sandbox(link).attributes().type(),
                      file_attributes::symbolic_link);
}

BOOST_AUTO_TEST_CASE(can_create_broken_symlink)
{
    path link = create_broken_symlink_in_sandbox().first;
    BOOST_CHECK(exists(filesystem(), sandbox() / link));
    BOOST_CHECK_EQUAL(find_file_in_sandbox(link).attributes().type(),
                      file_attributes::symbolic_link);
}

BOOST_AUTO_TEST_CASE(relative_symlinks_are_resolved_to_their_relative_target)
{
    pair<path, path> ends = create_relative_symlink_in_sandbox();

    path resolved_target =
        filesystem().resolve_link_target(sandbox() / ends.first);
    BOOST_CHECK_EQUAL(resolved_target, ends.second);
}

BOOST_AUTO_TEST_CASE(absolute_symlinks_are_resolved_to_their_absolute_target)
{
    pair<path, path> ends = create_absolute_symlink_in_sandbox();

    path resolved_target =
        filesystem().resolve_link_target(sandbox() / ends.first);
    BOOST_CHECK_EQUAL(resolved_target, absolute_sandbox() / ends.second);
}

BOOST_AUTO_TEST_CASE(broken_symlinks_are_resolved_to_their_non_existent_target)
{
    pair<path, path> ends = create_broken_symlink_in_sandbox();

    path resolved_target =
        filesystem().resolve_link_target(sandbox() / ends.first);
    BOOST_CHECK_EQUAL(resolved_target, ends.second);
}

/**
 * Resolve a symlink to a symlink.  The result should be the path of the
 * middle symlink, rather than the middle symlink's target.
 */
BOOST_AUTO_TEST_CASE(resolving_symlink_to_symlink_returns_middle_link)
{
    pair<path, path> ends = create_relative_symlink_in_sandbox();

    path target = ends.second;
    path middle_link = ends.first;
    path link_to_link = sandbox() / "link2";
    create_symlink(link_to_link, middle_link);

    path resolved_target = filesystem().resolve_link_target(link_to_link);

    BOOST_CHECK_EQUAL(resolved_target, middle_link);
}

BOOST_AUTO_TEST_CASE(canonicalising_relative_symlink_returns_absolute_path)
{
    pair<path, path> ends = create_relative_symlink_in_sandbox();

    path canonical_target = filesystem().canonical_path(sandbox() / ends.first);
    BOOST_CHECK_EQUAL(canonical_target, absolute_sandbox() / ends.second);
}

BOOST_AUTO_TEST_CASE(canonicalising_absolute_symlink_returns_absolute_path)
{
    pair<path, path> ends = create_absolute_symlink_in_sandbox();

    path canonical_target = filesystem().canonical_path(sandbox() / ends.first);
    BOOST_CHECK_EQUAL(canonical_target, absolute_sandbox() / ends.second);
}

BOOST_AUTO_TEST_CASE(
    canonicalising_symlink_to_symlink_return_absolute_path_of_final_target)
{
    pair<path, path> ends = create_relative_symlink_in_sandbox();

    path target = ends.second;
    path middle_link = ends.first;
    path link_to_link = sandbox() / "link2";
    create_symlink(link_to_link, middle_link);

    path canonical_target = filesystem().canonical_path(link_to_link);

    BOOST_CHECK_EQUAL(canonical_target, absolute_sandbox() / target);
}

BOOST_AUTO_TEST_CASE(attributes_file)
{
    path subject = new_file_in_sandbox();

    file_attributes attrs = filesystem().attributes(subject, false);

    BOOST_CHECK_EQUAL(attrs.type(), file_attributes::normal_file);

    attrs = filesystem().attributes(subject, true);

    BOOST_CHECK_EQUAL(attrs.type(), file_attributes::normal_file);
}

BOOST_AUTO_TEST_CASE(attributes_directory)
{
    path subject = sandbox() / "testdir";
    create_directory(filesystem(), subject);

    file_attributes attrs = filesystem().attributes(subject, false);

    BOOST_CHECK_EQUAL(attrs.type(), file_attributes::directory);

    attrs = filesystem().attributes(subject, true);

    BOOST_CHECK_EQUAL(attrs.type(), file_attributes::directory);
}

BOOST_AUTO_TEST_CASE(attributes_link)
{
    pair<path, path> ends = create_relative_symlink_in_sandbox();
    path link = sandbox() / ends.first;

    file_attributes attrs = filesystem().attributes(link, false);

    BOOST_CHECK_EQUAL(attrs.type(), file_attributes::symbolic_link);

    attrs = filesystem().attributes(link, true);

    BOOST_CHECK_EQUAL(attrs.type(), file_attributes::normal_file);
}

BOOST_AUTO_TEST_CASE(attributes_double_link)
{
    pair<path, path> ends = create_relative_symlink_in_sandbox();

    path middle_link = ends.first;
    path link_to_link = sandbox() / "link2";
    create_symlink(link_to_link, middle_link);

    file_attributes attrs = filesystem().attributes(link_to_link, true);

    BOOST_CHECK_EQUAL(attrs.type(), file_attributes::normal_file);
}

BOOST_AUTO_TEST_CASE(attributes_broken_link)
{
    pair<path, path> ends = create_broken_symlink_in_sandbox();

    BOOST_CHECK_THROW(filesystem().attributes(ends.first, true), system_error);
}

BOOST_AUTO_TEST_CASE(default_directory)
{
    path resolved_target = filesystem().canonical_path("");
    BOOST_CHECK_EQUAL(resolved_target, "/home/swish");
}

BOOST_AUTO_TEST_CASE(remove_nothing)
{
    path target = "gibberish";

    bool already_existed = remove(filesystem(), target);

    BOOST_CHECK(!exists(filesystem(), target));
    BOOST_CHECK(!already_existed);
}

BOOST_AUTO_TEST_CASE(remove_file)
{
    path target = new_file_in_sandbox();

    bool already_existed = remove(filesystem(), target);

    BOOST_CHECK(!exists(filesystem(), target));
    BOOST_CHECK(already_existed);
}

BOOST_AUTO_TEST_CASE(remove_empty_dir)
{
    path target = new_directory_in_sandbox();

    bool already_existed = remove(filesystem(), target);

    BOOST_CHECK(!exists(filesystem(), target));
    BOOST_CHECK(already_existed);
}

BOOST_AUTO_TEST_CASE(remove_non_empty_dir)
{
    path target = new_directory_in_sandbox();
    create_directory(filesystem(), target / "bob");

    BOOST_CHECK_THROW(remove(filesystem(), target), system_error);

    BOOST_CHECK(exists(filesystem(), target));
}

BOOST_AUTO_TEST_CASE(remove_link)
{
    path target = new_file_in_sandbox();
    path link = sandbox() / "link";
    create_symlink(link, target);

    bool already_existed = remove(filesystem(), link);

    BOOST_CHECK(!exists(filesystem(), link));
    BOOST_CHECK(exists(filesystem(), target)); // should only delete the link
    BOOST_CHECK(already_existed);
}

BOOST_AUTO_TEST_CASE(remove_nothing_recursive)
{
    path target = "gibberish";

    uintmax_t count = remove_all(filesystem(), target);

    BOOST_CHECK(!exists(filesystem(), target));
    BOOST_CHECK_EQUAL(count, 0U);
}

BOOST_AUTO_TEST_CASE(remove_file_recursive)
{
    path target = new_file_in_sandbox();

    uintmax_t count = remove_all(filesystem(), target);

    BOOST_CHECK(!exists(filesystem(), target));
    BOOST_CHECK_EQUAL(count, 1U);
}

BOOST_AUTO_TEST_CASE(remove_empty_dir_recursive)
{
    path target = new_directory_in_sandbox();

    uintmax_t count = remove_all(filesystem(), target);

    BOOST_CHECK(!exists(filesystem(), target));
    BOOST_CHECK_EQUAL(count, 1U);
}

BOOST_AUTO_TEST_CASE(remove_non_empty_dir_recursive)
{
    path target = new_directory_in_sandbox();
    create_directory(filesystem(), target / "bob");
    ofstream(filesystem(), target / "bob" / "sally");
    ofstream(filesystem(),
             target / "alice"); // Either side of bob alphabetically
    ofstream(filesystem(), target / "jim");

    uintmax_t count = remove_all(filesystem(), target);

    BOOST_CHECK(!exists(filesystem(), target));
    BOOST_CHECK_EQUAL(count, 5U);
}

BOOST_AUTO_TEST_CASE(remove_link_recursive)
{
    path target = new_directory_in_sandbox();
    create_directory(filesystem(), target / "bob");
    path link = sandbox() / "link";
    create_symlink(link, target);

    uintmax_t count = remove_all(filesystem(), link);

    BOOST_CHECK(!exists(filesystem(), link));
    BOOST_CHECK(exists(filesystem(), target)); // should only delete the link
    BOOST_CHECK(
        exists(filesystem(), target / "bob")); // should only delete the link
    BOOST_CHECK_EQUAL(count, 1U);
}

BOOST_AUTO_TEST_CASE(rename_file)
{
    path test_file = new_file_in_sandbox();
    path target = sandbox() / "target";

    rename(filesystem(), test_file, target,
           overwrite_behaviour::prevent_overwrite);
    BOOST_CHECK(!exists(filesystem(), test_file));
    BOOST_CHECK(exists(filesystem(), target));
}

BOOST_AUTO_TEST_CASE(rename_file_obstacle_no_overwrite)
{
    path test_file = new_file_in_sandbox();

    path target = new_file_in_sandbox("target");

    BOOST_CHECK_THROW(rename(filesystem(), test_file, target,
                             overwrite_behaviour::prevent_overwrite),
                      system_error);
    BOOST_CHECK(exists(filesystem(), test_file));
    BOOST_CHECK(exists(filesystem(), target));
}

BOOST_AUTO_TEST_CASE(rename_file_obstacle_allow_overwrite)
{
    path test_file = new_file_in_sandbox();

    path target = new_file_in_sandbox("target");

    // Using OpenSSH server which only supports SFTP 3 (no overwrite) so
    // failure expected
    BOOST_CHECK_THROW(rename(filesystem(), test_file, target,
                             overwrite_behaviour::allow_overwrite),
                      system_error);
    BOOST_CHECK(exists(filesystem(), test_file));
    BOOST_CHECK(exists(filesystem(), target));
}

BOOST_AUTO_TEST_CASE(rename_file_obstacle_atomic_overwrite)
{
    path test_file = new_file_in_sandbox();

    path target = new_file_in_sandbox("target");

    // Using OpenSSH server which only supports SFTP 3 (no overwrite) so
    // failure expected
    BOOST_CHECK_THROW(rename(filesystem(), test_file, target,
                             overwrite_behaviour::atomic_overwrite),
                      system_error);
    BOOST_CHECK(exists(filesystem(), test_file));
    BOOST_CHECK(exists(filesystem(), target));
}

BOOST_AUTO_TEST_CASE(exists_true)
{
    path test_file = new_file_in_sandbox();

    BOOST_CHECK(exists(filesystem(), test_file));
}

BOOST_AUTO_TEST_CASE(exists_false)
{
    path test_file = sandbox() / "I do not exist";
    BOOST_CHECK(!exists(filesystem(), test_file));
}

BOOST_AUTO_TEST_CASE(is_directory_returns_true_for_directories)
{
    path target = new_directory_in_sandbox();

    BOOST_CHECK(is_directory(filesystem(), target));
}

BOOST_AUTO_TEST_CASE(is_directory_returns_false_for_files)
{
    path target = new_file_in_sandbox();

    BOOST_CHECK(!is_directory(filesystem(), target));
}

BOOST_AUTO_TEST_CASE(is_directory_returns_false_for_non_existent_path)
{
    BOOST_CHECK(!is_directory(filesystem(), "i do not exist"));
}

BOOST_AUTO_TEST_CASE(new_directory)
{
    path target = new_directory_in_sandbox();
    remove(filesystem(), target);

    BOOST_CHECK(create_directory(filesystem(), target));
    BOOST_CHECK(exists(filesystem(), target));
    BOOST_CHECK(is_directory(filesystem(), target));
}

BOOST_AUTO_TEST_CASE(new_directory_already_there)
{
    path target = new_directory_in_sandbox();

    BOOST_CHECK(!create_directory(filesystem(), target));
    BOOST_CHECK(exists(filesystem(), target));
    BOOST_CHECK(is_directory(filesystem(), target));
}

BOOST_AUTO_TEST_CASE(new_directory_already_there_wrong_type)
{
    path target = new_file_in_sandbox();

    BOOST_CHECK_THROW(create_directory(filesystem(), target), system_error);
    BOOST_CHECK(exists(filesystem(), target));
    BOOST_CHECK(!is_directory(filesystem(), target));
}

BOOST_AUTO_TEST_CASE(status_returns_correct_file_permissions)
{
    path target = new_file_in_sandbox();
    perms permissions = status(filesystem(), target).permissions();
    BOOST_CHECK_EQUAL(permissions, perms::owner_read | perms::owner_write |
                                       perms::group_read | perms::others_read);
}

BOOST_AUTO_TEST_CASE(status_returns_correct_file_type)
{
    path target = new_file_in_sandbox();
    file_type type = status(filesystem(), target).type();
    BOOST_CHECK_EQUAL(type, file_type::regular);
}

BOOST_AUTO_TEST_CASE(status_returns_correct_directory_permissions)
{
    path target = new_directory_in_sandbox();
    perms permissions = status(filesystem(), target).permissions();
    BOOST_CHECK_EQUAL(permissions, perms::owner_all | perms::group_read |
                                       perms::group_exec | perms::others_read |
                                       perms::others_exec);
}

BOOST_AUTO_TEST_CASE(status_returns_correct_directory_type)
{
    path target = new_directory_in_sandbox();
    file_type type = status(filesystem(), target).type();
    BOOST_CHECK_EQUAL(type, file_type::directory);
}

BOOST_AUTO_TEST_CASE(status_does_not_throw_if_file_doesnt_exist)
{
    path target = "i don't exist";
    file_status s = status(filesystem(), target);
    BOOST_CHECK_EQUAL(s.permissions(), perms::unknown);
    BOOST_CHECK_EQUAL(s.type(), file_type::not_found);
}

BOOST_AUTO_TEST_CASE(can_set_file_permissions_exactly)
{
    path target = new_file_in_sandbox();
    permissions(filesystem(), target, perms::group_write);
    perms new_permissions = status(filesystem(), target).permissions();
    BOOST_CHECK_EQUAL(new_permissions, perms::group_write);
}

BOOST_AUTO_TEST_CASE(can_set_file_permissions_to_none)
{
    path target = new_file_in_sandbox();
    permissions(filesystem(), target, perms::none);
    perms new_permissions = status(filesystem(), target).permissions();
    BOOST_CHECK_EQUAL(new_permissions, perms::none);
}

BOOST_AUTO_TEST_CASE(can_add_file_permissions)
{
    path target = new_file_in_sandbox();
    permissions(filesystem(), target, perms::add_perms | perms::group_write);
    perms new_permissions = status(filesystem(), target).permissions();
    BOOST_CHECK_EQUAL(new_permissions, perms::group_write | perms::owner_read |
                                           perms::owner_write |
                                           perms::group_read |
                                           perms::others_read);
}

BOOST_AUTO_TEST_CASE(can_remove_file_permissions)
{
    path target = new_file_in_sandbox();
    permissions(filesystem(), target, perms::remove_perms | perms::group_read);
    perms new_permissions = status(filesystem(), target).permissions();
    BOOST_CHECK_EQUAL(new_permissions, perms::owner_read | perms::owner_write |
                                           perms::others_read);
}


BOOST_AUTO_TEST_SUITE_END();
