/*  Mixin class which augments CComObjects with a creator of AddReffed instances.

    Copyright (C) 2008  Alexander Lamaison <awl03@doc.ic.ac.uk>

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
*/

#pragma once

#include <atl.hpp>      // Common ATL setup

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
	 * @throws   CAtlException if creation fails.
	 */
	static ATL::CComPtr<T> CreateCoObject() throw(...)
	{
		ATL::CComObject<T> *pObject = NULL;
		HRESULT hr = ATL::CComObject<T>::CreateInstance(&pObject);
		ATLENSURE_SUCCEEDED(hr);

		return pObject;
	}
};