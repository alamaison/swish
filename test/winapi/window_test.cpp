/**
    @file

    Tests for window class (and, indirectly, window functions).

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

#include <winapi/gui/window.hpp> // test subject

#include <boost/mpl/list.hpp> // list
#include <boost/test/unit_test.hpp>

#include <string>

using winapi::gui::hwnd_t;
using winapi::gui::window;

using boost::shared_ptr;

using std::basic_string;

BOOST_AUTO_TEST_SUITE(window_tests)

namespace {

	typedef boost::mpl::list<char, wchar_t> api_types;

	template<typename T> hwnd_t create(DWORD style);

	template<>
	hwnd_t create<wchar_t>(DWORD style)
	{
		HWND h = ::CreateWindowExW(
			0, L"STATIC", L"test ", style, 0, 0, 100, 100, NULL, NULL,
			NULL, NULL);
		BOOST_REQUIRE(h);
		return hwnd_t(h, ::DestroyWindow);
	}

	template<>
	hwnd_t create<char>(DWORD style)
	{
		HWND h = ::CreateWindowExA(
			0, "STATIC", "test ", style, 0, 0, 100, 100, NULL, NULL,
			NULL, NULL);
		BOOST_REQUIRE(h);
		return hwnd_t(h, ::DestroyWindow);
	}

}

/**
 * A window must not try to destroy a Win32 window passed as a raw HWND.
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( create_raw, T, api_types )
{
	hwnd_t h = create<T>(0);

	{
		window<T> w(h.get());
	}

	BOOST_REQUIRE(::IsWindow(h.get()));
}

/**
 * A window created with WS_VISIBLE must satisfy is_visible() predicate.
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( is_visible_true, T, api_types )
{
	hwnd_t h = create<T>(WS_VISIBLE);
	window<T> w(h);
	BOOST_CHECK(w.is_visible());
}

/**
 * A window created without WS_VISIBLE must fail is_visible() predicate.
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( is_visible_false, T, api_types )
{
	hwnd_t h = create<T>(0);
	window<T> w(h);
	BOOST_CHECK(!w.is_visible());
}

/**
 * A visible window must no longer be after visible(false).
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( visible_true, T, api_types )
{
	window<T> w(create<T>(WS_VISIBLE));
	bool previous = w.visible(false);
	BOOST_CHECK(!w.is_visible());
	BOOST_CHECK(previous);
}

/**
 * An invisible window must be visible after visible(true).
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( visible_false, T, api_types )
{
	window<T> w(create<T>(0));
	bool previous = w.visible(true);
	BOOST_CHECK(w.is_visible());
	BOOST_CHECK(!previous);
}

/**
 * A window created with WS_DISABLED must fail is_enabled() predicate.
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( is_enabled_false, T, api_types )
{
	window<T> w(create<T>(WS_DISABLED));
	BOOST_CHECK(!w.is_enabled());
}

/**
 * A window created without WS_DISABLED must satisfy is_enabled() predicate.
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( is_enabled_true, T, api_types )
{
	window<T> w(create<T>(0));
	BOOST_CHECK(w.is_enabled());
}

/**
 * An enabled window must no longer be after enable(false).
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( enable_true, T, api_types )
{
	window<T> w(create<T>(0));
	bool previous = w.enable(false);
	BOOST_CHECK(!w.is_enabled());
	BOOST_CHECK(previous);
}

/**
 * A disabled window must be enabled after enable(true).
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( enable_false, T, api_types )
{
	window<T> w(create<T>(WS_DISABLED));
	bool previous = w.enable(true);
	BOOST_CHECK(w.is_enabled());
	BOOST_CHECK(!previous);
}

/**
 * Fetched window text should match what we created it with.
 * The character width of the text is unrelated to the overall window type.
 * We test both here.
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( text_out, T, api_types )
{
	window<T> w(create<T>(0));
	char* expected_narrow = "test ";
	BOOST_CHECK_EQUAL(w.text<char>(), expected_narrow);
	wchar_t* expected_wide = L"test ";
	BOOST_CHECK_EQUAL(w.text<wchar_t>(), expected_wide);
}

/**
 * Fetched window text should change after we set it.
 * The character width of the text is unrelated to the overall window type.
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( text_in_narrow, T, api_types )
{
	window<T> w(create<T>(0));
	char* new_text = " bob\n£\r";
	w.text(new_text);
	BOOST_CHECK_EQUAL(w.text<char>(), new_text);
}

/**
 * Fetched window text should change after we set it (Unicode).
 * The character width of the text is unrelated to the overall window type.
 */
BOOST_AUTO_TEST_CASE_TEMPLATE( text_in_wide, T, api_types )
{
	window<T> w(create<T>(0));
	wchar_t* new_text = L" bob\n£\r";
	w.text(new_text);
	BOOST_CHECK_EQUAL(w.text<wchar_t>(), new_text);
}

BOOST_AUTO_TEST_SUITE_END();
