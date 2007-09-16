/*  Manage creation/manipulation of PIDLs for files/folder in remote directory

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
#include "RemotePidlManager.h"

/*------------------------------------------------------------------------------
 * CRemotePidlManager::Create
 * Create a new terminated PIDL using the passed-in file information
 *----------------------------------------------------------------------------*/
HRESULT CRemotePidlManager::Create(
	__in LPCWSTR pwszPath, __in LPCWSTR pwszOwner, __in LPCWSTR pwszGroup,
	__in DWORD dwPermissions, __in ULONG uSize, __in ULONG dtModified,
	__in BOOL fIsFolder, __deref_out PITEMID_CHILD *ppidlOut )
{
	ATLASSERT(sizeof(REMOTEPIDL) % sizeof(DWORD) == 0); // DWORD-aligned

	PITEMID_CHILD pidl = NULL;

	// Allocate enough memory to hold a HOSTPIDL structure plus terminator
    pidl = (PITEMID_CHILD)CoTaskMemAlloc(sizeof(REMOTEPIDL) + sizeof(USHORT));
    if(!pidl)
		return E_OUTOFMEMORY;

	// Use first PIDL member as a REMOTEPIDL structure
	PREMOTEPIDL pidlRemote = (PREMOTEPIDL)pidl;

	// Fill members of the PIDL with data
	pidlRemote->cb = sizeof REMOTEPIDL;
	pidlRemote->dwFingerprint = REMOTEPIDL_FINGERPRINT; // Sign with fingerprint
	CopyWSZString(pidlRemote->wszPath, 
		ARRAYSIZE(pidlRemote->wszPath), pwszPath);
	CopyWSZString(pidlRemote->wszOwner, 
		ARRAYSIZE(pidlRemote->wszOwner), pwszOwner);
	CopyWSZString(pidlRemote->wszGroup, 
		ARRAYSIZE(pidlRemote->wszGroup), pwszGroup);
	pidlRemote->dwPermissions = dwPermissions;
	pidlRemote->uSize = uSize;
	pidlRemote->dtModified = dtModified;
	pidlRemote->fIsFolder = fIsFolder;

	// Create the terminating null PIDL by setting cb field to 0.
	LPITEMIDLIST pidlNext = GetNextItem(pidl);
	ATLASSERT(pidlNext != NULL);
	pidlNext->mkid.cb = 0;

	*ppidlOut = pidl;
	ATLASSERT(SUCCEEDED(IsValid(*ppidlOut)));
	ATLASSERT(ILGetNext(ILGetNext(*ppidlOut)) == NULL); // PIDL is terminated
	ATLASSERT(ILGetSize(*ppidlOut) == sizeof(REMOTEPIDL) + sizeof(USHORT));

    return S_OK;
}

/*------------------------------------------------------------------------------
 * CRemotePidlManager::Validate
 * Validate the fingerprint stored in the PIDL.
 * If fingerprint matches REMOTEPIDL return true/PIDL else return false/NULL
 *----------------------------------------------------------------------------*/
PREMOTEPIDL CRemotePidlManager::Validate( LPCITEMIDLIST pidl )
{
	PREMOTEPIDL pRemotePidl = NULL;

    if (pidl)
    {
        pRemotePidl = (PREMOTEPIDL)pidl;
		if (pRemotePidl->cb && 
			pRemotePidl->dwFingerprint == REMOTEPIDL_FINGERPRINT)
			return pRemotePidl; // equal to true
	}

    return NULL; // equal to false
}

/*------------------------------------------------------------------------------
 * CRemotePidlManager::IsValid
 * Check if the fingerprint stored in the PIDL corresponds to a REMOTEPIDL.
 *----------------------------------------------------------------------------*/
HRESULT CRemotePidlManager::IsValid( LPCITEMIDLIST pidl )
{
    PREMOTEPIDL pRemotePidl = Validate(pidl);
    return pRemotePidl ? S_OK : E_INVALIDARG;
}

