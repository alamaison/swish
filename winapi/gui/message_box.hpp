/**
    @file

    MessageBox C++ wrapper.

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

#ifndef WINAPI_GUI_MESSAGE_DIALOG_HPP
#define WINAPI_GUI_MESSAGE_DIALOG_HPP
#pragma once

#include <winapi/error.hpp> // last_error

#include <boost/exception/errinfo_api_function.hpp> // errinfo_api_function
#include <boost/exception/info.hpp> // errinfo
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <stdexcept> // invalid_argument
#include <string>

#include <Winuser.h> // MessageBox

namespace winapi {
namespace gui {
namespace message_box {

namespace detail {
	namespace native {

		inline int message_box(
			HWND hwnd, const char* text, const char* caption,
			unsigned int type)
		{ return ::MessageBoxA(hwnd, text, caption, type); }
		
		inline int message_box(
			HWND hwnd, const wchar_t* text, const wchar_t* caption,
			unsigned int type)
		{ return ::MessageBoxW(hwnd, text, caption, type); }
	}
}

namespace box_type
{
	enum type
	{
		ok,
		ok_cancel,
		abort_retry_ignore,
#if(WINVER >= 0x0500)
		cancel_try_continue,
#endif
		yes_no_cancel,
		yes_no,
		retry_cancel
	};
}

namespace icon_type
{
	enum type
	{
		none,
		question,
		warning,
		error,
		information,
	};
}

namespace button_type
{
	enum type
	{
		ok,
		cancel,
		abort,
		retry,
		ignore,
#if(WINVER >= 0x0400)
		close,
		help,
#endif
#if(WINVER >= 0x0500)
		try_again,
		cont,
#endif
		yes,
		no
	};
}

namespace detail {

	inline button_type::type mb_button_to_button_type(int button)
	{
		switch(button)
		{
		case IDOK:
			return button_type::ok;
		case IDCANCEL:
			return button_type::cancel;
		case IDABORT:
			return button_type::abort;
		case IDRETRY:
			return button_type::retry;
		case IDIGNORE:
			return button_type::ignore;
		case IDYES:
			return button_type::yes;
		case IDNO:
			return button_type::no;
#if(WINVER >= 0x0400)
		case IDCLOSE:
			return button_type::close;
		case IDHELP:
			return button_type::help;
#endif
#if(WINVER >= 0x0500)
		case IDTRYAGAIN:
			return button_type::try_again;
		case IDCONTINUE:
			return button_type::cont;
#endif
		default:
			BOOST_THROW_EXCEPTION(
				std::invalid_argument("Unknown button type"));
		}
	}

	inline long box_type_to_mb_box(box_type::type type)
	{
		switch (type)
		{
		case box_type::ok:
			return MB_OK;
		case box_type::ok_cancel:
			return MB_OKCANCEL;
		case box_type::abort_retry_ignore:
			return MB_ABORTRETRYIGNORE;
#if(WINVER >= 0x0500)
		case box_type::cancel_try_continue:
			return MB_CANCELTRYCONTINUE;
#endif
		case box_type::yes_no_cancel:
			return MB_YESNOCANCEL;
		case box_type::yes_no:
			return MB_YESNO;
		case box_type::retry_cancel:
			return MB_RETRYCANCEL;
		default:
			BOOST_THROW_EXCEPTION(
				std::invalid_argument("Unknown message box type"));
		}
	}

	inline unsigned int button_count_from_box_type(box_type::type type)
	{
		switch (type)
		{
		case box_type::ok:
			return 1;
		case box_type::ok_cancel:
			return 2;
		case box_type::abort_retry_ignore:
			return 3;
#if(WINVER >= 0x0500)
		case box_type::cancel_try_continue:
			return 3;
#endif
		case box_type::yes_no_cancel:
			return 3;
		case box_type::yes_no:
			return 2;
		case box_type::retry_cancel:
			return 2;
		default:
			BOOST_THROW_EXCEPTION(
				std::invalid_argument("Unknown message box type"));
		}
	}

	inline unsigned int default_to_mb_default(unsigned int button)
	{
		switch (button)
		{
		case 1:
			return MB_DEFBUTTON1;
		case 2:
			return MB_DEFBUTTON2;
		case 3:
			return MB_DEFBUTTON3;
		case 4:
			return MB_DEFBUTTON4;
		default:
			BOOST_THROW_EXCEPTION(
				std::invalid_argument("Impossible default button index"));
		}
	}

	inline long icon_type_to_mb_icon(icon_type::type type)
	{
		switch (type)
		{
		case icon_type::error:
			return MB_ICONHAND; // MB_ICONERROR
		case icon_type::warning:
			return MB_ICONEXCLAMATION; // MB_ICONWARNING
		case icon_type::information:
			return MB_ICONASTERISK; // MB_ICONASTERISK
		case icon_type::question:
			return MB_ICONQUESTION;
		case icon_type::none:
			return 0x00000000L;
		default:
			BOOST_THROW_EXCEPTION(
				std::invalid_argument("Unknown icon type"));
		}
	}

	template<typename T>
	inline button_type::type message_box(
		HWND hwnd, const std::basic_string<T>& message, 
		const std::basic_string<T>& title, box_type::type box,
		icon_type::type icon, unsigned int default_button, bool show_help)
	{
		unsigned int type = 0;
		type |= detail::box_type_to_mb_box(box);
		type |= detail::icon_type_to_mb_icon(icon);

		unsigned int max_default_button = button_count_from_box_type(box);
		if (show_help)
		{
			type |= MB_HELP;
			max_default_button++;
		}

		if (default_button > max_default_button)
			BOOST_THROW_EXCEPTION(
				std::invalid_argument("Default button out-of-range"));
		type |= default_to_mb_default(default_button);

		int rc = native::message_box(
			hwnd, message.c_str(), title.c_str(), type);
		if (rc == 0)
			BOOST_THROW_EXCEPTION(
				boost::enable_error_info(winapi::last_error()) << 
				boost::errinfo_api_function("MessageBox"));

		return detail::mb_button_to_button_type(rc);
	}

}

/**
 * Display a message to the user.
 *
 * @param hwnd            Handle to the parent window. 
 * @param message         Message text. 
 * @param title           Window title.
 * @param box             Button configuration for this box.
 * @param icon            Type of icon to display or none. 
 * @param show_help	      true to show, false to hide the help button.
 * @param default_button  Index of default button.
 *
 * @returns which button the user clicked.
 */
inline button_type::type message_box(
	HWND hwnd, const std::string& message, const std::string& title,
	box_type::type box, icon_type::type icon,
	unsigned int default_button=1, bool show_help=false)
{
	return detail::message_box(
		hwnd, message, title, box, icon, default_button, show_help);
}

/**
 * Display a message to the user.
 *
 * @param hwnd            Handle to the parent window. 
 * @param message         Message text. 
 * @param title           Window title.
 * @param box             Button configuration for this box.
 * @param icon            Type of icon to display or none. 
 * @param show_help	      true to show, false to hide the help button.
 * @param default_button  Index of default button.
 *
 * @returns which button the user clicked.
 */
inline button_type::type message_box(
	HWND hwnd, const std::wstring& message, const std::wstring& title,
	box_type::type box, icon_type::type icon,
	unsigned int default_button=1, bool show_help=false)
{
	return detail::message_box(
		hwnd, message, title, box, icon, default_button, show_help);
}

}}} // namespace winapi::gui::message_box

#endif
