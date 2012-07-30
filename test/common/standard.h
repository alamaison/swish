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

// Better type safety for PIDLs (must be before <shlobj.h>)
#define STRICT_TYPED_ITEMIDS