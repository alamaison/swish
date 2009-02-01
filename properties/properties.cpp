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

#include "stdafx.h"
#define INITGUID           // Causes PKEYs declared in header to be defined
#include "properties.h"

#include "../RemotePidl.h"

using namespace swish;

CComVariant properties::GetProperty(
	PCUITEMID_CHILD pidl, const SHCOLUMNID& scid)
{
	CRemoteItemHandle rpidl(pidl);
	ATLASSERT(!rpidl.IsEmpty());

	CComVariant var;

	// Display name (Label)
	if (IsEqualPropertyKey(scid, PKEY_ItemNameDisplay))
	{
		var = rpidl.GetFilename();
	}
	// Owner
	else if (IsEqualPropertyKey(scid, PKEY_FileOwner))
	{
		var = rpidl.GetOwner();
	}
	// Group
	else if (IsEqualPropertyKey(scid, PKEY_Group))
	{		
		var = rpidl.GetGroup();
	}
	// File permissions: drwxr-xr-x form
	else if (IsEqualPropertyKey(scid, PKEY_Permissions))
	{
		var = rpidl.GetPermissionsStr();
	}
	// File size in bytes
	else if (IsEqualPropertyKey(scid, PKEY_Size))
	{
		var = rpidl.GetFileSize();
	}
	// Last modified date
	else if (IsEqualPropertyKey(scid, PKEY_DateModified))
	{
		var = CComVariant(rpidl.GetDateModified(), VT_DATE);
	}
	else
	{
		AtlThrow(E_FAIL);
	}

	return var;
}