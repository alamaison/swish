/**
    @file

    Tests for SFTP subsytem.

    @if license

    Copyright (C) 2010, 2013  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

    In addition, as a special exception, the the copyright holders give you
    permission to combine this program with free software programs or the 
    OpenSSL project's "OpenSSL" library (or with modified versions of it, 
    with unchanged license). You may copy and distribute such a system 
    following the terms of the GNU GPL for this program and the licenses 
    of the other code concerned. The GNU General Public License gives 
    permission to release a modified version without this exception; this 
    exception also makes it possible to release a modified version which 
    carries forward this exception.

    @endif
*/

#include "sandbox_fixture.hpp" // sandbox_fixture
#include "session_fixture.hpp" // session_fixture

#include <ssh/filesystem.hpp> // test subject

#include <boost/bind.hpp> // bind
#include <boost/cstdint.hpp> // uintmax_t
#include <boost/filesystem/fstream.hpp> // ofstream
#include <boost/foreach.hpp> // BOOST_FOREACH
#include <boost/move/move.hpp>
#include <boost/system/system_error.hpp>
#include <boost/test/predicate_result.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/thread/future.hpp>
#include <boost/thread/thread.hpp>

#include <algorithm> // find
#include <string>

using ssh::session;
using ssh::filesystem::file_attributes;
using ssh::filesystem::sftp_filesystem;
using ssh::filesystem::sftp_file;
using ssh::filesystem::directory_iterator;
using ssh::filesystem::overwrite_behaviour;

using boost::bind;
using boost::filesystem::ofstream;
using boost::filesystem::path;
using boost::move;
using boost::packaged_task;
using boost::system::system_error;
using boost::test_tools::predicate_result;
using boost::thread;
using boost::uintmax_t;

using test::ssh::sandbox_fixture;
using test::ssh::session_fixture;

using std::auto_ptr;
using std::find;
using std::string;

namespace {

bool filename_matches(const string& filename, const sftp_file& remote_file)
{
    return filename == remote_file.name();
}

class sftp_fixture : public session_fixture, public sandbox_fixture
{
public:

    sftp_fixture() : m_filesystem(authenticate_and_create_sftp()) {}

    sftp_filesystem& filesystem()
    {
        return m_filesystem;
    }

    sftp_file find_file_in_remote_sandbox(const string& filename)
    {
        // Search for path in directory because we need the 'remote' form of it,
        // not the local filesystem version.
        directory_iterator it =
            filesystem().directory_iterator(to_remote_path(sandbox()));
        directory_iterator pos = find_if(
            it, filesystem().directory_iterator(),
            bind(filename_matches, filename, _1));
        BOOST_REQUIRE(pos != filesystem().directory_iterator());

        return *pos;
    }

    void create_symlink(path link, path target)
    {
        // Passing arguments in the wrong order to work around OpenSSH bug
        filesystem().create_symlink(to_remote_path(target), to_remote_path(link));
    }

private:

    sftp_filesystem authenticate_and_create_sftp()
    {
        session& s = test_session();
        s.authenticate_by_key_files(
            user(), public_key_path(), private_key_path(), "");
        return s.connect_to_filesystem();
    }

    sftp_filesystem m_filesystem;
};

predicate_result directory_is_empty(sftp_filesystem& fs, const path& p)
{
    predicate_result result(true);

    directory_iterator it = fs.directory_iterator(p);

    size_t entry_count = 0;
    while (it != fs.directory_iterator())
    {
        if (it->name() != "." && it->name() != "..")
        {
            ++entry_count;
        }

        ++it;
    }

    if (entry_count != 0)
    {
        result = false;

        result.message() << "Directory is not empty; contains "
            << entry_count << " entries";
    }

    return result;
}

}


BOOST_AUTO_TEST_SUITE(filesystem_tests)

BOOST_FIXTURE_TEST_CASE( construct_fail, session_fixture )
{
    session& s = test_session();

    // Session not authenticated so SFTP not possible
    BOOST_CHECK_THROW(s.connect_to_filesystem(), system_error);
}

