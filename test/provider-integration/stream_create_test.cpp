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

#include <comet/error.h> // com_error

#include <boost/test/unit_test.hpp>

using test::fixtures::com_stream_fixture;

using comet::com_error;

BOOST_FIXTURE_TEST_SUITE(stream_create, com_stream_fixture)

/**
 * Open a stream to a file that doesn't already exist.
 * The file should be created as only the `std::ios_base::out` flag is set.
 */
BOOST_AUTO_TEST_CASE(new_file)
{
    // Delete sandbox file before creating stream
    remove(filesystem(), test_file());

    BOOST_REQUIRE(!exists(filesystem(), test_file()));

    get_stream(std::ios_base::out);

    BOOST_REQUIRE(exists(filesystem(), test_file()));
}

/**
 * Open a stream for reading to a file that doesn't already exist.
 * This should fail and the file should not be created as the
 * `std::ios_base::out` flag isn't set which would cause the file to
 * be created.
 */
BOOST_AUTO_TEST_CASE(non_existent_file_fail)
{
    // Delete sandbox file before creating stream
    remove(filesystem(), test_file());

    BOOST_REQUIRE(!exists(filesystem(), test_file()));

    BOOST_REQUIRE_THROW(get_stream(std::ios_base::in), std::exception);

    BOOST_REQUIRE(!exists(filesystem(), test_file()));
}

BOOST_AUTO_TEST_SUITE_END()
