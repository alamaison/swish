/**
    @file

    Windows message crackers.

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

#ifndef WINAPI_GUI_COMMANDS_HPP
#define WINAPI_GUI_COMMANDS_HPP
#pragma once

#include "messages.hpp" // message<WM_COMMAND>

#include <winapi/trace.hpp> // trace

#include <WinDef.h> // HWND, LOWORD, HIWORD
#include <WinUser.h> // commands IDs

namespace winapi {
namespace gui {

/**
 * Base class for unknown command IDs (or commands whose IDs cannot be
 * determined at compile-time, for example in the @c default case of
 * a @c switch statement.
 */
class command_base : public message<WM_COMMAND>
{
public:
	command_base(WPARAM wparam, LPARAM lparam) : message(wparam, lparam) {}
};

/**
 * Command message (e.g. BN_CLICKED).
 *
 * A subtype of WM_COMMAND messages.
 */
template<int CommandId>
class command : public command_base
{
public:
	command(WPARAM wparam, LPARAM lparam) : command_base(wparam, lparam) {}
};

namespace detail {

	template<int COMMAND_CODE, typename T>
	inline void do_command_dispatch(T* obj, WPARAM wparam, LPARAM lparam)
	{
		obj->on(command<COMMAND_CODE>(wparam, lparam));
	}

}

class command_handler_mixin
{
private:

	template<int COMMAND_CODE>
	inline void do_dispatch(WPARAM wparam, LPARAM lparam)
	{
		on(command<COMMAND_CODE>(wparam, lparam));
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
			on(command_base(wparam, lparam));
			return;
		}
	}

private:
	virtual void on(command_base) {}
	virtual void on(command<BN_CLICKED>) {}
	virtual void on(command<BN_DOUBLECLICKED>) {}
	virtual void on(command<STN_DBLCLK>) {}
	virtual void on(command<EN_UPDATE>) {}
	virtual void on(command<EN_CHANGE>) {}
};

}} // namespace winapi::gui

#endif
