/**
    @file

    Set up WTL.

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

/**
 * @file
 *
 * This file must be included before any other WTL headers as they depend
 * on <atlapp.h> already being included.  Also, any macros defined
 * in this file must be allowed to affect the behaviour of other parts of 
 * ATL.  This is contrary to the usual top-down include order.
 */
#pragma once

#define _WTL_NO_AUTOMATIC_NAMESPACE

#include <atl.hpp>   // Common ATL setup

#include <atlapp.h>  // WTL