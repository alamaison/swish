/**
    @file

    GUI forms (aka dialogs)

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

#ifndef WINAPI_GUI_FORM_HPP
#define WINAPI_GUI_FORM_HPP
#pragma once

#include <winapi/dynamic_link.hpp> // module_handle
#include <winapi/gui/commands.hpp> // command
#include <winapi/gui/controls/control.hpp> // control
#include <winapi/gui/detail/dialog_template.hpp>
                                   // build_in_memory_dialog_template
#include <winapi/gui/detail/hooks.hpp> // creation_hooks
#include <winapi/gui/detail/hwnd_linking.hpp> // fetch_user_window_data
#include <winapi/gui/detail/window_impl.hpp> // window_impl
#include <winapi/gui/messages.hpp> // message

#include <boost/bind.hpp> // bind
#include <boost/exception/errinfo_api_function.hpp> // errinfo_api_function
#include <boost/exception/info.hpp> // errinfo
#include <boost/function.hpp> // function
#include <boost/make_shared.hpp> // make_shared
#include <boost/shared_ptr.hpp> // shared_ptr
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION
#include <boost/weak_ptr.hpp> // weak_ptr

#include <cassert> // assert
#include <string>
#include <vector>

#include <Winuser.h> // DialogBoxIndirectParam

namespace winapi {
namespace gui {

namespace detail {

	void catch_form_creation(
		HWND hwnd, unsigned int msg, LPARAM lparam);

	void catch_form_destruction(HWND hwnd, unsigned int msg);

	INT_PTR CALLBACK dialog_message_handler(
		HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam);

	/**
	 * Real form class implementation.
	 */
	class form_impl : public window_impl
	{
	public:

		using window_impl::on;

		form_impl(
			const std::wstring& title, short left, short top, short width,
			short height)
			:
			window_impl(title, left, top, width, height) {}

		std::wstring window_class() const { return L"#32770"; }
		DWORD style() const
		{ return DS_SETFONT | WS_VISIBLE | WS_POPUPWINDOW | DS_MODALFRAME; }

		void add_control(boost::shared_ptr<window_impl> control)
		{
			m_controls.push_back(control);
		}

		void show(HWND hwnd_owner)
		{
			std::vector<byte> buffer = build_dialog_template_in_memory(
				L"MS Shell Dlg", 8, text(), left(), top(), width(), height(),
				m_controls);

			hook_window_creation();
			try
			{
				INT_PTR rc = ::DialogBoxIndirectParamW(
					winapi::module_handle(),
					(buffer.empty()) ?
						NULL : reinterpret_cast<DLGTEMPLATE*>(&buffer[0]),
					hwnd_owner, dialog_message_handler,
					reinterpret_cast<LPARAM>(this));
				if (rc < 1)
					BOOST_THROW_EXCEPTION(
						boost::enable_error_info(winapi::last_error()) << 
						boost::errinfo_api_function(
							"DialogBoxIndirectParamW"));
			}
			catch (...)
			{
				unhook_window_creation();
				throw;
			}
		}

		void end()
		{
			// set the 2nd parameter to > 0 so we can detect error case from 
			// return value of DialogBoxIndirectParam
			if(!::EndDialog(hwnd(), 1))
				BOOST_THROW_EXCEPTION(
					boost::enable_error_info(winapi::last_error()) << 
					boost::errinfo_api_function("EndDialog"));
		}

	private:

		friend void catch_form_creation(
			HWND hwnd, unsigned int msg, LPARAM lparam);
		friend void catch_form_destruction(HWND hwnd, unsigned int msg);
		friend INT_PTR CALLBACK dialog_message_handler(
			HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam);

		void hook_window_creation()
		{
			m_hooks = boost::make_shared<creation_hooks<wchar_t> >();
		}

		void unhook_window_creation()
		{
			m_hooks.reset();
		}

		/// @name Message handlers
		// @{

		handling_outcome::type on(const message<WM_CLOSE>& /*message*/)
		{
			end();
			return handling_outcome::fully_handled;
		}

		/**
		 * What to do if this form is sent a command message by a child window.
		 *
		 * We first send the command to this form's command handlers and
		 * then reflect the command back to the control that sent it in case
		 * wants to react to it as well.  Commands handlers don't report 
		 * whether or not they handled messages so we always send it to both.
		 *
		 * XXX: is this true?  We could always add that capability.  Do we
		 *      want to?
		 */
		handling_outcome::type on(message<WM_COMMAND> command, LRESULT result)
		{
			this->dispatch_command_message(
				command.command_code(), command.wparam(), command.lparam());

			window_impl* w = window_from_hwnd(command.control_hwnd());
			
			w->dispatch_command_message(
				command.command_code(), command.wparam(), command.lparam());

			result = 0;
			return handling_outcome::fully_handled;
		}

		BOOL on(const message<WM_INITDIALOG>& /*message*/)
		{
			// All our controls should have been created by now so stop
			// monitoring window creation.  This prevents problems with
			// the system menu which is created later.
			unhook_window_creation();

			return TRUE; // give default control focus
		}
		
		// @}

		/**
		 * Dispatch the dialog message to the message handlers.
		 */
		handling_outcome::type dispatch_dialog_message(
			unsigned int msg, WPARAM wparam, LPARAM lparam, LRESULT& result)
		{
			switch (msg)
			{
			case WM_INITDIALOG:
				// no option not to handle this message
				result = on(message<WM_INITDIALOG>(wparam, lparam));
				return handling_outcome::fully_handled;

			case WM_CLOSE:
				return on(message<WM_CLOSE>());

			case WM_COMMAND:
				return on(message<WM_COMMAND>(wparam, lparam), result);

			default:
				result = 0;
				return handling_outcome::partially_handled;
			}
		}

		/**
		 * The collection of controls on this form.
		 * They are held as smart pointers to ensure they stay alive as long
		 * as the form, regardless of how they are passed to add_control.
		 */
		std::vector<boost::shared_ptr<window_impl> > m_controls;
		boost::shared_ptr<creation_hooks<wchar_t> > m_hooks;
	};

	/**
	 * Inform C++ wrapper of Win32 dialog window creation, if any.
	 */
	inline void catch_form_creation(
		HWND hwnd, unsigned int msg, LPARAM lparam)
	{
		if (msg != WM_INITDIALOG)
			return;

		// we stashed a pointer to our C++ form object in the creation
		// data in the dialog template
		// now we extract it here and use it to set up a two-way link
		// between the C++ form object and the Win32 dialog object
		form_impl* this_form = reinterpret_cast<form_impl*>(lparam);
		this_form->attach(hwnd);
	}

	/**
	 * Inform C++ wrapper of Win32 dialog window destruction, if any.
	 */
	inline void catch_form_destruction(HWND hwnd, unsigned int msg)
	{
		if (msg != WM_NCDESTROY)
			return;

		// inform our C++ wrapper that the Win32 object no longer exists
		// by setting its HWND to NULL

		// fetch pointer to C++ wrapper from the HWND
		form_impl* this_form = static_cast<form_impl*>(window_from_hwnd(hwnd));

		// break link
		// XXX: If this weren't done here it would be done anyway in the
		//      window hook so why are we doing it here?
		this_form->detach();
	}

	/**
	 * Handle the bizarre return value rules for the dialog proc.
	 */
	inline INT_PTR do_dialog_message_return(
		unsigned int message, bool was_message_handled, LRESULT result,
		HWND hwnd)
	{
		switch (message)
		{
			case WM_INITDIALOG:
			case WM_CHARTOITEM:
			case WM_COMPAREITEM:
			case WM_CTLCOLORBTN:
			case WM_CTLCOLORDLG:
			case WM_CTLCOLOREDIT:
			case WM_CTLCOLORLISTBOX:
			case WM_CTLCOLORSCROLLBAR:
			case WM_CTLCOLORSTATIC:
			case WM_QUERYDRAGICON:
			case WM_VKEYTOITEM:
				return result;
			default:
				::SetWindowLongW(hwnd, DWL_MSGRESULT, result);
				return was_message_handled;
		}
	}

	/**
	 * Dialog proc handling message dispatch for forms.
	 *
	 * @param hwnd    Handle of the parent dialog (form).
	 * @param msg     Message type
	 * @param wparam  1st message parameter
	 * @param lparam  2nd message parameter
	 */
	inline INT_PTR CALLBACK dialog_message_handler(
		HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam)
	{
		assert(msg != WM_CREATE); // apparently a dialog should never get this

		LRESULT result = 0;
		bool fully_handled = false;

		try
		{
			catch_form_creation(hwnd, msg, lparam);
		}
		catch (std::exception&) { /* ignore */ }

		try
		{
			form_impl* this_form = 
				static_cast<form_impl*>(window_from_hwnd(hwnd));

			assert(hwnd == this_form->hwnd());

			handling_outcome::type handled = 
				this_form->dispatch_dialog_message(
					msg, wparam, lparam, result);

			fully_handled = (handled == handling_outcome::fully_handled);
		}
		catch (std::exception&) { /* ignore */ }

		try
		{
			catch_form_destruction(hwnd, msg);
		}
		catch (std::exception&) { /* ignore */ }

		return do_dialog_message_return(msg, fully_handled, result, hwnd);
	}

}

