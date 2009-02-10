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

namespace swish {
namespace properties {

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
CComVariant GetProperty(
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
	// Last accessed date
	else if (IsEqualPropertyKey(scid, PKEY_DateAccessed))
	{
		var = CComVariant(rpidl.GetDateAccessed(), VT_DATE);
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


/**
 * Helpers for CompareProperty function.
 */
namespace { // private

	/**
	 * Comparison function for operands that can't be compared by subtraction.
	 *
	 * For example, when comparing two ULONGLONGs, a subtraction may overflow
	 * the size of a return value which is an int.  
	 *
	 * This method is a safe way to compare things and works around this and
	 * other problems.  However, if this approach is used to compare a 
	 * datatype such as a string, it may be slow as it can perform two
	 * comparisons (in the > or < cases).
	 */
	template <typename T>
	int Compare(const T& operand1, const T& operand2)
	{
		if (operand1 == operand2)
			return 0;
		else if (operand1 < operand2)
			return -1;
		else
			return 1;
	}

	/**
	 * Comparison function overloaded for VARIANTS.
	 *
	 * Variants have their own comparison function, VarCmp(), which should be
	 * quicker than the two-step comparison used in the generic template
	 * version.  For some reason, VarCmp() doesn't work for several of the
	 * numeric types (VT_I1, VT_INT, VT_UI2, VT_UI4, VT_UINT or VT_UI8) so
	 * we have to delegate these comparisons to the template version.
	 */
	int Compare(const CComVariant& var1, const CComVariant& var2)
	{
		ATLENSURE_RETURN_VAL(V_VT(&var1) == V_VT(&var2), -1);

		switch (V_VT(&var1))
		{
		// VarCmp cannot compare any of VT_I1, VT_INT, VT_UI2, VT_UI4, 
		// VT_UINT or VT_UI8 properly so we have to do it ourselves.
		case VT_I1:
			return Compare(V_I1(&var1), V_I1(&var2));
		case VT_INT:
			return Compare(V_INT(&var1), V_INT(&var2));
		case VT_UI2:
			return Compare(V_UI2(&var1), V_UI2(&var2));
		case VT_UI4:
			return Compare(V_UI4(&var1), V_UI4(&var2));
		case VT_UINT:
			return Compare(V_UINT(&var1), V_UINT(&var2));
		case VT_UI8:
			return Compare(V_UI8(&var1), V_UI8(&var2));
		default:
			// Subtracting 1 from VarCmp() gives expected 1/0/-1 result
			return ::VarCmp(
				static_cast<VARIANT*>(const_cast<CComVariant *>(&var1)),
				static_cast<VARIANT*>(const_cast<CComVariant *>(&var2)),
				LOCALE_USER_DEFAULT) - 1;
		}
	}
}

/**
 * Compare two PIDLs by one of their properties.
 *
 * @param pidl1  First PIDL in the comparison.
 * @param pidl2  Second PIDL in the comparison.
 * @param scid   Property on which to compare the two PIDLs.
 *
 * @retval -1 if pidl1 < pidl2 for chosen property.
 * @retval  0 if pidl1 == pidl2 for chosen property.
 * @retval  1 if pidl1 > pidl2 for chosen property.
 */
int CompareByProperty(
	PCUITEMID_CHILD pidl1, PCUITEMID_CHILD pidl2, const SHCOLUMNID& scid)
{
	CComVariant var1 = GetProperty(pidl1, scid);
	CComVariant var2 = GetProperty(pidl2, scid);

	return Compare(var1, var2);
}

}} // namespace swish::properties