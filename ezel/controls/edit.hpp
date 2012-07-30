/**
    @file

    GUI edit (text) control.

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

#ifndef EZEL_CONTROLS_EDIT_HPP
#define EZEL_CONTROLS_EDIT_HPP
#pragma once

#include <ezel/control.hpp> // control base class
#include <ezel/detail/window_impl.hpp> // window_impl

#include <boost/shared_ptr.hpp> // shared_ptr
#include <boost/signal.hpp> // signal

#include <string>

namespace ezel {
namespace controls {

class edit_impl : public ezel::detail::window_impl
{
public:
    typedef ezel::detail::window_impl super;

    typedef ezel::detail::command_map<EN_CHANGE, EN_UPDATE> commands;

    virtual void handle_command(
        WORD command_id, WPARAM wparam, LPARAM lparam)
    {
        dispatch_command(this, command_id, wparam, lparam);
    }

    edit_impl(
        const std::wstring& text, short left, short top, short width,
        short height, DWORD custom_style)
        :
        ezel::detail::window_impl(text, left, top, width, height),
        m_custom_style(custom_style) {}

    std::wstring window_class() const { return L"Edit"; }

    DWORD style() const
    {
        DWORD style = ezel::detail::window_impl::style() |
            WS_CHILD | ES_LEFT | WS_BORDER | ES_AUTOHSCROLL;
        
        style |= m_custom_style;

        return style;
    }

    boost::signal<void ()>& on_change() { return m_on_change; }
    boost::signal<void ()>& on_update() { return m_on_update; }

    void on(command<EN_CHANGE>) { m_on_change(); }
    void on(command<EN_UPDATE>) { m_on_update(); }

private:
    boost::signal<void ()> m_on_change;
    boost::signal<void ()> m_on_update;
    DWORD m_custom_style;
};

class edit : public ezel::control<edit_impl>
{
public:

    struct style
    {
        enum value
        {
            default = 0,
            password = ES_PASSWORD,
            force_lowercase = ES_LOWERCASE,
            only_allow_numbers = ES_NUMBER
        };
    };

    edit(
        const std::wstring& text, short left, short top, short width,
        short height, style::value custom_style=style::default)
        :
        control<edit_impl>(
            boost::shared_ptr<edit_impl>(
                new edit_impl(
                    text, left, top, width, height, custom_style))) {}

    boost::signal<void ()>& on_change() { return impl()->on_change(); }
    boost::signal<void ()>& on_update() { return impl()->on_update(); }
    
    boost::signal<void (const wchar_t*)>& on_text_change()
    { return impl()->on_text_change(); }
    boost::signal<void ()>& on_text_changed()
    { return impl()->on_text_changed(); }

    short left() const { return impl()->left(); }
    short top() const { return impl()->top(); }
    short width() const { return impl()->width(); }
    short height() const { return impl()->height(); }
};

}} // namespace ezel::controls

#endif
