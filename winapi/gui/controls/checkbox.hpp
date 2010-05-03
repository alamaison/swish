/**
    @file

    GUI check-box control.

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

#ifndef WINAPI_GUI_CONTROLS_CHECKBOX_HPP
#define WINAPI_GUI_CONTROLS_CHECKBOX_HPP
#pragma once

#include <winapi/gui/controls/control.hpp> // control base class
#include <winapi/gui/detail/window_impl.hpp> // window_impl

#include <boost/function.hpp> // function
#include <boost/shared_ptr.hpp> // shared_ptr

#include <string>

namespace winapi {
namespace gui {
namespace controls {

typedef boost::function<void (void)> on_click_callback;

class checkbox_impl : public winapi::gui::detail::window_impl
{
public:

	checkbox_impl(
		const std::wstring& text, short left, short top, short width,
		short height, on_click_callback click_callback)
		:
		winapi::gui::detail::window_impl(text, left, top, width, height),
		m_on_click(click_callback) {}

	std::wstring window_class() const { return L"button"; }

	DWORD style() const
	{
		return WS_CHILD | WS_VISIBLE | BS_CHECKBOX | WS_TABSTOP;
	}

	virtual void on_clicked() { m_on_click(); }

private:
	on_click_callback m_on_click;
};

class checkbox : public control<checkbox_impl>
{
public:
	checkbox(
		const std::wstring& text, short left, short top, short width,
		short height, on_click_callback click_callback=on_click_callback())
		:
		control<checkbox_impl>(
			boost::shared_ptr<checkbox_impl>(
				new checkbox_impl(
					text, left, top, width, height, click_callback))) {}

	std::wstring text() const { return impl()->text(); }
	short left() const { return impl()->left(); }
	short top() const { return impl()->top(); }
	short width() const { return impl()->width(); }
	short height() const { return impl()->height(); }
};

}}} // namespace winapi::gui::controls

#endif
