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
#include <boost/uuid/uuid_generators.hpp> // random_generator
#include <boost/uuid/uuid_io.hpp>         // to_string

#include <string>
#include <vector>

using ssh::filesystem::ifstream;
using ssh::filesystem::ofstream;
using ssh::filesystem::openmode;
using ssh::filesystem::path;
using ssh::filesystem::perms;
using ssh::filesystem::sftp_filesystem;

using boost::uuids::random_generator;
using boost::system::system_error;

using test::ssh::sftp_fixture;

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

string large_binary_data()
{
    string data;
    for (int i = 0; i < 32000; ++i)
    {
        data.push_back('a');
        data.push_back('\n');
        data.push_back('\0');
        data.push_back('\r');
        data.push_back('\n');
        data.push_back(-1);
    }

    return data;
}

void make_file_read_only(sftp_filesystem& filesystem, const path& target)
{
    permissions(filesystem, target, perms::owner_read);
}
}

BOOST_FIXTURE_TEST_SUITE(ofstream_tests, sftp_fixture)

BOOST_AUTO_TEST_CASE(output_stream_multiple_streams)
{
    path target1 = new_file_in_sandbox();
    path target2 = new_file_in_sandbox();

    sftp_filesystem& chan = filesystem();

    ofstream s1(chan, target1);
    ofstream s2(chan, target2);
}

BOOST_AUTO_TEST_CASE(output_stream_multiple_streams_to_same_file)
{
    path target = new_file_in_sandbox();

    sftp_filesystem& chan = filesystem();

    ofstream s1(chan, target);
    ofstream s2(chan, target);
}

