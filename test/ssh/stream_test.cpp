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

#include <boost/bind/bind.hpp>
#include <boost/system/system_error.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/thread/future.hpp> // packaged_task
#include <boost/thread/thread.hpp>
#include <boost/uuid/uuid_generators.hpp> // random_generator
#include <boost/uuid/uuid_io.hpp>         // to_string

#include <string>
#include <vector>

using ssh::filesystem::fstream;
using ssh::filesystem::ifstream;
using ssh::filesystem::ofstream;
using ssh::filesystem::openmode;
using ssh::filesystem::path;
using ssh::filesystem::perms;
using ssh::filesystem::sftp_filesystem;
using ssh::session;

using boost::bind;
using boost::packaged_task;
using boost::uuids::random_generator;
using boost::system::system_error;
using boost::thread;

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

string large_binary_data()
{
    string data;
    for (int i = 0; i < 32000; ++i)
    {
        data.push_back('a');
        data.push_back('\0');
        data.push_back(-1);
    }

    return data;
}

void make_file_read_only(sftp_filesystem& filesystem, const path& target)
{
    permissions(filesystem, target, perms::owner_read);
}

const wchar_t WIDE_STRING1[] = L"\x92e\x939\x938\x941\x938";
}

BOOST_AUTO_TEST_SUITE(stream_tests)

BOOST_FIXTURE_TEST_SUITE(ifstream_tests, sftp_fixture)

BOOST_AUTO_TEST_CASE(input_stream_multiple_streams)
{
    path target1 = new_file_in_sandbox();
    path target2 = new_file_in_sandbox();

    sftp_filesystem& chan = filesystem();

    ifstream s1(chan, target1);
    ifstream s2(chan, target2);
}

BOOST_AUTO_TEST_CASE(input_stream_multiple_streams_to_same_file)
{
    path target = new_file_in_sandbox();

    sftp_filesystem& chan = filesystem();

    ifstream s1(chan, target);
    ifstream s2(chan, target);
}

BOOST_AUTO_TEST_CASE(input_stream_readable)
{
    path target = new_file_in_sandbox_containing_data("gobbledy gook");

    ifstream s(filesystem(), target);

    string bob;

    BOOST_CHECK(s >> bob);
    BOOST_CHECK_EQUAL(bob, "gobbledy");
    BOOST_CHECK(s >> bob);
    BOOST_CHECK_EQUAL(bob, "gook");
    BOOST_CHECK(!(s >> bob));
    BOOST_CHECK(s.eof());
}

BOOST_AUTO_TEST_CASE(input_stream_unicode_readable)
{
    path target =
        new_file_in_sandbox_containing_data(WIDE_STRING1, "gobbledy gook");

    ifstream s(filesystem(), target);

    string bob;

    BOOST_CHECK(s >> bob);
    BOOST_CHECK_EQUAL(bob, "gobbledy");
    BOOST_CHECK(s >> bob);
    BOOST_CHECK_EQUAL(bob, "gook");
    BOOST_CHECK(!(s >> bob));
    BOOST_CHECK(s.eof());
}

BOOST_AUTO_TEST_CASE(input_stream_readable_multiple_buffers)
{
    // large enough to span multiple buffers
    string expected_data(large_data());

    path target = new_file_in_sandbox_containing_data(expected_data);

    sftp_filesystem& chan = filesystem();

    ifstream input_stream(chan, target);

    string bob;

    vector<char> buffer(expected_data.size());
    BOOST_CHECK(input_stream.read(&buffer[0], buffer.size()));

    BOOST_CHECK_EQUAL_COLLECTIONS(buffer.begin(), buffer.end(),
                                  expected_data.begin(), expected_data.end());
}

// Test with Boost.IOStreams buffer disabled.
// Should call directly to libssh2
BOOST_AUTO_TEST_CASE(input_stream_readable_no_buffer)
{
    string expected_data("gobbeldy gook");

    path target = new_file_in_sandbox_containing_data(expected_data);

    sftp_filesystem& chan = filesystem();

    ifstream input_stream(chan, target, openmode::in, 0);

    string bob;

    vector<char> buffer(expected_data.size());
    BOOST_CHECK(input_stream.read(&buffer[0], buffer.size()));

    BOOST_CHECK_EQUAL_COLLECTIONS(buffer.begin(), buffer.end(),
                                  expected_data.begin(), expected_data.end());
}

BOOST_AUTO_TEST_CASE(input_stream_readable_binary_data)
{
    string expected_data("gobbledy gook\0after-null\x12\11", 26);

    path target = new_file_in_sandbox_containing_data(expected_data);

    sftp_filesystem& chan = filesystem();

    ifstream input_stream(chan, target);

    string bob;

    vector<char> buffer(expected_data.size());
    BOOST_CHECK(input_stream.read(&buffer[0], buffer.size()));

    BOOST_CHECK_EQUAL_COLLECTIONS(buffer.begin(), buffer.end(),
                                  expected_data.begin(), expected_data.end());
}

