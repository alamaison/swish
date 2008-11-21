/*  Standard project includes that are unlikely to change often.

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

    In addition, as a special exception, the the copyright holders give you
	permission to combine this program with free software programs or the 
	OpenSSL project's "OpenSSL" library (or with modified versions of it, 
	with unchanged license). You may copy and distribute such a system 
	following the terms of the GNU GPL for this program and the licenses 
	of the other code concerned. The GNU General Public License gives 
	permission to release a modified version without this exception; this 
	exception also makes it possible to release a modified version which 
	carries forward this exception.
*/

#pragma once

#ifndef STRICT
#define STRICT
#endif

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER				// Allow use of features specific to Windows XP or later.
#define WINVER 0x0501		// Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.
#endif						

#ifndef _WIN32_WINDOWS		// Allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS 0x0410 // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE			// Allow use of features specific to IE 6.0 or later.
#define _WIN32_IE 0x0600	// Change this to the appropriate value to target other versions of IE.
#endif

#define _ATL_APARTMENT_THREADED
#define _ATL_NO_AUTOMATIC_NAMESPACE

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit

#define _CRT_SECURE_NO_WARNINGS 1

#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>
#include <atltypes.h>
#include <atlctl.h>
#include <atlhost.h>

using namespace ATL;

/* Debug macros ************************************************************* */

#ifdef UNREACHABLE
#undef UNREACHABLE
#endif
#ifdef _DEBUG
#define UNREACHABLE ATLASSERT(0);
#else
#define UNREACHABLE __assume(0);
#endif

#define TRACE(msg, ...) ATLTRACE(msg ## "\n", __VA_ARGS__)

#define ATLENSURE_REPORT_HR(expr, error, hr)                         \
do {                                                                 \
	int __atl_condVal=!!(expr);                                      \
	_ASSERT_EXPR(__atl_condVal, _com_error((error)).ErrorMessage()); \
	if(!(__atl_condVal)) return (hr);                                \
} while (0)

#ifdef _DEBUG
#define ATLASSERT_REPORT(expr, error)                                \
do {                                                                 \
	int __atl_condVal=!!(expr);                                      \
	_ASSERT_EXPR(__atl_condVal, _com_error((error)).ErrorMessage()); \
} while (0)
#else
#define ATLASSERT_REPORT(expr, error) ((void)0)
#endif // _DEBUG

#ifdef _DEBUG
#define ATLVERIFY_REPORT(expr, error)                                \
do {                                                                 \
	int __atl_condVal=!!(expr);                                      \
	_ASSERT_EXPR(__atl_condVal, _com_error((error)).ErrorMessage()); \
} while (0)
#else
#define ATLVERIFY_REPORT(expr, error) (expr)
#endif // _DEBUG

/* COM Exception handler **************************************************** */
#ifdef _DEBUG
#define catchCom()            \
catch (const _com_error& e)   \
{                             \
	ATLTRACE("Caught _com_error exception: %w", e.ErrorMessage()); \
	return e.Error();         \
}                             \
catch (const CAtlException& e)\
{                             \
	ATLTRACE("Caught CAtlException"); \
	return e;                 \
}
#else
#define catchCom()            \
catch (const _com_error& e)   \
{                             \
	return e.Error();         \
}                             \
catch (const std::bad_alloc&) \
{                             \
	return E_OUTOFMEMORY;     \
}                             \
catch (const std::exception&) \
{                             \
	return E_UNEXPECTED;      \
}                             \
catch (const CAtlException& e)\
{                             \
	return e;                 \
}                             \
catch (...)                   \
{                             \
	return E_UNEXPECTED;      \
}
#endif // _DEBUG

/* Includes ***************************************************************** */

#include <libssh2.h>
#include <libssh2_sftp.h>

// Swish type library
#import "libid:b816a838-5022-11dc-9153-0090f5284f85" raw_interfaces_only, raw_native_types, no_namespace, auto_search, embedded_idl
