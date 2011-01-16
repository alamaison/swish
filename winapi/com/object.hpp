/**
    @file

    COM object creators.

    @if license

    Copyright (C) 2010, 2011  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include <boost/exception/errinfo_api_function.hpp> // errinfo_api_function
#include <boost/exception/info.hpp> // errinfo
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <string>

#include <objbase.h> // CreateBindCtx, MkParseDisplayName
#include <ObjIdl.h> // IBindCtx, IMoniker

template<> struct comet::comtype<IBindCtx>
{
	static const IID& uuid() throw() { return IID_IBindCtx; }
	typedef IUnknown base;
};

template<> struct comet::comtype<IMoniker>
{
	static const IID& uuid() throw() { return IID_IMoniker; }
	typedef IPersistStream base;
};

namespace winapi {
namespace com {

/**
 * Instance of default system IBindCtx implementation.
 *
 * Corresponds to CreateBindCtx Windows API function.
 */
inline comet::com_ptr<IBindCtx> create_bind_context()
{
	comet::com_ptr<IBindCtx> bind_context;
	HRESULT hr = ::CreateBindCtx(0, bind_context.out());
	if (FAILED(hr))
		BOOST_THROW_EXCEPTION(
			boost::enable_error_info(comet::com_error(hr)) <<
			boost::errinfo_api_function("CreateBindCtx"));

	return bind_context;
}

/**
 * Get an object instance by its moniker display name.
 *
 * Corresponds to CoGetObject Windows API function but allows a custom bind
 * context.
 *
 * We have reimplemented CoGetObject rather than calling the Windows API
 * version as there is no other way to specify a bind context.  CoGetObject
 * only lets the caller specify a limited set of properties in a BIND_OPTS
 * structure.  Some tasks, such as setting a custom IBindStatusCallback
 * require a full-blown IBindCtx.
 */
template<typename T>
inline comet::com_ptr<T> object_from_moniker_name(
	const std::wstring& display_name, comet::com_ptr<IBindCtx> bind_context)
{
	comet::com_ptr<IMoniker> moniker;
	ULONG eaten = 0;
	HRESULT hr = ::MkParseDisplayName(
		bind_context.get(), display_name.c_str(), &eaten, moniker.out());
	if (FAILED(hr))
		BOOST_THROW_EXCEPTION(
			boost::enable_error_info(comet::com_error(hr)) <<
			boost::errinfo_api_function("MkParseDisplayName"));

	comet::com_ptr<T> object;
	hr = moniker->BindToObject(
		bind_context.get(), NULL, object.iid(),
		reinterpret_cast<void**>(object.out()));
	if (FAILED(hr))
		BOOST_THROW_EXCEPTION(comet::com_error_from_interface(moniker, hr));

	return object;
}

/**
 * Get an object instance by its moniker display name.
 *
 * Corresponds to CoGetObject Windows API function. BIND_OPTS parameter,
 * @a bind_options, can also be an instance of BIND_OPTS2 or BIND_OPTS3.
 */
template<typename T>
inline comet::com_ptr<T> object_from_moniker_name(
	const std::wstring& display_name, BIND_OPTS& bind_options)
{
	comet::com_ptr<IBindCtx> bind_context = create_bind_context();
	HRESULT hr = bind_context->SetBindOptions(&bind_options);
	if (FAILED(hr))
		BOOST_THROW_EXCEPTION(
			comet::com_error_from_interface(bind_context, hr));

	return object_from_moniker_name<T>(display_name, bind_context);
}

/**
 * Get an object instance by its moniker display name.
 *
 * Corresponds to CoGetObject Windows API function with default BIND_OPTS.
 */
template<typename T>
inline comet::com_ptr<T> object_from_moniker_name(
	const std::wstring& display_name)
{
	return object_from_moniker_name<T>(display_name, create_bind_context());
}

}} // namespace winapi::com

#endif
