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

#ifndef EZEL_DETAIL_WINDOW_IMPL_HPP
#define EZEL_DETAIL_WINDOW_IMPL_HPP
#pragma once

#include <ezel/detail/command_handler_mixin.hpp> // command_handler_mixin
#include <ezel/detail/message_dispatch.hpp> // message_dispatch
#include <ezel/detail/window_link.hpp> // window_link

#include <winapi/gui/messages.hpp> // message
#include <winapi/gui/commands.hpp> // command
#include <winapi/gui/window.hpp> // window
#include <winapi/trace.hpp> // trace

#include <boost/exception/diagnostic_information.hpp> // diagnostic_information
#include <boost/noncopyable.hpp> // noncopyable
#include <boost/signal.hpp> // signal

#include <cassert> // assert
#include <string>
#include <vector>

namespace ezel {
namespace detail {

/**
 * We are EXPLICITLY bringing the winapi::gui::message classes into this
 * namespace.
 *
 * @todo  Eventually this should move to the top-level ezel namespace,
 *        not detail.
 */
using winapi::gui::message;


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
class window_impl :
	private boost::noncopyable, public command_handler_mixin
{
public:

	window_impl(
		const std::wstring& text, short left, short top, short width,
		short height)
		:
		m_window(NULL), m_real_window_proc(NULL),
		m_enabled(true), m_visible(true), m_text(text),
		m_left(left), m_top(top), m_width(width), m_height(height) {}

	virtual ~window_impl()
	{
		assert(!m_link.attached()); // why have we not detached?
	}

	bool is_active() const { return m_link.attached(); }

	virtual std::wstring window_class() const = 0;
	virtual DWORD style() const
	{
		return WS_VISIBLE | WS_TABSTOP;
	}

	short left() const { return m_left; }
	short top() const { return m_top; }
	short width() const { return m_width; }
	short height() const { return m_height; }

	std::wstring text() const
	{
		if (!is_active())
			return m_text;
		else
			return window().text<wchar_t>();
	}

	void text(const std::wstring& new_text)
	{
		if (!is_active())
			m_text = new_text;
		else
			window().text(new_text);
	}

	void visible(bool state)
	{
		if (!is_active())
			m_visible = state;
		else
			window().visible(state);
	}

	void enable(bool state)
	{
		if (!is_active())
			m_enabled = state;
		else
			window().enable(state);
	}

	/**
	 * Main message loop.
	 *
	 * The main purpose of this handler is to synchronise C++ wrapper
	 * and the real Win32 window.  The wrapper's fields can be set before the
	 * real window is created and callers need access to the fields after the
	 * real window is destroyed.  Therefore we push the data out to the
	 * window when it's created and pull it back just before it's destroyed.
	 *
	 * We do this with the @c WM_CREATE and @c WM_DESTROY messages (rather
	 * than @c WM_NCCREATE and @c WM_NCDESTROY) as we can't be sure of the
	 * fields' integrity outside of this 'safe zone'.  For example, when
	 * common controls 6 is enabled setting an icon before @c WM_CREATE
	 * fails to show the icon.
	 *
	 * To prevent capturing creation of windows not directly part of our
	 * dialog template, such as the system menu, we engage our CBT hook for
	 * as short a period as possible.  Therefore we have to detach ourselves
	 * in this function rather than from the CBT hook.
	 *
	 * @see ezel::detail::cbt_hook_function.
	 */
	LRESULT handle_message(unsigned int message, WPARAM wparam, LPARAM lparam)
	{
		return m_dispatch.dispatch_message(this, message, wparam, lparam);
	}

	/// @name Event handlers
	// @{

	/**
	 * @name Lifetime events
	 *
	 * The main purpose of these handlers is to synchronise C++ wrapper
	 * and the real Win32 window.  The wrapper's fields can be set before the
	 * real window is created and callers need access to the fields after the
	 * real window is destroyed.  Therefore we push the data out to the
	 * window when it's created and pull it back just before it's destroyed.
	 *
	 * We do this with the @c WM_CREATE and @c WM_DESTROY messages (rather
	 * than @c WM_NCCREATE and @c WM_NCDESTROY) as we can't be sure of the
	 * fields' integrity outside of this 'safe zone'.  For example, when
	 * common controls 6 is enabled setting an icon before @c WM_CREATE
	 * fails to show the icon.
	 */

	LRESULT on(message<WM_CREATE> m)
	{
		LRESULT res = default_message_handler(m);
		push();
		return res;
	}

	LRESULT on(message<WM_DESTROY> m)
	{
		pull();
		return default_message_handler(m);
	}

	/**
	 * To prevent capturing creation of windows not directly part of our
	 * dialog template, such as the system menu, we engage our CBT hook for
	 * as short a period as possible.  Therefore we have to detach ourselves
	 * in this function rather than from the CBT hook.
	 *
	 * @see ezel::detail::cbt_hook_function.
	 */
	LRESULT on(message<WM_NCDESTROY> m)
	{
		detach();
		return default_message_handler(m);
	}

	// @}

	LRESULT on(message<WM_SETTEXT> m)
	{
		m_on_text_change(m.text<wchar_t>());
		LRESULT res = default_message_handler(m);
		m_on_text_changed();
		return res;
	}

	// @}

	/**
	 * Perform default processing for a message.
	 *
	 * This method must call the window procedure of the wrapped window.
	 * Here it's done through a call to CallWindowProc but dialog windows
	 * must do this differently so should override this method.
	 */
	virtual LRESULT default_message_handler(
		UINT message_id, WPARAM wparam, LPARAM lparam)
	{
		return ::CallWindowProcW(
			m_real_window_proc, m_link.hwnd(), message_id, wparam, lparam);
	}

	/**
	 * Default WM_COMMAND message handler.
	 *
	 * Command message that aren't handled elsewhere are dispatched to this
	 * function.  By default, it does nothing.  Override if you want to
	 * to handle unhandled command messages.
	 */
	virtual void on(const winapi::gui::command_base& unknown)
	{
#ifdef _DEBUG
		window_impl* w = window_from_hwnd(unknown.control_hwnd());
		winapi::trace(
			"Unhandled command (code 0x%x) from window with title '%s'")
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
	 * We don't push the wrapper fields out to the Win32 window yet as it's
	 * much too early.  This is called by the CBT hook and at this point the
	 * window hasn't even recieved an WM_NCCREATE message yet.
	 *
	 * @todo  What if we get a failure partway through?
	 */
	void attach(HWND hwnd)
	{
		assert(!m_link.attached()); // an instance should only be attached once
		
		m_link = window_link<window_impl>(hwnd, this);
		m_window = winapi::gui::window<wchar_t>(hwnd);

		// Replace the window's own Window proc with ours.
		m_real_window_proc =
			window().change_window_procedure(window_impl_proc);
	}

	/// @name Events
	// @{

	boost::signal<void (const wchar_t*)>& on_text_change()
	{ return m_on_text_change; }

	boost::signal<void ()>& on_text_changed()
	{ return m_on_text_changed; }

	// @}

protected:
	
	window_link<window_impl> m_link;
	HWND hwnd() const { return m_link.hwnd(); }
	winapi::gui::window<wchar_t>& window() { return m_window; }
	const winapi::gui::window<wchar_t>& window() const { return m_window; }

	/**
	 * Suck data from real Win32 window object into the wrapper class.
	 *
	 * This method exists so that properties of the window are still available
	 * after the real window has been destroyed.
	 *
	 * Override this method when subclasses have other fields that need to
	 * be sucked out of the window.  In most cases the overriding method must
	 * call this method of the base class to synchronise all the fields.
	 */
	virtual void pull()
	{
		assert(is_active());

		m_text = window().text<wchar_t>();
	}

	/**
	 * Update Win32 window object from fields in this wrapper class.
	 *
	 * Fields can be set in the wrapper before the Win32 window is created.
	 * This window pushes those values out to the real window once it is
	 * created.
	 *
	 * Override this method when subclasses have other fields that need to
	 * be pushed out to the window.  In most cases the overriding method must
	 * call this method of the base class to synchronise all the fields.
	 *
	 * @todo  Some of this pushing is redundant as the values are set in
	 *        the dialogue template.  Not necessarily harmful but worth
	 *        further thought.
	 */
	virtual void push()
	{
		assert(is_active());

		window().text(m_text);
		window().enable(m_enabled);
		window().visible(m_visible);
	}

private:

	template<UINT N>
	LRESULT default_message_handler(const message<N>& m)
	{
		return default_message_handler(N, m.wparam(), m.lparam());
	}

	/**
	 * Break the two-way link between this C++ wrapper object and the
	 * Win32 window object.
	 * 
	 * The fields of the Win32 window should have been pulled in by our window
	 * proc when it recieved WM_DESTROY.  This message is the last point at
	 * which we can be sure of the fields' integrity.
	 *
	 * @bug  If someone has subclassed us but not removed their hook by the
	 *       time they pass us the the WM_NCDESTROY message (bad!) then we
	 *       never remove our hooks as we're not at the bottom of the
	 *       subclass chain.  The UpDown control seems to do this when
	 *       it subclasses its buddy control.
	 *
	 * @todo  Investigate SetWindowSubclass/RemoveWindowSubclass and
	 *        whether it might fix the unsubclassing bug.  Unfortunately,
	 *        it may not work on earlier Windows versions.
	 */
	void detach()
	{
		assert(m_link.attached()); // why are we detaching a detached wrapper?

		// Remove our window proc and put back the one it came with
		WNDPROC current_wndproc = window().window_procedure();
		if (current_wndproc == window_impl_proc)
		{
			WNDPROC window_proc =
				window().change_window_procedure(m_real_window_proc);
			assert(window_proc == window_impl_proc); // mustn't remove someone
													 // else's window proc
		}

		m_window = winapi::gui::window<wchar_t>(NULL);
		m_link = window_link<window_impl>();
	}

	HWND m_hwnd;
	winapi::gui::window<wchar_t> m_window;
	WNDPROC m_real_window_proc; ///< Wrapped window's default message
	                            ///< handler
	message_dispatcher<
		WM_CREATE, WM_DESTROY, WM_NCDESTROY, WM_SETTEXT> m_dispatch;

	bool m_enabled;
	bool m_visible;
	std::wstring m_text;
	short m_left;
	short m_top;
	short m_width;
	short m_height;

	/// @name Events
	// @{

	boost::signal<void (const wchar_t*)> m_on_text_change;
	boost::signal<void ()> m_on_text_changed;

	// @}
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

}} // namespace ezel::detail

#endif
