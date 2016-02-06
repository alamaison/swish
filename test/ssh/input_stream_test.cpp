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
using ssh::filesystem::openmode;
using ssh::filesystem::path;
using ssh::filesystem::perms;
using ssh::filesystem::sftp_filesystem;

using boost::uuids::random_generator;
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

const wchar_t WIDE_STRING1[] = L"\x92e\x939\x938\x941\x938";
}

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
