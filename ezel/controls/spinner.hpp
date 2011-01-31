/**
    @file

    GUI spinner control.

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

#ifndef EZEL_CONTROLS_SPINNER_HPP
#define EZEL_CONTROLS_SPINNER_HPP
#pragma once

#include <ezel/control.hpp> // control base class
#include <ezel/detail/window_impl.hpp> // window_impl

#include <winapi/message.hpp> // send_message

#include <boost/shared_ptr.hpp> // shared_ptr
#include <boost/cstdint.hpp> // int32_t

#include <string>

namespace ezel {
namespace controls {

class spinner_impl : public ezel::detail::window_impl
{
protected:
	typedef ezel::detail::window_impl super;

public:

	spinner_impl(
		short left, short top, short width, short height,
		boost::int32_t minimum, boost::int32_t maximum,
		boost::int32_t initial_value, DWORD custom_style)
		:
		ezel::detail::window_impl(L"", left, top, width, height),
		m_min(minimum), m_max(maximum), m_value(initial_value),
		m_custom_style(custom_style) {}

	std::wstring window_class() const { return L"msctls_updown32"; }
	DWORD style() const
	{
		DWORD style = ezel::detail::window_impl::style() |
			WS_CHILD | UDS_ARROWKEYS | UDS_SETBUDDYINT | UDS_AUTOBUDDY;
		style |= m_custom_style;

		return style;
	}

	void range(boost::int32_t minimum, boost::int32_t maximum)
	{
		if (!is_active())
		{
			m_min = minimum;
			m_max = maximum;
		}
		else
		{
			winapi::send_message<wchar_t>(
				hwnd(), UDM_SETRANGE32, minimum, maximum);
		}
	}

	boost::int32_t value(boost::int32_t v)
	{
		if (!is_active())
			m_value = v;
		else
			return winapi::send_message_return<wchar_t, boost::int32_t>(
				hwnd(), UDM_SETPOS32, 0, v);
	}

protected:

	/**
	 * Set the initial range of the spinner.
	 */
	virtual void push()
	{
		super::push();

		winapi::send_message<wchar_t>(hwnd(), UDM_SETRANGE32, m_min, m_max);
		winapi::send_message<wchar_t>(hwnd(), UDM_SETPOS32, 0, m_value);
	}

private:

	boost::int32_t m_min;
	boost::int32_t m_max;
	boost::int32_t m_value;
	
	DWORD m_custom_style;
};

class spinner : public ezel::control<spinner_impl>
{
public:

	struct style
	{
		enum value
		{
			default = 0,
			no_thousand_separator = UDS_NOTHOUSANDS,
			wrap_sequence = UDS_WRAP
		};
	};

	spinner(
		short left, short top, short width, short height,
		boost::int32_t minimum, boost::int32_t maximum,
		boost::int32_t initial_value,
		style::value custom_style=style::default)
		:
		control<spinner_impl>(
			boost::shared_ptr<spinner_impl>(
				new spinner_impl(
				left, top, width, height, minimum, maximum, initial_value,
				custom_style)))
		{}

	void range(boost::int32_t minimum, boost::int32_t maximum)
	{ impl()->range(minimum, maximum); }

	boost::int32_t value(boost::int32_t new_value)
	{ return impl()->value(new_value); }

	short left() const { return impl()->left(); }
	short top() const { return impl()->top(); }
	short width() const { return impl()->width(); }
	short height() const { return impl()->height(); }
};

}} // namespace ezel::controls

#endif
