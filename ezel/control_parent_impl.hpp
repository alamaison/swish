/**
    @file

    Compound window parent.

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

#ifndef EZEL_CONTROL_PARENT_IMPL_HPP
#define EZEL_CONTROL_PARENT_IMPL_HPP
#pragma once

#include <ezel/detail/window_impl.hpp> // window_impl

#include <winapi/gui/messages.hpp> // message

#include <cassert> // assert

namespace ezel {
namespace detail {

/**
 * Parent of any window that receive WM_COMMAND message from one or more
 * children.
 */
class control_parent_impl : public window_impl
{
public:
	typedef window_impl super;

	typedef message_map<WM_COMMAND> messages;

	virtual LRESULT handle_message(
		UINT message, WPARAM wparam, LPARAM lparam)
	{
		return dispatch_message(this, message, wparam, lparam);
	}

	/**
	 * What to do if this window is sent a command message by a child window.
	 *
	 * We reflect the command back to the control that sent it in case
	 * wants to react to it.
	 */
	LRESULT on(message<WM_COMMAND> m)
	{
		window_impl* w = window_from_hwnd(m.control_hwnd());
		assert(w != this);
		
		w->handle_command(m.command_code(), m.wparam(), m.lparam());
		
		return default_message_handler(m);
	}

protected:

	control_parent_impl(
		const std::wstring& title, short left, short top, short width,
		short height)
		:
		window_impl(title, left, top, width, height) {}
};

}} // namespace ezel::detail

#endif
