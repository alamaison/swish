// Copyright 2013, 2016 Alexander Lamaison

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

#include "sftp_fixture.hpp"

#include <ssh/stream.hpp> // test subject

#include <boost/system/system_error.hpp>
#include <boost/test/unit_test.hpp>

#include <string>
#include <vector>

using ssh::filesystem::ifstream;
using ssh::filesystem::fstream;
using ssh::filesystem::openmode;
using ssh::filesystem::path;
using ssh::filesystem::perms;
using ssh::filesystem::sftp_filesystem;

using boost::system::system_error;

using test::ssh::sftp_fixture;

using std::runtime_error;
using std::string;
using std::vector;

namespace
{

// the large data must fill more than one stream buffer (currently set to
// 32768 (see DEFAULT_BUFFER_SIZE)

string large_data()
{
    string data;
    for (int i = 0; i < 32000; ++i)
    {
        data.push_back('a');
        data.push_back('m');
        data.push_back('z');
    }

    return data;
}

void make_file_read_only(sftp_filesystem& filesystem, const path& target)
{
    permissions(filesystem, target, perms::owner_read);
}
}

BOOST_FIXTURE_TEST_SUITE(io_stream_test, sftp_fixture)

BOOST_AUTO_TEST_CASE(io_stream_multiple_streams)
{
    path target1 = new_file_in_sandbox();
    path target2 = new_file_in_sandbox();

    sftp_filesystem& chan = filesystem();

    fstream s1(chan, target1);
    fstream s2(chan, target2);
}

BOOST_AUTO_TEST_CASE(io_stream_multiple_streams_to_same_file)
{
    path target = new_file_in_sandbox();

    sftp_filesystem& chan = filesystem();

    fstream s1(chan, target);
    fstream s2(chan, target);
}

BOOST_AUTO_TEST_CASE(io_stream_fails_to_open_read_only_by_default)
{
    path target = new_file_in_sandbox();
    make_file_read_only(filesystem(), target);

    BOOST_CHECK_THROW(fstream(filesystem(), target), system_error);
}

BOOST_AUTO_TEST_CASE(io_stream_out_flag_fails_to_open_read_only)
{
    path target = new_file_in_sandbox();
    make_file_read_only(filesystem(), target);

    BOOST_CHECK_THROW(fstream(filesystem(), target, openmode::out),
                      system_error);
}

BOOST_AUTO_TEST_CASE(io_stream_in_out_flag_fails_to_open_read_only)
{
    path target = new_file_in_sandbox();
    make_file_read_only(filesystem(), target);

    BOOST_CHECK_THROW(
        fstream(filesystem(), target, openmode::in | openmode::out),
        system_error);
}

BOOST_AUTO_TEST_CASE(io_stream_in_flag_opens_read_only)
{
    path target = new_file_in_sandbox();
    make_file_read_only(filesystem(), target);

    fstream(filesystem(), target, openmode::in);
}

BOOST_AUTO_TEST_CASE(io_stream_readable)
{
    path target = new_file_in_sandbox_containing_data("gobbledy gook");

    fstream s(filesystem(), target);

    string bob;

    BOOST_CHECK(s >> bob);
    BOOST_CHECK_EQUAL(bob, "gobbledy");
    BOOST_CHECK(s >> bob);
    BOOST_CHECK_EQUAL(bob, "gook");
    BOOST_CHECK(!(s >> bob));
    BOOST_CHECK(s.eof());
}

BOOST_AUTO_TEST_CASE(io_stream_readable_binary_data)
{
    string expected_data("gobbledy gook\0after-null\x12\11", 26);

    path target = new_file_in_sandbox_containing_data(expected_data);

    sftp_filesystem& chan = filesystem();

    fstream io_stream(chan, target);

    string bob;

    vector<char> buffer(expected_data.size());
    BOOST_CHECK(io_stream.read(&buffer[0], buffer.size()));

    BOOST_CHECK_EQUAL_COLLECTIONS(buffer.begin(), buffer.end(),
                                  expected_data.begin(), expected_data.end());
}

BOOST_AUTO_TEST_CASE(io_stream_readable_binary_data_stream_op)
{
    string expected_data("gobbledy gook\0after-null\x12\x11", 26);

    path target = new_file_in_sandbox_containing_data(expected_data);

    sftp_filesystem& chan = filesystem();

    fstream io_stream(chan, target);

    string bob;

    BOOST_CHECK(io_stream >> bob);
    BOOST_CHECK_EQUAL(bob, "gobbledy");

    BOOST_CHECK(io_stream >> bob);
    const char* gn = "gook\0after-null\x12\x11";
    BOOST_CHECK_EQUAL_COLLECTIONS(bob.begin(), bob.end(), gn, gn + 17);
    BOOST_CHECK(!(io_stream >> bob));
    BOOST_CHECK(io_stream.eof());
}

