/**
    @file

    Miscellanious Windows API utility code.

    @if license

    Copyright (C) 2009, 2010  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include <winapi/shell/shell.hpp> // known_folder_path

#include <comet/error.h> // com_error
#include <comet/ptr.h> // com_ptr

#include <boost/filesystem.hpp> // basic_path
#include <boost/system/system_error.hpp>
#include <boost/numeric/conversion/cast.hpp> // numeric_cast
#include <boost/throw_exception.hpp>  // BOOST_THROW_EXCEPTION

#include <WinNls.h>
#include <Objbase.h> // GetRunningObjectTable

#include <string>
#include <vector>
#include <cassert>

#pragma region WideCharToMultiByte/MultiByteToWideChar wrappers

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
	int len = T()(from.data(), size, NULL, 0);

	// Perform actual conversion
	if (len > 0)
	{
		std::vector<T::ToElem> buffer(len);
		len = T()(
			from.data(), size,
			&buffer[0], static_cast<int>(buffer.size()));
		if (len > 0)
		{
			assert(len == boost::numeric_cast<int>(buffer.size()));
			return T::ToType(&buffer[0], len);
		}
	}

	throw boost::system::system_error(
		::GetLastError(), boost::system::get_system_category());
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

}} // namespace swish::utils

#pragma endregion

#pragma region GetUserName wrapper

namespace {

	struct NarrowUserTraits
	{
		typedef char element_type;
		typedef std::basic_string<element_type> return_type;

		inline static BOOL get_user_name(
			element_type* out_buffer, DWORD* pcb_buffer)
		{
			return ::GetUserNameA(out_buffer, pcb_buffer);
		}
	};

	struct WideUserTraits
	{
		typedef wchar_t element_type;
		typedef std::basic_string<element_type> return_type;

		inline static BOOL get_user_name(
			element_type* out_buffer, DWORD* pcb_buffer)
		{
			return ::GetUserNameW(out_buffer, pcb_buffer);
		}
	};
}

namespace swish {
namespace utils {

namespace detail {

/**
 * Get the current user's username.
 */
template<typename T>
inline typename T::return_type current_user()
{
	// Calculate required size of output buffer
	DWORD len = 0;
	if (typename T::get_user_name(NULL, &len))
		return typename T::return_type();

	DWORD err = ::GetLastError();
	if (err != ERROR_INSUFFICIENT_BUFFER)
	{
		BOOST_THROW_EXCEPTION(
			boost::system::system_error(
				err, boost::system::get_system_category()));
	}

	// Repeat call with a buffer of required size
	if (len > 0)
	{
		std::vector<T::element_type> buffer(len);
		if (typename T::get_user_name(&buffer[0], &len))
		{
			return typename T::return_type(&buffer[0], len - 1);
		}
		else
		{
			BOOST_THROW_EXCEPTION(
				boost::system::system_error(
					::GetLastError(), boost::system::get_system_category()));
		}
	}

	return typename T::return_type();
}

} // detail

inline WideUserTraits::return_type current_user()
{
	return detail::current_user<WideUserTraits>();
}

inline NarrowUserTraits::return_type current_user_a()
{
	return detail::current_user<NarrowUserTraits>();
}

namespace detail {

	inline DWORD get_environment_variable(
		const char* key, char* buffer, DWORD size)
	{
		return ::GetEnvironmentVariableA(key, buffer, size);
	}

	inline DWORD get_environment_variable(
		const wchar_t* key, wchar_t* buffer, DWORD size)
	{
		return ::GetEnvironmentVariableW(key, buffer, size);
	}
}

/**
 * Fetch string value from an environment variable.
 * Returns empty string if the variable isn't present in the enviroment.
 */
template<typename T>
inline T environment_variable(const T& key)
{
	DWORD len = detail::get_environment_variable(key.c_str(), NULL, 0);
	if (len == 0)
		return T();

	std::vector<T::value_type> buf(len);
	len = detail::get_environment_variable(
		key.c_str(), &buf[0], boost::numeric_cast<DWORD>(buf.size()));
	if (len == 0)
		BOOST_THROW_EXCEPTION(
			boost::system::system_error(
				::GetLastError(), boost::system::get_system_category()));

	return T(buf.begin(), buf.begin() + len);
}

/**
 * Find home directory path.
 *
 * @throws std::exception if path can't be found.
 * @todo  Try other means to find directory including NetUserGetInfo.
 */
template<typename T>
inline T home_directory()
{
	// try SHGetKnowFolderPath
	T home = winapi::shell::special_folder_path<T::value_type>(CSIDL_PROFILE);
	if (!home.empty())
		return home;

	// fall back to %HOME%
	const T::value_type home_key[] = {'H', 'O', 'M', 'E', '\0'};
	home = environment_variable(T::string_type(home_key));
	if (!home.empty())
		return home;

	// fall back to %USERPROFILE%
	const T::value_type userprofile[] =
		{'U', 'S', 'E', 'R', 'P', 'R', 'O', 'F', 'I', 'L', 'E', '\0'};
	home = environment_variable(T::string_type(userprofile));
	if (!home.empty())
		return home;

	// fall back to %HOMEDRIVE%/%HOMEPATH%
	const T::value_type home_drive_key[] =
		{'H', 'O', 'M', 'E', 'D', 'R', 'I', 'V', 'E', '\0'};
	const T::value_type home_path_key[] =
		{'H', 'O', 'M', 'E', 'P', 'A', 'T', 'H', '\0'};

	T home_drive = environment_variable(T::string_type(home_drive_key));
	T home_path = environment_variable(T::string_type(home_path_key));
	home = home_drive / home_path;
	if (home.empty())
		BOOST_THROW_EXCEPTION(
			std::exception("Can't find home directory"));
	return home;
}

namespace com {

/**
 * Get the local Winstation Running Object Table.
 */
inline comet::com_ptr<IRunningObjectTable> running_object_table()
{
	comet::com_ptr<IRunningObjectTable> rot;

	HRESULT hr = ::GetRunningObjectTable(0, rot.out());
	assert(SUCCEEDED(hr));
	assert(rot);

	if (FAILED(hr))
		BOOST_THROW_EXCEPTION(comet::com_error(hr));

	return rot;
}

/**
 * Look up a CLSID in the registry using the ProgId.
 */
inline CLSID clsid_from_progid(const std::wstring& progid)
{
	CLSID clsid;
	HRESULT hr = ::CLSIDFromProgID(progid.c_str(), &clsid);
	if (FAILED(hr))
		BOOST_THROW_EXCEPTION(comet::com_error(hr));

	return clsid;
}

/**
 * Get the class object of a component by its CLSID.
 */
template<typename T>
inline comet::com_ptr<T> class_object(
	const CLSID& clsid, DWORD dw_class_context=CLSCTX_ALL)
{
	comet::com_ptr<T> object;
	HRESULT hr = ::CoGetClassObject(
		clsid, dw_class_context, NULL, comet::uuidof(object.in()),
		reinterpret_cast<void**>(object.out()));
	
	if (FAILED(hr))
		BOOST_THROW_EXCEPTION(comet::com_error(hr));

	return object;
}

/**
 * Get the class object of a component by its ProgId.
 */
template<typename T>
inline comet::com_ptr<T> class_object(
	const std::wstring& progid, DWORD dw_class_context=CLSCTX_ALL)
{
	CLSID clsid = clsid_from_progid(progid);
	return class_object<T>(clsid, dw_class_context);
}

} // namespace swish::utils::com

}} // namespace swish::utils

#pragma endregion
