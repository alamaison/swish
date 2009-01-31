/*  Explorer column details.

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
*/

#pragma once

#include <WinDef.h>     // Basic Windows types
#include <ShTypes.h>    // Basic shell types
#include <sal.h>        // Code analysis

namespace swish
{
	namespace properties
	{
		namespace column
		{
			HRESULT GetDetailsOf(
				__in_opt PCUITEMID_CHILD pidl, UINT iColumn, 
				__out SHELLDETAILS* psd);

			HRESULT GetDefaultColumn(
				__reserved DWORD dwReserved, 
				__out ULONG* pSort, __out ULONG* pDisplay);
			HRESULT GetDefaultColumnState(
				unsigned int iColumn, SHCOLSTATEF* pcsFlags);
			HRESULT MapColumnToSCID(
				__in unsigned int iColumn, __out PROPERTYKEY *pscid);

			UINT columnCount();
		}
	}
}