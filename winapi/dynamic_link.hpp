/**
    @file

    Dynamic linking and loading.

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

#ifndef WINAPI_DYNAMIC_LINK_HPP
#define WINAPI_DYNAMIC_LINK_HPP
#pragma once

#include "error.hpp" // last_error

#include <boost/exception/info.hpp> // errinfo
#include <boost/exception/errinfo_file_name.hpp> // errinfo_file_name
#include <boost/exception/errinfo_api_function.hpp> // errinfo_api_function
#include <boost/filesystem.hpp> // basic_path
#include <boost/shared_ptr.hpp> // shared_ptr
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION
#include <boost/type_traits/remove_pointer.hpp> // remove_pointer

#include <Winbase.h> // LoadLibrary, FreeLibrary, GetProcAddress

namespace winapi {

	typedef boost::shared_ptr<boost::remove_pointer<HMODULE>::type> hmodule;

namespace detail {
	namespace native {
	
		inline HMODULE load_library(const char* file)
		{ return ::LoadLibraryA(file); }

		inline HMODULE load_library(const wchar_t* file)
		{ return ::LoadLibraryW(file); }

	}

	/**
	 * Load a DLL by file name.
	 *
	 * This implementation works for wide or narrow paths.
	 *
	 * @todo  Use boost::errinfo_file_name(library_path) once we use
	 *        filesystem v3 which allows all paths to be converted
	 *        to std::string.
	 */
	template<typename T, typename Traits>
	inline hmodule load_library(
		const boost::filesystem::basic_path<T, Traits>& library_path)
	{
		HMODULE hinst = detail::native::load_library(
			library_path.external_file_string().c_str());
		if (hinst == NULL)
			BOOST_THROW_EXCEPTION(
				boost::enable_error_info(winapi::last_error()) << 
				boost::errinfo_api_function("LoadLibrary"));
		
		return hmodule(hinst, ::FreeLibrary);
	}
}

/**
 * Load a DLL by file name.
 */
inline hmodule load_library(const boost::filesystem::path& library_path)
{ return detail::load_library(library_path); }

/**
 * Load a DLL by file name.
 */
inline hmodule load_library(const boost::filesystem::wpath& library_path)
{ return detail::load_library(library_path); }

/**
 * Dynamically bind to function given by name.
 *
 * @param hmod  Module defining the requested function.
 * @param name  Name of the function.
 * @returns  Pointer to the function with signature T.
 */
template<typename T>
inline T proc_address(hmodule hmod, const std::string& name)
{
	FARPROC f = ::GetProcAddress(hmod.get(), name.c_str());
	if (f == NULL)
		BOOST_THROW_EXCEPTION(
			boost::enable_error_info(winapi::last_error()) << 
			boost::errinfo_api_function("GetProcAddress"));

	return reinterpret_cast<T>(f);
}

namespace detail {
	
	/**
	 * Dynamically bind to function given by name.
	 */
	template<typename T, typename String, typename Traits>
	inline T proc_address(
		const boost::filesystem::basic_path<String, Traits>& library_path,
		const std::string& name)
	{
		return ::winapi::proc_address<T>(
			::winapi::load_library(library_path), name);
	}
}

/**
 * Dynamically bind to function given by name.
 *
 * @param library_path  Path or filename of the DLL exporting the 
                        requested function.
 * @param name          Name of the function.
 * @returns  Pointer to the function with signature T.
 */
template<typename T>
inline T proc_address(
	const boost::filesystem::path& library_path, const std::string& name)
{ return detail::proc_address<T>(library_path, name); }

/**
 * Dynamically bind to function given by name.
 *
 * @param library_path  Path or filename of the DLL exporting the 
                        requested function.
 * @param name          Name of the function.
 * @returns  Pointer to the function with signature T.
 */
template<typename T>
inline T proc_address(
	const boost::filesystem::wpath& library_path, const std::string& name)
{ return detail::proc_address<T>(library_path, name); }

} // namespace winapi

#endif