BOOST_AUTO_TEST_CASE(io_stream_writeable)
{
    path target = new_file_in_sandbox();

    {
        fstream io_stream(filesystem(), target);

        io_stream << "gobbledy gook";
    }

    ifstream input_stream(filesystem(), target);

    string bob;

    BOOST_CHECK(input_stream >> bob);
    BOOST_CHECK_EQUAL(bob, "gobbledy");

    BOOST_CHECK(input_stream >> bob);
    BOOST_CHECK_EQUAL(bob, "gook");

    BOOST_CHECK(!(input_stream >> bob));
    BOOST_CHECK(input_stream.eof());
}

BOOST_AUTO_TEST_CASE(io_stream_write_multiple_buffers)
{
    // large enough to span multiple buffers
    string data(large_data());

    path target = new_file_in_sandbox();

    sftp_filesystem& chan = filesystem();

    fstream io_stream(chan, target);
    BOOST_CHECK(io_stream.write(data.data(), data.size()));
    io_stream.flush();

    ifstream input_stream(filesystem(), target);

    string bob;

    vector<char> buffer(data.size());
    BOOST_CHECK(input_stream.read(&buffer[0], buffer.size()));

    BOOST_CHECK_EQUAL_COLLECTIONS(buffer.begin(), buffer.end(), data.begin(),
                                  data.end());

    BOOST_CHECK(!input_stream.read(&buffer[0], buffer.size()));
    BOOST_CHECK(input_stream.eof());
}

// Test with Boost.IOStreams buffer disabled.
// Should call directly to libssh2
BOOST_AUTO_TEST_CASE(io_stream_write_no_buffer)
{
    string data("gobbeldy gook");

    path target = new_file_in_sandbox();

    sftp_filesystem& chan = filesystem();

    fstream io_stream(chan, target, openmode::in | openmode::out, 0);
    BOOST_CHECK(io_stream.write(data.data(), data.size()));

    ifstream input_stream(filesystem(), target);

    string bob;

    vector<char> buffer(data.size());
    BOOST_CHECK(input_stream.read(&buffer[0], buffer.size()));

    BOOST_CHECK_EQUAL_COLLECTIONS(buffer.begin(), buffer.end(), data.begin(),
                                  data.end());

    BOOST_CHECK(!input_stream.read(&buffer[0], buffer.size()));
    BOOST_CHECK(input_stream.eof());
}

// An IO stream may be able to open a read-only file when given the in flag,
// but it should still fail to write to it
BOOST_AUTO_TEST_CASE(io_stream_read_only_write_fails)
{
    path target = new_file_in_sandbox();
    make_file_read_only(filesystem(), target);

    fstream s(filesystem(), target, openmode::in);

    BOOST_CHECK(s << "gobbledy gook");
    BOOST_CHECK(!s.flush()); // Failure happens on the flush

    ifstream input_stream(filesystem(), target);

    string bob;

    BOOST_CHECK(!(input_stream >> bob));
    BOOST_CHECK_EQUAL(bob, string());
    BOOST_CHECK(input_stream.eof());
}

// Flush is not called explicitly so failure will happen in destructor
BOOST_AUTO_TEST_CASE(io_stream_read_only_write_fails_no_flush)
{
    path target = new_file_in_sandbox();
    make_file_read_only(filesystem(), target);

    {
        fstream s(filesystem(), target, openmode::in);

        BOOST_CHECK(s << "gobbledy gook");

        // No explicit flush
    }

    ifstream input_stream(filesystem(), target);

    string bob;

    BOOST_CHECK(!(input_stream >> bob));
    BOOST_CHECK_EQUAL(bob, string());
    BOOST_CHECK(input_stream.eof());
}

BOOST_AUTO_TEST_CASE(io_stream_write_binary_data)
{
    string data("gobbledy gook\0after-null\x12\x11", 26);

    path target = new_file_in_sandbox();

    sftp_filesystem& chan = filesystem();

    fstream io_stream(chan, target);
    BOOST_CHECK(io_stream.write(data.data(), data.size()));
    io_stream.flush();

    ifstream input_stream(filesystem(), target);

    string bob;

    vector<char> buffer(data.size());
    BOOST_CHECK(input_stream.read(&buffer[0], buffer.size()));

    BOOST_CHECK_EQUAL_COLLECTIONS(buffer.begin(), buffer.end(), data.begin(),
                                  data.end());

    BOOST_CHECK(!input_stream.read(&buffer[0], buffer.size()));
    BOOST_CHECK(input_stream.eof());
}

BOOST_AUTO_TEST_CASE(io_stream_write_binary_data_stream_op)
{
    string data("gobbledy gook\0after-null\x12\x11", 26);

    path target = new_file_in_sandbox();

    sftp_filesystem& chan = filesystem();

    fstream io_stream(chan, target);
    BOOST_CHECK(io_stream << data);
    io_stream.flush();

    ifstream input_stream(filesystem(), target);

    string bob;

    vector<char> buffer(data.size());
    BOOST_CHECK(input_stream.read(&buffer[0], buffer.size()));

    BOOST_CHECK_EQUAL_COLLECTIONS(buffer.begin(), buffer.end(), data.begin(),
                                  data.end());

    BOOST_CHECK(!input_stream.read(&buffer[0], buffer.size()));
    BOOST_CHECK(input_stream.eof());
}

