/**
    @file

    Unit tests for CSftpStream exercising the read mechanism.

    @if licence

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

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/system/system_error.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/throw_exception.hpp>  // BOOST_THROW_EXCEPTION

#include <string>
#include <vector>

#include <sys/stat.h>  // _S_IREAD

using test::provider::StreamFixture;

using ATL::CComPtr;

using boost::filesystem::ofstream;
using boost::filesystem::path;
using boost::system::system_error;
using boost::system::system_category;
using boost::shared_ptr;

using std::string;
using std::vector;

namespace { // private

	const string TEST_DATA = "Humpty dumpty\nsat on the wall.\n\rHumpty ...";

	/**
	 * Fixture for tests that need to read data from an existing file.
	 */
	class StreamReadFixture : public StreamFixture
	{
	public:

		/**
		 * Put test data into a file in our sandbox.
		 */
		StreamReadFixture() : StreamFixture()
		{
			ofstream file(m_local_path, std::ios::binary);
			file << ExpectedData() << std::flush;
		}

		/**
		 * Create an IStream instance open for reading on a temporary file 
		 * in our sandbox.  The file contained the same data that 
		 * ExpectedData() returns.
		 */
		CComPtr<IStream> GetReadStream()
		{
			return GetStream(CSftpStream::read);
		}

		/**
		 * Return the data we expect to be able to read using the IStream.
		 */
		string ExpectedData()
		{
			return TEST_DATA;
		}
	};
}

BOOST_FIXTURE_TEST_SUITE(StreamRead, StreamReadFixture)

/**
 * Simply get a stream.
 */
BOOST_AUTO_TEST_CASE( get )
{
	CComPtr<IStream> spStream = GetReadStream();
	BOOST_REQUIRE(spStream);
}

/**
 * Get a read stream to a read-only file.
 * This tests that we aren't inadvertently asking for more permissions than
 * we need.
 */
BOOST_AUTO_TEST_CASE( get_readonly )
{
	if (_wchmod(m_local_path.file_string().c_str(), _S_IREAD) != 0)
		BOOST_THROW_EXCEPTION(system_error(errno, system_category));

	CComPtr<IStream> spStream = GetReadStream();
	BOOST_REQUIRE(spStream);
}

/**
 * Read a sequence of characters.
 */
BOOST_AUTO_TEST_CASE( read_a_string )
{
	CComPtr<IStream> spStream = GetReadStream();

	string expected = ExpectedData();
	ULONG cbRead = 0;
	vector<char> buf(expected.size());
	BOOST_REQUIRE_OK(spStream->Read(&buf[0], buf.size(), &cbRead));
	BOOST_REQUIRE_EQUAL(cbRead, expected.size());

	// Test that the bytes we read match
	BOOST_REQUIRE_EQUAL_COLLECTIONS(
		buf.begin(), buf.end(), expected.begin(), expected.end());
	
	// Trying to read more should succeed but return 0 bytes read
	BOOST_REQUIRE_OK(spStream->Read(&buf[0], buf.size(), &cbRead));
	BOOST_REQUIRE_EQUAL(cbRead, 0U);
}

/**
 * Read a sequence of characters from a read-only file.
 */
BOOST_AUTO_TEST_CASE( read_a_string_readonly )
{
	if (_wchmod(m_local_path.file_string().c_str(), _S_IREAD) != 0)
		BOOST_THROW_EXCEPTION(system_error(errno, system_category));

	CComPtr<IStream> spStream = GetReadStream();

	string expected = ExpectedData();
	ULONG cbRead = 0;
	vector<char> buf(expected.size());
	BOOST_REQUIRE_OK(spStream->Read(&buf[0], buf.size(), &cbRead));

	// Test that the bytes we read match
	BOOST_REQUIRE_EQUAL_COLLECTIONS(
		buf.begin(), buf.end(), expected.begin(), expected.end());
}

/**
 * Read a sequence of characters via a symbolic link.
 */
BOOST_AUTO_TEST_CASE( read_via_symlink )
{
	path link = create_link(m_local_path, L"test-link");

	CComPtr<IStream> spStream = GetStream(link, CSftpStream::read);

	string expected = ExpectedData();
	ULONG cbRead = 0;
	vector<char> buf(expected.size());
	BOOST_REQUIRE_OK(spStream->Read(&buf[0], buf.size(), &cbRead));

	// Test that the bytes we read match
	BOOST_REQUIRE_EQUAL_COLLECTIONS(
		buf.begin(), buf.end(), expected.begin(), expected.end());
}

/**
 * Try to read from a locked file.
 * This tests how we deal with a failure in a read case.  In order to
 * force a failure we open the stream but then lock the first 30 bytes
 * of the file that's under it before trying to read from the stream.
 */

BOOST_AUTO_TEST_CASE( read_fail )
{
	CComPtr<IStream> spStream = GetReadStream();

	// Open stream's file
	shared_ptr<void> file_handle(
		::CreateFile(
			m_local_path.file_string().c_str(), 
			GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL),
		::CloseHandle);
	if (file_handle.get() == INVALID_HANDLE_VALUE)
		throw system_error(::GetLastError(), system_category);

	// Lock it
	if (!::LockFile(file_handle.get(), 0, 0, 30, 0))
		throw system_error(::GetLastError(), system_category);

	// Try to read from the stream
	try
	{
		string expected = ExpectedData();
		ULONG cbRead = 0;
		vector<char> buf(expected.size());
		BOOST_REQUIRE(FAILED(spStream->Read(&buf[0], buf.size(), &cbRead)));
		BOOST_REQUIRE_EQUAL(cbRead, 0U);
	}
	catch (...)
	{
		::UnlockFile(file_handle.get(), 0, 0, 30, 0);
		throw;
	}
}

BOOST_AUTO_TEST_SUITE_END()
