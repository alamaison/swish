/**
    @file

    Unit tests for defcm callback implementation.

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

#include "swish/nse/default_context_menu_callback.hpp" // test subject

#include "test/common_boost/helpers.hpp"
#include <boost/test/unit_test.hpp>

#include <comet/error.h> // com_error
#include <comet/ptr.h> // com_ptr

#include <string>
#include <vector>

using swish::nse::default_context_menu_callback;

using comet::com_error;
using comet::com_ptr;

using std::string;
using std::vector;
using std::wstring;

BOOST_AUTO_TEST_SUITE(default_context_menu_callback_tests)

BOOST_AUTO_TEST_CASE( create )
{
	default_context_menu_callback();
}

BOOST_AUTO_TEST_CASE( unhandled_message )
{
	default_context_menu_callback callback;
	HRESULT hr = callback(NULL, NULL, (UINT)-1, 6, 7);
	BOOST_CHECK_EQUAL(hr, E_NOTIMPL);
}

namespace {

	class verb_callback : public default_context_menu_callback
	{
		virtual void verb(HWND, com_ptr<IDataObject>, UINT, wstring& verb_out)
		{
			verb_out = L"test";
		}

		virtual void verb(HWND, com_ptr<IDataObject>, UINT, string& verb_out)
		{
			verb_out = "another test";
		}
	};

}

BOOST_AUTO_TEST_CASE( verbw )
{
	verb_callback callback;
	vector<wchar_t> buffer(5, L'Z');
	HRESULT hr = callback(
		NULL, NULL, DFM_GETVERBW, MAKELONG(6, buffer.size()),
		(LPARAM)(&buffer[0]));
	BOOST_REQUIRE_OK(hr);

	wchar_t* expected = L"test";
	BOOST_CHECK_EQUAL_COLLECTIONS(
		buffer.begin(), buffer.end(), expected, expected + 5);
}

BOOST_AUTO_TEST_CASE( verbw_buffer_too_small )
{
	verb_callback callback;
	vector<wchar_t> buffer(4, L'Z');
	HRESULT hr = callback(
		NULL, NULL, DFM_GETVERBW, MAKELONG(6, buffer.size()),
		(LPARAM)(&buffer[0]));
	BOOST_CHECK(FAILED(hr));
}

BOOST_AUTO_TEST_SUITE_END();
