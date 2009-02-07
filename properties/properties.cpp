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
#include "Mode.h"          // Unix-style permissions

#include <ShellAPI.h>      // For SHGetFileInfo()

using namespace swish;

namespace { // private

	/**
	 * Find the Windows friendly type name for the file given as a PIDL.
	 *
	 * This type name is the one used in Explorer details.  For example,
	 * something.txt is given the type name "Text Document" and a directory
	 * is called a "File Folder" regardless of its name.
	 */
	CString LookupFriendlyTypeName(CRemoteItemHandle pidl)
	{
		DWORD dwAttributes = 
			(pidl.IsFolder()) ?
				FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;

		UINT uInfoFlags = SHGFI_USEFILEATTRIBUTES | SHGFI_TYPENAME;

		SHFILEINFO shfi;
		ATLENSURE(::SHGetFileInfo(
			pidl.GetFilename(), dwAttributes, 
			&shfi, sizeof(shfi), uInfoFlags));
		
		return shfi.szTypeName;
	}
}

/**
 * Get the requested property for a file based on its PIDL.
 *
 * Many of these will be standard system properties but some are custom
 * to Swish if an appropriate one did not already exist.
 */
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
	// Owner ID (UID)
	else if (IsEqualPropertyKey(scid, PKEY_OwnerId))
	{
		var = rpidl.GetOwnerId();
	}
	// Group ID (GID)
	else if (IsEqualPropertyKey(scid, PKEY_GroupId))
	{		
		var = rpidl.GetGroupId();
	}
	// File permissions: drwxr-xr-x form
	else if (IsEqualPropertyKey(scid, PKEY_Permissions))
	{
		DWORD dwPerms = rpidl.GetPermissions();
		var = mode::Mode(dwPerms).toString().c_str();
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
	// Friendly type name
	else if (IsEqualPropertyKey(scid, PKEY_ItemTypeText))
	{
		var = LookupFriendlyTypeName(rpidl);
	}
	else
	{
		AtlThrow(E_FAIL);
	}

	return var;
}
