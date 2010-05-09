/**
    @file

    HWND wrapper class.

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

#ifndef WINAPI_GUI_WINDOW_HPP
#define WINAPI_GUI_WINDOW_HPP
#pragma once

#include "hwnd.hpp" // HWND-manipulating functions

#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION
#include <boost/type_traits/remove_pointer.hpp> // remove_pointer

#include <cassert> // assert
#include <stdexcept> // logic_error
#include <string> // basic_string

namespace winapi {
namespace gui {

typedef boost::shared_ptr<boost::remove_pointer<HWND>::type> hwnd_t;

namespace detail {

	namespace native {

	}

	struct null_deleter
	{
		void operator()(void const *) const {}
	};
}

class invalid_window_error : public std::logic_error
{
public:
	invalid_window_error() : std::logic_error(
		"Can't perform operation with a NULL window handle (HWND)") {}
};

template<typename T>
class window
{
public:

	/**
	 * Wrap a raw HWND without controlling its lifetime.
	 *
	 * The main purpose of this constructor is to access and modify a window
	 * that we didn't create and whose lifetime we don't own.  An example
	 * would be an HWND passed to us by the windows API.
	 *
	 * This contructor fakes a shared_ptr to the window by creating an hwnd_t
	 * with a destructor that doesn't destroy the window.
	 *
	 * The Win32 window must outlive this wrapper.  If any methods are called
	 * after it is destroyed it will likely result in a crash.
	 *
	 * A safer option is to use the constructor that takes a shared_ptr
	 * (hwnd_t) directly.  This will keep the window alive at least as long
	 * as the wrapper.
	 */
	explicit window(HWND hwnd) : m_hwnd(hwnd, detail::null_deleter()) {}
	explicit window(hwnd_t hwnd) : m_hwnd(hwnd) {}

	bool is_visible() const
	{
		throw_if_invalid();
		return is_window_visible(m_hwnd.get());
	}

	/**
	 * Show or hide window.
	 *
	 * @returns  Previous visibility.
	 */
	bool visible(bool state)
	{
		throw_if_invalid();
		return set_window_visibility(m_hwnd.get(), state);
	}

	bool is_enabled() const
	{
		throw_if_invalid();
		return is_window_enabled(m_hwnd.get());
	}

	/**
	 * Show or hide window.
	 *
	 * @returns  Previous enabled state.
	 */
	bool enable(bool state)
	{
		throw_if_invalid();
		return set_window_enablement(m_hwnd.get(), state);
	}

	/**
	 * The window's text.
	 *
	 * All Win32 windows have a text field.  Even icons and images.
	 *
	 * @todo  Add version that gets the text in the 'other' character width.
	 *        Regardless of the HWND type, we should be able to get the text
	 *        as both.
	 */
	template<typename U>
	std::basic_string<U> text() const
	{
		throw_if_invalid();
		return window_text<U>(m_hwnd.get());
	}

	/// Change window text.
	void text(const std::string& new_text) { generic_text(new_text); }

	/// Change window text. @overload text(const std::string& new_text)
	void text(const std::wstring& new_text) { generic_text(new_text); }

private:

	/**
	 * Change window text using a wide or a narrow string.
	 *
	 * This method exists to facilitate generic programming.  The public text()
	 * methods use concrete string types to allow their parameters to be
	 * given as a char* or a wchar_t* pointer (this isn't possible using
	 * a basic_string).  Those methods delegate to this one.
	 */
	template<typename U>
	void generic_text(const std::basic_string<U>& new_text)
	{
		throw_if_invalid();
		return window_text(m_hwnd.get(), new_text);
	}

	void throw_if_invalid() const
	{
		if (!m_hwnd)
			BOOST_THROW_EXCEPTION(invalid_window_error());

		// This additional check is not reliable enough to put in a release
		// build because I don't understand the MSDN remarks regarding
		// threading
		assert(::IsWindow(m_hwnd.get()));
	}

	hwnd_t m_hwnd;
};

}} // namespace winapi::gui

#endif
