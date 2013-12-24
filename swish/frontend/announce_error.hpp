/**
    @file

    Reporting exceptions to the user.

    @if license

    Copyright (C) 2010, 2011, 2013  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#ifndef SWISH_FRONTEND_ANNOUNCE_ERROR_HPP
#define SWISH_FRONTEND_ANNOUNCE_ERROR_HPP
#pragma once

#include <string>

#include <WinDef.h> // HWND

namespace swish {
namespace frontend {

void announce_error(
    HWND hwnd, const std::wstring& problem,
    const std::wstring& suggested_resolution, const std::wstring& details);

void announce_last_exception(
    HWND hwnd, const std::wstring& title,
    const std::wstring& suggested_resolution, bool force_ui=false);

}} // namespace swish::frontend

#endif
