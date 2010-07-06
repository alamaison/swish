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

#ifndef EZEL_FORM_HPP
#define EZEL_FORM_HPP
#pragma once

#include <ezel/control.hpp> // control
#include <ezel/control_parent_impl.hpp> // control_parent_impl
#include <ezel/detail/dialog_template.hpp>
                                   // build_in_memory_dialog_template
#include <ezel/detail/hooks.hpp> // creation_hooks
#include <ezel/detail/hwnd_linking.hpp> // fetch_user_window_data
#include <ezel/detail/window_impl.hpp> // window_impl
#include <ezel/detail/window_proc.hpp> // dialog_proc
#include <ezel/window.hpp> // window

#include <winapi/dynamic_link.hpp> // module_handle
#include <winapi/gui/commands.hpp> // command
#include <winapi/gui/messages.hpp> // message

#include <boost/bind.hpp> // bind
#include <boost/exception/errinfo_api_function.hpp> // errinfo_api_function
#include <boost/exception/info.hpp> // errinfo
#include <boost/function.hpp> // function
#include <boost/make_shared.hpp> // make_shared
#include <boost/shared_ptr.hpp> // shared_ptr
#include <boost/signal.hpp> // signal
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION
#include <boost/weak_ptr.hpp> // weak_ptr

#include <cassert> // assert
#include <string>
#include <vector>

#include <Winuser.h> // DialogBoxIndirectParam

namespace ezel {

namespace detail {

	INT_PTR CALLBACK dialog_message_handler(
		HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
	INT_PTR CALLBACK dialog_creation_message_handler(
		HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	/**
	 * Real form class implementation.
	 */
	class form_impl : public control_parent_impl
	{
	public:
		typedef control_parent_impl super;

		virtual LRESULT handle_message(
			UINT message, WPARAM wparam, LPARAM lparam)
		{
			return dispatch_message(this, message, wparam, lparam);
		}

		typedef message_map<WM_INITDIALOG, WM_ACTIVATE, WM_CLOSE> messages;

		form_impl(
			const std::wstring& title, short left, short top, short width,
			short height)
			:
			control_parent_impl(title, left, top, width, height) {}

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
					hwnd_owner, dialog_creation_message_handler,
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

		/// @name Event delegates
		// @{

		boost::signal<bool ()>& on_create() { return m_on_create; }

		boost::signal<void (bool)>& on_activating() { return m_on_activating; }
		boost::signal<void (bool)>& on_activate() { return m_on_activate; }

		boost::signal<void ()>& on_deactivating() { return m_on_deactivating; }
		boost::signal<void ()>& on_deactivate() { return m_on_deactivate; }

		// @}

		/// @name Message handlers
		// @{

		LRESULT on(message<WM_CLOSE> m)
		{
			end();
			return default_message_handler(m);
		}

		LRESULT on(message<WM_ACTIVATE> m)
		{
			if (m.active())
				m_on_activating(m.by_mouse());
			else if (m.deactive())
				m_on_deactivating();
			else
				assert(!"Inconsistent message state");

			LRESULT res = default_message_handler(m);

			if (m.active())
				m_on_activate(m.by_mouse());
			else if (m.deactive())
				m_on_deactivate();
			else
				assert(!"Inconsistent message state");

			return res;
		}

		LRESULT on(message<WM_INITDIALOG> /*message*/)
		{
			// All our controls should have been created by now so stop
			// monitoring window creation.  This prevents problems with
			// the system menu which is created later.
			unhook_window_creation();
			push();

			if (!m_on_create.empty())
				return m_on_create() == TRUE;
			else
				return TRUE; // give default control focus
		}
		
		// @}

	private:

		/**
		 * Replace the window's own window proc with ours.
		 */
		virtual void install_window_procedure()
		{
			window_procedure() =
				boost::make_shared<dialog_proc>(
					hwnd(), dialog_message_handler);
		}

		friend INT_PTR CALLBACK dialog_creation_message_handler(
			HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam);

		void hook_window_creation()
		{
			m_hooks = boost::make_shared<creation_hooks<wchar_t> >();
		}

		void unhook_window_creation()
		{
			m_hooks.reset();
		}

		/**
		 * The collection of controls on this form.
		 * They are held as smart pointers to ensure they stay alive as long
		 * as the form, regardless of how they are passed to add_control.
		 */
		std::vector<boost::shared_ptr<window_impl> > m_controls;
		boost::shared_ptr<creation_hooks<wchar_t> > m_hooks;


		/// @name Events
		// @{
		
		boost::signal<bool ()> m_on_create;

		boost::signal<void (bool)> m_on_activating;
		boost::signal<void (bool)> m_on_activate;
		boost::signal<void ()> m_on_deactivating;
		boost::signal<void ()> m_on_deactivate;

		// @}
	};

	/**
	 * Handle the bizarre return value rules for the dialog proc.
	 *
	 * @TODO  We always return false here after setting DWL_MSGRESULT. This
	 *        means that default processing is always invoked.  However, in
	 *        some cases we may wish to be able to suppress that.
	 *        A solution might be to implement one of the strategies explained
	 *        by Raymond Chen.
	 *
	 * @see http://blogs.msdn.com/b/oldnewthing/archive/2003/11/07/55619.aspx
	 * @see http://blogs.msdn.com/b/oldnewthing/archive/2003/11/12/55659.aspx
	 * @see http://blogs.msdn.com/b/oldnewthing/archive/2003/11/13/55662.aspx
	 */
	inline INT_PTR do_dialog_message_return(
		UINT message, LRESULT result, HWND hwnd)
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
				return FALSE; // always do default processing?
		}
	}

