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

#include <ssh/filesystem.hpp> // test subject
#include <ssh/stream.hpp>

#include <boost/move/move.hpp>
#include <boost/system/system_error.hpp>
#include <boost/test/unit_test.hpp>

#include <memory> // auto_ptr

using ssh::filesystem::ofstream;
using ssh::filesystem::sftp_filesystem;
using ssh::session;

using boost::move;
using boost::system::system_error;

using test::ssh::session_fixture;

using std::auto_ptr;

BOOST_AUTO_TEST_SUITE(filesystem_construction_test)

BOOST_FIXTURE_TEST_CASE(construct_fail, session_fixture)
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
BOOST_FIXTURE_TEST_CASE(move_session_after_connecting_filesystem,
                        session_fixture)
{
    session& s = test_session();
    s.authenticate_by_key_files(user(), public_key_path(), private_key_path(),
                                "");

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
BOOST_FIXTURE_TEST_CASE(move_session_with_live_filesystem_connection,
                        session_fixture)
{
    session& s = test_session();
    s.authenticate_by_key_files(user(), public_key_path(), private_key_path(),
                                "");

    sftp_filesystem fs = s.connect_to_filesystem();
    session s2(move(s));

    // The rules are that the last session must outlive the last FS so moving
    // FS to inner scope ensures this
    sftp_filesystem(move(fs));
}

// This is the third part of the session-movement tests. It strengthens the
// requirements a bit more to ensure the filesystem is not just valid for
// destruction but also still functions as a filesystem connection.
BOOST_FIXTURE_TEST_CASE(moving_session_leaves_working_filesystem,
                        session_fixture)
{
    session& s = test_session();
    s.authenticate_by_key_files(user(), public_key_path(), private_key_path(),
                                "");

    sftp_filesystem fs = s.connect_to_filesystem();
    session s2(move(s));

    // The rules are that the last session must outlive the last FS so moving
    // FS to inner scope ensures this
    sftp_filesystem fs2(move(fs));
    ofstream(fs2, "/tmp/bob.txt").close();

    BOOST_CHECK(exists(fs2, "/tmp/bob.txt"));
}

BOOST_FIXTURE_TEST_CASE(swap_session_with_live_filesystem_connection,
                        session_fixture)
{
    // Both sockets must outlive both session objects
    auto_ptr<boost::asio::ip::tcp::socket> socket1(connect_additional_socket());
    auto_ptr<boost::asio::ip::tcp::socket> socket2(connect_additional_socket());

    session s(socket1->native());
    session t(socket2->native());
    s.authenticate_by_key_files(user(), public_key_path(), private_key_path(),
                                "");
    t.authenticate_by_key_files(user(), public_key_path(), private_key_path(),
                                "");

    sftp_filesystem fs = s.connect_to_filesystem();

    boost::swap(t, s);
}

BOOST_AUTO_TEST_SUITE_END();
