/**
    @file

    GUI horizontal line control.

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

#ifndef EZEL_CONTROLS_LINE_HPP
#define EZEL_CONTROLS_LINE_HPP
#pragma once

#include <ezel/control.hpp> // control base class
#include <ezel/detail/window_impl.hpp> // window_impl

#include <boost/shared_ptr.hpp> // shared_ptr

#include <string>

namespace ezel {
namespace controls {

class line_impl : public ezel::detail::window_impl
{
public:

    line_impl(short left, short top, short width)
        :
        ezel::detail::window_impl(L"", left, top, width, 1) {}

    std::wstring window_class() const { return L"static"; }
    DWORD style() const
    {
        return WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ | WS_GROUP | SS_NOTIFY;
    }
};

class line : public ezel::control<line_impl>
{
public:
    line(
        short left, short top, short width)
        :
        control<line_impl>(
            boost::shared_ptr<line_impl>(new line_impl(left, top, width)))
        {}

    short left() const { return impl()->left(); }
    short top() const { return impl()->top(); }
    short width() const { return impl()->width(); }
    short height() const { return impl()->height(); }
};

}} // namespace ezel::controls

#endif
