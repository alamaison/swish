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

#include <ssh/sftp.hpp> // test subject

#include <boost/bind.hpp> // bind
#include <boost/cstdint.hpp> // uintmax_t
#include <boost/filesystem/fstream.hpp> // ofstream
#include <boost/foreach.hpp> // BOOST_FOREACH
#include <boost/test/unit_test.hpp>

#include <algorithm> // find
#include <string>

using ssh::session;
using ssh::ssh_error;
using ssh::sftp::file_attributes;
using ssh::sftp::sftp_channel;
using ssh::sftp::sftp_error;
using ssh::sftp::sftp_file;
using ssh::sftp::directory_iterator;
using ssh::sftp::overwrite_behaviour;

using boost::bind;
using boost::filesystem::ofstream;
using boost::filesystem::path;
using boost::uintmax_t;

using test::ssh::sandbox_fixture;
using test::ssh::session_fixture;

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
    sftp_channel channel()
    {
        session s = test_session();
        s.authenticate_by_key_files(
            user(), public_key_path(), private_key_path(), "");
        sftp_channel channel(s);

        return channel;
    }

    sftp_file find_file_in_remote_sandbox(const string& filename)
    {
        // Search for path in directory because we need the 'remote' form of it,
        // not the local filesystem version.
        directory_iterator it(channel(), to_remote_path(sandbox()));
        directory_iterator pos = find_if(
            it, directory_iterator(), bind(filename_matches, filename, _1));
        BOOST_REQUIRE(pos != directory_iterator());

        return *pos;
    }

    void create_symlink(path link, path target)
    {
        // Passing arguments in the wrong order to work around OpenSSH bug
        ssh::sftp::create_symlink(
            channel(), to_remote_path(target), to_remote_path(link));
    }
};

}

BOOST_FIXTURE_TEST_SUITE(sftp_tests, sftp_fixture)

/**
 * List an empty directory.
 *
 * Will contain . and ..
 */
BOOST_AUTO_TEST_CASE( empty_dir )
{
    directory_iterator it(channel(), to_remote_path(sandbox()));
    it++;
    it++;
    BOOST_CHECK(it == directory_iterator());
}

/**
 * List a directory that doesn't exist.  Must throw.
 */
BOOST_AUTO_TEST_CASE( missing_dir )
{
    sftp_channel c = channel();
    BOOST_CHECK_THROW(directory_iterator(c, "/i/dont/exist"), ssh_error);
}

/**
 * List an empty directory.
 *
 * Will contain . and ..
 */
BOOST_AUTO_TEST_CASE( dir_with_one_file )
{
    path test_file = new_file_in_sandbox();

    directory_iterator it(channel(), to_remote_path(sandbox()));

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

    BOOST_CHECK(it == directory_iterator());
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
        resolve_link_target(channel(), find_file_in_remote_sandbox("link"));
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
        canonical_path(channel(), find_file_in_remote_sandbox("link"));
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
        canonical_path(channel(), find_file_in_remote_sandbox("link2"));
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
        resolve_link_target(channel(), find_file_in_remote_sandbox("link2"));
    BOOST_CHECK_EQUAL(resolved_target, remote_target);
}

BOOST_AUTO_TEST_CASE( attributes_file )
{
    path subject = new_file_in_sandbox();

    file_attributes attrs = attributes(
        channel(), to_remote_path(subject), false);

    BOOST_CHECK_EQUAL(attrs.type(), file_attributes::normal_file);

    attrs = attributes(channel(), to_remote_path(subject), true);

    BOOST_CHECK_EQUAL(attrs.type(), file_attributes::normal_file);
}

BOOST_AUTO_TEST_CASE( attributes_directory )
{
    path subject = sandbox() / "testdir";
    create_directory(subject);

    file_attributes attrs = attributes(
        channel(), to_remote_path(subject), false);

    BOOST_CHECK_EQUAL(attrs.type(), file_attributes::directory);

    attrs = attributes(channel(), to_remote_path(subject), true);

    BOOST_CHECK_EQUAL(attrs.type(), file_attributes::directory);
}

BOOST_AUTO_TEST_CASE( attributes_link )
{
    path target = new_file_in_sandbox();
    path link = sandbox() / "link";
    create_symlink(link, target);

    file_attributes attrs = attributes(channel(), to_remote_path(link), false);

    BOOST_CHECK_EQUAL(attrs.type(), file_attributes::symbolic_link);

    attrs = attributes(channel(), to_remote_path(link), true);

    BOOST_CHECK_EQUAL(attrs.type(), file_attributes::normal_file);
}

BOOST_AUTO_TEST_CASE( attributes_double_link )
{
    path target = new_file_in_sandbox();
    path middle_link = sandbox() / "link1";
    path link = sandbox() / "link2";
    create_symlink(middle_link, target);
    create_symlink(link, middle_link);

    file_attributes attrs = attributes(channel(), to_remote_path(link), false);

    BOOST_CHECK_EQUAL(attrs.type(), file_attributes::symbolic_link);

    attrs = attributes(channel(), to_remote_path(link), true);

    BOOST_CHECK_EQUAL(attrs.type(), file_attributes::normal_file);
}

