/**
    @file

    Mixin class which gives CComObjects a creator of AddReffed instances.

    @if licence

    Copyright (C) 2008, 2009  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "atl.hpp"      // Common ATL setup

namespace swish {

template <typename T>
class CCoFactory
{
public:
	/**
	 * Static factory method.
	 *
	 * This creator method provides a CComObject-based class with a way to 
	 * create instances with exception-safe lifetimes.  The created 
	 * instance is AddReffed, unlike those create by CreateInstance which 
	 * have a reference count of 0.
	 *
	 * @returns  Smart pointer to the CComObject<T>-based COM object.
	 * @throws   com_exception if creation fails.
	 */
	static ATL::CComPtr<T> CreateCoObject() throw(...)
	{
		ATL::CComObject<T> *pObject = NULL;
		HRESULT hr = ATL::CComObject<T>::CreateInstance(&pObject);
		ATLENSURE_SUCCEEDED(hr);

		return pObject;
	}
};

}; // namespace swish
