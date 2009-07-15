/**
    @file

    Tests common utils.

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

#include "swish/utils.hpp"  // Test subject

#include "test/common_boost/helpers.hpp"

#include <boost/test/unit_test.hpp>

#include <string>

using std::string;
using std::wstring;

namespace { // private

}

BOOST_AUTO_TEST_SUITE(SwishUtils)

/**
 * Narrow a wide string.
 */
BOOST_AUTO_TEST_CASE( narrowing_string )
{
	wstring wide = L"This was a wide-char string";
	string narrow = "This was a wide-char string";

	string out = swish::utils::WideStringToUtf8String(wide);

	BOOST_REQUIRE_EQUAL(out, narrow);
}

/**
 * Narrowing an empty string produces an empty string.
 */
BOOST_AUTO_TEST_CASE( narrowing_empty_string )
{
	wstring wide = L"";
	string narrow = "";

	string out = swish::utils::WideStringToUtf8String(wide);

	BOOST_REQUIRE_EQUAL(out, narrow);
}

/**
 * Widening a narrow string.
 */
BOOST_AUTO_TEST_CASE( widening_string )
{
	wstring wide = L"This was a wide-char string";
	string narrow = "This was a wide-char string";

	wstring out = swish::utils::Utf8StringToWideString(narrow);

	BOOST_REQUIRE_EQUAL(out, wide);
}

/**
 * Widening an empty string produces an empty string.
 */
BOOST_AUTO_TEST_CASE( widening_empty_string )
{
	wstring wide = L"";
	string narrow = "";

	wstring out = swish::utils::Utf8StringToWideString(narrow);

	BOOST_REQUIRE_EQUAL(out, wide);
}

/**
 * Test getting current user's username.
 */
BOOST_AUTO_TEST_CASE( get_current_user )
{
	wstring name = swish::utils::GetCurrentUser();

	BOOST_REQUIRE_GE(name.size(), wstring(L"a").size());
}

BOOST_AUTO_TEST_SUITE_END();
