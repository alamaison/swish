// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

/*
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
*/

/* Strictness *************************************************************** */

#ifndef STRICT
#define STRICT
#endif

// Better type safety for PIDLs (must be before <shlobj.h>)
#define STRICT_TYPED_ITEMIDS

// Ensure strictly correct usage of SAL attributes
#ifndef __SPECSTRINGS_STRICT_LEVEL
#define __SPECSTRINGS_STRICT_LEVEL 1 //3 // see specstrings_strict.h
#endif

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER				// Allow use of features specific to Windows 2k or later.
#define WINVER 0x0400		// Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows 2k or later.                   
#define _WIN32_WINNT 0x0500	// Change this to the appropriate value to target other versions of Windows.
#endif						

#ifndef _WIN32_WINDOWS		// Allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS 0x0400 // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE			// Allow use of features specific to IE 6.0 or later.
#define _WIN32_IE 0x0400	// Change this to the appropriate value to target other versions of IE.
#endif

/* ATL Setup **************************************************************** */
#define _ATL_APARTMENT_THREADED
#define _ATL_NO_AUTOMATIC_NAMESPACE

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit

#ifdef _DEBUG
#define _ATL_DEBUG_QI
#define _ATL_DEBUG_INTERFACES
#endif

#include <atlbase.h>          // base ATL classes
#include <atlcom.h>
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

/* Includes ***************************************************************** */

#include <libssh2.h>
#include <libssh2_sftp.h>

// Swish type library
#import "libid:b816a838-5022-11dc-9153-0090f5284f85" raw_interfaces_only, raw_native_types, auto_search/*, no_namespace, embedded_idl*/

using namespace Swish;