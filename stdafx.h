// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

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

/* ATL Setup **************************************************************** */
#define _ATL_APARTMENT_THREADED
#define _ATL_NO_AUTOMATIC_NAMESPACE

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit

#ifdef _DEBUG
#define _ATL_DEBUG_QI
#define _ATL_DEBUG_INTERFACES
#endif

#include <atlbase.h>       // base ATL classes
#include <atlcom.h>
#include <atlwin.h>        // ATL GUI classes
#include <atltypes.h>
#include <atlctl.h>
#include <atlhost.h>

using namespace ATL;

/* WTL Setup **************************************************************** */
#define _WTL_USE_CSTRING

#include <atlapp.h>	       // base WTL classes
//extern CAppModule _Module; // WTL version of CComModule
#define _Module (*_pModule)  // WTL alternate version that works with attributes
#include <atlframe.h>      // WTL frame window classes
#include <atlmisc.h>       // WTL utility classes like CString
#include <atlcrack.h>      // WTL enhanced msg map macros

#include <atlctrls.h>

#include <shlobj.h>

#include <vector>

#ifdef UNREACHABLE
#undef UNREACHABLE
#endif
#ifdef _DEBUG
#define UNREACHABLE ATLASSERT(0);
#else
#define UNREACHABLE __assume(0);
#endif

// Globals
extern WTL::CImageList g_ImglistSmall, g_ImglistLarge;

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
