/**
    @file

    Unit tests for CSftpStream exercising the write mechanism.

    @if license

    Copyright (C) 2009  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "swish/provider/SftpStream.hpp"  // Test subject

#include "test/provider/StreamFixture.hpp"
#include "test/common_boost/helpers.hpp"

#include "swish/atl.hpp"

#include <comet/error.h> // com_error

#include <boost/numeric/conversion/cast.hpp>  // numeric_cast
#include <boost/test/unit_test.hpp>
#include <boost/system/system_error.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/throw_exception.hpp>  // BOOST_THROW_EXCEPTION

#include <string>
#include <vector>

#include <sys/stat.h>  // _S_IREAD

using test::provider::StreamFixture;

using ATL::CComPtr;

using comet::com_error;

using boost::numeric_cast;
using boost::system::system_error;
using boost::system::get_system_category;
using boost::shared_ptr;

using std::string;
using std::vector;

BOOST_FIXTURE_TEST_SUITE(StreamWrite, StreamFixture)

namespace {
	void read_and_verify_return(
		char* data, ULONG data_size, IStream* stream)
	{
		ULONG total_bytes_read = 0;
		HRESULT hr = E_FAIL;
		do {
			ULONG bytes_requested = data_size - total_bytes_read;
			ULONG bytes_read = 0;

			hr = stream->Read(
				data + total_bytes_read, bytes_requested, &bytes_read);
			if (hr == S_OK)
			{
				// S_OK indicates a complete read so make sure this read
				// however many bytes were left from any previous (possibly
				// none) short reads
				BOOST_REQUIRE_EQUAL(bytes_read, bytes_requested);
				return;
			}
			else if (hr == S_FALSE)
			{
				// S_FALSE indicated a 'short' read so make sure it really
				// is short
				BOOST_CHECK_LT(bytes_read, bytes_requested);
				total_bytes_read += bytes_read;
			}
			else
			{
				// not really requiring S_OK; S_FALSE is fine too
				BOOST_REQUIRE_OK(hr);
			}
		} while (SUCCEEDED(hr) && (total_bytes_read < data_size));

		// Trying to read more should succeed but return 0 bytes read
		char buf[10];
		BOOST_REQUIRE_OK(stream->Read(buf, sizeof(buf), &total_bytes_read));
		BOOST_REQUIRE_EQUAL(total_bytes_read, 0U);
	}
}

/**
 * Simply get a stream.
 */
BOOST_AUTO_TEST_CASE( get )
{
	CComPtr<IStream> spStream = GetStream();
	BOOST_REQUIRE(spStream);
}

/**
 * Try to get a writable stream to a read-only file.
 * This how we deal with opening failures.
 */
BOOST_AUTO_TEST_CASE( get_readonly )
{
	if (_wchmod(m_local_path.file_string().c_str(), _S_IREAD) != 0)
		BOOST_THROW_EXCEPTION(system_error(errno, get_system_category()));

	BOOST_REQUIRE_THROW(GetStream(), com_error);
}

/**
 * Write one byte to stream, read it back and check that it is the same.
 */
BOOST_AUTO_TEST_CASE( write_one_byte )
{
	CComPtr<IStream> spStream = GetStream();

	// Write the character 'M' to the file
	char in[1] = {'M'};
	ULONG cbWritten = 0;
	BOOST_REQUIRE_OK(spStream->Write(in, sizeof(in), &cbWritten));
	BOOST_REQUIRE_EQUAL(cbWritten, sizeof(in));

	// Reset seek pointer to beginning and read back
	LARGE_INTEGER move = {0};
	BOOST_REQUIRE_OK(spStream->Seek(move, STREAM_SEEK_SET, NULL));

	char out[1];
	read_and_verify_return(out, sizeof(out), spStream);
	BOOST_REQUIRE_EQUAL('M', out[0]);
}

/**
 * Write a sequence of characters.
 */
BOOST_AUTO_TEST_CASE( write_a_string )
{
	CComPtr<IStream> spStream = GetStream();

	string in = "Lorem ipsum dolor sit amet. ";
	ULONG cbWritten = 0;
	BOOST_REQUIRE_OK(
		spStream->Write(&in[0], numeric_cast<ULONG>(in.size()), &cbWritten));
	BOOST_REQUIRE_EQUAL(cbWritten, in.size());

	// Reset seek pointer to beginning and read back
	LARGE_INTEGER move = {0};
	BOOST_REQUIRE_OK(spStream->Seek(move, STREAM_SEEK_SET, NULL));

	vector<char> out(in.size());
	read_and_verify_return(&out[0], numeric_cast<ULONG>(out.size()), spStream);
	BOOST_REQUIRE_EQUAL_COLLECTIONS(
		out.begin(), out.end(), in.begin(), in.end());
}

/**
 * Write a large buffer.
 */
BOOST_AUTO_TEST_CASE( write_large )
{
	CComPtr<IStream> spStream = GetStream();

	vector<char> in(1000000); // Doesn't need to be initialised

	ULONG cbWritten = 0;
	BOOST_REQUIRE_OK(
		spStream->Write(&in[0], numeric_cast<ULONG>(in.size()), &cbWritten));
	BOOST_REQUIRE_EQUAL(cbWritten, in.size());

	// Reset seek pointer to beginning and read back
	LARGE_INTEGER move = {0};
	BOOST_REQUIRE_OK(spStream->Seek(move, STREAM_SEEK_SET, NULL));

	vector<char> out(in.size());
	read_and_verify_return(&out[0], numeric_cast<ULONG>(out.size()), spStream);
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
	CComPtr<IStream> spStream = GetStream();

	// Open stream's file
	shared_ptr<void> file_handle(
		::CreateFile(
			m_local_path.file_string().c_str(), 
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
			spStream->Write(&in[0], numeric_cast<ULONG>(in.size()),
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
