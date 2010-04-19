/**
    @file

    Tests for shell utility functions.

    @if licence

    Copyright (C) 2010  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include <test/common_boost/helpers.hpp> // wstring output

#include <winapi/shell/shell.hpp> // test subject

#include <boost/test/unit_test.hpp>

using namespace winapi::shell;

BOOST_AUTO_TEST_SUITE(shell_tests)

/**
 * Round-trip STRRET test (narrow in, wide out).
 */
BOOST_AUTO_TEST_CASE( ansi_to_wide )
{
	STRRET strret = string_to_strret("Narrow (ANSI) test string");

	BOOST_CHECK_EQUAL(
		strret_to_string<wchar_t>(strret), L"Narrow (ANSI) test string");
}

/**
 * Round-trip STRRET test (wide in, narrow out)..
 */
BOOST_AUTO_TEST_CASE( wide_to_ansi )
{
	STRRET strret = string_to_strret(L"Wide (Unicode) test string");

	BOOST_CHECK_EQUAL(
		strret_to_string<char>(strret), "Wide (Unicode) test string");
}


BOOST_AUTO_TEST_SUITE_END();
