/*  Standard project includes that are unlikely to change often.

    Copyright (C) 2007, 2008  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

/* Platform ***************************************************************** */

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

#ifndef _WIN32_IE			// Allow use of features specific to IE 5.0 or later.
#define _WIN32_IE 0x0500	// Change this to the appropriate value to target other versions of IE.
#endif

// This is here only to tell VC7 Class Wizard this is an ATL project
#ifdef ___VC7_CLWIZ_ONLY___
CComModule
CExeModule
#endif

/* ATL Setup **************************************************************** */

#define _ATL_APARTMENT_THREADED
#define _ATL_NO_AUTOMATIC_NAMESPACE

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit

#ifdef _DEBUG
//#define _ATL_DEBUG_QI
//#define _ATL_DEBUG_INTERFACES
#endif

#include <atlbase.h>          // base ATL classes
#include <atlcom.h>
#include <atlwin.h>           // ATL GUI classes
#include <atltypes.h>
#include <atlctl.h>
#include <atlhost.h>
#include <atltime.h>          // CTime class

using namespace ATL;

/* WTL Setup **************************************************************** */

#define _WTL_NO_CSTRING

#include <atlapp.h>	          // base WTL classes
//extern CAppModule _Module;  // WTL version of CComModule
#define _Module (*_pModule)   // WTL alternate version, works with attributes
//#include <atlframe.h>         // WTL frame window classes
//#include <atlmisc.h>          // WTL utility classes like CString
#include <atlcrack.h>         // WTL enhanced msg map macros
#include <atlctrls.h>         // WTL control wrappers

/* Handler prototypes ******************************************************* */

// LRESULT MessageHandler(
//         UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
#define MESSAGE_HANDLER_PARAMS UINT, WPARAM, LPARAM, BOOL&
// LRESULT CommandHandler(
//         WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
#define COMMAND_HANDLER_PARAMS WORD, WORD, HWND, BOOL&
// LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
#define NOTIFY_HANDLER_PARAMS int, LPNMHDR, BOOL&

/* Debug macros ************************************************************* */
#define FUNCTION_TRACE ATLTRACE(__FUNCTION__" called\n");
#define METHOD_TRACE ATLTRACE(__FUNCTION__" called (this=%p)\n", this);

#define VERIFY(f)          ATLVERIFY(f)
#define ASSERT(f)          ATLASSERT(f)
#ifdef _DEBUG
#define REPORT(expr)  { \
    LPVOID lpMsgBuf; \
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, \
		NULL, ::GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), \
        (LPTSTR) &lpMsgBuf, 0, NULL ); \
	_ASSERT_EXPR((expr), (LPTSTR)lpMsgBuf); LocalFree(lpMsgBuf); }
#else
#define REPORT(expr) (expr)
#endif

#ifdef UNREACHABLE
#undef UNREACHABLE
#endif
#ifdef _DEBUG
#define UNREACHABLE ATLASSERT(0);
#else
#define UNREACHABLE __assume(0);
#endif

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
#endif // DEBUG

/* COM Exception handler **************************************************** */

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
}

/* Includes ***************************************************************** */

#include <vector>
#include <strsafe.h>
#include <shlobj.h>   // Typical Shell header file
#include <comdef.h>   // For _com_error

#include "Swish.h"    // Header generated for our hand-written IDL file(s)

/* Manifests **************************************************************** */

#if defined _M_IX86
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
