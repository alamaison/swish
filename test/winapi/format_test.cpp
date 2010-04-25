/**
    @file

    Tests for shell formatting functions.

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

#include <winapi/shell/format.hpp> // test subject

#include <comet/datetime.h> // datetime_t

#include <boost/test/unit_test.hpp>

#include <string>
#include <vector>

using winapi::shell::format_date_time;
using winapi::shell::format_filesize_kilobytes;

using comet::datetime_t;

using std::basic_string;
using std::string;
using std::vector;
using std::wstring;

BOOST_AUTO_TEST_SUITE(shell_format_tests)

namespace {
	
	datetime_t date()
	{
		return datetime_t(2010, 4, 21, 1, 2, 3, 4);
	}

	int get_date_format(
		LCID locale, DWORD flags, const SYSTEMTIME* date,
		const wchar_t* format, wchar_t* date_out, int buffer_size)
	{
		return ::GetDateFormatW(
			locale, flags, date, format, date_out, buffer_size);
	}

	int get_date_format(
		LCID locale, DWORD flags, const SYSTEMTIME* date,
		const char* format, char* date_out, int buffer_size)
	{
		return ::GetDateFormatA(
			locale, flags, date, format, date_out, buffer_size);
	}

	int get_time_format(
		LCID locale, DWORD flags, const SYSTEMTIME* date,
		const wchar_t* format, wchar_t* date_out, int buffer_size)
	{
		return ::GetTimeFormatW(
			locale, flags, date, format, date_out, buffer_size);
	}

	int get_time_format(
		LCID locale, DWORD flags, const SYSTEMTIME* date,
		const char* format, char* date_out, int buffer_size)
	{
		return ::GetTimeFormatA(
			locale, flags, date, format, date_out, buffer_size);
	}

	/**
	 * Type of pointer to get_date_format() and get_time_format() formatting 
	 * functions.
	 */
	template<typename T>
	struct formatter
	{
		typedef int (*type)(
			LCID locale, DWORD flags, const SYSTEMTIME* date,
			const T* format, T* date_out, int buffer_size);
	};

	/**
	 * Call given formatting function on given date and return as CString.
	 *
	 * The given function in **called twice**.  Once with a null buffer to
	 * determine necessary length and then with a buffer of that length to
	 * receive the output string.
	 *
	 * @param formatFunction  Date or time formatting function to call.
	 * @param st              Datetime to be formatted.
	 * @param dwFlags         Flags to control formatting (passed to formatting
	 *                        function).
	 */
	template<typename T>
	basic_string<T> do_format_function(
		typename formatter<T>::type formatter, const SYSTEMTIME& st,
		DWORD dwFlags=0)
	{
		int size = formatter(
			LOCALE_USER_DEFAULT, dwFlags, &st, NULL, NULL, 0);
		assert(size > 0);

		if (size > 0)
		{
			vector<T> buffer(size);
			size = formatter(
				LOCALE_USER_DEFAULT, dwFlags, &st, NULL, &buffer[0],
				static_cast<UINT>(buffer.size()));
			assert(size > 0);

			return basic_string<T>(
				&buffer[0], std::min<int>( // Must not embed NULL
					size, static_cast<int>(buffer.size())) - 1);
		}

		return basic_string<T>();
	}

	/**
	 * Format date portion of SYSTEMTIME according to user's locale.
	 */
	template<typename T>
	basic_string<T> format_date(const SYSTEMTIME& st)
	{
		return do_format_function<T>(&get_date_format, st);
	}

	/**
	 * Format time portion of SYSTEMTIME according to user's locale.
	 */
	template<typename T>
	basic_string<T> format_time(const SYSTEMTIME& st, DWORD flags=0)
	{
		return do_format_function<T>(&get_time_format, st, flags);
	}

	/**
	 * Format the date and time according to user locale but without seconds.
	 */
	template<typename T>
	basic_string<T> expected_default_date(const datetime_t& date)
	{
		SYSTEMTIME st;
		date.to_systemtime(&st);

		basic_string<T>::value_type space[] = {' ', '\0'};

		return format_date<T>(st) + space + format_time<T>(st, TIME_NOSECONDS);
	}
}

/**
 * Format default shell date.
 */
BOOST_AUTO_TEST_CASE( date_narrow )
{
	string str = format_date_time<char>(
		date(), FDTF_DEFAULT | FDTF_NOAUTOREADINGORDER);
	string expected = expected_default_date<char>(date());
	BOOST_CHECK_EQUAL(str, expected);
}

/**
 * Format default shell date (Unicode).
 */
BOOST_AUTO_TEST_CASE( date_wide )
{
	wstring str = format_date_time<wchar_t>(
		date(), FDTF_DEFAULT | FDTF_NOAUTOREADINGORDER);
	wstring expected = expected_default_date<wchar_t>(date());
	BOOST_CHECK_EQUAL(str, expected);
}

/**
 * Format a number as kilobytes.
 */
BOOST_AUTO_TEST_CASE( kb_narrow )
{
	string str = format_filesize_kilobytes<char>(549484123);
	BOOST_CHECK_GT(str.size(), 6U);
}

/**
 * Format a number as kilobytes (Unicode).
 */
BOOST_AUTO_TEST_CASE( kb_wide )
{
	wstring str = format_filesize_kilobytes<wchar_t>(549484123);
	BOOST_CHECK_GT(str.size(), 6U);
}

BOOST_AUTO_TEST_SUITE_END();
