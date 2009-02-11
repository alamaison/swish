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
#include "../resource.h"

#include "properties.h" // GetProperty() etc.

#include <propkey.h>    // Predefined shell property keys
#include <ShlWapi.h>    // Shell helper functions
#include <CommCtrl.h>   // For LVCFMT_* list view constants
#include <OleAuto.h>    // VARIANT functions

#include <atldef.h>     // Main ATL macro definitions
#include <ATLComTime.h> // COleDateTime
#include <atlstr.h>     // CString
#include <atlcomcli.h>  // ATL smart types

#include <vector>

using std::vector;

namespace swish {
namespace properties {
namespace column {

/**
 * Functions and data private to this compilation unit.
 */
namespace { // private

	/**
	 * Static column information.
	 * Order of entries must correspond to the indices in columnIndices.
	 */
	const struct {
		int colnameid;
		PROPERTYKEY pkey;
		int pcsFlags;
		int fmt;
		int cxChar;
	} aColumns[] = {
		{ IDS_COLUMN_FILENAME, PKEY_ItemNameDisplay,   // Display name (Label)
		  SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT, 30 }, 
		{ IDS_COLUMN_SIZE, PKEY_Size,                  // Size
		  SHCOLSTATE_TYPE_INT | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 15 },
		{ IDS_COLUMN_TYPE, PKEY_ItemTypeText,          // Friendly type
		  SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT, 20 },
		{ IDS_COLUMN_MODIFIED, PKEY_DateModified,      // Modified date
		  SHCOLSTATE_TYPE_DATE | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT, 20 },
		{ IDS_COLUMN_ACCESSED, PKEY_DateAccessed,      // Accessed date
		  SHCOLSTATE_TYPE_DATE, LVCFMT_LEFT, 20 },
		{ IDS_COLUMN_PERMISSIONS, PKEY_Permissions,    // Permissions
		  SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT, 12 },
		{ IDS_COLUMN_OWNER, PKEY_FileOwner,            // Owner
		  SHCOLSTATE_TYPE_STR, LVCFMT_LEFT, 12 },
		{ IDS_COLUMN_GROUP, PKEY_Group,                // Group
		  SHCOLSTATE_TYPE_STR, LVCFMT_LEFT, 12 },
		{ IDS_COLUMN_OWNER_ID, PKEY_OwnerId,           // Owner ID (UID)
		  SHCOLSTATE_TYPE_INT, LVCFMT_LEFT, 10 },
		{ IDS_COLUMN_GROUP_ID, PKEY_GroupId,           // Group ID (GID)
		  SHCOLSTATE_TYPE_INT, LVCFMT_LEFT, 10 }
	};

	/**
	 * Return number of columns.
	 */
	UINT Count()
	{
		return sizeof(aColumns) / sizeof(aColumns[0]);
	}

	/**
	 * Return the localised heading of the column with index iColumn.
	 */
	CString Header(UINT iColumn)
	{
		return CString(MAKEINTRESOURCE(aColumns[iColumn].colnameid));
	}
}

/**
 * Returns the default state for the column specified by index iColumn.
 */
SHCOLSTATEF GetDefaultState(UINT iColumn)
{
	if (iColumn >= Count())
		AtlThrow(E_FAIL);
	
	return aColumns[iColumn].pcsFlags;
}

/**
 * Convert index to appropriate property set ID (FMTID) and property ID (PID).
 *
 * @warning
 * This function defines which details are supported as GetDetailsOf() just 
 * forwards the columnID here.  The first column that we throw E_FAIL for 
 * marks the end of the supported details.
 */
SHCOLUMNID MapColumnIndexToSCID(UINT iColumn)
{
	if (iColumn >= Count())
		AtlThrow(E_FAIL);

	return aColumns[iColumn].pkey;
}

/**
 * Get the heading for the column with index iColumn.
 *
 * If the index is out-of-range, we throw E_FAIL.  This is how Explorer
 * finds the end of the supported details.
 *
 * As well as the text to use as a label, the returned SHELLDETAILS
 * holds the width of the column in characters, cxChar, and the
 * formatting information about the data the column will hold 
 * (e.g. SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT).
 *
 * @warning
 * The returned SHELLDETAILS holds the label as a pointer to a
 * string allocated with CoTaskMemAlloc.  This must be properly
 * freed to avoid a memory leak. 
 */
SHELLDETAILS GetHeader(UINT iColumn)
{
	SHELLDETAILS sd;
	::ZeroMemory(&sd, sizeof(SHELLDETAILS));

	if (iColumn >= Count())
		AtlThrow(E_FAIL);

	sd.str.uType = STRRET_WSTR;
	::SHStrDup(Header(iColumn), &sd.str.pOleStr);
	
	sd.fmt = aColumns[iColumn].fmt;
	sd.cxChar = aColumns[iColumn].cxChar;

	return sd;
}

namespace { // private

	/**
	 * Format date portion of SYSTEMTIME according to user's locale.
	 */
	CString FormatDate(const SYSTEMTIME& st)
	{
		int size = ::GetDateFormat(
			LOCALE_USER_DEFAULT, 0, &st, NULL, NULL, 0);
		ATLASSERT(size > 0);
		if (size > 0)
		{
			vector<wchar_t> buffer(size);
			if (buffer.size() > 0)
			{
				size = ::GetDateFormat(
					LOCALE_USER_DEFAULT, 0, &st, NULL, &buffer[0],
					static_cast<UINT>(buffer.size()));
				ATLASSERT(size > 0);

				return CString(
					&buffer[0], std::min<int>( // Must not embed NULL
						size, static_cast<int>(buffer.size())) - 1);
			}
		}

		ATLASSERT(!"About to return an empty date string");
		return L"";
	}