class form
{
public:

	form(
		const std::wstring& title, short left, short top, short width,
		short height)
		: m_impl(new detail::form_impl(title, left, top, width, height)) {}

	template<typename T>
	void add_control(const winapi::gui::control<T>& control)
	{
		m_impl->add_control(control.impl());
	}

	void show(HWND hwnd_owner=NULL)
	{
		m_impl->show(hwnd_owner);
	}

	void end()
	{
		m_impl->end();
	}

	/**
	 * A functor that can be called to destroy the form instance.
	 *
	 * This allows users to write :
	 * @code btn.on_click().connect(frm.killer()) @endcode
	 * instead of
	 * @code btn.on_click().connect(bind(&form::end, ref(frm))) @endcode
	 *
	 * The functor holds a *weak* reference to the form to prevent circular
	 * references.  If it held a strong reference, when the functor is passed
	 * to a control owned by the form, the form would indirectly hold a
	 * reference and would never be destroyed.
	 */
	boost::function<void ()> killer()
	{
		typedef boost::weak_ptr<detail::form_impl> weak_form_reference;
		weak_form_reference weak_ref = m_impl;

		return boost::bind(
			&detail::form_impl::end, boost::bind(
				&weak_form_reference::lock, weak_ref));
	}

	std::wstring text() const { return m_impl->text(); }

private:
	boost::shared_ptr<detail::form_impl> m_impl; // pimpl
};

}} // namespace winapi::gui

#endif
