/**
    @file

    System errors.

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

#ifndef WINAPI_ERROR_HPP
#define WINAPI_ERROR_HPP
#pragma once

#include <boost/system/system_error.hpp> // system_error, system_category

#include <Winbase.h> // GetLastError

namespace winapi {

inline boost::system::system_error last_error()
{
	return boost::system::system_error(
		::GetLastError(), boost::system::system_category);
}

} // namespace winapi

#endif