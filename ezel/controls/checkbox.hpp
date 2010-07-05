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

#ifndef EZEL_CONTROLS_CHECKBOX_HPP
#define EZEL_CONTROLS_CHECKBOX_HPP
#pragma once

#include <ezel/control.hpp> // control base class
#include <ezel/detail/window_impl.hpp> // window_impl

#include <boost/function.hpp> // function
#include <boost/shared_ptr.hpp> // shared_ptr

#include <string>

namespace ezel {
namespace controls {

typedef boost::function<void (void)> on_click_callback;

class checkbox_impl : public ezel::detail::window_impl
{
public:
	typedef ezel::detail::window_impl super;

	typedef ezel::detail::command_map<BN_CLICKED> commands;

	virtual void handle_command(
		WORD command_id, WPARAM wparam, LPARAM lparam)
	{
		dispatch_command(this, command_id, wparam, lparam);
	}

	checkbox_impl(
		const std::wstring& text, short left, short top, short width,
		short height)
		:
		ezel::detail::window_impl(text, left, top, width, height)
		{}

	std::wstring window_class() const { return L"button"; }

	DWORD style() const
	{
		return WS_CHILD | WS_VISIBLE | BS_CHECKBOX | WS_TABSTOP;
	}

	boost::signal<void ()>& on_click() { return m_on_click; }

	void on(command<BN_CLICKED>) { m_on_click(); }

private:

	boost::signal<void ()> m_on_click;
	bool m_default;
};

class checkbox : public ezel::control<checkbox_impl>
{
public:
	checkbox(
		const std::wstring& text, short left, short top, short width,
		short height)
		:
		control<checkbox_impl>(
			boost::shared_ptr<checkbox_impl>(
				new checkbox_impl(text, left, top, width, height))) {}

	std::wstring text() const { return impl()->text(); }
	short left() const { return impl()->left(); }
	short top() const { return impl()->top(); }
	short width() const { return impl()->width(); }
	short height() const { return impl()->height(); }
};

}} // namespace ezel::controls

#endif
