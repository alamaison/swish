/**
    @file

    Window functions working on HWNDs.

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

#ifndef WINAPI_HWND_HPP
#define WINAPI_HWND_HPP
#pragma once

#include <winapi/error.hpp> // last_error

#include <boost/exception/errinfo_api_function.hpp> // errinfo_api_function
#include <boost/exception/info.hpp> // errinfo
#include <boost/numeric/conversion/cast.hpp> // numeric_cast
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <cassert> // assert
#include <stdexcept> // logic_error
#include <string>

#include <Winuser.h> // SetWindowLongPtr, GetWindowLongPtr, GetWindowTextLength
                     // GetWindowText, SetWindowText
namespace winapi {

namespace detail {

	namespace native {

		/// @name SetWindowLongPtr
		// @{
		template<typename T>
		inline LONG_PTR set_window_long_ptr(
			HWND hwnd, int index, LONG_PTR new_long);

		template<> inline LONG_PTR set_window_long_ptr<char>(
			HWND hwnd, int index, LONG_PTR new_long)
		{ return ::SetWindowLongPtrA(hwnd, index, new_long); }

		template<> inline LONG_PTR set_window_long_ptr<wchar_t>(
			HWND hwnd, int index, LONG_PTR new_long)
		{ return ::SetWindowLongPtrW(hwnd, index, new_long); }
		// @}

		/// @name GetWindowLongPtr
		// @{
		template<typename T>
		inline LONG_PTR get_window_long_ptr(HWND hwnd, int index);

		template<> inline LONG_PTR get_window_long_ptr<char>(
			HWND hwnd, int index)
		{ return ::GetWindowLongPtrA(hwnd, index); }

		template<> inline LONG_PTR get_window_long_ptr<wchar_t>(
			HWND hwnd, int index)
		{ return ::GetWindowLongPtrW(hwnd, index); }
		// @}

		/// @name GetWindowTextLength
		// @{
		template<typename T>
		inline int get_window_text_length(HWND hwnd);

		template<> inline int get_window_text_length<char>(HWND hwnd)
		{ return ::GetWindowTextLengthA(hwnd); }

		template<> inline int get_window_text_length<wchar_t>(HWND hwnd)
		{ return ::GetWindowTextLengthW(hwnd); }
		// @}

		/// @name GetWindowText
		// @{
		inline int get_window_text(
			HWND hwnd, char* out_buffer, int out_buffer_size)
		{ return ::GetWindowTextA(hwnd, out_buffer, out_buffer_size); }

		inline int get_window_text(
			HWND hwnd, wchar_t* out_buffer, int out_buffer_size)
		{ return ::GetWindowTextW(hwnd, out_buffer, out_buffer_size); }
		// @}
		// @}

		/// @name SetWindowText
		// @{
		inline BOOL set_window_text(HWND hwnd, const char* text)
		{ return ::SetWindowTextA(hwnd, text); }

		inline BOOL set_window_text(HWND hwnd, const wchar_t* text)
		{ return ::SetWindowTextW(hwnd, text); }
		// @}

	}
	
	/**
	 * Set a window's text.
	 */
	template<typename T>
	inline void window_text(HWND hwnd, const std::basic_string<T>& text)
	{
		if (!native::set_window_text(hwnd, text.c_str()))
			BOOST_THROW_EXCEPTION(
				boost::enable_error_info(winapi::last_error()) << 
				boost::errinfo_api_function("SetWindowText"));
	}
}

/**
 * Store a value in the given field of the window descriptor.
 *
 * The value type must be no bigger than a LONG_PTR.
 *
 * @param hwnd   Window handle whose data we are setting.
 * @param field  Which data field are we storing this data in.
 * @param value   Data to store.
 *
 * @return  The previous value.
 */
template<typename T, typename U>
inline U set_window_field(HWND hwnd, int field, const U& value)
{
	BOOST_STATIC_ASSERT(sizeof(U) <= sizeof(LONG_PTR));

	::SetLastError(0);

	LONG_PTR previous = detail::native::set_window_long_ptr<T>(
		hwnd, field, reinterpret_cast<LONG_PTR>(value));

	if (previous == 0 && ::GetLastError() != 0)
		BOOST_THROW_EXCEPTION(
			boost::enable_error_info(winapi::last_error()) <<
			boost::errinfo_api_function("SetWindowLongPtr"));

	return reinterpret_cast<U>(previous);
}

/**
 * Get a value previously stored in the window descriptor.
 *
 * The value type must be no bigger than a LONG_PTR.
 *
 * @param hwnd   Window handle whose data we are getting.
 * @param field  Which data field are we getting the value of.
 *
 * @throws  A system_error if @c no_throw is @c false and there is an
 *          error or if no value has yet been stored.  If no_throw is true
 *          it returns 0 instead.
 *
 * @note  Storing 0 will count as not having a previous value so will
 *        throw an exception unless @no_throw is @c true.
 */
template<typename T, typename U>
inline U window_field(HWND hwnd, int field, bool no_throw=false)
{
	BOOST_STATIC_ASSERT(sizeof(U) <= sizeof(LONG_PTR));

	LONG_PTR value = detail::native::get_window_long_ptr<T>(hwnd, field);
	if (value == 0 && !no_throw)
		BOOST_THROW_EXCEPTION(
			boost::enable_error_info(winapi::last_error()) << 
			boost::errinfo_api_function("GetWindowLongPtr"));

	return reinterpret_cast<U>(value);
}

/**
 * The *lower bound* on the length of a window's text.
 *
 * In other words, the text may actually be shorter but never longer.
 */
template<typename T>
inline size_t window_text_length(HWND hwnd)
{
	::SetLastError(0);

	int cch = detail::native::get_window_text_length<T>(hwnd);

	if (cch < 0)
		BOOST_THROW_EXCEPTION(
			std::logic_error("impossible (negative) text length"));
	if (cch == 0 && ::GetLastError() != 0)
		BOOST_THROW_EXCEPTION(
			boost::enable_error_info(winapi::last_error()) << 
			boost::errinfo_api_function("GetWindowTextLength"));

	return boost::numeric_cast<size_t>(cch);
}

/**
 * A window's text from its handle.
 */
template<typename T>
inline std::basic_string<T> window_text(HWND hwnd)
{
	std::vector<T> buffer(window_text_length<T>(hwnd) + 1); // +space for NULL

	::SetLastError(0);

	int cch = detail::native::get_window_text(
		hwnd, (buffer.empty()) ? NULL : &buffer[0], buffer.size());

	if (cch < 0)
		BOOST_THROW_EXCEPTION(
			std::logic_error("impossible (negative) text length"));
	if (cch == 0 && ::GetLastError() != 0)
		BOOST_THROW_EXCEPTION(
			boost::enable_error_info(winapi::last_error()) << 
			boost::errinfo_api_function("GetWindowText"));

	assert((unsigned long)cch <= buffer.size());

	if (buffer.empty())
		return std::basic_string<T>();
	else
		return std::basic_string<T>(
			&buffer[0],  boost::numeric_cast<size_t>(cch));
}

/**
 * Set a window's text (ANSI version).
 */
inline void window_text(HWND hwnd, const std::string& text)
{ return detail::window_text(hwnd, text); }

/**
 * Set a window's text.
 */
inline void window_text(HWND hwnd, const std::wstring& text)
{ return detail::window_text(hwnd, text); }

} // namespace winapi

#endif
