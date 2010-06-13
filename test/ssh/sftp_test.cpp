/**
    @file

    Tests for SFTP subsytem.

    @if licence

    Copyright (C) 2010  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include <boost/foreach.hpp> // BOOST_FOREACH
#include <boost/test/unit_test.hpp>

using ssh::exception::ssh_error;
using ssh::session::session;
using ssh::sftp::sftp_channel;
using ssh::sftp::sftp_file;
using ssh::sftp::directory_iterator;

using test::ssh::sandbox_fixture;
using test::ssh::session_fixture;

class sftp_fixture : public session_fixture, public sandbox_fixture
{
public:
	sftp_channel channel()
	{
		session s = test_session();
		s.authenticate_by_key(
			user(), public_key_path(), private_key_path(), "");
		sftp_channel channel(s);

		return channel;
	}
};

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
	boost::filesystem::path test_file = new_file_in_sandbox();

	directory_iterator it(channel(), to_remote_path(sandbox()));

	sftp_file file = *it;
	BOOST_CHECK(file.name() == ".");
	BOOST_CHECK(it->name() == ".");
	BOOST_CHECK_GT(it->long_entry().size(), 0U);

	it++;
	
	BOOST_CHECK((*it).name() == "..");
	file = *it;
	BOOST_CHECK(file.name() == "..");

	it++;

	BOOST_CHECK(it->name() == test_file.filename());

	it++;

	BOOST_CHECK(it == directory_iterator());
}

BOOST_AUTO_TEST_SUITE_END();
