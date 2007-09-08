/*  Manage the creation and manipulatation of PIDLs representing connections

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
 * CPidlManager::Create
 * Create a new terminated PIDL using the passed-in connection information
 *----------------------------------------------------------------------------*/
HRESULT CPidlManager::Create(__in LPCWSTR pwszLabel, __in LPCWSTR pwszUser, 
							 __in LPCWSTR pwszHost, __in LPCWSTR pwszPath, 
							 __in USHORT uPort, 
							 __deref_out PITEMID_CHILD *ppidlOut )
{
	ATLASSERT(sizeof(HOSTPIDL) % sizeof(DWORD) == 0); // DWORD-aligned

	PITEMID_CHILD pidl = NULL;

	// Allocate enough memory to hold a HOSTPIDL structure plus terminator
    pidl = (PITEMID_CHILD)CoTaskMemAlloc(sizeof(HOSTPIDL) + sizeof(USHORT));
    if(!pidl)
		return E_OUTOFMEMORY;

	// Use first PIDL member as a HOSTPIDL structure
	LPHOSTPIDL pidlHost = (LPHOSTPIDL)pidl;

	// Fill members of the PIDL with data
	pidlHost->cb = sizeof HOSTPIDL;
	pidlHost->dwFingerprint = HOSTPIDL_FINGERPRINT; // Sign with fingerprint
	CopyWSZString(pidlHost->wszLabel, ARRAYSIZE(pidlHost->wszLabel), pwszLabel);
	CopyWSZString(pidlHost->wszUser, ARRAYSIZE(pidlHost->wszUser), pwszUser);
	CopyWSZString(pidlHost->wszHost, ARRAYSIZE(pidlHost->wszHost), pwszHost);
	CopyWSZString(pidlHost->wszPath, ARRAYSIZE(pidlHost->wszPath), pwszPath);
	pidlHost->uPort = uPort;

	// Create the terminating null PIDL by setting cb field to 0.
	LPITEMIDLIST pidlNext = GetNextItem(pidl);
	ATLASSERT(pidlNext != NULL);
	pidlNext->mkid.cb = 0;

	*ppidlOut = pidl;
	ATLASSERT(SUCCEEDED(IsValid(*ppidlOut)));
	ATLASSERT(ILGetNext(ILGetNext(*ppidlOut)) == NULL); // PIDL is terminated
	ATLASSERT(ILGetSize(*ppidlOut) == sizeof(HOSTPIDL) + sizeof(USHORT));

    return S_OK;
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
 * CPidlManager::Validate
 * Validate the fingerprint stored in the PIDL.
 * If fingerprint matches HOSTPIDL return true/PIDL else return false/NULL
 *----------------------------------------------------------------------------*/
LPHOSTPIDL CPidlManager::Validate( LPCITEMIDLIST pidl )
{
	LPHOSTPIDL pHostPidl = NULL;

    if (pidl)
    {
        pHostPidl = (LPHOSTPIDL)pidl;
		if (pHostPidl->cb && pHostPidl->dwFingerprint == HOSTPIDL_FINGERPRINT)
			return pHostPidl; // equal to true
	}

    return NULL; // equal to false
}

/*------------------------------------------------------------------------------
 * CPidlManager::IsValid
 * Check if the fingerprint stored in the PIDL corresponds to a HOSTPIDL.
 *----------------------------------------------------------------------------*/
HRESULT CPidlManager::IsValid( LPCITEMIDLIST pidl )
{
    LPHOSTPIDL pHostPidl = Validate(pidl);
    return pHostPidl ? S_OK : E_INVALIDARG;
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
 * CPidlManager::GetLabel
 * Returns the friendly display name from the (possibly multilevel) PIDL.
 *----------------------------------------------------------------------------*/
CString CPidlManager::GetLabel( LPCITEMIDLIST pidl )
{
	if (pidl == NULL) return _T("");

	return GetData(pidl)->wszLabel;
}

/*------------------------------------------------------------------------------
 * CPidlManager::GetUser
 * Returns the username from the (possibly multilevel) PIDL.
 *----------------------------------------------------------------------------*/
CString CPidlManager::GetUser( LPCITEMIDLIST pidl )
{
	if (pidl == NULL) return _T("");

	return GetData(pidl)->wszUser;
}

/*------------------------------------------------------------------------------
 * CPidlManager::GetHost
 * Returns the hostname from the (possibly multilevel) PIDL.
 *----------------------------------------------------------------------------*/
CString CPidlManager::GetHost( LPCITEMIDLIST pidl )
{
	if (pidl == NULL) return _T("");

	return GetData(pidl)->wszHost;
}

/*------------------------------------------------------------------------------
 * CPidlManager::GetPath
 * Returns the remote directory path from the (possibly multilevel) PIDL.
 *----------------------------------------------------------------------------*/
CString CPidlManager::GetPath( LPCITEMIDLIST pidl )
{
	if (pidl == NULL) return _T("");

	return GetData(pidl)->wszPath;
}

/*------------------------------------------------------------------------------
 * CPidlManager::GetPort
 * Returns the SFTP port number from the (possibly multilevel) PIDL.
 *----------------------------------------------------------------------------*/
USHORT CPidlManager::GetPort( LPCITEMIDLIST pidl )
{
	if (pidl == NULL) return 0;

	return GetData(pidl)->uPort;
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
 * CPidlManager::GetData
 * Walk to last item in PIDL (if multilevel) and returns item as a HOSTPIDL.
 *----------------------------------------------------------------------------*/
LPHOSTPIDL CPidlManager::GetData( LPCITEMIDLIST pidl )
{
    // Get the last item of the PIDL to make sure we get the right value
    // in case of multiple nesting levels
	return (LPHOSTPIDL)GetLastItem( pidl );
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
