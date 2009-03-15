/**
 * @file
 *
 * Declarations which must be included by @b every file.
 *
 * @warning
 * This header must be the @b first thing after the precompiled is included.
 * Macros defined here may change the behaviour of subsequent includes.
 *
 * @warning
 * The precompiled header file, pch.h, should include this file if it includes
 * and and, again, this must be @b first.
 */

#pragma once

/* API versions ************************************************************ */

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501 // Allow features specific to Windows XP or later
#endif						

#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers

/* ATL Setup **************************************************************** */

#define _ATL_APARTMENT_THREADED
#define _ATL_NO_AUTOMATIC_NAMESPACE

// Make some CString constructors explicit
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS

#ifdef _DEBUG
#define _ATL_DEBUG_QI
#define _ATL_DEBUG_INTERFACES
#endif

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