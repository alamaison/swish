/**
    @file

    Miscellanious utility code.

    @if licence

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

    In addition, as a special exception, the the copyright holders give you
    permission to combine this program with free software programs or the 
    OpenSSL project's "OpenSSL" library (or with modified versions of it, 
    with unchanged license). You may copy and distribute such a system 
    following the terms of the GNU GPL for this program and the licenses 
    of the other code concerned. The GNU General Public License gives 
    permission to release a modified version without this exception; this 
    exception also makes it possible to release a modified version which 
    carries forward this exception.

    @endif
*/

#pragma once

#include <boost/system/system_error.hpp>
#include <boost/numeric/conversion/cast.hpp> // numeric_cast

#include <WinNls.h>

#include <string>
#include <vector>
#include <cassert>

namespace {

	template<typename _FromElem, typename _ToElem>
	struct Converter
	{
		typedef _FromElem FromElem;
		typedef _ToElem ToElem;
		typedef std::basic_string<FromElem> FromType;
		typedef std::basic_string<ToElem> ToType;
	};

	struct Narrow : Converter<wchar_t, char>
	{
		int operator()(
			const FromElem* pszWide, int cchWide, 
			ToElem* pszNarrow, int cbNarrow)
		{
			return ::WideCharToMultiByte(
				CP_UTF8, 0, pszWide, cchWide, pszNarrow, cbNarrow, NULL, NULL);
		}
	};

	struct Widen : Converter<char, wchar_t>
	{
		int operator()(
			const FromElem* pszNarrow, int cbNarrow, 
			ToElem* pszWide, int cchWide)
		{
			return ::MultiByteToWideChar(
				CP_UTF8, 0, pszNarrow, cbNarrow, pszWide, cchWide);
		}
	};
}

namespace swish {
namespace utils {

/**
 * Convert a basic_string-style string from one element type to another.
 *
 * @templateparam T  Converter functor to perform the actual conversion.
 */
template<typename T>
inline typename T::ToType ConvertString(const typename T::FromType& from)
{
	const int size = boost::numeric_cast<int>(from.size());
	if (size == 0)
		return T::ToType();

	// Calculate necessary buffer size
	int len = T()(from.c_str(), size, NULL, 0);

	// Perform actual conversion
	if (len > 0)
	{
		std::vector<T::ToElem> buffer(len);
		len = T()(
			from.c_str(), size,
			&buffer[0], static_cast<int>(buffer.size()));
		if (len > 0)
		{
			assert(len == boost::numeric_cast<int>(buffer.size()));
			return T::ToType(&buffer[0], buffer.size());
		}
	}

	throw boost::system::system_error(
		::GetLastError(), boost::system::system_category);
}

/**
 * Convert a Windows wide string to a UTF-8 (multi-byte) string.
 */
inline std::string WideStringToUtf8String(const std::wstring& wide)
{
	return swish::utils::ConvertString<Narrow>(wide);
}

/**
 * Convert a UTF-8 (multi-byte) string to a Windows wide string.
 */
inline std::wstring Utf8StringToWideString(const std::string& narrow)
{
	return swish::utils::ConvertString<Widen>(narrow);
}

/**
 * Get the current user's username.
 */
inline std::wstring GetCurrentUser()
{
	// Calculate required size of output buffer
	DWORD len = 0;
	if (::GetUserNameW(NULL, &len))
		return std::wstring();

	DWORD err = ::GetLastError();
	if (err != ERROR_INSUFFICIENT_BUFFER)
	{
		throw boost::system::system_error(err, boost::system::system_category);
	}

	// Repeat call with a buffer of required size
	if (len > 0)
	{
		std::vector<wchar_t> buffer(len);
		if (::GetUserName(&buffer[0], &len))
		{
			return std::wstring(&buffer[0], buffer.size());
		}
		else
		{
			throw boost::system::system_error(
				::GetLastError(), boost::system::system_category);
		}
	}

	return std::wstring();
}

}} // namespace swish::utils
