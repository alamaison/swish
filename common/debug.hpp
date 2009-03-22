/**
    @file

    Debug macros.

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

    @endif
*/

#pragma once

#include <ComDef.h> // For _com_error

#define TRACE(msg, ...) ATLTRACE(msg ## "\n", __VA_ARGS__)
#define FUNCTION_TRACE TRACE(__FUNCTION__" called");
#define METHOD_TRACE TRACE(__FUNCTION__" called (this=%p)", this);

#ifdef _DEBUG
#define REPORT(expr) \
do { \
    LPVOID lpMsgBuf; \
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, \
		NULL, ::GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), \
        (LPTSTR) &lpMsgBuf, 0, NULL ); \
	_ASSERT_EXPR((expr), (LPTSTR)lpMsgBuf); LocalFree(lpMsgBuf); \
} while(0)
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

#define ATLENSURE_REPORT_THROW(expr, error, hr)                      \
do {                                                                 \
	int __atl_condVal=!!(expr);                                      \
	_ASSERT_EXPR(__atl_condVal, _com_error((error)).ErrorMessage()); \
	if(!(__atl_condVal)) AtlThrow(hr);                               \
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
