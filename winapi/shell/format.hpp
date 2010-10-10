/**
    @file

    Shell string-formatting functions.

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

#ifndef WINAPI_SHELL_FORMAT_HPP
#define WINAPI_SHELL_FORMAT_HPP
#pragma once

#include <comet/datetime.h> // datetime_t

#include <boost/exception/errinfo_api_function.hpp> // errinfo_api_function
#include <boost/exception/info.hpp> // errinfo
#include <boost/numeric/conversion/cast.hpp> // numeric_cast
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <string>
#include <vector>

#include <Shlwapi.h> // SHFormatDateTime, StrFormatKBSize

namespace winapi {
namespace shell {

namespace detail {
	namespace native {

		inline int sh_format_date_time(
			const FILETIME* pft, DWORD* pdwFlags, char* buffer, UINT size)
		{ return ::SHFormatDateTimeA(pft, pdwFlags, buffer, size); }

		inline int sh_format_date_time(
			const FILETIME* pft, DWORD* pdwFlags, wchar_t* buffer, UINT size)
		{ return ::SHFormatDateTimeW(pft, pdwFlags, buffer, size); }

		inline char* str_format_kb_size(
			LONGLONG file_size, char* buffer, UINT size)
		{ return ::StrFormatKBSizeA(file_size, buffer, size); }

		inline wchar_t* str_format_kb_size(
			LONGLONG file_size, wchar_t* buffer, UINT size)
		{ return ::StrFormatKBSizeW(file_size, buffer, size); }

	}
}

/**
 * Format a date the way comonly used by the Windows shell.
 *
 * The following shows an example of the string returned for different
 * flag values:
 *
 * FDTF_DEFAULT (or no flags)
 *      5/13/2059 4:36 AM
 * FDTF_LONGDATE | FDTF_SHORTTIME
 *      Tuesday, May 13, 2059, 4:36 AM
 * FDTF_LONGDATE | FDTF_LONGTIME | FDTF_RELATIVE
 *      Today, May 13, 2059, 4:36:06 AM
 *
 * Corresponds to SHFormatDateTime.
 */
template<typename T>
inline std::basic_string<T> format_date_time(
	const comet::datetime_t& date, DWORD flags=FDTF_DEFAULT)
{
	FILETIME ft;
	date.to_filetime(&ft);

	std::vector<T> buffer(512); // XXX: Unfortunately we have to hard-code this

	int len = detail::native::sh_format_date_time(
		&ft, &flags, &buffer[0], boost::numeric_cast<UINT>(buffer.size()));
	// flags gets modifed but that's OK as its a local copy.  However, that
	// means we can't return any information about these changes to the caller

	if (len == 0)
		BOOST_THROW_EXCEPTION(
			boost::enable_error_info(
				std::runtime_error("Couldn't convert date to a string")) <<
			boost::errinfo_api_function("SHFormatDateTime"));

	return std::basic_string<T>(&buffer[0], len - 1);
}

/**
 * Format a number as a file size in kilobytes.
 *
 * For example 3023 will be converted to some thing like "3,023 KB" depending
 * on locale.
 */
template<typename T>
inline std::basic_string<T> format_filesize_kilobytes(LONGLONG file_size)
{
	std::vector<T> buf(64); // XXX: Unfortunately we have to hard-code this
	T* str = detail::native::str_format_kb_size(
		file_size, &buf[0], boost::numeric_cast<UINT>(buf.size()));

	return (str) ? str : std::basic_string<T>();
}

}} // namespace winapi::shell

#endif
