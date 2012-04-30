/**
    @file

    Helper functions for tests that involve IStreams.

    @if license

    Copyright (C) 2011  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

    @endif
*/

#include "test/common_boost/helpers.hpp" // BOOST_REQUIRE_OK

#include <boost/numeric/conversion/cast.hpp> // numeric_cast
#include <boost/test/test_tools.hpp> // BOOST_CHECK, BOOST_REQUIRE

using boost::numeric_cast;

using comet::com_ptr;

namespace test {
namespace stream_utils {

namespace {

size_t verify_stream_read_(
	void* data, ULONG data_size, com_ptr<IStream> stream)
{
	ULONG total_bytes_read = 0;
	HRESULT hr = E_FAIL;
	do {
		ULONG bytes_requested = data_size - total_bytes_read;
		ULONG bytes_read = 0;

		hr = stream->Read(
			static_cast<unsigned char*>(data) + total_bytes_read,
			bytes_requested, &bytes_read);
		if (hr == S_OK)
		{
			// S_OK indicates a complete read so make sure this read
			// however many bytes were left from any previous (possibly
			// none) short reads
			BOOST_REQUIRE_EQUAL(bytes_read, bytes_requested);
			return total_bytes_read + bytes_read;
		}
		else if (hr == S_FALSE)
		{
			// S_FALSE indicated a 'short' read so make sure it really
			// is short
			BOOST_CHECK_LT(bytes_read, bytes_requested);
			total_bytes_read += bytes_read;
			break; // end of file
		}
		else
		{
			// not really requiring S_OK; S_FALSE is fine too
			BOOST_REQUIRE_OK(hr);
		}
	} while (hr == S_OK && (total_bytes_read < data_size));

	// Trying to read more should succeed but return 0 bytes read
	char buf[10];
	ULONG should_remain_zero = 0;
	BOOST_REQUIRE(
		SUCCEEDED(stream->Read(buf, sizeof(buf), &should_remain_zero)));
	BOOST_REQUIRE_EQUAL(should_remain_zero, 0U);

	return total_bytes_read;
}

}

size_t verify_stream_read(
	void* data, size_t data_size, com_ptr<IStream> stream)
{
	return verify_stream_read_(data, numeric_cast<ULONG>(data_size), stream);
}

}} // namespace test::stream_utils