// This tests the very basic requirements of any sensible relationship between
// a filesystem and a session.  It must be possible to create a filesystem
// before moving the session.  That's it.
//
// In particular, we destroy the filesystem before moving the object because
// we don't want to test an added requirement that the filesystem's lifetime
// can extend beyond the session's move.  Whatever else we might decide the
// semantics of the session-filesystem relationship should be now or in the
// future, this tests must pass.  Anything else would mean moving depends on
// what you've used the session for in the past, which would just be broken.
// 
// In other words, even the most careful caller would run into trouble
// if this test failed.
BOOST_FIXTURE_TEST_CASE(
    move_session_after_connecting_filesystem, session_fixture )
{
    session& s = test_session();
    s.authenticate_by_key_files(
        user(), public_key_path(), private_key_path(), "");

    {
        sftp_filesystem(s.connect_to_filesystem());
    }

    session(move(s));
}

// This builds slightly on the previous test by checking that the filesystem
// can be destroyed _after_ the session is moved.  It still isn't a test that
// the filesystem is usable afterwards (though we probably want that
// property too), just that the object is valid (can be destroyed).
//
// In an earlier version, the filesystem destructor tried to use the moved
// session causing a crash.  It's very hard sometimes to ensure the filesystem
// is destroyed before the exact (non-moved) session it came from, so its
// important we allow destruction to happen after moving the session (but
// before moved-to session is destroyed).
BOOST_FIXTURE_TEST_CASE(
    move_session_with_live_filesystem_connection, session_fixture )
{
    session& s = test_session();
    s.authenticate_by_key_files(
        user(), public_key_path(), private_key_path(), "");

    sftp_filesystem fs = s.connect_to_filesystem();
    session s2(move(s));

    // The rules are that the last session must outlive the last FS so moving
    // FS to inner scope ensures this
    sftp_filesystem(move(fs));
}

namespace {

    class basic_sftp_fixture : public session_fixture, public sandbox_fixture
    {};

}

// This is the third part of the session-movement tests. It strengthens the
// requirements a bit more to ensure the filesystem is not just valid for
// destruction but also still functions as a filesystem connection.
BOOST_FIXTURE_TEST_CASE(
    moving_session_leaves_working_filesystem, basic_sftp_fixture )
{
    session& s = test_session();
    s.authenticate_by_key_files(
        user(), public_key_path(), private_key_path(), "");

    sftp_filesystem fs = s.connect_to_filesystem();
    session s2(move(s));

    // The rules are that the last session must outlive the last FS so moving
    // FS to inner scope ensures this
    sftp_filesystem fs2(move(fs));

    BOOST_CHECK(directory_is_empty(fs2, to_remote_path(sandbox())));
}

BOOST_FIXTURE_TEST_CASE(
    swap_session_with_live_filesystem_connection, session_fixture )
{
    // Both sockets must outlive both session objects
    auto_ptr<boost::asio::ip::tcp::socket> socket1(connect_additional_socket());
    auto_ptr<boost::asio::ip::tcp::socket> socket2(connect_additional_socket());

    session s(socket1->native());
    session t(socket2->native());
    s.authenticate_by_key_files(
        user(), public_key_path(), private_key_path(), "");
    t.authenticate_by_key_files(
        user(), public_key_path(), private_key_path(), "");

    sftp_filesystem fs = s.connect_to_filesystem();

    boost::swap(t, s);
}

// Tests assume an authenticated session and established SFTP filesystem
BOOST_FIXTURE_TEST_SUITE(channel_running_tests, sftp_fixture)

/**
 * List an empty directory.
 *
 * Will contain . and ..
 */
BOOST_AUTO_TEST_CASE( empty_dir )
{
    sftp_filesystem& c = filesystem();

    BOOST_CHECK(directory_is_empty(c, to_remote_path(sandbox())));
}

/**
 * List a directory that doesn't exist.  Must throw.
 */
BOOST_AUTO_TEST_CASE( missing_dir )
{
    sftp_filesystem& c = filesystem();
    BOOST_CHECK_THROW(c.directory_iterator("/i/dont/exist"), system_error);
}

