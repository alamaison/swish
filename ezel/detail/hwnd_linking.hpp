/**
    @file

    Low-level HWND manipulation.

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

#ifndef EZEL_DETAIL_WINDOW_HPP
#define EZEL_DETAIL_WINDOW_HPP
#pragma once

#include <winapi/error.hpp> // last_error
#include <winapi/gui/hwnd.hpp> // set_window_field, window_field

#include <boost/exception/errinfo_api_function.hpp> // errinfo_api_function
#include <boost/exception/info.hpp> // errinfo
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION
#include <boost/type_traits/remove_pointer.hpp> // remove_pointer

#include <Winuser.h> // SetWindowsHookEx
#include <Winbase.h> // GetCurrentThreadId

namespace ezel {
namespace detail {

/**
 * Store a value in the GWLP_USERDATA segment of the window descriptor.
 *
 * The value type must be no bigger than a LONG_PTR.
 */
template<typename T, typename U>
inline void store_user_window_data(HWND hwnd, const U& data)
{
	winapi::gui::set_window_field<T>(hwnd, GWLP_USERDATA, data);
}

/**
 * Store a value in the DWLP_USER segment of the window descriptor.
 *
 * The value type must be no bigger than a LONG_PTR.
 */
template<typename T, typename U>
inline void store_dialog_window_data(HWND hwnd, const U& data)
{
	winapi::gui::set_window_field<T>(hwnd, DWLP_USER, data);
}

/**
 * Get a value previously stored in the GWLP_USERDATA segment of the 
 * window descriptor.
 *
 * The value type must be no bigger than a LONG_PTR.
 *
 * @throws system_error if there is an error or if no value has yet been
 *         stored.
 * @note  Storing 0 will count as not having a previous value so will
 *        throw an exception.
 */
template<typename T, typename U>
inline U fetch_user_window_data(HWND hwnd)
{
	return winapi::gui::window_field<T, U>(hwnd, GWLP_USERDATA);
}

/**
 * Get a value previously stored in the DWLP_USER segment of the 
 * window descriptor.
 *
 * The value type must be no bigger than a LONG_PTR.
 *
 * @throws system_error if there is an error or if no value has yet been
 *         stored.
 * @note  Storing 0 will count as not having a previous value so will
 *        throw an exception.
 */
template<typename T, typename U>
inline U fetch_dialog_window_data(HWND hwnd)
{
	return winapi::gui::window_field<T, U>(hwnd, DWLP_USER);
}

typedef boost::shared_ptr<boost::remove_pointer<HHOOK>::type> hhook;

/**
 * Install a windows hook function for the current thread.
 *
 * The hook is uninstalled when the returned hhook goes out of scope.
 */
inline hhook windows_hook(int type, HOOKPROC hook_function)
{
	HHOOK hook = ::SetWindowsHookExW(
		type, hook_function, NULL, ::GetCurrentThreadId());
	if (hook == NULL)
		BOOST_THROW_EXCEPTION(
			boost::enable_error_info(winapi::last_error()) << 
			boost::errinfo_api_function("SetWindowsHookExW"));

	return hhook(hook, ::UnhookWindowsHookEx);
}

}} // namespace ezel::detail

#endif
