/**
    @file

    COM object creators.

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

#ifndef WINAPI_COM_OBJECT_HPP
#define WINAPI_COM_OBJECT_HPP
#pragma once

#include <comet/error.h> // com_error
#include <comet/ptr.h> // com_ptr
#include <comet/interface.h> // uuidof

#include <boost/exception/errinfo_api_function.hpp> // errinfo_api_function
#include <boost/exception/info.hpp> // errinfo
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <string>

#include <objbase.h> // CoGetObject

namespace winapi {
namespace com {

/**
 * Get an object instance by its moniker display name.
 *
 * Corresponds to CoGetObject Windows API function with default BIND_OPTS.
 */
template<typename T>
comet::com_ptr<T> object_from_moniker_name(const std::wstring& display_name)
{
	comet::com_ptr<T> object;
	HRESULT hr = ::CoGetObject(
		display_name.c_str(), NULL, 
		comet::uuidof(object.in()), reinterpret_cast<void**>(object.out()));
	if (FAILED(hr))
		BOOST_THROW_EXCEPTION(
			boost::enable_error_info(comet::com_error(hr)) <<
			boost::errinfo_api_function("CoGetObject"));

	return object;
}

}} // namespace winapi::com

#endif
