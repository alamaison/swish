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
#define INITGUID // Causes PKEYs declared in header to be defined
#include "properties.h"

#include "../RemotePidl.h"

using namespace swish;

HRESULT properties::GetProperty(
	PCUITEMID_CHILD pidl, const SHCOLUMNID* pscid, VARIANT* pv)
{
	bool fHeader = (pidl == NULL);

	CRemoteItemHandle rpidl(pidl);

	// If pidl: Request is for an item detail.  Retrieve from pidl and
	//          return string
	// Else:    Request is for a column heading.  Return heading as BSTR

	// Display name (Label)
	if (IsEqualPropertyKey(*pscid, PKEY_ItemNameDisplay))
	{
		TRACE("\t\tRequest: PKEY_ItemNameDisplay\n");
		
		return _FillDetailsVariant(
			fHeader ? L"Name" : rpidl.GetFilename(), pv);
	}
	// Owner
	else if (IsEqualPropertyKey(*pscid, PKEY_FileOwner))
	{
		TRACE("\t\tRequest: PKEY_FileOwner\n");

		return _FillDetailsVariant(
			fHeader ? L"Owner" : rpidl.GetOwner(), pv);
	}
	// Group
	else if (IsEqualPropertyKey(*pscid, PKEY_Group))
	{
		TRACE("\t\tRequest: PKEY_SwishRemoteGroup\n");
		
		return _FillDetailsVariant(
			fHeader ? L"Group" : rpidl.GetGroup(), pv);
	}
	// File permissions: drwxr-xr-x form
	else if (IsEqualPropertyKey(*pscid, PKEY_Permissions))
	{
		TRACE("\t\tRequest: PKEY_SwishRemotePermissions\n");
		
		return _FillDetailsVariant(
			fHeader ? L"Permissions" : rpidl.GetPermissionsStr(), pv);
	}
	// File size in bytes
	else if (IsEqualPropertyKey(*pscid, PKEY_Size))
	{
		TRACE("\t\tRequest: PKEY_Size\n");

		return fHeader ?
			_FillDetailsVariant(L"Size", pv) : 
			_FillUI8Variant(rpidl.GetFileSize(), pv);
	}
	// Last modified date
	else if (IsEqualPropertyKey(*pscid, PKEY_DateModified))
	{
		TRACE("\t\tRequest: PKEY_DateModified\n");

		return fHeader ?
			_FillDetailsVariant(L"Last Modified", pv) : 
			_FillDateVariant(rpidl.GetDateModified(), pv);
	}
	TRACE("\t\tRequest: <unknown>\n");

	// Assert unless request is one of the supported properties
	// TODO: System.FindData tiggers this
	// UNREACHABLE;

	return E_FAIL;
}

/**
 * Initialise the VARIANT whose pointer is passed and fill with string data.
 * The string data can be passed in as a wchar array or a CString.  We allocate
 * a new BSTR and store it in the VARIANT.
 */
HRESULT properties::_FillDetailsVariant(PCWSTR pwszDetail, VARIANT* pv)
{
	::VariantInit(pv);
	pv->vt = VT_BSTR;
	pv->bstrVal = ::SysAllocString(pwszDetail);

	return pv->bstrVal ? S_OK : E_OUTOFMEMORY;
}

/**
 * Initialise the VARIANT whose pointer is passed and fill with date info.
 */
HRESULT properties::_FillDateVariant(DATE date, VARIANT* pv)
{
	::VariantInit( pv );
	pv->vt = VT_DATE;
	pv->date = date;

	return S_OK;
}

/**
 * Initialise the VARIANT whose pointer is passed and fill with 64-bit unsigned.
 */
HRESULT properties::_FillUI8Variant(ULONGLONG ull, VARIANT* pv)
{
	::VariantInit( pv );
	pv->vt = VT_UI8;
	pv->ullVal = ull;

	return S_OK; // TODO: return success of VariantInit
}