BOOST_AUTO_TEST_CASE(io_stream_seek_input_absolute)
{
    path target = new_file_in_sandbox_containing_data("gobbledy gook");

    fstream s(filesystem(), target);
    s.seekg(1, std::ios_base::beg);

    string bob;
    BOOST_CHECK(s >> bob);
    BOOST_CHECK_EQUAL(bob, "obbledy");
}

BOOST_AUTO_TEST_CASE(io_stream_seek_input_relative)
{
    path target = new_file_in_sandbox_containing_data("gobbledy gook");

    fstream s(filesystem(), target);
    s.seekg(1, std::ios_base::cur);
    s.seekg(1, std::ios_base::cur);

    string bob;
    BOOST_CHECK(s >> bob);
    BOOST_CHECK_EQUAL(bob, "bbledy");
}

BOOST_AUTO_TEST_CASE(io_stream_seek_input_end)
{
    path target = new_file_in_sandbox_containing_data("gobbledy gook");

    fstream s(filesystem(), target);
    s.seekg(-3, std::ios_base::end);

    string bob;
    BOOST_CHECK(s >> bob);
    BOOST_CHECK_EQUAL(bob, "ook");
}

BOOST_AUTO_TEST_CASE(io_stream_seek_input_too_far_absolute)
{
    path target = new_file_in_sandbox();

    fstream s(filesystem(), target);
    s.exceptions(std::ios_base::badbit | std::ios_base::eofbit |
                 std::ios_base::failbit);
    s.seekg(1, std::ios_base::beg);

    string bob;
    BOOST_CHECK_THROW(s >> bob, runtime_error);
}

BOOST_AUTO_TEST_CASE(io_stream_seek_input_too_far_relative)
{
    path target = new_file_in_sandbox_containing_data("gobbledy gook");

    fstream s(filesystem(), target);
    s.exceptions(std::ios_base::badbit | std::ios_base::eofbit |
                 std::ios_base::failbit);
    s.seekg(9, std::ios_base::cur);
    s.seekg(4, std::ios_base::cur);

    string bob;
    BOOST_CHECK_THROW(s >> bob, runtime_error);
}

BOOST_AUTO_TEST_CASE(io_stream_seek_output_absolute)
{
    path target = new_file_in_sandbox_containing_data("gobbledy gook");

    fstream s(filesystem(), target);
    s.seekp(1, std::ios_base::beg);

    BOOST_CHECK(s << "r");

    s.flush();

    ifstream input_stream(filesystem(), target);

    string bob;

    BOOST_CHECK(input_stream >> bob);
    BOOST_CHECK_EQUAL(bob, "grbbledy");
}

BOOST_AUTO_TEST_CASE(io_stream_seek_output_relative)
{
    path target = new_file_in_sandbox_containing_data("gobbledy gook");

    fstream s(filesystem(), target);
    s.seekp(1, std::ios_base::cur);
    s.seekp(1, std::ios_base::cur);

    BOOST_CHECK(s << "r");

    s.flush();

    ifstream input_stream(filesystem(), target);

    string bob;

    BOOST_CHECK(input_stream >> bob);
    BOOST_CHECK_EQUAL(bob, "gorbledy");
}

BOOST_AUTO_TEST_CASE(io_stream_seek_output_end)
{
    path target = new_file_in_sandbox_containing_data("gobbledy gook");

    fstream s(filesystem(), target);
    s.seekp(-3, std::ios_base::end);

    BOOST_CHECK(s << "r");

    s.flush();

    ifstream input_stream(filesystem(), target);

    string bob;

    BOOST_CHECK(input_stream >> bob);
    BOOST_CHECK_EQUAL(bob, "gobbledy");
    BOOST_CHECK(input_stream >> bob);
    BOOST_CHECK_EQUAL(bob, "grok");
}

BOOST_AUTO_TEST_CASE(io_stream_seek_interleaved)
{
    path target = new_file_in_sandbox_containing_data("gobbledy gook");

    fstream s(filesystem(), target);
    s.seekp(1, std::ios_base::beg);

    BOOST_CHECK(s << "r");

    s.seekg(2, std::ios_base::cur);

    string bob;

    BOOST_CHECK(s >> bob);
    // not "bbledy" because read and write head are combined
    BOOST_CHECK_EQUAL(bob, "ledy");

    s.seekp(-4, std::ios_base::end);

    BOOST_CHECK(s << "ahh");

    BOOST_CHECK(s >> bob);
    BOOST_CHECK_EQUAL(bob, "k");

    s.flush();

    ifstream input_stream(filesystem(), target);

    BOOST_CHECK(input_stream >> bob);
    BOOST_CHECK_EQUAL(bob, "grbbledy");
    BOOST_CHECK(input_stream >> bob);
    BOOST_CHECK_EQUAL(bob, "ahhk");
}

BOOST_AUTO_TEST_SUITE_END();
