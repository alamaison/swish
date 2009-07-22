/**
    @file

    Tests for the SFTP directory listing helper functions.

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
#include "swish/provider/SessionFactory.hpp"

#include "test/provider/StreamFixture.hpp"
#include "test/common_boost/helpers.hpp"

#include "swish/atl.hpp"

#include <boost/test/unit_test.hpp>

#include <string>
#include <vector>
#include <memory>

using test::provider::StreamFixture;

using ATL::CComPtr;

using std::string;
using std::vector;
using std::auto_ptr;


BOOST_FIXTURE_TEST_SUITE(StreamWrite, StreamFixture)

/**
 * Simply get a stream.
 */
BOOST_AUTO_TEST_CASE( get )
{
	CComPtr<IStream> spStream = GetStream();
	BOOST_REQUIRE(spStream);
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
	ULONG cbRead = 0;
	BOOST_REQUIRE_OK(spStream->Read(out, sizeof(out), &cbRead));
	BOOST_REQUIRE_EQUAL(cbRead, sizeof(out));
	BOOST_REQUIRE_EQUAL('M', out[0]);
	
	// Reading another byte should succeed but return 0 bytes read
	BOOST_REQUIRE_OK(spStream->Read(out, sizeof(out), &cbRead));
	BOOST_REQUIRE_EQUAL(cbRead, 0U);
}

/**
 * Write a sequence of characters.
 */
BOOST_AUTO_TEST_CASE( write_a_string )
{
	CComPtr<IStream> spStream = GetStream();

	string in = "Lorem ipsum dolor sit amet. ";
	ULONG cbWritten = 0;
	BOOST_REQUIRE_OK(spStream->Write(&in[0], in.size(), &cbWritten));
	BOOST_REQUIRE_EQUAL(cbWritten, in.size());

	// Reset seek pointer to beginning and read back
	LARGE_INTEGER move = {0};
	BOOST_REQUIRE_OK(spStream->Seek(move, STREAM_SEEK_SET, NULL));

	vector<char> out(in.size());
	ULONG cbRead = 0;
	BOOST_REQUIRE_OK(spStream->Read(&out[0], out.size(), &cbRead));
	BOOST_REQUIRE_EQUAL(cbRead, out.size());
	BOOST_REQUIRE_EQUAL_COLLECTIONS(
		out.begin(), out.end(), in.begin(), in.end());
	
	// Trying to read more should succeed but return 0 bytes read
	BOOST_REQUIRE_OK(spStream->Read(&out[0], out.size(), &cbRead));
	BOOST_REQUIRE_EQUAL(cbRead, 0U);
}

BOOST_AUTO_TEST_SUITE_END()