BOOST_AUTO_TEST_CASE(input_stream_readable_binary_data_multiple_buffers)
{
    // large enough to span multiple buffers
    string expected_data(large_binary_data());

    path target = new_file_in_sandbox_containing_data(expected_data);

    sftp_filesystem& chan = filesystem();

    ifstream input_stream(chan, target);

    string bob;

    vector<char> buffer(expected_data.size());
    BOOST_CHECK(input_stream.read(&buffer[0], buffer.size()));

    BOOST_CHECK_EQUAL_COLLECTIONS(buffer.begin(), buffer.end(),
                                  expected_data.begin(), expected_data.end());
}

BOOST_AUTO_TEST_CASE(input_stream_readable_binary_data_stream_op)
{
    string expected_data("gobbledy gook\0after-null\x12\x11", 26);

    path target = new_file_in_sandbox_containing_data(expected_data);

    sftp_filesystem& chan = filesystem();

    ifstream input_stream(chan, target);

    string bob;

    BOOST_CHECK(input_stream >> bob);
    BOOST_CHECK_EQUAL(bob, "gobbledy");

    BOOST_CHECK(input_stream >> bob);
    const char* gn = "gook\0after-null\x12\x11";
    BOOST_CHECK_EQUAL_COLLECTIONS(bob.begin(), bob.end(), gn, gn + 17);
    BOOST_CHECK(!(input_stream >> bob));
    BOOST_CHECK(input_stream.eof());
}

BOOST_AUTO_TEST_CASE(input_stream_does_not_create_by_default)
{
    random_generator generator;
    path target = to_string(generator());

    BOOST_REQUIRE(!exists(filesystem(), target));
    BOOST_CHECK_THROW(ifstream(filesystem(), target), system_error);
    BOOST_CHECK(!exists(filesystem(), target));
}

/* FIXME: find why this is failing in libssh2
BOOST_AUTO_TEST_CASE(
    input_stream_does_not_create_with_ridiculously_large_filename )
{
    // We intentionally pass a large amount of data as the filename.
    // When we did this accidentally, we found it was not getting an error code
but hit an assertion because opening the file failed.
    path target = large_data();
    BOOST_REQUIRE(!exists(filesystem(), target));
    BOOST_CHECK_THROW(
        ifstream(filesystem(), target), system_error);
    BOOST_CHECK(!exists(filesystem(), target));
}
*/

BOOST_AUTO_TEST_CASE(input_stream_opens_read_only_by_default)
{
    path target = new_file_in_sandbox();
    make_file_read_only(filesystem(), target);

    ifstream(filesystem(), target);
}

BOOST_AUTO_TEST_CASE(input_stream_in_flag_does_not_create)
{
    random_generator generator;
    path target = to_string(generator());

    BOOST_CHECK_THROW(ifstream(filesystem(), target, openmode::in),
                      system_error);
    BOOST_CHECK(!exists(filesystem(), target));
}

BOOST_AUTO_TEST_CASE(input_stream_std_in_flag_does_not_create)
{
    random_generator generator;
    path target = to_string(generator());

    BOOST_CHECK_THROW(ifstream(filesystem(), target, std::ios_base::in),
                      system_error);
    BOOST_CHECK(!exists(filesystem(), target));
}

BOOST_AUTO_TEST_CASE(input_stream_in_flag_opens_read_only)
{
    path target = new_file_in_sandbox();
    make_file_read_only(filesystem(), target);

    ifstream(filesystem(), target, openmode::in);
}

BOOST_AUTO_TEST_CASE(input_stream_out_flag_does_not_create)
{
    // Because ifstream forces in as well as out an in suppresses creation

    random_generator generator;
    path target = to_string(generator());

    BOOST_CHECK_THROW(ifstream(filesystem(), target, openmode::out),
                      system_error);
    BOOST_CHECK(!exists(filesystem(), target));
}

BOOST_AUTO_TEST_CASE(input_stream_out_flag_fails_to_open_read_only)
{
    path target = new_file_in_sandbox();
    make_file_read_only(filesystem(), target);

    BOOST_CHECK_THROW(ifstream(filesystem(), target, openmode::out),
                      system_error);
}

BOOST_AUTO_TEST_CASE(input_stream_out_trunc_flag_creates)
{
    random_generator generator;
    path target = to_string(generator());

    ifstream input_stream(filesystem(), target,
                          openmode::out | openmode::trunc);
    BOOST_CHECK(exists(filesystem(), target));
}

BOOST_AUTO_TEST_CASE(input_stream_std_out_trunc_flag_creates)
{
    random_generator generator;
    path target = to_string(generator());

    ifstream input_stream(filesystem(), target,
                          std::ios_base::out | std::ios_base::trunc);
    BOOST_CHECK(exists(filesystem(), target));
}

BOOST_AUTO_TEST_CASE(input_stream_out_trunc_nocreate_flag_fails)
{
    random_generator generator;
    path target = to_string(generator());

    BOOST_CHECK_THROW(ifstream(filesystem(), target, openmode::out |
                                                         openmode::trunc |
                                                         openmode::nocreate),
                      system_error);
    BOOST_CHECK(!exists(filesystem(), target));
}