	/**
	 * Dialog proc hooking form classes to HWNDs on WM_INITDIALOG.
	 */
	inline INT_PTR CALLBACK dialog_creation_message_handler(
		HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		assert(msg != WM_CREATE); // apparently a dialog should never get this

		if (msg != WM_INITDIALOG)
			return FALSE;

		try
		{
			// we stashed a pointer to our C++ form object in the creation
			// data in the dialog template
			// now we extract it here and use it to set up a two-way link
			// between the C++ form object and the Win32 dialog object
			form_impl* this_form = reinterpret_cast<form_impl*>(lparam);
			this_form->attach(hwnd);

			LRESULT result = this_form->handle_message(msg, wparam, lparam);
			return do_dialog_message_return(msg, result, hwnd);
		}
		catch (const std::exception&) { /* ignore */ }

		return FALSE;
	}

	/**
	 * Dialog proc handling message dispatch for forms.
	 */
	inline INT_PTR CALLBACK dialog_message_handler(
		HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		assert(msg != WM_CREATE); // apparently a dialog should never get this

		try
		{
			form_impl* this_form = 
				static_cast<form_impl*>(window_from_hwnd(hwnd));

			LRESULT result = this_form->handle_message(msg, wparam, lparam);
			return do_dialog_message_return(msg, result, hwnd);
		}
		catch (const std::exception&) { /* ignore */ }

		return FALSE;
	}
}

class form : public window<detail::form_impl>
{
public:

	form(
		const std::wstring& title, short left, short top, short width,
		short height)
		: window(
			boost::make_shared<detail::form_impl>(
				title, left, top, width, height)) {}

	template<typename T>
	void add_control(const control<T>& control)
	{
		impl()->add_control(control.impl());
	}

	void show(HWND hwnd_owner=NULL)
	{
		impl()->show(hwnd_owner);
	}

	void end()
	{
		impl()->end();
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
		weak_form_reference weak_ref = impl();

		return boost::bind(
			&detail::form_impl::end, boost::bind(
				&weak_form_reference::lock, weak_ref));
	}

	/// @name  Event delegates.
	// @{

	boost::signal<bool ()>& on_create() { return impl()->on_create(); }
	boost::signal<void (bool)>& on_activating() { return impl()->on_activating(); }
	boost::signal<void (bool)>& on_activate() { return impl()->on_activate(); }

	boost::signal<void ()>& on_deactivating() { return impl()->on_deactivating(); }
	boost::signal<void ()>& on_deactivate() { return impl()->on_deactivate(); }

	// @}
};

} // namespace ezel

#endif
