/**
 * Precompiled-header.
 *
 * This file exists @b solely to include other headers which should be
 * precompiled to reduce build times.  The source files which include this
 * header should not depend on anything in it.  In other words, this file
 * is an optimisation alone and all files should still compile if
 * USING_PRECOMPILED_HEADERS is not defined.
 *
 * @note  It is specifically forbidden to add anything other than #include
 * statements to this file.  If code must be included in all the other files
 * in the project, it should be added to standard.h.
 *
 * @warning  standard.h must be the first item included in this header
 * otherwise the behaviour will change based on USING_PRECOMPILED_HEADERS:
 * macros in standard.h must be able to affect all the other includes.
 *
 * @warning  Do not include pch.h in any header files.  External clients
 * should not ne affected by anything in it.
 */

#pragma once

#ifdef USING_PRECOMPILED_HEADERS

#include "standard.h"

#endif // USING_PRECOMPILED_HEADERS - do not add anything below this line