	/**
	 * Format time portion of SYSTEMTIME according to user's locale but
	 * without including seconds.
	 */
	CString FormatTime(const SYSTEMTIME& st)
	{
		int size = ::GetTimeFormat(
			LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st, NULL, NULL, 0);
		ATLASSERT(size > 0);
		if (size > 0)
		{
			vector<wchar_t> buffer(size);
			if (buffer.size() > 0)
			{
				size = ::GetTimeFormat(
					LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st, NULL, &buffer[0],
					static_cast<UINT>(buffer.size()));
				ATLASSERT(size > 0);

				return CString(
					&buffer[0], std::min<int>( // Must not embed NULL
						size, static_cast<int>(buffer.size())) - 1);
			}
		}

		ATLASSERT(!"About to return an empty time string");
		return L"";
	}

	/**
	 * Format the date and time according to user locale but without seconds.
	 *
	 * Other methods such as COleDateTime.Format() provide no way to suppress
	 * displaying seconds without abandoning local-independence entirely.  
	 * This function should render the date and time in the same way that
	 * Windows Explorer does for any particular locale.
	 */
	CString FormatDateAndTime(const COleDateTime& date)
	{
		SYSTEMTIME st;
		ATLVERIFY(date.GetAsSystemTime(st));

		return FormatDate(st) + L" " + FormatTime(st);
	}
}

/**
 * Get the contents of the column with index iColumn for the given PIDL.
 *
 * Regardless of the type of the underlying data, this function always
 * returns the data as a string.  If any formatting is required, it must
 * be done in this function.
 *
 * @warning
 * The returned SHELLDETAILS holds a pointer to a string allocated with 
 * CoTaskMemAlloc.  This must be properly freed to avoid a memory leak.
 *
 * Most of the work is delegated to the properties functions by converting 
 * the column index to a PKEY with MapColumnIndexToSCID.
 *
 * @throws  E_FAIL if the column index is out of range.
 */
SHELLDETAILS GetDetailsFor(PCUITEMID_CHILD pidl, UINT iColumn)
{
	// Lookup PKEY and use it to call GetProperty
	PROPERTYKEY pkey = MapColumnIndexToSCID(iColumn);

	// Get details and convert VARIANT result to SHELLDETAILS for return
	CComVariant var = properties::GetProperty(pidl, pkey);

	CString strSrc;
	switch (var.vt)
	{
	case VT_BSTR:
		strSrc = var.bstrVal;
		break;
	case VT_UI4:
		strSrc.Format(L"%u", var.ulVal);
		break;
	case VT_UI8:
		if (IsEqualPropertyKey(pkey, PKEY_Size))
		{
			// File size if a special case.  We need to format this 
			// as a value in kilobytes (e.g. 2,348 KB) rather than 
			// returning it as a number
			
			vector<wchar_t> buf(64);
			::StrFormatKBSize(
				var.ullVal, &buf[0], static_cast<UINT>(buf.size()));
			strSrc = &buf[0];
		}
		else
		{
			strSrc.Format(L"%u", var.ullVal);
		}
		break;
	case VT_DATE:
		strSrc = FormatDateAndTime(COleDateTime(var));
		break;
	default:
		ATLASSERT(!"GetProperty() returned a VARIANT type we don't handle");
	}
	
	SHELLDETAILS sd;
	::ZeroMemory(&sd, sizeof(SHELLDETAILS));
	sd.str.uType = STRRET_WSTR;
	::SHStrDup(strSrc, &sd.str.pOleStr);

	return sd;
}

/**
 * Helpers for CompareDetailsOf function.
 */
namespace { // private

	/**
	 * Compare two PIDLs by the property in column iColumn.
	 */
	inline int CompareByColumn(
		__in PCUITEMID_CHILD pidl1, __in PCUITEMID_CHILD pidl2, UINT iColumn)
	{
		SHCOLUMNID scid = MapColumnIndexToSCID(iColumn);

		return properties::CompareByProperty(pidl1, pidl2, scid);
	}
}


/**
 * Compare two PIDLs by the detail in a column or by all columns.
 *
 * If fCompareAllFields is false, the PIDLs are compared by the property in
 * column iColumn.  Otherwise, all their properties are included in the 
 * comparison.  In this case, the first non-equal field is used to decide
 * is a property is less-that or greater-than.
 *
 * @retval -1 if pidl1 < pidl2 in column iColumn (or for all columns).
 * @retval  0 if pidl1 == pidl2 in column iColumn (or for all columns).
 * @retval  1 if pidl1 > pidl2 in column iColumn (or for all columns).
 */
int CompareDetailOf(
	PCUITEMID_CHILD pidl1, PCUITEMID_CHILD pidl2, UINT iColumn, 
	bool fCompareAllFields, bool fCanonical)
{
	(void)fCanonical; // I think our comparisons are always canonical

	if (fCompareAllFields) // Wants a complete equality test
	{
		ATLASSERT(iColumn == 0);

		for (UINT i = 0; i < Count(); ++i)
		{
			int result = CompareByColumn(pidl1, pidl2, iColumn);
			if (result != 0)
				return result;
		}

		return 0;
	}
	else
	{
		return CompareByColumn(pidl1, pidl2, iColumn);
	}
}

}}} // namespace swish::properties::column