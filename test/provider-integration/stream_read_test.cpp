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
#include <ssh/stream.hpp>

#include <comet/ptr.h> // com_ptr

#include <boost/test/unit_test.hpp>
#include <boost/numeric/conversion/cast.hpp> // numeric_cast
#include <boost/shared_ptr.hpp>
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <exception>
#include <string>
#include <vector>

using test::fixtures::com_stream_fixture;
using test::stream_utils::verify_stream_read;

using comet::com_ptr;

using ssh::filesystem::ofstream;
using ssh::filesystem::perms;

using boost::numeric_cast;
using boost::shared_ptr;

using std::exception;
using std::string;
using std::vector;

namespace
{ // private

const string TEST_DATA = "Humpty dumpty\nsat on the wall.\n\rHumpty ...";

/**
 * Fixture for tests that need to read data from an existing file.
 */
class stream_read_fixture : public com_stream_fixture
{
public:
    /**
     * Put test data into a file in our sandbox.
     */
    stream_read_fixture() : com_stream_fixture()
    {
        ofstream file(filesystem(), test_file(), std::ios::binary);
        file << expected_data() << std::flush;
    }

    /**
     * Create an IStream instance open for reading on a temporary file
     * in our sandbox.  The file contained the same data that
     * ExpectedData() returns.
     */
    com_ptr<IStream> get_read_stream()
    {
        return get_stream(std::ios_base::in);
    }

    /**
     * Return the data we expect to be able to read using the IStream.
     */
    string expected_data()
    {
        return TEST_DATA;
    }
};
}

BOOST_FIXTURE_TEST_SUITE(stream_read, stream_read_fixture)

/**
 * Simply get a stream.
 */
BOOST_AUTO_TEST_CASE(get)
{
    com_ptr<IStream> stream = get_read_stream();
    BOOST_REQUIRE(stream);
}

/**
 * Get a read stream to a read-only file.
 * This tests that we aren't inadvertently asking for more permissions than
 * we need.
 */
BOOST_AUTO_TEST_CASE(get_readonly)
{
    permissions(filesystem(), test_file(), perms::owner_read);

    com_ptr<IStream> stream = get_read_stream();
    BOOST_REQUIRE(stream);
}

/**
 * Try to get a stream to a non-readable file.
 * Test how we deal with opening failures.
 */
BOOST_AUTO_TEST_CASE(throws_exception_opening_unreadable_file)
{
    permissions(filesystem(), test_file(), perms::none);

    BOOST_REQUIRE_THROW(get_read_stream(), exception);
}

/**
 * Read a sequence of characters.
 */
BOOST_AUTO_TEST_CASE(read_a_string)
{
    com_ptr<IStream> stream = get_read_stream();

    string expected = expected_data();
    vector<char> buf(expected.size());

    size_t bytes_read =
        verify_stream_read(&buf[0], numeric_cast<ULONG>(buf.size()), stream);

    BOOST_CHECK_EQUAL(bytes_read, expected.size());

    // Test that the bytes we read match
    BOOST_REQUIRE_EQUAL_COLLECTIONS(buf.begin(), buf.end(), expected.begin(),
                                    expected.end());
}

/**
 * Read a sequence of characters from a read-only file.
 */
BOOST_AUTO_TEST_CASE(read_a_string_readonly)
{
    permissions(filesystem(), test_file(), perms::owner_read);

    com_ptr<IStream> stream = get_read_stream();

    string expected = expected_data();
    vector<char> buf(expected.size());

    size_t bytes_read =
        verify_stream_read(&buf[0], numeric_cast<ULONG>(buf.size()), stream);

    BOOST_CHECK_EQUAL(bytes_read, expected.size());

    // Test that the bytes we read match
    BOOST_REQUIRE_EQUAL_COLLECTIONS(buf.begin(), buf.end(), expected.begin(),
                                    expected.end());
}

BOOST_AUTO_TEST_SUITE_END()
