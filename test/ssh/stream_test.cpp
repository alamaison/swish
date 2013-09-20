/**
    @file

    Tests for SFTP streams.

    @if license

    Copyright (C) 2013  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include <ssh/stream.hpp> // test subject

#include <boost/filesystem/fstream.hpp> // ofstream
#include <boost/test/unit_test.hpp>

#include <string>
#include <vector>

using ssh::session;
using ssh::sftp::openmode;
using ssh::sftp::sftp_channel;
using ssh::sftp::sftp_error;

using boost::filesystem::path;

using test::ssh::sandbox_fixture;
using test::ssh::session_fixture;

using std::string;
using std::vector;

namespace {

class stream_fixture : public session_fixture, public sandbox_fixture
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

    using sandbox_fixture::new_file_in_sandbox;

    path new_file_in_sandbox(const string& data)
    {
        path p = new_file_in_sandbox();
        boost::filesystem::ofstream s(p);

        s.write(data.data(), data.size());

        return p;
    }
};

}

BOOST_FIXTURE_TEST_SUITE(stream_tests, stream_fixture)

BOOST_FIXTURE_TEST_SUITE(istream_tests, stream_fixture)

BOOST_AUTO_TEST_CASE( input_stream_multiple_streams )
{
    path target1 = new_file_in_sandbox();
    path target2 = new_file_in_sandbox();

    sftp_channel chan = channel();

    ssh::sftp::ifstream s1(chan, to_remote_path(target1));
    ssh::sftp::ifstream s2(chan, to_remote_path(target2));
}

BOOST_AUTO_TEST_CASE( input_stream_multiple_streams_to_same_file )
{
    path target = new_file_in_sandbox();

    sftp_channel chan = channel();

    ssh::sftp::ifstream s1(chan, to_remote_path(target));
    ssh::sftp::ifstream s2(chan, to_remote_path(target));
}

BOOST_AUTO_TEST_CASE( input_stream_readable )
{
    path target = new_file_in_sandbox("gobbledy gook");

    ssh::sftp::ifstream s(channel(), to_remote_path(target));

    string bob;

    BOOST_CHECK(s >> bob);
    BOOST_CHECK_EQUAL(bob, "gobbledy");
    BOOST_CHECK(s >> bob);
    BOOST_CHECK_EQUAL(bob, "gook");
    BOOST_CHECK(!(s >> bob));
    BOOST_CHECK(s.eof());
}

BOOST_AUTO_TEST_CASE( input_stream_readable_binary_data )
{
    string expected_data("gobbledy gook\0after-null\x12\11", 26);

    path target = new_file_in_sandbox(expected_data);

    sftp_channel chan = channel();

    ssh::sftp::ifstream remote_stream(chan, to_remote_path(target));

    string bob;

    vector<char> buffer(expected_data.size());
    BOOST_CHECK(remote_stream.read(&buffer[0], buffer.size()));

    BOOST_CHECK_EQUAL_COLLECTIONS(
        buffer.begin(), buffer.end(),
        expected_data.begin(), expected_data.end());
}

BOOST_AUTO_TEST_CASE( input_stream_readable_binary_data_stream_op )
{
    string expected_data("gobbledy gook\0after-null\x12\x11", 26);

    path target = new_file_in_sandbox(expected_data);

    sftp_channel chan = channel();

    ssh::sftp::ifstream remote_stream(chan, to_remote_path(target));

    string bob;

    BOOST_CHECK(remote_stream >> bob);
    BOOST_CHECK_EQUAL(bob, "gobbledy");

    BOOST_CHECK(remote_stream >> bob);
    const char* gn = "gook\0after-null\x12\x11";
    BOOST_CHECK_EQUAL_COLLECTIONS(bob.begin(), bob.end(), gn, gn+17);
    BOOST_CHECK(!(remote_stream >> bob));
    BOOST_CHECK(remote_stream.eof());
}

BOOST_AUTO_TEST_CASE( input_stream_does_not_create_by_default )
{
    path target = new_file_in_sandbox();
    remove(target);

    BOOST_CHECK_THROW(
        ssh::sftp::ifstream(channel(), to_remote_path(target)), sftp_error);
    BOOST_CHECK(!exists(target));
}

BOOST_AUTO_TEST_CASE( input_stream_in_flag_does_not_create )
{
    path target = new_file_in_sandbox();
    remove(target);

    BOOST_CHECK_THROW(
        ssh::sftp::ifstream(
            channel(), to_remote_path(target), openmode::in), sftp_error);
    BOOST_CHECK(!exists(target));
}

BOOST_AUTO_TEST_CASE( input_stream_out_flag_does_not_create )
{
    // Because ifstream forces in as well as out an in supresses creation

    path target = new_file_in_sandbox();
    remove(target);

    BOOST_CHECK_THROW(
        ssh::sftp::ifstream(
        channel(), to_remote_path(target), openmode::out), sftp_error);
    BOOST_CHECK(!exists(target));
}

BOOST_AUTO_TEST_CASE( input_stream_out_trunc_flag_creates )
{
    path target = new_file_in_sandbox();
    remove(target);

    ssh::sftp::ifstream remote_stream(
        channel(), to_remote_path(target),
        openmode::out | openmode::trunc);
    BOOST_CHECK(exists(target));
}

BOOST_AUTO_TEST_CASE( input_stream_out_trunc_nocreate_flag_fails )
{
    path target = new_file_in_sandbox();
    remove(target);

    BOOST_CHECK_THROW(
        ssh::sftp::ifstream(
            channel(), to_remote_path(target),
            openmode::out | openmode::trunc | openmode::nocreate),
        sftp_error);
    BOOST_CHECK(!exists(target));
}

BOOST_AUTO_TEST_CASE( input_stream_out_trunc_noreplace_flag_fails )
{
    path target = new_file_in_sandbox();

    BOOST_CHECK_THROW(
        ssh::sftp::ifstream(
            channel(), to_remote_path(target),
            openmode::out | openmode::trunc | openmode::noreplace),
        sftp_error);
    BOOST_CHECK(exists(target));
}

BOOST_AUTO_TEST_SUITE_END();

BOOST_FIXTURE_TEST_SUITE(ofstream_tests, stream_fixture)

BOOST_AUTO_TEST_CASE( output_stream_multiple_streams )
{
    path target1 = new_file_in_sandbox();
    path target2 = new_file_in_sandbox();

    sftp_channel chan = channel();

    ssh::sftp::ofstream s1(chan, to_remote_path(target1));
    ssh::sftp::ofstream s2(chan, to_remote_path(target2));
}

BOOST_AUTO_TEST_CASE( output_stream_multiple_streams_to_same_file )
{
    path target = new_file_in_sandbox();

    sftp_channel chan = channel();

    ssh::sftp::ofstream s1(chan, to_remote_path(target));
    ssh::sftp::ofstream s2(chan, to_remote_path(target));
}

BOOST_AUTO_TEST_CASE( output_stream_writeable )
{
    path target = new_file_in_sandbox();

    {
        ssh::sftp::ofstream remote_stream(channel(), to_remote_path(target));

        remote_stream << "gobbledy gook";
    }

    boost::filesystem::ifstream local_stream(target);

    string bob;

    BOOST_CHECK(local_stream >> bob);
    BOOST_CHECK_EQUAL(bob, "gobbledy");

    BOOST_CHECK(local_stream >> bob);
    BOOST_CHECK_EQUAL(bob, "gook");

    BOOST_CHECK(!(local_stream >> bob));
    BOOST_CHECK(local_stream.eof());
}

BOOST_AUTO_TEST_CASE( ouput_stream_write_binary_data )
{
    string data("gobbledy gook\0after-null\x12\x11", 26);

    path target = new_file_in_sandbox();

    sftp_channel chan = channel();

    ssh::sftp::ofstream remote_stream(chan, to_remote_path(target));
    BOOST_CHECK(remote_stream.write(data.data(), data.size()));
    remote_stream.flush();

    boost::filesystem::ifstream local_stream(target);

    string bob;

    vector<char> buffer(data.size());
    BOOST_CHECK(local_stream.read(&buffer[0], buffer.size()));

    BOOST_CHECK_EQUAL_COLLECTIONS(
        buffer.begin(), buffer.end(), data.begin(), data.end());

    BOOST_CHECK(!local_stream.read(&buffer[0], buffer.size()));
    BOOST_CHECK(local_stream.eof());
}

BOOST_AUTO_TEST_CASE( ouput_stream_write_binary_data_stream_op )
{
    string data("gobbledy gook\0after-null\x12\x11", 26);

    path target = new_file_in_sandbox();

    sftp_channel chan = channel();

    ssh::sftp::ofstream remote_stream(chan, to_remote_path(target));
    BOOST_CHECK(remote_stream << data);
    remote_stream.flush();

    boost::filesystem::ifstream local_stream(target);

    string bob;

    vector<char> buffer(data.size());
    BOOST_CHECK(local_stream.read(&buffer[0], buffer.size()));

    BOOST_CHECK_EQUAL_COLLECTIONS(
        buffer.begin(), buffer.end(), data.begin(), data.end());

    BOOST_CHECK(!local_stream.read(&buffer[0], buffer.size()));
    BOOST_CHECK(local_stream.eof());
}


BOOST_AUTO_TEST_CASE( output_stream_creates_by_default )
{
    path target = new_file_in_sandbox();
    remove(target);

    ssh::sftp::ofstream remote_stream(channel(), to_remote_path(target));
    BOOST_CHECK(exists(target));
}

BOOST_AUTO_TEST_CASE( output_stream_nocreate_flag )
{
    path target = new_file_in_sandbox();

    ssh::sftp::ofstream(
        channel(), to_remote_path(target), openmode::nocreate);
    BOOST_CHECK(exists(target));
}

BOOST_AUTO_TEST_CASE( output_stream_nocreate_flag_fails )
{
    path target = new_file_in_sandbox();
    remove(target);

    BOOST_CHECK_THROW(
        ssh::sftp::ofstream(
            channel(), to_remote_path(target), openmode::nocreate),
        sftp_error);
    BOOST_CHECK(!exists(target));
}

BOOST_AUTO_TEST_CASE( output_stream_noreplace_flag )
{
    path target = new_file_in_sandbox();
    remove(target);

    ssh::sftp::ofstream(
        channel(), to_remote_path(target), openmode::noreplace);
    BOOST_CHECK(exists(target));
}

BOOST_AUTO_TEST_CASE( output_stream_noreplace_flag_fails )
{
    path target = new_file_in_sandbox();

    BOOST_CHECK_THROW(
        ssh::sftp::ofstream(
            channel(), to_remote_path(target), openmode::noreplace),
        sftp_error);
    BOOST_CHECK(exists(target));
}

BOOST_AUTO_TEST_CASE( output_stream_out_flag_creates )
{
    path target = new_file_in_sandbox();
    remove(target);

    ssh::sftp::ofstream remote_stream(
        channel(), to_remote_path(target), openmode::out);
    BOOST_CHECK(exists(target));
}

BOOST_AUTO_TEST_CASE( output_stream_out_flag_truncates )
{
    path target = new_file_in_sandbox("gobbledy gook");

    {
        ssh::sftp::ofstream remote_stream(
            channel(), to_remote_path(target), openmode::out);
        BOOST_CHECK(exists(target));

        BOOST_CHECK(remote_stream << "abcdef");
    }

    boost::filesystem::ifstream local_stream(target);

    string bob;

    BOOST_CHECK(local_stream >> bob);
    BOOST_CHECK_EQUAL(bob, "abcdef");

    BOOST_CHECK(!(local_stream >> bob));
    BOOST_CHECK(local_stream.eof());
}

BOOST_AUTO_TEST_CASE( output_stream_out_nocreate_flag )
{
    path target = new_file_in_sandbox();

    ssh::sftp::ofstream remote_stream(
        channel(), to_remote_path(target), openmode::out | openmode::nocreate);

    BOOST_CHECK(remote_stream << "abcdef");
}

BOOST_AUTO_TEST_CASE( output_stream_out_nocreate_flag_fails )
{
    path target = new_file_in_sandbox();
    remove(target);

    BOOST_CHECK_THROW(
        ssh::sftp::ofstream(
            channel(), to_remote_path(target),
            openmode::out | openmode::nocreate),
        sftp_error);
    BOOST_CHECK(!exists(target));
}

BOOST_AUTO_TEST_CASE( output_stream_out_noreplace_flag )
{
    path target = new_file_in_sandbox();
    remove(target);

    ssh::sftp::ofstream remote_stream(
        channel(), to_remote_path(target), openmode::out | openmode::noreplace);

    BOOST_CHECK(exists(target));
    BOOST_CHECK(remote_stream << "abcdef");
}

BOOST_AUTO_TEST_CASE( output_stream_out_noreplace_flag_fails )
{
    path target = new_file_in_sandbox();

    BOOST_CHECK_THROW(
        ssh::sftp::ofstream(
            channel(), to_remote_path(target),
            openmode::out | openmode::noreplace),
        sftp_error);
    BOOST_CHECK(exists(target));
}

BOOST_AUTO_TEST_CASE( output_stream_in_flag_does_not_create )
{
    // In flag suppresses creation.  Matches standard lib ofstream.

    path target = new_file_in_sandbox();
    remove(target);

    BOOST_CHECK_THROW(
        ssh::sftp::ofstream(
            channel(), to_remote_path(target), openmode::in),
        sftp_error);
    BOOST_CHECK(!exists(target));
}

BOOST_AUTO_TEST_CASE( output_stream_in_out_does_not_create )
{
    path target = new_file_in_sandbox();
    remove(target);

    BOOST_CHECK_THROW(
        ssh::sftp::ofstream(
            channel(), to_remote_path(target),
            openmode::in | openmode::out), sftp_error);

    BOOST_CHECK(!exists(target));
}

BOOST_AUTO_TEST_CASE( output_stream_in_out_flag_updates )
{
    // Unlike the out flag for output-only streams, which truncates, the
    // out flag on an input stream leaves the existing contents because the
    // input stream forces the in flag and in|out means update existing

    path target = new_file_in_sandbox("gobbledy gook");

    {
        ssh::sftp::ofstream remote_stream(
            channel(), to_remote_path(target),
            openmode::in | openmode::out);
        BOOST_CHECK(exists(target));

        BOOST_CHECK(remote_stream << "abcdef");
    }

    boost::filesystem::ifstream local_stream(target);

    string bob;

    BOOST_CHECK(local_stream >> bob);
    BOOST_CHECK_EQUAL(bob, "abcdefdy");

    BOOST_CHECK(local_stream >> bob);
    BOOST_CHECK_EQUAL(bob, "gook");

    BOOST_CHECK(!(local_stream >> bob));
    BOOST_CHECK(local_stream.eof());
}

BOOST_AUTO_TEST_CASE( output_stream_out_trunc_flag_creates )
{
    path target = new_file_in_sandbox();
    remove(target);

    ssh::sftp::ofstream remote_stream(
        channel(), to_remote_path(target),
        openmode::out | openmode::trunc);
    BOOST_CHECK(exists(target));
}

BOOST_AUTO_TEST_CASE( output_stream_out_trunc_nocreate_flag )
{
    path target = new_file_in_sandbox();

    ssh::sftp::ofstream remote_stream(
        channel(), to_remote_path(target),
        openmode::out | openmode::trunc | openmode::nocreate);
    BOOST_CHECK(exists(target));
}

BOOST_AUTO_TEST_CASE( output_stream_out_trunc_nocreate_flag_fails )
{
    path target = new_file_in_sandbox();
    remove(target);

    BOOST_CHECK_THROW(
        ssh::sftp::ofstream remote_stream(
            channel(), to_remote_path(target),
            openmode::out | openmode::trunc | openmode::nocreate),
        sftp_error);
    BOOST_CHECK(!exists(target));
}

BOOST_AUTO_TEST_CASE( output_stream_out_trunc_noreplace_flag )
{
    path target = new_file_in_sandbox();
    remove(target);

    ssh::sftp::ofstream remote_stream(
        channel(), to_remote_path(target),
        openmode::out | openmode::trunc | openmode::noreplace);
    BOOST_CHECK(exists(target));
}

BOOST_AUTO_TEST_CASE( output_stream_out_trunc_noreplace_flag_fails )
{
    path target = new_file_in_sandbox();

    BOOST_CHECK_THROW(
        ssh::sftp::ofstream remote_stream(
            channel(), to_remote_path(target),
            openmode::out | openmode::trunc | openmode::noreplace),
        sftp_error);
    BOOST_CHECK(exists(target));
}

BOOST_AUTO_TEST_CASE( output_stream_out_trunc_flag_truncates )
{
    path target = new_file_in_sandbox("gobbledy gook");

    {
        ssh::sftp::ofstream remote_stream(
            channel(), to_remote_path(target),
            openmode::out | openmode::trunc);

        BOOST_CHECK(remote_stream << "abcdef");
    }

    boost::filesystem::ifstream local_stream(target);

    string bob;

    BOOST_CHECK(local_stream >> bob);
    BOOST_CHECK_EQUAL(bob, "abcdef");

    BOOST_CHECK(!(local_stream >> bob));
    BOOST_CHECK(local_stream.eof());
}

BOOST_AUTO_TEST_CASE( output_stream_in_out_trunc_flag_creates )
{
    path target = new_file_in_sandbox();
    remove(target);

    ssh::sftp::ofstream remote_stream(
        channel(), to_remote_path(target),
        openmode::in | openmode::out | openmode::trunc);
    BOOST_CHECK(exists(target));
}

BOOST_AUTO_TEST_CASE( output_stream_in_out_trunc_flag_truncates )
{
    path target = new_file_in_sandbox("gobbledy gook");

    {
        ssh::sftp::ofstream remote_stream(
            channel(), to_remote_path(target),
            openmode::in | openmode::out | openmode::trunc);

        BOOST_CHECK(remote_stream << "abcdef");
    }

    boost::filesystem::ifstream local_stream(target);

    string bob;

    BOOST_CHECK(local_stream >> bob);
    BOOST_CHECK_EQUAL(bob, "abcdef");

    BOOST_CHECK(!(local_stream >> bob));
    BOOST_CHECK(local_stream.eof());
}

BOOST_AUTO_TEST_CASE( output_stream_out_append_flag_creates )
{
    path target = new_file_in_sandbox();
    remove(target);

    ssh::sftp::ofstream remote_stream(
        channel(), to_remote_path(target),
        openmode::out | openmode::app);
    BOOST_CHECK(exists(target));
}

BOOST_AUTO_TEST_CASE( output_stream_out_append_flag_appends )
{
    path target = new_file_in_sandbox("gobbledy gook");

    {
        ssh::sftp::ofstream remote_stream(
            channel(), to_remote_path(target),
            openmode::out | openmode::app);

        BOOST_CHECK(remote_stream << "abcdef");
    }

    boost::filesystem::ifstream local_stream(target);

    string bob;

    /*
    XXX: Should be as follows but OpenSSH doesn't support FXF_APPEND

    BOOST_CHECK(local_stream >> bob);
    BOOST_CHECK_EQUAL(bob, "gobbeldy");

    BOOST_CHECK(local_stream >> bob);
    BOOST_CHECK_EQUAL(bob, "gookabcdef");
    */

    BOOST_CHECK(local_stream >> bob);
    BOOST_CHECK_EQUAL(bob, "abcdefdy");

    BOOST_CHECK(local_stream >> bob);
    BOOST_CHECK_EQUAL(bob, "gook");

    BOOST_CHECK(!(local_stream >> bob));
    BOOST_CHECK(local_stream.eof());
}

BOOST_AUTO_TEST_SUITE_END();

BOOST_AUTO_TEST_SUITE_END();
