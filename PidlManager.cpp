/*  Manage the creation and manipulation of PIDLs

    Copyright (C) 2007  Alexander Lamaison <awl03@doc.ic.ac.uk>

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
#include "PidlManager.h"

CPidlManager::CPidlManager()
{
}

CPidlManager::~CPidlManager()
{
}

/*------------------------------------------------------------------------------
 * CPidlManager::Delete
 * Free the PIDL using the Shell PIDL allocator.
 * Apparently, this function Validates that the memory being freed is
 * a PIDL when in a Debug build.
 *----------------------------------------------------------------------------*/
void CPidlManager::Delete( LPITEMIDLIST pidl )
{
    ILFree(pidl);
}

/*------------------------------------------------------------------------------
 * CPidlManager::GetNextItem
 * Returns a pointer to the next item ID in the list passed as pidl.
 * If pidl points to the last *non-terminator* SHITEMID the terminator is 
 * returned.  If pidl points to the terminator already or is NULL the function
 * returns NULL.  This is not made clear in the MSDN ILGetNext documentation.
 *----------------------------------------------------------------------------*/
LPITEMIDLIST CPidlManager::GetNextItem( LPCITEMIDLIST pidl )
{
    return ILGetNext(pidl);
}

/*------------------------------------------------------------------------------
 * CPidlManager::GetLastItem
 * Returns a pointer to the last *non-terminator* item ID in the list pidl.
 * This is not made clear in the MSDN ILGetNext documentation.  It is also
 * unclear what happens of the pidl were to be the terminator or NULL.
 *----------------------------------------------------------------------------*/
LPITEMIDLIST CPidlManager::GetLastItem( LPCITEMIDLIST pidl )
{
	ATLENSURE(pidl);
	ATLENSURE(pidl->mkid.cb); // pidl is not the terminator

    return ILFindLastID(pidl);
}

/*------------------------------------------------------------------------------
 * CPidlManager::GetSize
 * The total size of the passed in pidl in bytes including the zero terminator.
 *----------------------------------------------------------------------------*/
UINT CPidlManager::GetSize( LPCITEMIDLIST pidl )
{
	return ILGetSize(pidl);
}

/*------------------------------------------------------------------------------
 * CPidlManager::CopyWSZString
 * Copies a WString into provided buffer and performs checking.
 * Length in BYTEs of return buffer is given as cchDest.
 *----------------------------------------------------------------------------*/
HRESULT CPidlManager::CopyWSZString( __out_ecount(cchDest) PWSTR pwszDest,
									 __in USHORT cchDest,
									 __in PCWSTR pwszSrc)
{
	// Neither source nor destination of StringCbCopyW can be NULL
	ATLASSERT(pwszSrc != NULL && pwszDest != NULL);

	HRESULT hr = StringCchCopyW(pwszDest, cchDest, pwszSrc);

	ATLASSERT(SUCCEEDED(hr));
	return hr;
}

/*------------------------------------------------------------------------------
 * CPidlManager::GetDataSegment
 * Walk to last item in PIDL (if multilevel) and returns item as a HOSTPIDL.
 *----------------------------------------------------------------------------*/
PITEMID_CHILD CPidlManager::GetDataSegment( LPCITEMIDLIST pidl )
{
    // Get the last item of the PIDL to make sure we get the right value
    // in case of multiple nesting levels
	return (PITEMID_CHILD)GetLastItem( pidl );
}

/*------------------------------------------------------------------------------
 * CPidlManager::Copy
 * Duplicate a PIDL.
 *----------------------------------------------------------------------------*/
LPITEMIDLIST CPidlManager::Copy( LPCITEMIDLIST pidlSrc )
{
	LPITEMIDLIST pidlTarget = ILClone( pidlSrc );
	
	ATLASSERT(GetSize(pidlSrc) == GetSize(pidlTarget));
	ATLASSERT(!memcmp(pidlSrc, pidlTarget, GetSize(pidlSrc)));

	return pidlTarget;
}
