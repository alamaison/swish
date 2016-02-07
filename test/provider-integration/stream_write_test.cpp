// Copyright 2009, 2011, 2013, 2016 Alexander Lamaison

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

#include "test/fixtures/com_stream_fixture.hpp"

#include "test/common_boost/helpers.hpp"
#include "test/common_boost/stream_utils.hpp" // verify_stream_read

#include <ssh/filesystem.hpp>

#include <comet/error.h> // com_error
#include <comet/ptr.h>   // com_ptr

#include <boost/numeric/conversion/cast.hpp> // numeric_cast
#include <boost/test/unit_test.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <algorithm> // generate
#include <cstdlib>   // rand
#include <string>
#include <vector>

using test::fixtures::com_stream_fixture;
using test::stream_utils::verify_stream_read;

using ssh::filesystem::perms;

using comet::com_error;
using comet::com_ptr;

using boost::numeric_cast;
using boost::shared_ptr;

using std::exception;
using std::generate;
using std::rand;
using std::string;
using std::vector;

BOOST_FIXTURE_TEST_SUITE(stream_write, com_stream_fixture)

/**
 * Simply get a stream.
 */
BOOST_AUTO_TEST_CASE(get)
{
    com_ptr<IStream> stream = get_stream();
    BOOST_REQUIRE(stream);
}

/**
 * Try to get a writable stream to a read-only file.
 * Test how we deal with opening failures.
 */
BOOST_AUTO_TEST_CASE(get_readonly)
{
    permissions(filesystem(), test_file(), perms::owner_read);

    BOOST_REQUIRE_THROW(get_stream(), exception);
}

/**
 * Write one byte to stream, read it back and check that it is the same.
 */
BOOST_AUTO_TEST_CASE(write_one_byte)
{
    com_ptr<IStream> stream = get_stream();

    // Write the character 'M' to the file
    char in[1] = {'M'};
    ULONG cbWritten = 0;
    BOOST_REQUIRE_OK(stream->Write(in, sizeof(in), &cbWritten));
    BOOST_REQUIRE_EQUAL(cbWritten, sizeof(in));

    // Reset seek pointer to beginning and read back
    LARGE_INTEGER move = {0};
    BOOST_REQUIRE_OK(stream->Seek(move, STREAM_SEEK_SET, NULL));

    char out[1];
    verify_stream_read(out, sizeof(out), stream);
    BOOST_REQUIRE_EQUAL('M', out[0]);
}

/**
 * Write a sequence of characters.
 */
BOOST_AUTO_TEST_CASE(write_a_string)
{
    com_ptr<IStream> stream = get_stream();

    string in = "Lorem ipsum dolor sit amet. ";
    ULONG cbWritten = 0;
    BOOST_REQUIRE_OK(
        stream->Write(&in[0], numeric_cast<ULONG>(in.size()), &cbWritten));
    BOOST_REQUIRE_EQUAL(cbWritten, in.size());

    // Reset seek pointer to beginning and read back
    LARGE_INTEGER move = {0};
    BOOST_REQUIRE_OK(stream->Seek(move, STREAM_SEEK_SET, NULL));

    vector<char> out(in.size());
    verify_stream_read(&out[0], numeric_cast<ULONG>(out.size()), stream);
    BOOST_REQUIRE_EQUAL_COLLECTIONS(out.begin(), out.end(), in.begin(),
                                    in.end());
}

namespace
{

vector<int> random_buffer(size_t buffer_size)
{
    vector<int> buffer(buffer_size);
    generate(buffer.begin(), buffer.end(), rand);
    return buffer;
}
}

/**
 * Write a large buffer.
 */
BOOST_AUTO_TEST_CASE(write_large)
{
    com_ptr<IStream> stream = get_stream();

    vector<int> in = random_buffer(1000000);

    ULONG cbWritten = 0;
    ULONG size_in_bytes = numeric_cast<ULONG>(in.size() * sizeof(int));
    BOOST_REQUIRE_OK(stream->Write(&in[0], size_in_bytes, &cbWritten));
    BOOST_REQUIRE_EQUAL(cbWritten, size_in_bytes);

    // Reset seek pointer to beginning and read back
    LARGE_INTEGER move = {0};
    BOOST_REQUIRE_OK(stream->Seek(move, STREAM_SEEK_SET, NULL));

    vector<int> out(in.size());
    verify_stream_read(&out[0], size_in_bytes, stream);
    BOOST_REQUIRE_EQUAL_COLLECTIONS(out.begin(), out.end(), in.begin(),
                                    in.end());
}

BOOST_AUTO_TEST_SUITE_END()