BOOST_AUTO_TEST_CASE( swap_filesystems )
{
    sftp_filesystem& fs1 = filesystem();
    sftp_filesystem fs2 = test_session().connect_to_filesystem();

    boost::swap(fs1, fs2);

    BOOST_CHECK(directory_is_empty(fs1, to_remote_path(sandbox())));
    BOOST_CHECK(directory_is_empty(fs2, to_remote_path(sandbox())));
}

BOOST_AUTO_TEST_CASE( move_construct )
{
    sftp_filesystem& c = filesystem();
    sftp_filesystem d(move(c));

    BOOST_CHECK(directory_is_empty(d, to_remote_path(sandbox())));
}

BOOST_AUTO_TEST_CASE( move_assign )
{
    sftp_filesystem& c = filesystem();
    sftp_filesystem d(test_session().connect_to_filesystem());

    d = move(c);

    BOOST_CHECK(directory_is_empty(d, to_remote_path(sandbox())));
}

BOOST_AUTO_TEST_CASE( dir_with_one_file )
{
    path test_file = new_file_in_sandbox();

    directory_iterator it =
        filesystem().directory_iterator(to_remote_path(sandbox()));

    sftp_file file = *it;
    BOOST_CHECK_EQUAL(file.name(), ".");
    BOOST_CHECK_EQUAL(it->name(), ".");
    BOOST_CHECK_GT(it->long_entry().size(), 0U);

    it++;
    
    BOOST_CHECK_EQUAL((*it).name(), "..");
    file = *it;
    BOOST_CHECK_EQUAL(file.name(), "..");

    it++;

    BOOST_CHECK_EQUAL(it->name(), test_file.filename());

    it++;

    BOOST_CHECK(it == filesystem().directory_iterator());
}

BOOST_AUTO_TEST_CASE( dir_with_multiple_files )
{
    path test_file1 = new_file_in_sandbox();
    path test_file2 = new_file_in_sandbox();

    directory_iterator it =
        filesystem().directory_iterator(to_remote_path(sandbox()));

    sftp_file file = *it;
    BOOST_CHECK_EQUAL(file.name(), ".");
    BOOST_CHECK_EQUAL(it->name(), ".");
    BOOST_CHECK_GT(it->long_entry().size(), 0U);

    it++;

    BOOST_CHECK_EQUAL((*it).name(), "..");
    file = *it;
    BOOST_CHECK_EQUAL(file.name(), "..");

    it++;

    BOOST_CHECK(
        it->name() == test_file1.filename() ||
        it->name() == test_file2.filename());
    it++;

    BOOST_CHECK(
        it->name() == test_file1.filename() ||
        it->name() == test_file2.filename());

    it++;

    BOOST_CHECK(it == filesystem().directory_iterator());
}

BOOST_AUTO_TEST_CASE( move_construct_iterator )
{
    path test_file1 = new_file_in_sandbox();
    path test_file2 = new_file_in_sandbox();

    directory_iterator it =
        filesystem().directory_iterator(to_remote_path(sandbox()));

    sftp_file file = *it;
    BOOST_CHECK_EQUAL(file.name(), ".");
    BOOST_CHECK_EQUAL(it->name(), ".");
    BOOST_CHECK_GT(it->long_entry().size(), 0U);

    it++;

    BOOST_CHECK_EQUAL((*it).name(), "..");
    file = *it;
    BOOST_CHECK_EQUAL(file.name(), "..");

    it++;

    BOOST_CHECK(
        it->name() == test_file1.filename() ||
        it->name() == test_file2.filename());

    directory_iterator itm(move(it));

    itm++;

    BOOST_CHECK(
        itm->name() == test_file1.filename() ||
        itm->name() == test_file2.filename());

    itm++;

    BOOST_CHECK(itm == filesystem().directory_iterator());
}

/**
 * Create a symbolic link.
 */
