/**
    @file

    MessageBox C++ wrapper.

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

#ifndef WINAPI_SHELL_SHELL_HPP
#define WINAPI_SHELL_SHELL_HPP
#pragma once

#include <winapi/detail/path_traits.hpp> // choose_path

#include <boost/exception/errinfo_api_function.hpp> // errinfo_api_function
#include <boost/exception/info.hpp> // errinfo
#include <boost/filesystem/path.hpp> // basic_path
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <stdexcept> // runtime_error
#include <vector>
	
#include <shlobj.h> // SHGetSpecialFolderPath

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

}} // namespace winapi::shell

#endif