BOOST_AUTO_TEST_CASE(input_stream_out_trunc_noreplace_flag_fails)
{
    path target = new_file_in_sandbox();

    BOOST_CHECK_THROW(ifstream(filesystem(), target, openmode::out |
                                                         openmode::trunc |
                                                         openmode::noreplace),
                      system_error);
    BOOST_CHECK(exists(filesystem(), target));
}

BOOST_AUTO_TEST_CASE(input_stream_seek_input_absolute)
{
    path target = new_file_in_sandbox_containing_data("gobbledy gook");

    ifstream s(filesystem(), target);
    s.seekg(1, std::ios_base::beg);

    string bob;
    BOOST_CHECK(s >> bob);
    BOOST_CHECK_EQUAL(bob, "obbledy");
}

BOOST_AUTO_TEST_CASE(input_stream_seek_input_relative)
{
    path target = new_file_in_sandbox_containing_data("gobbledy gook");

    ifstream s(filesystem(), target);
    s.seekg(1, std::ios_base::cur);
    s.seekg(1, std::ios_base::cur);

    string bob;
    BOOST_CHECK(s >> bob);
    BOOST_CHECK_EQUAL(bob, "bbledy");
}

BOOST_AUTO_TEST_CASE(input_stream_seek_input_end)
{
    path target = new_file_in_sandbox_containing_data("gobbledy gook");

    ifstream s(filesystem(), target);
    s.seekg(-3, std::ios_base::end);

    string bob;
    BOOST_CHECK(s >> bob);
    BOOST_CHECK_EQUAL(bob, "ook");
}

BOOST_AUTO_TEST_CASE(input_stream_seek_input_too_far_absolute)
{
    path target = new_file_in_sandbox();

    ifstream s(filesystem(), target);
    s.exceptions(std::ios_base::badbit | std::ios_base::eofbit |
                 std::ios_base::failbit);
    s.seekg(1, std::ios_base::beg);

    string bob;
    BOOST_CHECK_THROW(s >> bob, runtime_error);
}

BOOST_AUTO_TEST_CASE(input_stream_seek_input_too_far_relative)
{
    path target = new_file_in_sandbox_containing_data("gobbledy gook");

    ifstream s(filesystem(), target);
    s.exceptions(std::ios_base::badbit | std::ios_base::eofbit |
                 std::ios_base::failbit);
    s.seekg(9, std::ios_base::cur);
    s.seekg(4, std::ios_base::cur);

    string bob;
    BOOST_CHECK_THROW(s >> bob, runtime_error);
}

BOOST_AUTO_TEST_SUITE_END();

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

BOOST_FIXTURE_TEST_SUITE(fstream_tests, sftp_fixture)

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

BOOST_FIXTURE_TEST_SUITE(threading_tests, sftp_fixture)

namespace
{

string get_first_token(ifstream& stream)
{
    string r;
    stream >> r;
    return r;
}
}

BOOST_AUTO_TEST_CASE(stream_read_on_different_threads)
{
    path target1 = new_file_in_sandbox_containing_data("humpty dumpty sat");
    path target2 = new_file_in_sandbox_containing_data("on the wall");

    sftp_filesystem& chan = filesystem();

    ifstream s1(chan, target1);
    ifstream s2(chan, target2);

    packaged_task<string> p1(boost::bind(get_first_token, boost::ref(s1)));
    packaged_task<string> p2(boost::bind(get_first_token, boost::ref(s2)));

    thread(boost::ref(p1)).detach();
    thread(boost::ref(p2)).detach();

    BOOST_CHECK_EQUAL(p1.get_future().get(), "humpty");
    BOOST_CHECK_EQUAL(p2.get_future().get(), "on");
}

// There was a bug in our session locking that meant we locked the session
// when opening a file but didn't when closing it (because it happened in
// the shared_ptr destructor).  This test case triggers that bug by opening
// a file (locks and unlocks session), starting to read from a second file
// (locks) session and then closing the first file.  This will cause all
// sorts of bad behaviour of the closure doesn't lock the session so we can
// detect it if it regresses.
BOOST_AUTO_TEST_CASE(parallel_file_closing)
{
    string data = large_data();

    path read_me = new_file_in_sandbox_containing_data(data);
    path test_me = new_file_in_sandbox();

    ifstream stream1(filesystem(), read_me);
    ifstream stream2(filesystem(), test_me);

    // Using a long-running stream read operation to make sure the session
    // is still locked when we try to close the other file
    packaged_task<string> ps(bind(get_first_token, boost::ref(stream1)));
    thread(boost::ref(ps)).detach();

    thread(bind(&ifstream::close, &stream2)).detach();

    BOOST_CHECK_EQUAL(ps.get_future().get(), data);
}

BOOST_AUTO_TEST_SUITE_END();

BOOST_AUTO_TEST_SUITE_END();
