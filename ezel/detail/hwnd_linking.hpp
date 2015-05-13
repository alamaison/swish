/**
    @file

    Low-level HWND manipulation.

    @if license

    Copyright (C) 2010, 2011  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include <washer/error.hpp> // last_error
#include <washer/gui/hwnd.hpp> // set_window_field, window_field

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
    washer::gui::set_window_field<T>(hwnd, GWLP_USERDATA, data);
}

/**
 * Store a value in the DWLP_USER segment of the window descriptor.
 *
 * The value type must be no bigger than a LONG_PTR.
 */
template<typename T, typename U>
inline void store_dialog_window_data(HWND hwnd, const U& data)
{
    washer::gui::set_window_field<T>(hwnd, DWLP_USER, data);
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
    return washer::gui::window_field<T, U>(hwnd, GWLP_USERDATA);
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
    return washer::gui::window_field<T, U>(hwnd, DWLP_USER);
}

}} // namespace ezel::detail

#endif
