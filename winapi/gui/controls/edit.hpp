/**
    @file

    GUI edit (text) control.

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

#ifndef WINAPI_GUI_CONTROLS_EDIT_HPP
#define WINAPI_GUI_CONTROLS_EDIT_HPP
#pragma once

#include <winapi/gui/controls/control.hpp> // control base class
#include <winapi/gui/detail/window_impl.hpp> // window_impl

#include <boost/function.hpp> // function
#include <boost/shared_ptr.hpp> // shared_ptr

#include <string>

namespace winapi {
namespace gui {
namespace controls {

typedef boost::function<void (void)> on_update_callback;

class edit_impl : public winapi::gui::detail::window_impl
{
public:

	edit_impl(
		const std::wstring& text, short left, short top, short width,
		short height, bool password, on_update_callback update_callback)
		:
		winapi::gui::detail::window_impl(text, left, top, width, height),
		m_on_update(update_callback), m_password(password) {}

	std::wstring window_class() const { return L"Edit"; }

	DWORD style() const
	{
		DWORD style = WS_CHILD | WS_VISIBLE | ES_LEFT | WS_BORDER | WS_TABSTOP
			| ES_AUTOHSCROLL;
		
		if (m_password)
			style |= ES_PASSWORD;

		return style;
	}

	virtual void on_update() { m_on_update(); }

private:
	on_update_callback m_on_update;
	bool m_password;
};

class edit : public control<edit_impl>
{
public:
	edit(
		const std::wstring& text, short left, short top, short width,
		short height, bool password=false,
		on_update_callback update_callback=on_update_callback())
		:
		control<edit_impl>(
			boost::shared_ptr<edit_impl>(
				new edit_impl(
					text, left, top, width, height, password,
					update_callback))) {}

	std::wstring text() const { return impl()->text(); }
	short left() const { return impl()->left(); }
	short top() const { return impl()->top(); }
	short width() const { return impl()->width(); }
	short height() const { return impl()->height(); }
};

}}} // namespace winapi::gui::controls

#endif
