/**
    @file

    Tests for icon functions.

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

#include <winapi/gui/icon.hpp> // test subject

#include <boost/test/unit_test.hpp>

#include <string>

using winapi::gui::load_icon;
using winapi::gui::standard_icon_type;

using std::string;
using std::wstring;

BOOST_AUTO_TEST_SUITE(icon_tests)

namespace {

	namespace native {

		inline HICON load_icon(HINSTANCE hinstance, const char* name)
		{ return ::LoadIconA(hinstance, name); }
		
		inline HICON load_icon(HINSTANCE hinstance, const wchar_t* name)
		{ return ::LoadIconW(hinstance, name); }
	}

	HICON load_legacy_icon(standard_icon_type::type type)
	{
		HICON icon = native::load_icon(
			NULL, MAKEINTRESOURCE(
				winapi::gui::detail::icon_type_to_ici_icon_num(type)));
		BOOST_REQUIRE(icon);

		return icon;
	}
}

/**
 * Load each type of system icon.
 */
BOOST_AUTO_TEST_CASE( load_system_icon )
{
	BOOST_CHECK(load_standard_icon(standard_icon_type::application));
	BOOST_CHECK(load_standard_icon(standard_icon_type::question));
	BOOST_CHECK(load_standard_icon(standard_icon_type::warning));
	BOOST_CHECK(load_standard_icon(standard_icon_type::error));
	BOOST_CHECK(load_standard_icon(standard_icon_type::information));
#if(WINVER >= 0x0400)
	BOOST_CHECK(load_standard_icon(standard_icon_type::windows_logo));
#endif
#if(WINVER >= 0x0600)
	BOOST_CHECK(load_standard_icon(standard_icon_type::shield));
#endif
}

/**
 * Loading the same system icon twice should yield the same handle.
 */
BOOST_AUTO_TEST_CASE( repeat_load_system_icon )
{
	HICON icon1 = load_standard_icon(standard_icon_type::error);
	HICON icon2 = load_standard_icon(standard_icon_type::error);
	BOOST_CHECK_EQUAL(icon1, icon2);
}

/**
 * Loading a system icon via the wrapped version of LoadImage should yield
 * the same handle returned by the legacy LoadIcon (wrapped only here in the
 * tests).
 */
BOOST_AUTO_TEST_CASE( load_image_load_icon )
{
	HICON icon1 = load_standard_icon(standard_icon_type::error);
	HICON icon2 = load_legacy_icon(standard_icon_type::error);
	BOOST_CHECK_EQUAL(icon1, icon2);
}

BOOST_AUTO_TEST_SUITE_END();
