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
		/**
		 * Column-specific aspects of remote item property retrieval.
		 *
		 * The functions in this namespace are effectively accessors for
		 * aColumns, the array of constant column data in column.cpp.
		 * This data determines the layout and format of the columns in
		 * the Explorer view.
		 *
		 * Columns are fetched by index.  Typically, the first thing
		 * Explorer does is query for column headers with increasing index
		 * until a failure occurs.  This marks the end of the columns.
		 *
		 * Fetching column details is left to the generic property system
		 * in swish::properties.  This namespace takes responsibility for
		 * formatting this data suitable for use by 
		 * IShellDetails/IShellFolder::GetDetailsOf().
		 */
		namespace column
		{
			/**
			 * Column indices.
			 * Must start at 0 and be consecutive.  Must be identical to
			 * the order of entries in s_aColumns (see column.cpp).
			 */
			const enum columnIndices {
				FILENAME = 0,
				SIZE,
				TYPE,
				MODIFIED_DATE,
				PERMISSIONS,
				OWNER,
				GROUP,
				OWNER_ID,
				GROUP_ID
			};

			SHELLDETAILS GetHeader(UINT iColumn);

			SHELLDETAILS GetDetailsFor(
				__in PCUITEMID_CHILD pidl, UINT iColumn);

			SHCOLUMNID MapColumnIndexToSCID(UINT iColumn);

			SHCOLSTATEF GetDefaultState(UINT iColumn);
		}
	}
}