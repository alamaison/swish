/**
    @file

    Source file for the precompiled header.

    @if licence

    Copyright (C) 2010  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

/**
 * @file
 *
 * Any project that force-includes a precompiled header 'pch.h' should have
 * this file in its list of source files.  This file should also have
 * Create Precompiled Header (/Yc) set in the project properties.  
 *
 * The combination of the two step above causes the compiler to place a
 * precompiled header in the intermediate directory at the start of compilation.
 * The precompiled header will be $(TargetName).pch and the intermediate file 
 * pch.obj will contain the pre-compiled type information.
 *
 * @warning
 * Do not add any code nor include any other headers in this file.
 */
