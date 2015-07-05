/**
    @file

    GUI icon control.

    @if license

    Copyright (C) 2010, 2013  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#ifndef EZEL_CONTROLS_ICON_HPP
#define EZEL_CONTROLS_ICON_HPP
#pragma once

#include <ezel/control.hpp> // control base class
#include <ezel/detail/window_impl.hpp> // window_impl

#include <washer/window/icon.hpp> // icon_window

#include <boost/shared_ptr.hpp> // shared_ptr

#include <string>

namespace ezel {
namespace controls {

class icon_impl : public ezel::detail::window_impl
{
protected:
    typedef ezel::detail::window_impl super;

public:

    icon_impl(short left, short top, short width, short height)
        :
        ezel::detail::window_impl(L"", left, top, width, height),
        m_icon(NULL) {}

    std::wstring window_class() const { return L"static"; }
    DWORD style() const
    {
        DWORD style = ezel::detail::window_impl::style() |
            WS_CHILD | SS_ICON | WS_GROUP | SS_NOTIFY | SS_CENTERIMAGE;

        return style;
    }

    HICON change_icon(HICON new_icon)
    {
        if (!is_active())
        {
            HICON previous = m_icon;
            m_icon = new_icon;
            return previous;
        }
        else
        {
            washer::window::icon_window<wchar_t> icon(
                washer::window::window_handle::foster_handle(hwnd()));
            return icon.change_icon(new_icon);
        }
    }

protected:

    /**
     * Set the source of the icon to whatever the user set via change_icon.
     */
    virtual void push()
    {
        super::push();

        washer::send_message<wchar_t>(
            hwnd(), STM_SETIMAGE, IMAGE_ICON, m_icon);
    }

private:
    HICON m_icon;
};

class icon : public ezel::control<icon_impl>
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

    icon(short left, short top, short width, short height)
        :
        control<icon_impl>(
            boost::shared_ptr<icon_impl>(
                new icon_impl(left, top, width, height))) {}

    HICON change_icon(HICON new_icon) { return impl()->change_icon(new_icon); }

    short left() const { return impl()->left(); }
    short top() const { return impl()->top(); }
    short width() const { return impl()->width(); }
    short height() const { return impl()->height(); }
};

}} // namespace ezel::controls

#endif
