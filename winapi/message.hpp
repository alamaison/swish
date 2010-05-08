/**
    @file

    Windows message system.

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

#ifndef WINAPI_MESSAGE_HPP
#define WINAPI_MESSAGE_HPP
#pragma once

#include <Winuser.h> // SendMessage

namespace winapi {

namespace detail {
	namespace native {

		template<typename T>
		LRESULT send_message(
			HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

		template<>
		LRESULT send_message<char>(
			HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
		{ return ::SendMessageA(hwnd, message, wparam, lparam); }

		template<>
		LRESULT send_message<wchar_t>(
			HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
		{ return ::SendMessageW(hwnd, message, wparam, lparam); }
	}
}

template<typename T, typename R, typename W, typename L>
inline R send_message(HWND hwnd, UINT message, W wparam, L lparam)
{
	return (R)(
		detail::native::send_message<T>(
			hwnd, message, (WPARAM)wparam, (LPARAM)lparam));
}

} // namespace winapi

#endif