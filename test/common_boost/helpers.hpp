/**
    @file

    Helper functions for Boost.Test,

    @if license

    Copyright (C) 2009  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#pragma once

#include <swish/utils.hpp>

#include <boost/system/error_code.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/filesystem.hpp>

#include <string>
#include <ostream>

namespace std {

	inline std::ostream& operator<<(
		std::ostream& out, const std::wstring& wide_in)
	{
		out << swish::utils::WideStringToUtf8String(wide_in);
		return out;
	}

	inline std::ostream& operator<<(
		std::ostream& out, const wchar_t* wide_in)
	{
		out << std::wstring(wide_in);
		return out;
	}

	inline std::ostream& operator<<(
		std::ostream& out, const boost::filesystem::wpath& path)
	{
		out << path.string();
		return out;
	}
}

/**
 * COM HRESULT-specific assertions
 * @{
 */
#define BOOST_REQUIRE_OK(hr)                                    \
do                                                              \
{                                                               \
	boost::system::error_code hr_error(                         \
		(hr), boost::system::get_system_category());            \
                                                                \
	std::string message("COM return status was not S_OK: ");    \
	message += hr_error.message();                              \
                                                                \
	BOOST_REQUIRE_MESSAGE(hr_error.value() == S_OK, message);   \
} while (0)
/* @} */
