// Copyright 2009, 2010, 2011, 2013, 2016 Alexander Lamaison

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

#ifndef SWISH_TEST_FIXTURES_COM_STREAM_FIXTURE_HPP
#define SWISH_TEST_FIXTURES_COM_STREAM_FIXTURE_HPP

#include "sftp_fixture.hpp"

#include <ssh/filesystem.hpp>

#include <comet/ptr.h> // com_ptr

namespace test
{
namespace fixtures
{

/**
 * Extends the Sandbox fixture by allowing the creation of swish::provider
 * IStreams that pass through the OpenSSH server pointing to files in the
 * sandbox.
 */
class com_stream_fixture : virtual public sftp_fixture
{
public:
    /**
     * Initialise the test fixture with the path of a new, empty file
     * in the sandbox.
     */
    com_stream_fixture();

    /**
     * Create an IStream instance open on a temporary file in our sandbox.
     * By default the stream is open for reading and writing but different
     * flags can be passed to change this.
     */
    comet::com_ptr<IStream> get_stream(
        std::ios_base::openmode flags = std::ios_base::in | std::ios_base::out);

    ssh::filesystem::path test_file();

private:
    ssh::filesystem::path m_path;
};
}
} // namespace test::fixtures

#endif
