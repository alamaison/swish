/**
    @file

    Shell utility functions.

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

#ifndef WINAPI_SHELL_SHELL_HPP
#define WINAPI_SHELL_SHELL_HPP
#pragma once

#include <winapi/detail/path_traits.hpp> // choose_path
#include <winapi/shell/pidl.hpp> // cpidl_t

#include <comet/error.h> // com_error

#include <boost/exception/errinfo_api_function.hpp> // errinfo_api_function
#include <boost/exception/info.hpp> // errinfo
#include <boost/filesystem/path.hpp> // basic_path
#include <boost/shared_ptr.hpp> // shared_ptr
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <stdexcept> // runtime_error
#include <vector>
	
#include <shlobj.h> // SHGetSpecialFolderPath
#include <Shlwapi.h> // StrRetToStr, SHStrDup

namespace winapi {
namespace shell {

namespace detail {
	namespace native {

		inline HRESULT special_folder_path(
			HWND hwnd, char* path_out, int folder, BOOL create)
		{ return ::SHGetSpecialFolderPathA(hwnd, path_out, folder, create); }

		inline HRESULT special_folder_path(
			HWND hwnd, wchar_t* path_out, int folder, BOOL create)
		{ return ::SHGetSpecialFolderPathW(hwnd, path_out, folder, create); }


		inline HRESULT str_ret_to_str(
			STRRET* strret, const ITEMID_CHILD* pidl, char** string_out)
		{ return ::StrRetToStrA(strret, pidl, string_out); }

		inline HRESULT str_ret_to_str(
			STRRET* strret, const ITEMID_CHILD* pidl, wchar_t** string_out)
		{ return ::StrRetToStrW(strret, pidl, string_out); }


		inline HRESULT sh_str_dup(const char* narrow_in, wchar_t** wide_out)
		{ return ::SHStrDupA(narrow_in, wide_out); }

		inline HRESULT sh_str_dup(const wchar_t* wide_in, wchar_t** wide_out)
		{ return ::SHStrDupW(wide_in, wide_out); }
	}

	/**
	 * Create a STRRET from a string.
	 *
	 * @note  Stores as wide string regardless of input type to avoid MAX_PATH
	 *        size limit of cStr.
	 */
	template<typename T>
	inline STRRET string_to_strret(const std::basic_string<T>& str)
	{
		STRRET strret;
		strret.uType = STRRET_WSTR;
		strret.pOleStr = NULL;

		HRESULT hr = detail::native::sh_str_dup(str.c_str(), &strret.pOleStr);
		if (FAILED(hr))
			BOOST_THROW_EXCEPTION(
				boost::enable_error_info(
					comet::com_error(
						"Failed to copy string", hr, "SHStrDup")) <<
				boost::errinfo_api_function("SHStrDup"));

		return strret;
	}
}

/**
 * Common system folder path by CSIDL.
 *
 * @example
 *   @code known_folder_path(CSIDL_PROFILE) @endcode returns something like
 *   @code C:\\Users\\Username @endcode
 */
template<typename T>
inline typename winapi::detail::choose_path<T>::type special_folder_path(
	int folder, bool create_if_missing=false)
{
	std::vector<T> buffer(MAX_PATH);
	BOOL found = detail::native::special_folder_path(
		NULL, &buffer[0], folder, create_if_missing);

	if (!found)
		BOOST_THROW_EXCEPTION(
			boost::enable_error_info(
				std::runtime_error("Couldn't find special folder")) << 
			boost::errinfo_api_function("SHGetSpecialFolderPath"));

	buffer[buffer.size() - 1] = T(); // null-terminate

	return typename winapi::detail::choose_path<T>::type(&buffer[0]);
}

/**
 * Convert a STRRET structure to a string.
 *
 * If the STRRET is using its pOleStr member to store the data (rather
 * than holding it directly or extracting it from the PIDL offset)
 * the data will be freed.  In other words, this function destroys
 * the STRRET passed to it.
 *
 * @param strret  STRRET to convert into string.  Its contents are destroyed
 *                by this function if using STRRET_WSTR type.
 * @param pidl    PIDL in which the string data may be embedded (optional).
 */
template<typename T>
inline std::basic_string<T> strret_to_string(
	STRRET& strret, const pidl::cpidl_t& pidl=pidl::cpidl_t())
{
	T* str = NULL;
	HRESULT hr = detail::native::str_ret_to_str(&strret, pidl.get(), &str);

	// RAII for CoTaskMemAlloced string
	boost::shared_ptr<T> str_lifetime(str, ::CoTaskMemFree);

	if (FAILED(hr))
		BOOST_THROW_EXCEPTION(
			boost::enable_error_info(
				comet::com_error(
					"Failed to convert STRRET to string", hr,
					"StrRetToStr")) << 
			boost::errinfo_api_function("StrRetToStr"));
	

	return (str) ? std::basic_string<T>(str) : std::basic_string<T>();
}

/**
 * Create a STRRET from an ANSI string.
 *
 * @note  Stores the string in the STRRET's unicode string field.
 */
inline STRRET string_to_strret(const std::string& str)
{ return detail::string_to_strret(str); }

/**
 * Create a STRRET from a Unicode string.
 */
inline STRRET string_to_strret(const std::wstring& str)
{ return detail::string_to_strret(str); }

}} // namespace winapi::shell

#endif
