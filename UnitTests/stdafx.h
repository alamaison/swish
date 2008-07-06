// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

#ifndef STRICT
#define STRICT
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


/* Includes ***************************************************************** */

#include <vector>
#include <strsafe.h>
#include <shlobj.h>           // Typical Shell header file

// Swish type library
#import "libid:b816a838-5022-11dc-9153-0090f5284f85" raw_interfaces_only, raw_native_types, auto_search, no_namespace, embedded_idl

/* Miscellanious ************************************************************ */

// This is here only to tell VC7 Class Wizard this is an ATL project
#ifdef ___VC7_CLWIZ_ONLY___
CComModule
CExeModule
#endif

#if defined _M_IX86
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif


#undef __SPECSTRINGS_STRICT_LEVEL
#define __SPECSTRINGS_STRICT_LEVEL 3 // see specstrings_strict.h
