/*
    Swish main (header)

    Copyright (C) 2006  Alexander Lamaison <awl03 (at) doc.ic.ac.uk>

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

#ifndef SWISH_H_
#define SWISH_H_

// NDEBUG for asserts
#ifndef _DEBUG
#define NDEBUG
#else
#undef NDEBUG
#endif

#include <cassert>

#include <iostream>
using std::endl;
using std::cout;
using std::cerr;
using std::cin;

#include <afxwin.h>

#include "libssh/libssh.h"
#include "libssh/sftp.h"

#include "server.h"

// Debug-only print macro
#ifdef _DEBUG
#include <sstream>
extern std::ostringstream traceStream;

#define DPRINT(x) traceStream << x; TRACE(traceStream.str().c_str());
#define DPRINTLN(x) DPRINT(x); DPRINT("\n")
#else
#define DPRINT(x)
#define DPRINTLN(x)
#endif

#endif /*SWISH_H_*/

/* $Id$ */
