/**
    @file

    HWND wrapper implementation.

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

#ifndef WINAPI_GUI_DETAIL_WINDOW_IMPL_HPP
#define WINAPI_GUI_DETAIL_WINDOW_IMPL_HPP
#pragma once

#include <winapi/gui/detail/hwnd_linking.hpp> // store_user_window_data
#include <winapi/gui/messages.hpp> // message
#include <winapi/gui/commands.hpp> // command_handler_mixin
#include <winapi/hwnd.hpp> // window_text, window_field
#include <winapi/trace.hpp> // trace

#include <boost/exception/diagnostic_information.hpp> // diagnostic_information
#include <boost/noncopyable.hpp> // noncopyable

#include <cassert> // assert
#include <string>
#include <vector>

namespace winapi {
namespace gui {
namespace detail {

void catch_form_creation(HWND hwnd, unsigned int msg, LPARAM lparam);
void catch_form_destruction(HWND hwnd, unsigned int msg);

LRESULT CALLBACK window_impl_proc(
	HWND hwnd, unsigned int message, WPARAM wparam, LPARAM lparam);

struct handling_outcome
{
	enum type
	{
		fully_handled,
		partially_handled
	};
};

class window_impl;

/**
 * Fetch C++ wrapper pointer embedded in WND data.
 */
inline window_impl* window_from_hwnd(HWND hwnd)
{
	return fetch_user_window_data<wchar_t, window_impl*>(hwnd);
}

/**
 * Window handle (HWND) wrapper class.
 *
 * Only one instance must exist per HWND so this class in non-copyable.
 * Clients use it via facade classes that reference the single instances
 * via a shared pointer.
 *
 * The lifetime of the class has three phases:
 *
 *  - before it is connected to an HWND.
 *    The data in the fields of this class are the data that the Win32 window
 *    will be initialised with (via a dialog template) when the Window dialog
 *    manager calls CreateWindow.
 *
 *  - while connected to an HWND.
 *    Method of this class fetch their data directly from the Win32 object.
 *    Member fields are ignored.
 *
 *  - after detaching from an HWND (when the Win32 window is destroyed)
 *    The Win32 data is pulled in just before destruction and stored in the
 *    member fields.  Subsequent method calls use this data.
 */
class window_impl : private boost::noncopyable, public command_handler_mixin
{
public:

	//using command_handler_mixin::on;

	window_impl(
		const std::wstring& text, short left, short top, short width,
		short height)
		:
		m_hwnd(NULL), m_real_window_proc(NULL), m_text(text),
		m_left(left), m_top(top), m_width(width), m_height(height) {}

	virtual ~window_impl() {}

	bool is_active() const { return m_hwnd != NULL; }
	HWND hwnd() const { return m_hwnd; }


	virtual std::wstring window_class() const = 0;
	virtual DWORD style() const = 0;

	short left() const { return m_left; }
	short top() const { return m_top; }
	short width() const { return m_width; }
	short height() const { return m_height; }

	std::wstring text() const
	{
		if (!is_active())
			return m_text;
		
		return winapi::window_text<wchar_t>(hwnd());
	}

	void text(const std::wstring& new_text)
	{
		if (!is_active())
			m_text = new_text;
		
		winapi::window_text(hwnd(), new_text);
	}

	LRESULT handle_message(unsigned int message, WPARAM wparam, LPARAM lparam)
	{
		switch (message)
		{
		case WM_NCDESTROY:
			detach();
			break;

		default:
			break;
		}
		
		return ::CallWindowProcW(
			m_real_window_proc, m_hwnd, message, wparam, lparam);
	}


	/**
	 * Default WM_COMMAND message handler.
	 *
	 * Command message that aren't handled elsewhere are dispatched to this
	 * function.  By default, it does nothing.  Override if you want to
	 * to handle unhandled command messages.
	 */
	virtual void on(command_base unknown)
	{
#ifdef _DEBUG
		window_impl* w = window_from_hwnd(unknown.control_hwnd());
		trace("Unhandled command (code %x) from window with title '%s'")
			% unknown.command_code() % w->text();
#endif
	}

	/**
	 * Establish a two-way link between this C++ wrapper object and the
	 * Win32 window object.
	 *
	 * Also replace the Win32 window's message handling procedure (WNDPROC)
	 * with our own so that we can intercept any messages it is sent. This
	 * is otherwise known as subclassing.
	 *
	 * @todo  What if we get a failure partway through?
	 */
	void attach(HWND hwnd)
	{
		assert(m_hwnd == NULL); // an instance should only be attached once

		// Store object pointer in HWND and HWND in object
		store_user_window_data<wchar_t>(hwnd, this);
		m_hwnd = hwnd;

		// Replace the window's own Window proc with ours.
		m_real_window_proc = winapi::set_window_field<wchar_t>(
			m_hwnd, GWLP_WNDPROC, &window_impl_proc);
	}

	/**
	 * Break the two-way link between this C++ wrapper object and the
	 * Win32 window object.
	 *
	 * Before breaking the link, all data is pulled from the Win32 window
	 * object so this wrapper can continue to serve data after the real
	 * window's destruction.
	 */
	void detach()
	{
		assert(m_hwnd); // why are we trying to detach a detached wrapper?

		sync(); // pull in Win32 data

		// Remove our window proc and put back the one it came with
		WNDPROC window_proc = winapi::set_window_field<wchar_t>(
			m_hwnd, GWLP_WNDPROC, &window_impl_proc);
		assert(window_proc == window_impl_proc); // mustn't remove someone
		                                         // else's window proc
		                                         
		// Unlink the HWND
		store_user_window_data<wchar_t, window_impl*>(m_hwnd, 0);
		m_hwnd = NULL;
	}

private:

	/**
	 * Update fields in this wrapper class from data in Win32 window object.
	 */
	void sync()
	{
		m_text = winapi::window_text<wchar_t>(hwnd());
	}

	HWND m_hwnd;
	WNDPROC m_real_window_proc; ///< Wrapped window's default message
	                            ///< handler
	
	std::wstring m_text;
	short m_left;
	short m_top;
	short m_width;
	short m_height;
};

/**
 * Custom window procedure for wrapping HWNDs and intercepting their
 * messages.
 */
inline LRESULT CALLBACK window_impl_proc(
	HWND hwnd, unsigned int message, WPARAM wparam, LPARAM lparam)
{
	try
	{
		window_impl* w = window_from_hwnd(hwnd);
		return w->handle_message(message, wparam, lparam);
	}
	catch (std::exception& e)
	{
		// We should always be able to get our window.  If we were able to
		// replace the window proc with this one then we must have hooked
		// it correctly so why can't we find it now?

		winapi::trace("window_impl_proc exception: %s")
			% boost::diagnostic_information(e);

		assert(!"Something went very wrong here - we couldn't get our window");

		return ::DefWindowProcW(hwnd, message, wparam, lparam);
	}
}

}}} // namespace winapi::gui::detail

#endif
