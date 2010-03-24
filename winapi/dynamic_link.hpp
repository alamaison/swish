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

#include "detail/path_traits.hpp" // choose_path
#include "error.hpp" // last_error

#include <boost/exception/info.hpp> // errinfo
#include <boost/exception/errinfo_file_name.hpp> // errinfo_file_name
#include <boost/exception/errinfo_api_function.hpp> // errinfo_api_function
#include <boost/filesystem.hpp> // basic_path
#include <boost/shared_ptr.hpp> // shared_ptr
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION
#include <boost/type_traits/remove_pointer.hpp> // remove_pointer

#include <Winbase.h> // LoadLibrary, FreeLibrary, GetProcAddress, 
                     // GetModuleHandle, GetModuleFileName

namespace winapi {

typedef boost::shared_ptr<boost::remove_pointer<HMODULE>::type> hmodule;

namespace detail {
	
	inline HMODULE get_handle(HMODULE hmod) { return hmod; }
	inline HMODULE get_handle(hmodule hmod) { return hmod.get(); }

	namespace native {
	
		inline HMODULE load_library(const char* file)
		{ return ::LoadLibraryA(file); }

		inline HMODULE load_library(const wchar_t* file)
		{ return ::LoadLibraryW(file); }
	
		inline HMODULE get_module_handle(const char* file)
		{ return ::GetModuleHandleA(file); }

		inline HMODULE get_module_handle(const wchar_t* file)
		{ return ::GetModuleHandleW(file); }
	
		inline DWORD module_filename(HMODULE hmod, char* file_out, DWORD size)
		{ return ::GetModuleFileNameA(hmod, file_out, size); }

		inline DWORD module_filename(
			HMODULE hmod, wchar_t* file_out, DWORD size)
		{ return ::GetModuleFileNameW(hmod, file_out, size); }

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
		HMODULE hinst = native::load_library(
			library_path.external_file_string().c_str());
		if (hinst == NULL)
			BOOST_THROW_EXCEPTION(
				boost::enable_error_info(winapi::last_error()) << 
				boost::errinfo_api_function("LoadLibrary"));
		
		return hmodule(hinst, ::FreeLibrary);
	}

	/**
	 * Get handle of an already-loaded DLL by file name.
	 *
	 * This implementation works for wide or narrow paths.
	 *
	 * @todo  Use boost::errinfo_file_name(library_path) once we use
	 *        filesystem v3 which allows all paths to be converted
	 *        to std::string.
	 */
	template<typename T, typename Traits>
	inline HMODULE module_handle(
		const boost::filesystem::basic_path<T, Traits>& module_path)
	{
		HMODULE hinst = native::get_module_handle(
			(module_path.empty()) ?
				NULL : module_path.external_file_string().c_str());
		if (hinst == NULL)
			BOOST_THROW_EXCEPTION(
				boost::enable_error_info(winapi::last_error()) << 
				boost::errinfo_api_function("GetModuleHandle"));
		
		return hinst;
	}

	/**
	 * Get handle of currently-loaded executable.
	 */
	inline HMODULE module_handle()
	{
		return module_handle(boost::filesystem::wpath());
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
 * Get handle of an already-loaded module by file name.
 */
inline HMODULE module_handle(const boost::filesystem::path& module_path)
{ return detail::module_handle(module_path); }

/**
 * Get handle of an already-loaded module by file name.
 */
inline HMODULE module_handle(const boost::filesystem::wpath& module_path)
{ return detail::module_handle(module_path); }

/**
 * Get handle of current executable.
 */
inline HMODULE module_handle() { return detail::module_handle(); }

/**
 * Path to the module whose handle is @p module which has been loaded by the
 * current process.
 */
template<typename T, typename H>
inline typename detail::choose_path<T>::type module_path(H module)
{
	std::vector<T> buffer(MAX_PATH);
	DWORD size = detail::native::module_filename(
		detail::get_handle(module), &buffer[0], buffer.size());

	if (size >= buffer.size())
		BOOST_THROW_EXCEPTION(
			boost::enable_error_info(
				std::logic_error("Insufficient buffer space")) << 
			boost::errinfo_api_function("GetModuleFileName"));
	if (size == 0)
		BOOST_THROW_EXCEPTION(
			boost::enable_error_info(winapi::last_error()) << 
			boost::errinfo_api_function("GetModuleFileName"));

	return typename detail::choose_path<T>::type(
		buffer.begin(), buffer.begin() + size);
}

/**
 * Path to the current executable.
 */
template<typename T>
inline typename detail::choose_path<T>::type module_path()
{
	return module_path(NULL);
}

/**
 * Dynamically bind to function given by name.
 *
 * @param hmod  Module defining the requested function.
 * @param name  Name of the function.
 * @returns  Pointer to the function with signature T.
 */
template<typename T>
inline T proc_address(HMODULE hmod, const std::string& name)
{
	FARPROC f = ::GetProcAddress(detail::get_handle(hmod), name.c_str());
	if (f == NULL)
		BOOST_THROW_EXCEPTION(
			boost::enable_error_info(winapi::last_error()) << 
			boost::errinfo_api_function("GetProcAddress"));

	return reinterpret_cast<T>(f);
}

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
	return proc_address<T>(hmod.get(), name);
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
