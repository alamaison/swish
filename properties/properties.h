/*  File property handling.

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
#include <WTypes.h>     // Windows COM types
#include <sal.h>        // Code analysis

#include <PropIdl.h>    // Property constants
#include <propkey.h>    // Predefined shell property keys
#include <propkeydef.h> // Property key definition and comparison

#include <atlcomcli.h>  // ATL smart types

namespace swish
{
	namespace properties
	{
		/**
		 * Custom property IDs (PIDs) for remote folder.
		 */
		enum propertyIds {
			PID_GROUP = PID_FIRST_USABLE,
			PID_PERMISSIONS,
			PID_OWNER_ID,
			PID_GROUP_ID
		};

		/**
		 * Custom properties (PKEYs) for Swish remote folder.
		 *
		 * Ideally, we want as few of these as possible.  If an appropriate
		 * one already exists in propkey.h, that should be used instead.
		 *
		 * The Swish remote folder FMTID GUID which collects all the custom
		 * properties together is @c {b816a851-5022-11dc-9153-0090f5284f85}.
		 */
		DEFINE_PROPERTYKEY(
			PKEY_Group, \
			0xb816a851, 0x5022, 0x11dc, 0x91, 0x53, 0x00, 0x90, 0xf5, 0x28, \
			0x4f, 0x85, PID_GROUP);
		DEFINE_PROPERTYKEY(
			PKEY_Permissions, \
			0xb816a851, 0x5022, 0x11dc, 0x91, 0x53, 0x00, 0x90, 0xf5, 0x28, \
			0x4f, 0x85, PID_PERMISSIONS);
		DEFINE_PROPERTYKEY(
			PKEY_OwnerId, \
			0xb816a851, 0x5022, 0x11dc, 0x91, 0x53, 0x00, 0x90, 0xf5, 0x28, \
			0x4f, 0x85, PID_OWNER_ID);
		DEFINE_PROPERTYKEY(
			PKEY_GroupId, \
			0xb816a851, 0x5022, 0x11dc, 0x91, 0x53, 0x00, 0x90, 0xf5, 0x28, \
			0x4f, 0x85, PID_GROUP_ID);

		CComVariant GetProperty(
			__in PCUITEMID_CHILD pidl, __in const SHCOLUMNID& scid);
		
		int CompareByProperty(
			__in PCUITEMID_CHILD pidl1, __in PCUITEMID_CHILD pidl2,
			__in const SHCOLUMNID& scid);
	}
}