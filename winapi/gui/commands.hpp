/**
    @file

    Windows message crackers.

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

#ifndef WINAPI_GUI_COMMANDS_HPP
#define WINAPI_GUI_COMMANDS_HPP
#pragma once

#include "messages.hpp" // message<WM_COMMAND>

namespace winapi {
namespace gui {

/**
 * Generic command.
 *
 * Base of all command and generaly used to indicate a message whose ID
 * was not found in a message map in order to invoke default handling.
 *
 * message<WM_COMMAND> could be used insteam but this intermediate class
 * explicitly indicates that the command is to be treated as a command
 * rather than as a message.  The handling may be different in these cases.
 */
class command_base : public message<WM_COMMAND>
{
public:
	command_base(WPARAM wparam, LPARAM lparam)
		: message<WM_COMMAND>(wparam, lparam) {}
};

/**
 * Command with specific ID.
 *
 * All commands are cracked the same where so there is only once command
 * template in contrast to messages which have individual template
 * specialisations for each message type.
 */
template<WORD CommandId>
class command : public message<WM_COMMAND>
{
public:
	command(WPARAM wparam, LPARAM lparam)
		: message<WM_COMMAND>(wparam, lparam) {}
};

}} // namespace winapi::gui

#endif