/*------------------------------------------------------------------------------
 * CRemotePidlManager::GetPath
 * Returns the path of the file from the (possibly multilevel) PIDL.
 *----------------------------------------------------------------------------*/
CString CRemotePidlManager::GetPath( LPCITEMIDLIST pidl )
{
	if (pidl == NULL) return _T("");

	return GetDataSegment(pidl)->wszPath;
}

/*------------------------------------------------------------------------------
 * CRemotePidlManager::GetOwner
 * Returns the file owner's username from the (possibly multilevel) PIDL.
 *----------------------------------------------------------------------------*/
CString CRemotePidlManager::GetOwner( LPCITEMIDLIST pidl )
{
	if (pidl == NULL) return _T("");

	return GetDataSegment(pidl)->wszOwner;
}

/*------------------------------------------------------------------------------
 * CRemotePidlManager::GetGroup
 * Returns the name of file's group from the (possibly multilevel) PIDL.
 *----------------------------------------------------------------------------*/
CString CRemotePidlManager::GetGroup( LPCITEMIDLIST pidl )
{
	if (pidl == NULL) return _T("");

	return GetDataSegment(pidl)->wszGroup;
}

/*------------------------------------------------------------------------------
 * CRemotePidlManager::GetPermissions
 * Returns the file permissions as numeric value from the PIDL.
 *----------------------------------------------------------------------------*/
DWORD CRemotePidlManager::GetPermissions( LPCITEMIDLIST pidl )
{
	if (pidl == NULL) return 0;

	return GetDataSegment(pidl)->dwPermissions;
}

/*------------------------------------------------------------------------------
 * CRemotePidlManager::GetPermissionsStr
 * Returns the file permissions in typical drwxrwxrwx form as a CString
 *----------------------------------------------------------------------------*/
CString CRemotePidlManager::GetPermissionsStr( LPCITEMIDLIST pidl )
{
	if (pidl == NULL) return _T("");

	// TODO: call old permissions function from swish prototype
	return _T("todo");
}

/*------------------------------------------------------------------------------
 * CRemotePidlManager::GetSize
 * Returns the file's size in bytes.
 *----------------------------------------------------------------------------*/
DWORD CRemotePidlManager::GetSize( LPCITEMIDLIST pidl )
{
	if (pidl == NULL) return 0;

	return GetDataSegment(pidl)->uSize;
}

/*------------------------------------------------------------------------------
 * CRemotePidlManager::GetLastModified
 * Returns the file permissions as numeric value from the PIDL.
 *----------------------------------------------------------------------------*/
ULONG CRemotePidlManager::GetLastModified( LPCITEMIDLIST pidl )
{
	if (pidl == NULL) return 0;

	return GetDataSegment(pidl)->dtModified;
}

/*------------------------------------------------------------------------------
 * CRemotePidlManager::IsFolder
 * Returns the file is a folder or a regular file.
 *----------------------------------------------------------------------------*/
BOOL CRemotePidlManager::IsFolder( LPCITEMIDLIST pidl )
{
	if (pidl == NULL) return 0;

	return GetDataSegment(pidl)->fIsFolder;
}

/*------------------------------------------------------------------------------
 * CRemotePidlManager::GetDataSegment
 * Walk to last item in PIDL (if multilevel) and returns item as a REMOTEPIDL.
 * If the item was not a valid REMOTEPIDL then we return NULL.
 *----------------------------------------------------------------------------*/
PREMOTEPIDL CRemotePidlManager::GetDataSegment( LPCITEMIDLIST pidl )
{
    // Get the last item of the PIDL to make sure we get the right value
    // in case of multiple nesting levels
	PITEMID_CHILD pidlRemote = CPidlManager::GetDataSegment( pidl );

	ATLASSERT(SUCCEEDED(IsValid( pidlRemote )));
	// If not, this is unexpected behviour. Why were we passed this PIDL?

	return Validate( pidlRemote );
}
