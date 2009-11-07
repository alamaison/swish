/**
    @file

    Tests for swish::exception namespace.

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

#include "swish/exception.hpp"  // Test subject

#include "test/common_boost/helpers.hpp"

#include <boost/test/unit_test.hpp>

#include <string>

using std::string;

BOOST_AUTO_TEST_SUITE(SwishException)

/**
 * HRESULT operator should return the HRESULT passed to the constructor.
 */
BOOST_AUTO_TEST_CASE( com_hresult )
{
	swish::exception::com_exception ex(E_FAIL);
	HRESULT hr = ex;
	BOOST_REQUIRE_EQUAL(hr, E_FAIL);
}

/**
 * Message returned by what() for a standard COM error, E_FAIL.
 * @warning  Not sure how this works with internationalisation.
 */
BOOST_AUTO_TEST_CASE( com_what_e_fail )
{
	swish::exception::com_exception ex(E_FAIL);
	BOOST_REQUIRE_EQUAL(ex.what(), "Unspecified error");
}

/**
 * Calling what() twice.
 * This tests string caching in what() which delay-renders the message 
 * on the first call but then simply returns the same message on subsequent 
 * calls.
 * @warning  Not sure how this works with internationalisation.
 */
BOOST_AUTO_TEST_CASE( com_what_twice )
{
	swish::exception::com_exception ex(E_FAIL);
	BOOST_REQUIRE_EQUAL(ex.what(), "Unspecified error");
	BOOST_REQUIRE_EQUAL(ex.what(), "Unspecified error");
}

/**
 * Call what() for an 'exotic' HRESULT.  
 * This test is trying to see if what() fails gracefully for non-Win32 
 * HRESULTs.
 */
BOOST_AUTO_TEST_CASE( com_what_exotic )
{
	const HRESULT WIA_ERROR_COVER_OPEN =
		MAKE_HRESULT(SEVERITY_ERROR, 33, 16);
	swish::exception::com_exception ex(WIA_ERROR_COVER_OPEN);
	BOOST_REQUIRE_EQUAL(ex.what(), "Unknown HRESULT: 0x80210010");
}

/**
 * Call make_com_exeption_from_win32 with a Win32 error code.
 * @warning  Not sure how this works with internationalisation.
 */
BOOST_AUTO_TEST_CASE( make_from_win32 )
{
	swish::exception::com_exception ex = 
		swish::exception::com_exception_from_win32(
			ERROR_SXS_DUPLICATE_IID);
	BOOST_REQUIRE_EQUAL(
		ex.what(),
		"Two or more components referenced directly or indirectly by the "
		"application manifest have proxies for the same COM interface IIDs.");
}

BOOST_AUTO_TEST_SUITE_END();
