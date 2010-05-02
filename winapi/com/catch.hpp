/**
    @file

    C++ exception to COM error translator.

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

#ifndef WINAPI_COM_CATCH_HPP
#define WINAPI_COM_CATCH_HPP
#pragma once

#include <boost/current_function.hpp> // BOOST_CURRENT_FUNCTION

#include <comet/handle_except.h> // comet_exception_handler, source_info_t
#include <comet/uuid.h> // uuid_t

namespace winapi {
namespace com {

	inline HRESULT rethrow(
		const char* function, const char* /*file*/, int /*line*/,
		const comet::uuid_t& iid=comet::uuid_t(),
		const comet::uuid_t& /*clsid*/=comet::uuid_t())
	{
		return comet::comet_exception_handler<true>::rethrow(
			comet::source_info_t(function, iid));
	}

}} // namespace winapi::com

/**
 * COM-interface boundary exception catcher that infers IID from interface_is.
 *
 * This should be used in a class implementing an interface and having a
 * typedef @c interface_is giving the IID of the interface the current method
 * is part of.  A comtype for the interface must also be in scope.
 */
#define COM_CATCH_AUTO_INTERFACE() \
	catch (...) \
	{ \
		return ::winapi::com::rethrow( \
			BOOST_CURRENT_FUNCTION, __FILE__, __LINE__, \
			::comet::comtype<interface_is>::uuid()); \
	}

#endif
