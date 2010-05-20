/**
    @file

    Command dispatch.

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

#ifndef EZEL_COMMAND_HANDLER_MIXIN_HPP
#define EZEL_COMMAND_HANDLER_MIXIN_HPP
#pragma once

#include <winapi/gui/commands.hpp> // command_base, command

#include <winapi/trace.hpp> // trace

#include <WinUser.h> // commands IDs

namespace ezel {
namespace detail {

template<int COMMAND_CODE, typename T>
inline void do_command_dispatch(T* obj, WPARAM wparam, LPARAM lparam)
{
	obj->on(command<COMMAND_CODE>(wparam, lparam));
}

class command_handler_mixin
{
private:

	template<int COMMAND_CODE>
	inline void do_dispatch(WPARAM wparam, LPARAM lparam)
	{
		on(winapi::gui::command<COMMAND_CODE>(wparam, lparam));
	}

public:

	/**
	 * Dispatch a command message to the command handlers of an object.
	 *
	 * @param obj           Object with command handlers to dispatch the
	 *                      message to as a command object.
	 * @param command_code  Command ID e.g. BN_CLICKED.
	 * @param wparam        Message parameter 1.
	 * @param lparam        Message parameter 2.
	 */
	inline void dispatch_command_message(
		unsigned int command_code, WPARAM wparam, LPARAM lparam)
	{
		switch (command_code)
		{
		case BN_CLICKED: // also STN_CLICKED
			do_dispatch<BN_CLICKED>(wparam, lparam);
			return;
		case BN_DOUBLECLICKED:
			do_dispatch<BN_DOUBLECLICKED>(wparam, lparam);
			return;
		case STN_DBLCLK:
			do_dispatch<STN_DBLCLK>(wparam, lparam);
			return;
		case EN_UPDATE:
			do_dispatch<EN_UPDATE>(wparam, lparam);
			return;
		case EN_CHANGE:
			do_dispatch<EN_CHANGE>(wparam, lparam);
			return;
		default:
			on(winapi::gui::command_base(wparam, lparam));
			return;
		}
	}

private:
	virtual void on(const winapi::gui::command_base&) {}
	virtual void on(const winapi::gui::command<BN_CLICKED>&) {}
	virtual void on(const winapi::gui::command<BN_DOUBLECLICKED>&) {}
	virtual void on(const winapi::gui::command<STN_DBLCLK>&) {}
	virtual void on(const winapi::gui::command<EN_UPDATE>&) {}
	virtual void on(const winapi::gui::command<EN_CHANGE>&) {}
};

}} // namespace winapi::gui

#endif
