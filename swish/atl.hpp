/**
    @file

    Set up ATL support.

    @if licence

    Copyright (C) 2009  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

/**    
 * @file
 *
 * Any files that need ATL support should include this header to ensure
 * that ATL is set up consistently across different files and projects.
 *
 * This file must be included before any other ATL headers as they depend
 * on <atlbase.h> and <atlcom.h> already being included.  Also, any macros
 * in this file must be allowed to affect the behaviour of other parts of 
 * ATL.  This is contrary to the usual top-down include order.
 */

#pragma once

#define _ATL_FREE_THREADED

#ifdef _ATL_SINGLE_THREADED
#error "_ATL_SINGLE_THREADED conflicts with _ATL_FREE_THREADED"
#endif

#ifdef _ATL_APARTMENT_THREADED
#error "_ATL_APARTMENT_THREADED conflicts with _ATL_FREE_THREADED"
#endif


#define _ATL_NO_AUTOMATIC_NAMESPACE

// Make some CString constructors explicit
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS

// (U)LONGLONG CComVariant support
#define _ATL_SUPPORT_VT_I8

#ifdef _DEBUG
#define _ATL_DEBUG_QI
#define _ATL_DEBUG_INTERFACES
#endif

#define STRICT_TYPED_ITEMIDS ///< Better type safety for PIDLs (must be before
                             ///< <shtypes.h> or <shlobj.h>).  As <atlbase.h>
                             ///< includes <shtypes.h> via <shlwapi.h> we 
                             ///< need to define this here.

#define _ATL_CUSTOM_THROW
#include "exception.hpp"
/**
 * Custom ATL thrower which throws std::exception derived exceptions.
 */
__declspec(noreturn) inline void AtlThrow(HRESULT hr)
{
	throw swish::exception::com_exception(hr);
};

#include <atlbase.h> // base ATL classes
#include <atlcom.h>