BOOST_AUTO_TEST_CASE( symlink_creation )
{
    create_symlink(sandbox() / "link", new_file_in_sandbox());
    BOOST_CHECK(exists(sandbox() / "link") || exists(sandbox() / "link.lnk"));
}

/**
 * Recognise a symbolic link.
 */
BOOST_AUTO_TEST_CASE( symlink_recognition )
{
    create_symlink(sandbox() / "link", new_file_in_sandbox());

    BOOST_CHECK_EQUAL(
        find_file_in_remote_sandbox("link").attributes().type(),
        file_attributes::symbolic_link);
}

/**
 * Resolve a symbolic link.
 */
BOOST_AUTO_TEST_CASE( symlink_resolution )
{
    path target =  new_file_in_sandbox();
    create_symlink(sandbox() / "link", target);

    path remote_target = to_remote_path(target);
    path resolved_target = 
        resolve_link_target(filesystem(), find_file_in_remote_sandbox("link"));
    BOOST_CHECK_EQUAL(resolved_target, remote_target);
}

/**
 * Canonicalise path.
 */
BOOST_AUTO_TEST_CASE( canonicalisation )
{
    path target =  new_file_in_sandbox();
    create_symlink(sandbox() / "link", target);

    path remote_target = to_remote_path(target);
    path resolved_target = 
        canonical_path(filesystem(), find_file_in_remote_sandbox("link"));
    BOOST_CHECK_EQUAL(resolved_target, remote_target);
}

/**
 * Canonicalise a path that consists of two symlinks.
 */
BOOST_AUTO_TEST_CASE( two_hop_canonicalisation )
{
    path target =  new_file_in_sandbox();
    create_symlink(sandbox() / "link1", target);
    create_symlink(sandbox() / "link2", sandbox() / "link1");

    path remote_target = to_remote_path(target);
    path resolved_target = 
        canonical_path(filesystem(), find_file_in_remote_sandbox("link2"));
    BOOST_CHECK_EQUAL(resolved_target, remote_target);
}

/**
 * Resolve a symlink to a symlink.  The result should be the path of the
 * second symlink, rather than the second symlink's target.
 */
BOOST_AUTO_TEST_CASE( symlink_to_symlink )
{
    path target =  new_file_in_sandbox();
    create_symlink(sandbox() / "link1", target);
    create_symlink(sandbox() / "link2", sandbox() / "link1");

    path remote_target = to_remote_path(sandbox() / "link1");
    path resolved_target = 
        resolve_link_target(filesystem(), find_file_in_remote_sandbox("link2"));
    BOOST_CHECK_EQUAL(resolved_target, remote_target);
}

BOOST_AUTO_TEST_CASE( attributes_file )
{
    path subject = new_file_in_sandbox();

    file_attributes attrs = filesystem().attributes(to_remote_path(subject), false);

    BOOST_CHECK_EQUAL(attrs.type(), file_attributes::normal_file);

    attrs = filesystem().attributes(to_remote_path(subject), true);

    BOOST_CHECK_EQUAL(attrs.type(), file_attributes::normal_file);
}

BOOST_AUTO_TEST_CASE( attributes_directory )
{
    path subject = sandbox() / "testdir";
    create_directory(subject);

    file_attributes attrs = filesystem().attributes(to_remote_path(subject), false);

    BOOST_CHECK_EQUAL(attrs.type(), file_attributes::directory);

    attrs = filesystem().attributes(to_remote_path(subject), true);

    BOOST_CHECK_EQUAL(attrs.type(), file_attributes::directory);
}

BOOST_AUTO_TEST_CASE( attributes_link )
{
    path target = new_file_in_sandbox();
    path link = sandbox() / "link";
    create_symlink(link, target);

    file_attributes attrs = filesystem().attributes(to_remote_path(link), false);

    BOOST_CHECK_EQUAL(attrs.type(), file_attributes::symbolic_link);

    attrs = filesystem().attributes(to_remote_path(link), true);

    BOOST_CHECK_EQUAL(attrs.type(), file_attributes::normal_file);
}