BOOST_AUTO_TEST_CASE( attributes_broken_link )
{
    path target = new_file_in_sandbox();
    path link = sandbox() / "link";
    create_symlink(link, target);
    remove(target);

    file_attributes attrs = attributes(channel(), to_remote_path(link), false);

    BOOST_CHECK_EQUAL(attrs.type(), file_attributes::symbolic_link);

    BOOST_CHECK_THROW(
        attributes(channel(), to_remote_path(link), true), sftp_error);
}

BOOST_AUTO_TEST_CASE( default_directory )
{
    path resolved_target = canonical_path(channel(), "");
    BOOST_CHECK(!resolved_target.empty());
}

BOOST_AUTO_TEST_CASE( remove_nothing )
{
    path target = "gibberish";

    bool already_existed = remove(channel(), to_remote_path(target));

    BOOST_CHECK(!exists(target));
    BOOST_CHECK(!already_existed);
}

BOOST_AUTO_TEST_CASE( remove_file )
{
    path target = new_file_in_sandbox();

    bool already_existed = remove(channel(), to_remote_path(target));

    BOOST_CHECK(!exists(target));
    BOOST_CHECK(already_existed);
}

BOOST_AUTO_TEST_CASE( remove_empty_dir )
{
    path target = new_directory_in_sandbox();

    bool already_existed = remove(channel(), to_remote_path(target));

    BOOST_CHECK(!exists(target));
    BOOST_CHECK(already_existed);
}

BOOST_AUTO_TEST_CASE( remove_non_empty_dir )
{
    path target = new_directory_in_sandbox();
    create_directory(target / "bob");

    BOOST_CHECK_THROW(remove(channel(), to_remote_path(target)), sftp_error);
}

BOOST_AUTO_TEST_CASE( remove_link )
{
    path target = new_file_in_sandbox();
    path link = sandbox() / "link";
    create_symlink(link, target);

    bool already_existed = remove(channel(), to_remote_path(link));

    BOOST_CHECK(!exists(link));
    BOOST_CHECK(exists(target)); // should only delete the link
    BOOST_CHECK(already_existed);
}

BOOST_AUTO_TEST_CASE( remove_nothing_recursive )
{
    path target = "gibberish";

    uintmax_t count = remove_all(channel(), to_remote_path(target));

    BOOST_CHECK(!exists(target));
    BOOST_CHECK_EQUAL(count, 0U);
}

BOOST_AUTO_TEST_CASE( remove_file_recursive )
{
    path target = new_file_in_sandbox();

    uintmax_t count = remove_all(channel(), to_remote_path(target));

    BOOST_CHECK(!exists(target));
    BOOST_CHECK_EQUAL(count, 1U);
}

BOOST_AUTO_TEST_CASE( remove_empty_dir_recursive )
{
    path target = new_directory_in_sandbox();

    uintmax_t count = remove_all(channel(), to_remote_path(target));

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

    uintmax_t count = remove_all(channel(), to_remote_path(target));

    BOOST_CHECK(!exists(target));
    BOOST_CHECK_EQUAL(count, 5U);
}

BOOST_AUTO_TEST_CASE( remove_link_recursive )
{
    path target = new_directory_in_sandbox();
    create_directory(target / "bob");
    path link = sandbox() / "link";
    create_symlink(link, target);

    uintmax_t count = remove_all(channel(), to_remote_path(link));

    BOOST_CHECK(!exists(link));
    BOOST_CHECK(exists(target)); // should only delete the link
    BOOST_CHECK(exists(target / "bob")); // should only delete the link
    BOOST_CHECK_EQUAL(count, 1U);
}

BOOST_AUTO_TEST_CASE( rename_file )
{
    path test_file = new_file_in_sandbox();
    path target = sandbox() / "target";

    rename(
        channel(), to_remote_path(test_file), to_remote_path(target),
        overwrite_behaviour::prevent_overwrite);
    BOOST_CHECK(!exists(test_file));
    BOOST_CHECK(exists(target));
}

BOOST_AUTO_TEST_CASE( rename_file_obstacle_no_overwrite )
{
    path test_file = new_file_in_sandbox();

    path target = new_file_in_sandbox("target");
    
    BOOST_CHECK_THROW(
        rename(
            channel(), to_remote_path(test_file), to_remote_path(target),
            overwrite_behaviour::prevent_overwrite), sftp_error);
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
        rename(
            channel(), to_remote_path(test_file), to_remote_path(target),
            overwrite_behaviour::allow_overwrite), sftp_error);
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
        rename(
        channel(), to_remote_path(test_file), to_remote_path(target),
        overwrite_behaviour::atomic_overwrite), sftp_error);
    BOOST_CHECK(exists(test_file));
    BOOST_CHECK(exists(target));
}

BOOST_AUTO_TEST_CASE( exists_true )
{
    path test_file = new_file_in_sandbox();

    BOOST_CHECK(exists(channel(), to_remote_path(test_file)));
}

BOOST_AUTO_TEST_CASE( exists_false )
{
    path test_file = sandbox() / "I do not exist";
    BOOST_CHECK(!exists(channel(), to_remote_path(test_file)));
}

BOOST_AUTO_TEST_SUITE_END();
