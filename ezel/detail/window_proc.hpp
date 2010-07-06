/**
    @file

    Window procedures.

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

#ifndef EZEL_WINDOW_PROC_HPP
#define EZEL_WINDOW_PROC_HPP
#pragma once

#include <winapi/gui/windows/dialog.hpp> // dialog
#include <winapi/gui/windows/window.hpp> // window
#include <winapi/trace.hpp> // trace

#include <cassert> // assert
#include <exception>

namespace ezel {
namespace detail {

class window_proc_base
{
public:
	virtual ~window_proc_base() {}

	virtual LRESULT do_default_handling(
		UINT message_id, WPARAM wparam, LPARAM lparam) = 0;
};

/**
 * Subclass a window with a standard window procdure (WNDPROC).
 */
class window_proc : public window_proc_base
{
public:

	/**
	 * Subclass window.
	 */
	window_proc(HWND hwnd, WNDPROC new_proc) :
		m_window(hwnd), m_proc(new_proc),
		m_sub_proc(m_window.change_window_procedure(m_proc)) {}

	/**
	 * Unsubclass window.
	 */
	~window_proc()
	{
		try
		{
			WNDPROC current_wndproc = m_window.window_procedure();
			if (current_wndproc == m_proc)
			{
				WNDPROC proc = m_window.change_window_procedure(m_sub_proc);
				(void)proc;
				assert(proc == m_proc); // mustn't remove someone else's
				                        // window procedure
			}
		}
		catch (const std::exception& e)
		{
			winapi::trace("window_proc destructor threw exception: %s")
				% boost::diagnostic_information(e);
		}
	}

	virtual LRESULT do_default_handling(
		UINT message_id, WPARAM wparam, LPARAM lparam)
	{
		return ::CallWindowProcW(
			m_sub_proc, m_window.hwnd(), message_id, wparam, lparam);
	}

protected:
	winapi::gui::window<wchar_t>& window() { return m_window; }

private:
	winapi::gui::window<wchar_t> m_window;
	WNDPROC m_proc;
	WNDPROC m_sub_proc; ///< Subclassed window's default message handler
};


/**
 * Window proc for dialog window.
 *
 * Delegates default processing to DefDlgProc as in the alternative
 * dialog handling method explained by Raymond Chen.
 *
 * Your dialog loop will still be called but only if default processing
 * is invoked.  Generally you should just return FALSE to allow the
 * dialog manager to handle the message.
 * 
 * @see http://blogs.msdn.com/b/oldnewthing/archive/2003/11/13/55662.aspx
 */
class dialog_proc : public window_proc
{
public:

	dialog_proc(HWND hwnd, WNDPROC new_proc) : window_proc(hwnd, new_proc) {}

	virtual LRESULT do_default_handling(
		UINT message_id, WPARAM wparam, LPARAM lparam)
	{
		return ::DefDlgProc(window().hwnd(), message_id, wparam, lparam);
	}
};

}} // namespace ezel::detail

#endif