BOOST_AUTO_TEST_CASE( attributes_double_link )
{
    path target = new_file_in_sandbox();
    path middle_link = sandbox() / "link1";
    path link = sandbox() / "link2";
    create_symlink(middle_link, target);
    create_symlink(link, middle_link);

    file_attributes attrs = filesystem().attributes(to_remote_path(link), false);

    BOOST_CHECK_EQUAL(attrs.type(), file_attributes::symbolic_link);

    attrs = filesystem().attributes(to_remote_path(link), true);

    BOOST_CHECK_EQUAL(attrs.type(), file_attributes::normal_file);
}

BOOST_AUTO_TEST_CASE( attributes_broken_link )
{
    path target = new_file_in_sandbox();
    path link = sandbox() / "link";
    create_symlink(link, target);
    remove(target);

    file_attributes attrs = filesystem().attributes(to_remote_path(link), false);

    BOOST_CHECK_EQUAL(attrs.type(), file_attributes::symbolic_link);

    BOOST_CHECK_THROW(
        filesystem().attributes(to_remote_path(link), true), system_error);
}

BOOST_AUTO_TEST_CASE( default_directory )
{
    path resolved_target = filesystem().canonical_path("");
    BOOST_CHECK(!resolved_target.empty());
}

BOOST_AUTO_TEST_CASE( remove_nothing )
{
    path target = "gibberish";

    bool already_existed = filesystem().remove(to_remote_path(target));

    BOOST_CHECK(!exists(target));
    BOOST_CHECK(!already_existed);
}

BOOST_AUTO_TEST_CASE( remove_file )
{
    path target = new_file_in_sandbox();

    bool already_existed = filesystem().remove(to_remote_path(target));

    BOOST_CHECK(!exists(target));
    BOOST_CHECK(already_existed);
}

BOOST_AUTO_TEST_CASE( remove_empty_dir )
{
    path target = new_directory_in_sandbox();

    bool already_existed = filesystem().remove(to_remote_path(target));

    BOOST_CHECK(!exists(target));
    BOOST_CHECK(already_existed);
}

BOOST_AUTO_TEST_CASE( remove_non_empty_dir )
{
    path target = new_directory_in_sandbox();
    create_directory(target / "bob");

    BOOST_CHECK_THROW(filesystem().remove(to_remote_path(target)), system_error);
    BOOST_CHECK(exists(target));
}

BOOST_AUTO_TEST_CASE( remove_link )
{
    path target = new_file_in_sandbox();
    path link = sandbox() / "link";
    create_symlink(link, target);

    bool already_existed = filesystem().remove(to_remote_path(link));

    BOOST_CHECK(!exists(link));
    BOOST_CHECK(exists(target)); // should only delete the link
    BOOST_CHECK(already_existed);
}

BOOST_AUTO_TEST_CASE( remove_nothing_recursive )
{
    path target = "gibberish";

    uintmax_t count = filesystem().remove_all(to_remote_path(target));

    BOOST_CHECK(!exists(target));
    BOOST_CHECK_EQUAL(count, 0U);
}

BOOST_AUTO_TEST_CASE( remove_file_recursive )
{
    path target = new_file_in_sandbox();

    uintmax_t count = filesystem().remove_all(to_remote_path(target));

    BOOST_CHECK(!exists(target));
    BOOST_CHECK_EQUAL(count, 1U);
}

BOOST_AUTO_TEST_CASE( remove_empty_dir_recursive )
{
    path target = new_directory_in_sandbox();

    uintmax_t count = filesystem().remove_all(to_remote_path(target));

    BOOST_CHECK(!exists(target));
    BOOST_CHECK_EQUAL(count, 1U);
}

BOOST_AUTO_TEST_CASE( remove_non_empty_dir_recursive )
{
    path target = new_directory_in_sandbox();
    create_directory(target / "bob");
    ofstream(target / "bob" / "sally");
    ofstream(target / "alice"); // Either side of bob alphabetically
    ofstream(target / "jim");

    uintmax_t count = filesystem().remove_all(to_remote_path(target));

    BOOST_CHECK(!exists(target));
    BOOST_CHECK_EQUAL(count, 5U);
}

