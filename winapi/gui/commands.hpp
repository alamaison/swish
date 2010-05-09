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

}} // namespace winapi::gui

#endif
