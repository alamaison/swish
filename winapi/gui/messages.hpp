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

#ifndef WINAPI_GUI_MESSAGES_HPP
#define WINAPI_GUI_MESSAGES_HPP
#pragma once

#include <WinDef.h> // HWND, LOWORD, HIWORD
#include <WinUser.h> // WM_*

namespace winapi {
namespace gui {

class message_base
{
protected:
	message_base(WPARAM wparam, LPARAM lparam)
		: m_wparam(wparam), m_lparam(lparam) {}

	WPARAM wparam() const { return m_wparam; }
	LPARAM lparam() const { return m_lparam; }

private:
	WPARAM m_wparam;
	LPARAM m_lparam;
};

template<int MessageId>
class message;

template<>
class message<WM_COMMAND> : public message_base
{
public:
	message(WPARAM wparam, LPARAM lparam) : message_base(wparam, lparam) {}

	/**
	 * What happened (e.g. BN_CLICKED)?
	 *
	 * @returns
	 *  - 0 if the user selected a menu item
	 *  - 1 if the user invokes a keyboard accelerator
	 *  - Notification sent by the control that caused this message
	 *
	 * @note  Controls can send notification codes of 0 (e.g. BN_CLICKED)
	 *        or 1 (e.g. BN_PAINT) making the return value appear to
	 *        indicate a menu selection or keyboard accelerator.
	 *        call @c from_menu() or @c from_accelerator() if you need 
	 *        to know for sure.
	 */
	WORD command_code() const { return HIWORD(wparam()); }

	/**
	 * Who did it happen to (control ID)?    
	 */
	WORD control_id() const { return LOWORD(wparam()); }

	/**
	 * Who did it happen to (window HANDLE)?    
	 */
	HWND control_hwnd() const { return reinterpret_cast<HWND>(lparam()); }

	/** Is the source of this command message a control window? */
	bool from_control() const { return control_hwnd() == NULL; }

	/** Is the source of this command message a menu selection? */
	bool from_menu() const
	{ return command_code() == 0 && !from_control(); }

	/** Is the source of this command message a translated accelerator? */
	bool from_accelerator() const
	{ return command_code() == 1 && !from_control(); }

	/** Access to raw WPARAM for use when converting to command object. */
	WPARAM wparam() const { return message_base::wparam(); }
	
	/** Access to raw LPARAM for use when converting to command object. */
	LPARAM lparam() const { return message_base::lparam(); }
};

template<>
class message<WM_INITDIALOG> : public message_base
{
public:
	message(WPARAM wparam, LPARAM lparam) : message_base(wparam, lparam) {}

	/**
	 * Handle to the control that might receive default keyboard.
	 *
	 * To prevent this control receiving focus, return FALSE from the
	 * message handler.
	 */
	HWND control_hwnd() const { return reinterpret_cast<HWND>(lparam()); }

	/**
	 * Extra initialisation data set via DialogBoxIndirectParam.
	 */
	LPARAM init_data() const { return lparam(); }
};

template<>
class message<WM_CLOSE> {};

}} // namespace winapi::gui

#endif
