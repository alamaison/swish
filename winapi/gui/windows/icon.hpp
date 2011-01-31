/**
    @file

    Icon HWND wrapper class.

    @if license

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

#ifndef WINAPI_GUI_WINDOWS_ICON_HPP
#define WINAPI_GUI_WINDOWS_ICON_HPP
#pragma once

#include <winapi/error.hpp> // last_error
#include <winapi/gui/windows/window.hpp> // window base class
#include <winapi/message.hpp> // send_message

#include <boost/exception/errinfo_api_function.hpp> // errinfo_api_function
#include <boost/exception/info.hpp> // errinfo
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <cassert> // assert

namespace winapi {
namespace gui {

/**
 * Wrapper around an icon (a STATIC window with SS_ICON style).
 *
 * @todo  Consider if it is worth checking class and style and throwing
 *        an exception if it fails.  
 */
template<typename T>
class icon_window : public window<T>
{
public:
	explicit icon_window(HWND hwnd) : window(hwnd) {}
	explicit icon_window(hwnd_t hwnd) : window(hwnd) {}

	HICON change_icon(HICON new_icon)
	{
		return winapi::send_message_return<T, HICON>(
			hwnd(), STM_SETIMAGE, IMAGE_ICON, new_icon);
	}
};

}} // namespace winapi::gui

#endif
