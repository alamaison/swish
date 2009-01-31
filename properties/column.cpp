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

#include "stdafx.h"
#include "column.h"
#include "..\resource.h"

#include "properties.h" // GetProperty() etc.

#include <propkey.h>    // Predefined shell property keys
#include <ShlWapi.h>    // Shell helper functions
#include <CommCtrl.h>   // For LVCFMT_* list view constants

#include <atlstr.h>     // CString
#include <ATLComTime.h> // COleDateTime
#include <OleAuto.h>    // VARIANT functions

#include <vector>
using std::vector;

namespace swish {
namespace properties {
namespace column {

	/**
	 * Column indices.
	 * Must start at 0 and be consecutive.  Must be identical to the order of
	 * entries in s_aColumns, below.
	 */
	static const enum s_columnIndices {
		FILENAME = 0,
		SIZE,
		PERMISSIONS,
		MODIFIED_DATE,
		OWNER,
		GROUP
	};

	/**
	 * Static column information.
	 */
	static const struct {
		int colnameid;
		PROPERTYKEY pkey;
		int pcsFlags;
		int fmt;
		int cxChar;
	} s_aColumns[] = {
		{ IDS_COLUMN_FILENAME, PKEY_ItemNameDisplay,   // Display name (Label)
		  SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT, 30 }, 
		{ IDS_COLUMN_SIZE, PKEY_Size,                  // Size
		  SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 15 },
		{ IDS_COLUMN_PERMISSIONS, PKEY_Permissions,    // Permissions
		  SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT, 20 },
		{ IDS_COLUMN_MODIFIED, PKEY_DateModified,      // Modified date
		  SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT, 20 },
		{ IDS_COLUMN_OWNER, PKEY_FileOwner,            // Owner
		  SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT, 12 },
		{ IDS_COLUMN_GROUP, PKEY_Group,                // Group
		  SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT, 12 }
	};
}}}

using namespace swish::properties;

/**
 * Return number of columns.
 */
UINT column::columnCount()
{
	return sizeof(s_aColumns) / sizeof(s_aColumns[0]);
}

/**
 * Get the default sorting and display columns.
 */
HRESULT column::GetDefaultColumn(
	DWORD /*dwReserved*/, ULONG* pSort, ULONG* pDisplay)
{
	// Sort and display by the filename
	*pSort = column::FILENAME;
	*pDisplay = column::FILENAME;

	return S_OK;
}

/**
 * Returns the default state for the column specified by index.
 */
HRESULT column::GetDefaultColumnState(
	UINT iColumn, SHCOLSTATEF* pcsFlags)
{
	*pcsFlags = 0;

	if (iColumn < columnCount())
	{
		*pcsFlags = s_aColumns[iColumn].pcsFlags;
		return S_OK;
	}
	else
	{
		return E_FAIL;
	}
}

/**
 * Convert column to appropriate property set ID (FMTID) and property ID (PID).
 *
 * @warning
 * This function defines which details are supported as GetDetailsOf() just 
 * forwards the columnID here.  The first column that we return E_FAIL for 
 * marks the end of the supported details.
 */
HRESULT column::MapColumnToSCID(
	UINT iColumn, PROPERTYKEY *pscid)
{
	::ZeroMemory(pscid, sizeof(PROPERTYKEY));

	if (iColumn < columnCount())
	{
		*pscid = s_aColumns[iColumn].pkey;
		return S_OK;
	}
	else
	{
		return E_FAIL;
	}
}

/**
 * Returns detailed information on the items in a folder.
 *
 * @implementing IShellDetails
 *
 * This function operates in two distinctly different ways:
 * If pidl is NULL:
 *     Retrieves the information on the view columns, i.e., the names of
 *     the columns themselves.  The index of the desired column is given
 *     in iColumn.  If this column does not exist we return E_FAIL.
 * If pidl is not NULL:
 *     Retrieves the specific item information for the given pidl and the
 *     requested column.
 * The information is returned in the SHELLDETAILS structure.
 *
 * Most of the work is delegated to GetDetailsEx by converting the column
 * index to a PKEY with MapColumnToSCID.  This function also now determines
 * what the index of the last supported detail is.
 */
HRESULT column::GetDetailsOf(
	PCUITEMID_CHILD pidl, UINT iColumn, SHELLDETAILS* psd)
{
	::ZeroMemory(psd, sizeof(SHELLDETAILS));

	PROPERTYKEY pkey;

	// Lookup PKEY and use it to call GetProperty
	HRESULT hr = MapColumnToSCID(iColumn, &pkey);
	if (SUCCEEDED(hr))
	{
		VARIANT pv;

		// Get details and convert VARIANT result to SHELLDETAILS for return
		hr = properties::GetProperty(pidl, &pkey, &pv);
		if (SUCCEEDED(hr))
		{
			CString strSrc;

			switch (pv.vt)
			{
			case VT_BSTR:
				strSrc = pv.bstrVal;
				::SysFreeString(pv.bstrVal);
				break;
			case VT_UI8:
				if (!IsEqualPropertyKey(pkey, PKEY_Size))
				{
					strSrc.Format(L"%u", pv.ullVal);
				}
				else
				{
					// File size if a special case.  We need to format this 
					// as a value in kilobytes (e.g. 2,348 KB) rather than 
					// returning it as a number
					
					vector<wchar_t> buf(64);
					::StrFormatKBSize(
						pv.ullVal, &buf[0], static_cast<UINT>(buf.size()));
					strSrc = &buf[0];
				}
				break;
			case VT_DATE:
				strSrc = COleDateTime(pv.date).Format();
				break;
			default:
				UNREACHABLE;
			}
			
			psd->str.uType = STRRET_WSTR;
			::SHStrDup(strSrc, &psd->str.pOleStr);
			
			if(!pidl) // Header requested
			{
				psd->fmt = s_aColumns[iColumn].fmt;
				psd->cxChar = s_aColumns[iColumn].cxChar;
			}
		}

		::VariantClear(&pv);
	}

	return hr;
}