BOOST_AUTO_TEST_CASE(output_stream_is_writeable)
{
    path target = new_file_in_sandbox();

    {
        ofstream output_stream(filesystem(), target);
        output_stream << "gobbledy gook";
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

BOOST_AUTO_TEST_CASE(output_stream_write_multiple_buffers)
{
    // large enough to span multiple buffers
    string data(large_data());

    path target = new_file_in_sandbox();

    sftp_filesystem& chan = filesystem();

    {
        ofstream output_stream(chan, target);
        BOOST_CHECK(output_stream.write(data.data(), data.size()));
    }

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
BOOST_AUTO_TEST_CASE(output_stream_write_no_buffer)
{
    string data("gobbeldy gook");

    path target = new_file_in_sandbox();

    sftp_filesystem& chan = filesystem();

    ofstream output_stream(chan, target, openmode::out, 0);
    BOOST_CHECK(output_stream.write(data.data(), data.size()));

    ifstream input_stream(filesystem(), target);

    string bob;

    vector<char> buffer(data.size());
    BOOST_CHECK(input_stream.read(&buffer[0], buffer.size()));

    BOOST_CHECK_EQUAL_COLLECTIONS(buffer.begin(), buffer.end(), data.begin(),
                                  data.end());

    BOOST_CHECK(!input_stream.read(&buffer[0], buffer.size()));
    BOOST_CHECK(input_stream.eof());
}

BOOST_AUTO_TEST_CASE(output_stream_write_binary_data)
{
    string data("gobbledy gook\0after-null\x12\x11", 26);

    path target = new_file_in_sandbox();

    sftp_filesystem& chan = filesystem();

    {
        ofstream output_stream(chan, target);
        BOOST_CHECK(output_stream.write(data.data(), data.size()));
    }

    ifstream input_stream(filesystem(), target);

    string bob;

    vector<char> buffer(data.size());
    BOOST_CHECK(input_stream.read(&buffer[0], buffer.size()));

    BOOST_CHECK_EQUAL_COLLECTIONS(buffer.begin(), buffer.end(), data.begin(),
                                  data.end());

    BOOST_CHECK(!input_stream.read(&buffer[0], buffer.size()));
    BOOST_CHECK(input_stream.eof());
}

BOOST_AUTO_TEST_CASE(output_stream_write_binary_data_multiple_buffers)
{
    // large enough to span multiple buffers
    string data(large_binary_data());

    path target = new_file_in_sandbox();

    sftp_filesystem& chan = filesystem();

    {
        ofstream output_stream(chan, target);
        BOOST_CHECK(output_stream.write(data.data(), data.size()));
    }

    ifstream input_stream(filesystem(), target);

    string bob;

    vector<char> buffer(data.size());
    BOOST_CHECK(input_stream.read(&buffer[0], buffer.size()));

    BOOST_CHECK_EQUAL_COLLECTIONS(buffer.begin(), buffer.end(), data.begin(),
                                  data.end());

    BOOST_CHECK(!input_stream.read(&buffer[0], buffer.size()));
    BOOST_CHECK(input_stream.eof());
}

BOOST_AUTO_TEST_CASE(output_stream_write_binary_data_stream_op)
{
    string data("gobbledy gook\0after-null\x12\x11", 26);

    path target = new_file_in_sandbox();

    sftp_filesystem& chan = filesystem();

    {
        ofstream output_stream(chan, target);
        BOOST_CHECK(output_stream << data);
    }

    ifstream input_stream(filesystem(), target);

    string bob;

    vector<char> buffer(data.size());
    BOOST_CHECK(input_stream.read(&buffer[0], buffer.size()));

    BOOST_CHECK_EQUAL_COLLECTIONS(buffer.begin(), buffer.end(), data.begin(),
                                  data.end());

    BOOST_CHECK(!input_stream.read(&buffer[0], buffer.size()));
    BOOST_CHECK(input_stream.eof());
}

BOOST_AUTO_TEST_CASE(output_stream_creates_by_default)
{
    random_generator generator;
    path target = to_string(generator());

    ofstream output_stream(filesystem(), target);
    BOOST_CHECK(exists(filesystem(), target));
}

BOOST_AUTO_TEST_CASE(output_stream_nocreate_flag)
{
    path target = new_file_in_sandbox();

    ofstream(filesystem(), target, openmode::nocreate);
    BOOST_CHECK(exists(filesystem(), target));
}

BOOST_AUTO_TEST_CASE(output_stream_nocreate_flag_fails)
{
    random_generator generator;
    path target = to_string(generator());

    BOOST_CHECK_THROW(ofstream(filesystem(), target, openmode::nocreate),
                      system_error);
    BOOST_CHECK(!exists(filesystem(), target));
}

BOOST_AUTO_TEST_CASE(output_stream_noreplace_flag)
{
    random_generator generator;
    path target = to_string(generator());

    ofstream(filesystem(), target, openmode::noreplace);
    BOOST_CHECK(exists(filesystem(), target));
}

BOOST_AUTO_TEST_CASE(output_stream_noreplace_flag_fails)
{
    path target = new_file_in_sandbox();

    BOOST_CHECK_THROW(ofstream(filesystem(), target, openmode::noreplace),
                      system_error);
    BOOST_CHECK(exists(filesystem(), target));
}

BOOST_AUTO_TEST_CASE(output_stream_out_flag_creates)
{
    random_generator generator;
    path target = to_string(generator());

    ofstream output_stream(filesystem(), target, openmode::out);
    BOOST_CHECK(exists(filesystem(), target));
}

BOOST_AUTO_TEST_CASE(output_stream_out_flag_truncates)
{
    path target = new_file_in_sandbox_containing_data("gobbledy gook");

    {
        ofstream output_stream(filesystem(), target, openmode::out);
        BOOST_CHECK(exists(filesystem(), target));

        BOOST_CHECK(output_stream << "abcdef");
    }

    ifstream input_stream(filesystem(), target);

    string bob;

    BOOST_CHECK(input_stream >> bob);
    BOOST_CHECK_EQUAL(bob, "abcdef");

    BOOST_CHECK(!(input_stream >> bob));
    BOOST_CHECK(input_stream.eof());
}

BOOST_AUTO_TEST_CASE(output_stream_out_nocreate_flag)
{
    path target = new_file_in_sandbox();

    ofstream output_stream(filesystem(), target,
                           openmode::out | openmode::nocreate);

    BOOST_CHECK(output_stream << "abcdef");
}

BOOST_AUTO_TEST_CASE(output_stream_out_nocreate_flag_fails)
{
    random_generator generator;
    path target = to_string(generator());

    BOOST_CHECK_THROW(
        ofstream(filesystem(), target, openmode::out | openmode::nocreate),
        system_error);
    BOOST_CHECK(!exists(filesystem(), target));
}

BOOST_AUTO_TEST_CASE(output_stream_out_noreplace_flag)
{
    random_generator generator;
    path target = to_string(generator());

    ofstream output_stream(filesystem(), target,
                           openmode::out | openmode::noreplace);

    BOOST_CHECK(exists(filesystem(), target));
    BOOST_CHECK(output_stream << "abcdef");
}

BOOST_AUTO_TEST_CASE(output_stream_out_noreplace_flag_fails)
{
    path target = new_file_in_sandbox();

    BOOST_CHECK_THROW(
        ofstream(filesystem(), target, openmode::out | openmode::noreplace),
        system_error);
    BOOST_CHECK(exists(filesystem(), target));
}

BOOST_AUTO_TEST_CASE(output_stream_in_flag_does_not_create)
{
    // In flag suppresses creation.  Matches standard lib ofstream.

    random_generator generator;
    path target = to_string(generator());

    BOOST_CHECK_THROW(ofstream(filesystem(), target, openmode::in),
                      system_error);
    BOOST_CHECK(!exists(filesystem(), target));
}

BOOST_AUTO_TEST_CASE(output_stream_in_out_does_not_create)
{
    random_generator generator;
    path target = to_string(generator());

    BOOST_CHECK_THROW(
        ofstream(filesystem(), target, openmode::in | openmode::out),
        system_error);

    BOOST_CHECK(!exists(filesystem(), target));
}

BOOST_AUTO_TEST_CASE(output_stream_in_out_flag_updates)
{
    // Unlike the out flag for output-only streams, which truncates, the
    // out flag on an input stream leaves the existing contents because the
    // input stream forces the in flag and in|out means update existing

    path target = new_file_in_sandbox_containing_data("gobbledy gook");

    {
        ofstream output_stream(filesystem(), target,
                               openmode::in | openmode::out);
        BOOST_CHECK(exists(filesystem(), target));

        BOOST_CHECK(output_stream << "abcdef");
    }

    ifstream input_stream(filesystem(), target);

    string bob;

    BOOST_CHECK(input_stream >> bob);
    BOOST_CHECK_EQUAL(bob, "abcdefdy");

    BOOST_CHECK(input_stream >> bob);
    BOOST_CHECK_EQUAL(bob, "gook");

    BOOST_CHECK(!(input_stream >> bob));
    BOOST_CHECK(input_stream.eof());
}

BOOST_AUTO_TEST_CASE(output_stream_out_trunc_flag_creates)
{
    random_generator generator;
    path target = to_string(generator());

    ofstream output_stream(filesystem(), target,
                           openmode::out | openmode::trunc);
    BOOST_CHECK(exists(filesystem(), target));
}

BOOST_AUTO_TEST_CASE(output_stream_out_trunc_nocreate_flag)
{
    path target = new_file_in_sandbox();

    ofstream output_stream(filesystem(), target, openmode::out |
                                                     openmode::trunc |
                                                     openmode::nocreate);
    BOOST_CHECK(exists(filesystem(), target));
}

BOOST_AUTO_TEST_CASE(output_stream_out_trunc_nocreate_flag_fails)
{
    random_generator generator;
    path target = to_string(generator());

    BOOST_CHECK_THROW(ofstream output_stream(filesystem(), target,
                                             openmode::out | openmode::trunc |
                                                 openmode::nocreate),
                      system_error);
    BOOST_CHECK(!exists(filesystem(), target));
}

BOOST_AUTO_TEST_CASE(output_stream_out_trunc_noreplace_flag)
{
    random_generator generator;
    path target = to_string(generator());

    ofstream output_stream(filesystem(), target, openmode::out |
                                                     openmode::trunc |
                                                     openmode::noreplace);
    BOOST_CHECK(exists(filesystem(), target));
}

BOOST_AUTO_TEST_CASE(output_stream_out_trunc_noreplace_flag_fails)
{
    path target = new_file_in_sandbox();

    BOOST_CHECK_THROW(ofstream output_stream(filesystem(), target,
                                             openmode::out | openmode::trunc |
                                                 openmode::noreplace),
                      system_error);
    BOOST_CHECK(exists(filesystem(), target));
}

BOOST_AUTO_TEST_CASE(output_stream_out_trunc_flag_truncates)
{
    path target = new_file_in_sandbox_containing_data("gobbledy gook");

    {
        ofstream output_stream(filesystem(), target,
                               openmode::out | openmode::trunc);

        BOOST_CHECK(output_stream << "abcdef");
    }

    ifstream input_stream(filesystem(), target);

    string bob;

    BOOST_CHECK(input_stream >> bob);
    BOOST_CHECK_EQUAL(bob, "abcdef");

    BOOST_CHECK(!(input_stream >> bob));
    BOOST_CHECK(input_stream.eof());
}

BOOST_AUTO_TEST_CASE(output_stream_in_out_trunc_flag_creates)
{
    random_generator generator;
    path target = to_string(generator());

    ofstream output_stream(filesystem(), target,
                           openmode::in | openmode::out | openmode::trunc);
    BOOST_CHECK(exists(filesystem(), target));
}

BOOST_AUTO_TEST_CASE(output_stream_in_out_trunc_flag_truncates)
{
    path target = new_file_in_sandbox_containing_data("gobbledy gook");

    {
        ofstream output_stream(filesystem(), target,
                               openmode::in | openmode::out | openmode::trunc);

        BOOST_CHECK(output_stream << "abcdef");
    }

    ifstream input_stream(filesystem(), target);

    string bob;

    BOOST_CHECK(input_stream >> bob);
    BOOST_CHECK_EQUAL(bob, "abcdef");

    BOOST_CHECK(!(input_stream >> bob));
    BOOST_CHECK(input_stream.eof());
}

BOOST_AUTO_TEST_CASE(output_stream_out_append_flag_creates)
{
    random_generator generator;
    path target = to_string(generator());

    ofstream output_stream(filesystem(), target, openmode::out | openmode::app);
    BOOST_CHECK(exists(filesystem(), target));
}

BOOST_AUTO_TEST_CASE(output_stream_out_append_flag_appends)
{
    path target = new_file_in_sandbox_containing_data("gobbledy gook");

    {
        ofstream output_stream(filesystem(), target,
                               openmode::out | openmode::app);

        BOOST_CHECK(output_stream << "abcdef");
    }

    ifstream input_stream(filesystem(), target);

    string bob;

    /* If the tests fail here, the version of OpenSSH being used
       is probably old and doesn't support FXF_APPEND */

    BOOST_CHECK(input_stream >> bob);
    BOOST_CHECK_EQUAL(bob, "gobbledy");

    BOOST_CHECK(input_stream >> bob);
    BOOST_CHECK_EQUAL(bob, "gookabcdef");

    BOOST_CHECK(!(input_stream >> bob));
    BOOST_CHECK(input_stream.eof());
}

BOOST_AUTO_TEST_CASE(output_stream_fails_to_open_read_only_by_default)
{
    path target = new_file_in_sandbox();
    make_file_read_only(filesystem(), target);

    BOOST_CHECK_THROW(ofstream(filesystem(), target), system_error);
}

BOOST_AUTO_TEST_CASE(output_stream_out_flag_fails_to_open_read_only)
{
    path target = new_file_in_sandbox();
    make_file_read_only(filesystem(), target);

    BOOST_CHECK_THROW(ofstream(filesystem(), target, openmode::out),
                      system_error);
}

BOOST_AUTO_TEST_CASE(output_stream_in_out_flag_fails_to_open_read_only)
{
    path target = new_file_in_sandbox();
    make_file_read_only(filesystem(), target);

    BOOST_CHECK_THROW(
        ofstream(filesystem(), target, openmode::in | openmode::out),
        system_error);
}

// Because output streams force out flag, they can't open read-only files
BOOST_AUTO_TEST_CASE(output_stream_in_flag_fails_to_open_read_only)
{
    path target = new_file_in_sandbox();
    make_file_read_only(filesystem(), target);

    BOOST_CHECK_THROW(ofstream(filesystem(), target, openmode::in),
                      system_error);
}

// By default ostreams overwrite the file so seeking will cause subsequent
// output to write after the file end.  The skipped bytes should be filled
// with NUL
BOOST_AUTO_TEST_CASE(output_stream_seek_output_absolute_overshoot)
{
    path target = new_file_in_sandbox_containing_data("gobbledy gook");

    ofstream s(filesystem(), target);
    s.seekp(2, std::ios_base::beg);

    BOOST_CHECK(s << "r");

    s.flush();

    ifstream input_stream(filesystem(), target);

    string expected_data("\0\0r", 3);

    vector<char> buffer(expected_data.size());
    BOOST_CHECK(input_stream.read(&buffer[0], buffer.size()));

    BOOST_CHECK_EQUAL_COLLECTIONS(buffer.begin(), buffer.end(),
                                  expected_data.begin(), expected_data.end());
}

BOOST_AUTO_TEST_CASE(output_stream_seek_output_absolute)
{
    path target = new_file_in_sandbox_containing_data("gobbledy gook");

    ofstream s(filesystem(), target, openmode::in);
    s.seekp(1, std::ios_base::beg);

    BOOST_CHECK(s << "r");

    s.flush();

    ifstream input_stream(filesystem(), target);

    string bob;

    BOOST_CHECK(input_stream >> bob);
    BOOST_CHECK_EQUAL(bob, "grbbledy");
}

// By default ostreams overwrite the file so seeking will cause subsequent
// output to write after the file end.  The skipped bytes should be filled
// with NUL
BOOST_AUTO_TEST_CASE(output_stream_seek_output_relative_overshoot)
{
    path target = new_file_in_sandbox_containing_data("gobbledy gook");

    ofstream s(filesystem(), target);
    s.seekp(1, std::ios_base::cur);
    s.seekp(1, std::ios_base::cur);

    BOOST_CHECK(s << "r");

    s.flush();

    ifstream input_stream(filesystem(), target);

    string expected_data("\0\0r", 3);

    vector<char> buffer(expected_data.size());
    BOOST_CHECK(input_stream.read(&buffer[0], buffer.size()));

    BOOST_CHECK_EQUAL_COLLECTIONS(buffer.begin(), buffer.end(),
                                  expected_data.begin(), expected_data.end());
}

BOOST_AUTO_TEST_CASE(output_stream_seek_output_relative)
{
    path target = new_file_in_sandbox_containing_data("gobbledy gook");

    ofstream s(filesystem(), target, openmode::in);
    s.seekp(1, std::ios_base::cur);
    s.seekp(1, std::ios_base::cur);

    BOOST_CHECK(s << "r");

    s.flush();

    ifstream input_stream(filesystem(), target);

    string bob;

    BOOST_CHECK(input_stream >> bob);
    BOOST_CHECK_EQUAL(bob, "gorbledy");
}

// By default ostreams overwrite the file.  Seeking TO the end of this empty
// file will just start writing from the beginning.  No NUL bytes are
// inserted anywhere
BOOST_AUTO_TEST_CASE(output_stream_seek_output_end)
{
    path target = new_file_in_sandbox_containing_data("gobbledy gook");

    ofstream s(filesystem(), target);
    s.seekp(0, std::ios_base::end);

    BOOST_CHECK(s << "r");

    s.flush();

    ifstream input_stream(filesystem(), target);

    string bob;

    BOOST_CHECK(input_stream >> bob);
    BOOST_CHECK_EQUAL(bob, "r");
    BOOST_CHECK(!(input_stream >> bob));
    BOOST_CHECK_EQUAL(bob, "r");
}

// By default ostreams overwrite the file.  Seeking past the end  will cause
// subsequent output to write after the file end.  The skipped bytes will
// be filled with NUL.
BOOST_AUTO_TEST_CASE(output_stream_seek_output_end_overshoot)
{
    path target = new_file_in_sandbox_containing_data("gobbledy gook");

    ofstream s(filesystem(), target);
    s.seekp(3, std::ios_base::end);

    BOOST_CHECK(s << "r");

    s.flush();

    ifstream input_stream(filesystem(), target);

    string expected_data("\0\0\0r", 4);

    vector<char> buffer(expected_data.size());
    BOOST_CHECK(input_stream.read(&buffer[0], buffer.size()));

    BOOST_CHECK_EQUAL_COLLECTIONS(buffer.begin(), buffer.end(),
                                  expected_data.begin(), expected_data.end());
}

BOOST_AUTO_TEST_CASE(output_stream_seek_output_before_end)
{
    path target = new_file_in_sandbox_containing_data("gobbledy gook");

    ofstream s(filesystem(), target, openmode::in);
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

BOOST_AUTO_TEST_SUITE_END();
