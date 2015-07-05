/**
    @file

    Unit tests for CSftpStream exercising the write mechanism.

    @if license

    Copyright (C) 2009, 2011, 2013  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "test/provider/StreamFixture.hpp"
#include "test/common_boost/helpers.hpp"
#include "test/common_boost/stream_utils.hpp" // verify_stream_read

#include <comet/error.h> // com_error
#include <comet/ptr.h> // com_ptr

#include <boost/numeric/conversion/cast.hpp>  // numeric_cast
#include <boost/test/unit_test.hpp>
#include <boost/system/system_error.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/throw_exception.hpp>  // BOOST_THROW_EXCEPTION

#include <algorithm> // generate
#include <cstdlib> // rand
#include <string>
#include <vector>

#include <sys/stat.h>  // _S_IREAD

using test::provider::StreamFixture;
using test::stream_utils::verify_stream_read;

using comet::com_error;
using comet::com_ptr;

using boost::numeric_cast;
using boost::system::system_error;
using boost::system::get_system_category;
using boost::shared_ptr;

using std::exception;
using std::generate;
using std::rand;
using std::string;
using std::vector;

BOOST_FIXTURE_TEST_SUITE(StreamWrite, StreamFixture)

/**
 * Simply get a stream.
 */
BOOST_AUTO_TEST_CASE( get )
{
    com_ptr<IStream> stream = GetStream();
    BOOST_REQUIRE(stream);
}

/**
 * Try to get a writable stream to a read-only file.
 * This how we deal with opening failures.
 */
BOOST_AUTO_TEST_CASE( get_readonly )
{
    if (_wchmod(m_local_path.wstring().c_str(), _S_IREAD) != 0)
        BOOST_THROW_EXCEPTION(system_error(errno, get_system_category()));

    BOOST_REQUIRE_THROW(GetStream(), exception);
}

/**
 * Write one byte to stream, read it back and check that it is the same.
 */
BOOST_AUTO_TEST_CASE( write_one_byte )
{
    com_ptr<IStream> stream = GetStream();

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
BOOST_AUTO_TEST_CASE( write_a_string )
{
    com_ptr<IStream> stream = GetStream();

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
    BOOST_REQUIRE_EQUAL_COLLECTIONS(
        out.begin(), out.end(), in.begin(), in.end());
}

namespace {

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
BOOST_AUTO_TEST_CASE( write_large )
{
    com_ptr<IStream> stream = GetStream();

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
    BOOST_REQUIRE_EQUAL_COLLECTIONS(
        out.begin(), out.end(), in.begin(), in.end());
}

/**
 * Try to write to a locked file.
 * This tests how we deal with a failure in a write case.  In order to
 * force a failure we open the stream but then lock the first 30 bytes
 * of the file that's under it before trying to write to the stream.
 */
BOOST_AUTO_TEST_CASE( write_fail )
{
    com_ptr<IStream> stream = GetStream();

    // Open stream's file
    shared_ptr<void> file_handle(
        ::CreateFileW(
            m_local_path.wstring().c_str(), 
            GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL),
        ::CloseHandle);
    if (file_handle.get() == INVALID_HANDLE_VALUE)
        throw system_error(::GetLastError(), get_system_category());

    // Lock it
    if (!::LockFile(file_handle.get(), 0, 0, 30, 0))
        throw system_error(::GetLastError(), get_system_category());

    // Try to write to it via the stream
    try
    {
        string in = "Lorem ipsum dolor sit amet.\nbob\r\nsally";
        ULONG cbWritten = 0;
        BOOST_REQUIRE(FAILED(
            stream->Write(&in[0], numeric_cast<ULONG>(in.size()),
            &cbWritten)));
        BOOST_REQUIRE_EQUAL(cbWritten, 0U);
    }
    catch (...)
    {
        ::UnlockFile(file_handle.get(), 0, 0, 30, 0);
        throw;
    }
}

BOOST_AUTO_TEST_SUITE_END()
