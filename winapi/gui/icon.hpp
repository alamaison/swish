/**
    @file

    Icon management.

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

#ifndef WINAPI_GUI_ICON_HPP
#define WINAPI_GUI_ICON_HPP
#pragma once

#include "winapi/error.hpp" // last_error

#include <boost/exception/errinfo_api_function.hpp> // errinfo_api_function
#include <boost/exception/info.hpp> // errinfo
#include <boost/shared_ptr.hpp> // shared_ptr
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION
#include <boost/type_traits/remove_pointer.hpp> // remove_pointer

#include <stdexcept> // invalid_argument
#include <string>

#include <Winuser.h> // LoadImage

namespace winapi {
namespace gui {
	
typedef boost::shared_ptr<boost::remove_pointer<HICON>::type> hicon;

namespace detail {
	namespace native {

		inline HANDLE load_image(
			HMODULE module, const char* name, UINT type, int cx, int cy,
			UINT fuLoad)
		{ return ::LoadImageA(module, name, type, cx, cy, fuLoad); }

		inline HANDLE load_image(
			HMODULE module, const wchar_t* name, UINT type, int cx, int cy,
			UINT fuLoad)
		{ return ::LoadImageW(module, name, type, cx, cy, fuLoad); }
	}
}

struct standard_icon_type
{
	enum type
	{
		application,
		question,
		warning,
		error,
#if(WINVER >= 0x0400)
		windows_logo,
#endif
#if(WINVER >= 0x0600)
		shield,
#endif
		information
	};
};

namespace detail {

	/**
	 * Convert the enum value to the *numeric* form of the IDI_* resource ID.
	 *
	 * MAKEINTRESOURCE must be applied to the number before using in LoadIcon.
	 */
	inline int icon_type_to_ici_icon_num(standard_icon_type::type type)
	{
		switch (type)
		{
		case standard_icon_type::error:
			return 32513; // IDI_ERROR / IDI_HAND
		case standard_icon_type::warning:
			return 32515; // IDI_EXCLAMATION / IDI_WARNING
		case standard_icon_type::information:
			return 32516; // IDI_ASTERISK / IDI_INFORMATION
		case standard_icon_type::question:
			return 32514; // IDI_QUESTION
		case standard_icon_type::application:
			return 32512; // IDI_APPLICATION
#if(WINVER >= 0x0400)
		case standard_icon_type::windows_logo:
			return 32517; // IDI_WINLOGO
#endif
#if(WINVER >= 0x0600)
		case standard_icon_type::shield:
			return 32518; // IDI_SHIELD
#endif
		default:
			BOOST_THROW_EXCEPTION(
				std::invalid_argument("Unknown icon type"));
		}
	}

	/**
	 * Load an icon from a module by resource name.
	 *
	 * @param module         Handle to module containing the icon.
	 * @param resource_name  Name of the icon resource.
	 * @param width   Width to display icon or use actual width if 0.
	 * @param height  Height to display icon or use actual height if 0.
	*/
	template<typename T>
	inline hicon load_icon(
		HMODULE module, const T* name_or_ordinal, int width, int height)
	{
		HANDLE icon = detail::native::load_image(
			module, name_or_ordinal, IMAGE_ICON, width, height,
			LR_DEFAULTCOLOR);
		if (icon == NULL)
			BOOST_THROW_EXCEPTION(
				boost::enable_error_info(winapi::last_error()) << 
				boost::errinfo_api_function("LoadImage"));

		return hicon(static_cast<HICON>(icon), ::DestroyIcon);
	}
}

/**
 * Load an icon from a module by resource name.
 *
 * @param module         Handle to module containing the icon.
 * @param resource_name  Name of the icon resource.
 * @param width   Width to display icon or use actual width if 0.
 * @param height  Height to display icon or use actual height if 0.
 */
template<typename T>
inline HICON load_icon(
	HMODULE module, const std::basic_string<T>& resource_name,
	int width, int height)
{
	return detail::load_icon(module, name.c_str(), width, height);
}

/**
 * Load an icon from a module by resource ordinal.
 *
 * @param module  Handle to module containing the icon.
 * @param width   Width to display icon or use actual width if 0.
 * @param height  Height to display icon or use actual height if 0.
 *
 * @todo  We don't use the W form of LoadIcon here.  Should we?
 */
inline hicon load_icon(
	HMODULE module, int ordinal, int width, int height)
{
	return detail::load_icon(module, MAKEINTRESOURCE(ordinal), width, height);
}

/**
 * Load one of the standard system icons e.g., error and warning.
 *
 * The icon is loaded from the system cache so can't be resized or otherwise
 * modified.
 *
 * @param type    Which system icon to display.
 *
 * @todo  We don't use the W form of LoadIcon here.  Should we?
 * @todo  Wine passes LR_DEFAULTSIZE.  Do we need it?
 *
 * Although we use LoadImage to implement this function, the way we call
 * it makes it identical to a call to LoadIcon.
 *
 * @see http://source.winehq.org/source/dlls/user32/cursoricon.c#L1868
 */
inline HICON load_standard_icon(standard_icon_type::type type)
{
	HANDLE icon = detail::native::load_image(
		NULL, MAKEINTRESOURCE(detail::icon_type_to_ici_icon_num(type)),
		IMAGE_ICON, 0, 0, LR_SHARED);
	if (icon == NULL)
		BOOST_THROW_EXCEPTION(
			boost::enable_error_info(winapi::last_error()) << 
			boost::errinfo_api_function("LoadImage"));

	return static_cast<HICON>(icon);
}

}} // namespace winapi::gui

#endif
