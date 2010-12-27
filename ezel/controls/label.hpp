/**
    @file

    GUI label (static text) control.

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

#ifndef EZEL_CONTROLS_LABEL_HPP
#define EZEL_CONTROLS_LABEL_HPP
#pragma once

#include <ezel/control.hpp> // control base class
#include <ezel/detail/window_impl.hpp> // window_impl

#include <boost/shared_ptr.hpp> // shared_ptr

#include <string>

namespace ezel {
namespace controls {

class label_impl : public ezel::detail::window_impl
{
public:

	label_impl(
		const std::wstring& text, short left, short top, short width,
		short height, DWORD custom_style)
		:
		ezel::detail::window_impl(text, left, top, width, height),
		m_custom_style(custom_style) {}

	std::wstring window_class() const { return L"static"; }
	DWORD style() const
	{
		DWORD style = ezel::detail::window_impl::style() |
			WS_CHILD | SS_LEFT | WS_GROUP | SS_NOTIFY;
		
		style &= ~WS_TABSTOP;
		style |= m_custom_style;

		return style;
	}

private:
	DWORD m_custom_style;
};

class label : public ezel::control<label_impl>
{
public:

	struct style
	{
		enum value
		{
			default = 0,
			ampersand_not_special = SS_NOPREFIX
		};
	};

	label(
		const std::wstring& text, short left, short top, short width,
		short height, style::value custom_style=style::default)
		:
		control<label_impl>(
			boost::shared_ptr<label_impl>(
				new label_impl(
					text, left, top, width, height, custom_style))) {}

	short left() const { return impl()->left(); }
	short top() const { return impl()->top(); }
	short width() const { return impl()->width(); }
	short height() const { return impl()->height(); }
};

}} // namespace ezel::controls

#endif
