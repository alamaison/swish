/**
    @file

    C-like interface to Swish meta-data.

    @if license

    Copyright (C) 2009-2012 Vaclav Slavik
    Copyright (C) 2013, 2014  Alexander Lamaison <awl03@doc.ic.ac.uk>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.

    @endif
*/

// Based on winsparkle-version.h.

#ifndef SWISH_METADATA_H
#define SWISH_METADATA_H

// IMPORTANT: The version information in this file must be digestable by the
// resource (RC) compiler.  No C++ allowed.

#define SWISH_MAJOR_VERSION 0
#define SWISH_MINOR_VERSION 8
#define SWISH_BUGFIX_VERSION 2

#define SWISH_MAKE_STRING(x) #x

// Have to indirect via this helper macro to get version numbers expanded out
#define SWISH_MAKE_VERSION_STRING(a,b,c) \
        SWISH_MAKE_STRING(a) "." SWISH_MAKE_STRING(b) "." SWISH_MAKE_STRING(c)

#define SWISH_VERSION_STRING \
    SWISH_MAKE_VERSION_STRING(SWISH_MAJOR_VERSION, \
                              SWISH_MINOR_VERSION, \
                              SWISH_BUGFIX_VERSION)

#define SWISH_PROGRAM_NAME "Swish"

#define SWISH_COPYRIGHT "Copyright (C) 2007-2014 Alexander Lamaison and contributers."

#define SWISH_DESCRIPTION "Easy SFTP for Windows Explorer"

#endif