BOOST_AUTO_TEST_CASE( remove_link_recursive )
{
    path target = new_directory_in_sandbox();
    create_directory(target / "bob");
    path link = sandbox() / "link";
    create_symlink(link, target);

    uintmax_t count = filesystem().remove_all(to_remote_path(link));

    BOOST_CHECK(!exists(link));
    BOOST_CHECK(exists(target)); // should only delete the link
    BOOST_CHECK(exists(target / "bob")); // should only delete the link
    BOOST_CHECK_EQUAL(count, 1U);
}

BOOST_AUTO_TEST_CASE( rename_file )
{
    path test_file = new_file_in_sandbox();
    path target = sandbox() / "target";

    filesystem().rename(
        to_remote_path(test_file), to_remote_path(target),
        overwrite_behaviour::prevent_overwrite);
    BOOST_CHECK(!exists(test_file));
    BOOST_CHECK(exists(target));
}

BOOST_AUTO_TEST_CASE( rename_file_obstacle_no_overwrite )
{
    path test_file = new_file_in_sandbox();

    path target = new_file_in_sandbox("target");
    
    BOOST_CHECK_THROW(
        filesystem().rename(
            to_remote_path(test_file), to_remote_path(target),
            overwrite_behaviour::prevent_overwrite), system_error);
    BOOST_CHECK(exists(test_file));
    BOOST_CHECK(exists(target));
}

BOOST_AUTO_TEST_CASE( rename_file_obstacle_allow_overwrite )
{
    path test_file = new_file_in_sandbox();

    path target = new_file_in_sandbox("target");

    // Using OpenSSH server which only supports SFTP 3 (no overwrite) so
    // failure expected
    BOOST_CHECK_THROW(
        filesystem().rename(
            to_remote_path(test_file), to_remote_path(target),
            overwrite_behaviour::allow_overwrite), system_error);
    BOOST_CHECK(exists(test_file));
    BOOST_CHECK(exists(target));
}

BOOST_AUTO_TEST_CASE( rename_file_obstacle_atomic_overwrite )
{
    path test_file = new_file_in_sandbox();

    path target = new_file_in_sandbox("target");

    // Using OpenSSH server which only supports SFTP 3 (no overwrite) so
    // failure expected
    BOOST_CHECK_THROW(
        filesystem().rename(
            to_remote_path(test_file), to_remote_path(target),
            overwrite_behaviour::atomic_overwrite),
        system_error);
    BOOST_CHECK(exists(test_file));
    BOOST_CHECK(exists(target));
}

BOOST_AUTO_TEST_CASE( exists_true )
{
    path test_file = new_file_in_sandbox();

    BOOST_CHECK(exists(filesystem(), to_remote_path(test_file)));
}

BOOST_AUTO_TEST_CASE( exists_false )
{
    path test_file = sandbox() / "I do not exist";
    BOOST_CHECK(!exists(filesystem(), to_remote_path(test_file)));
}

BOOST_AUTO_TEST_CASE( new_directory )
{
    path target = new_directory_in_sandbox();
    remove(target);

    BOOST_CHECK(filesystem().create_directory(to_remote_path(target)));
    BOOST_CHECK(exists(target));
    BOOST_CHECK(is_directory(target));
}

BOOST_AUTO_TEST_CASE( new_directory_already_there )
{
    path target = new_directory_in_sandbox();

    BOOST_CHECK(!filesystem().create_directory(to_remote_path(target)));
    BOOST_CHECK(exists(target));
    BOOST_CHECK(is_directory(target));
}

BOOST_AUTO_TEST_CASE( new_directory_already_there_wrong_type )
{
    path target = new_file_in_sandbox();

    BOOST_CHECK_THROW(
        filesystem().create_directory(to_remote_path(target)),
        system_error);
    BOOST_CHECK(exists(target));
    BOOST_CHECK(!is_directory(target));
}

BOOST_AUTO_TEST_SUITE_END();

BOOST_AUTO_TEST_SUITE_END();
