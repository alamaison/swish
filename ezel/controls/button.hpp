/**
    @file

    GUI button control.

    @if license

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

#ifndef EZEL_CONTROLS_BUTTON_HPP
#define EZEL_CONTROLS_BUTTON_HPP
#pragma once

#include <ezel/control.hpp> // control base class
#include <ezel/detail/command_dispatch.hpp> // command_map
#include <ezel/detail/window_impl.hpp> // window_impl

#include <winapi/gui/commands.hpp> // command

#include <boost/shared_ptr.hpp> // shared_ptr
#include <boost/signal.hpp> // signal

#include <string>

namespace ezel {
namespace controls {

class button_impl : public ezel::detail::window_impl
{
public:
	typedef ezel::detail::window_impl super;

	typedef ezel::detail::command_map<BN_CLICKED> commands;

	virtual void handle_command(
		WORD command_id, WPARAM wparam, LPARAM lparam)
	{
		dispatch_command(this, command_id, wparam, lparam);
	}

	button_impl(
		const std::wstring& title, short left, short top, short width,
		short height, bool default)
		:
		ezel::detail::window_impl(title, left, top, width, height),
		m_default(default) {}

	std::wstring window_class() const { return L"button"; }

	DWORD style() const
	{
		DWORD style = WS_CHILD | WS_VISIBLE | WS_TABSTOP;
		
		style |= (m_default) ? BS_DEFPUSHBUTTON : BS_PUSHBUTTON;

		return style;
	}

	boost::signal<void ()>& on_click() { return m_on_click; }

	void on(command<BN_CLICKED>) { m_on_click(); }

private:

	boost::signal<void ()> m_on_click;
	bool m_default;
};

class button : public ezel::control<button_impl>
{
public:
	button(
		const std::wstring& title, short left, short top, short width,
		short height, bool default=false)
		:
		control<button_impl>(
			boost::shared_ptr<button_impl>(
				new button_impl(title, left, top, width, height, default))) {}

	boost::signal<void ()>& on_click() { return impl()->on_click(); }

	short left() const { return impl()->left(); }
	short top() const { return impl()->top(); }
	short width() const { return impl()->width(); }
	short height() const { return impl()->height(); }
};

}} // namespace ezel::controls

#endif
