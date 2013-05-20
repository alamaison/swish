/**
    @file

    Progress callback.

    @if license

    Copyright (C) 2012, 2013  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#ifndef SWISH_DROP_TARGET_PROGRESS_HPP
#define SWISH_DROP_TARGET_PROGRESS_HPP
#pragma once

#include <boost/noncopyable.hpp>

#include <string>

namespace swish {
namespace drop_target {

class Progress : private boost::noncopyable
{
public:
    virtual ~Progress() {}
    virtual bool user_cancelled() = 0;
    virtual void line(DWORD index, const std::wstring& text) = 0;
    virtual void line_path(DWORD index, const std::wstring& text) = 0;
    virtual void update(ULONGLONG so_far, ULONGLONG out_of) = 0;
    virtual void hide() = 0;
    virtual void show() = 0;
};

}}

#endif