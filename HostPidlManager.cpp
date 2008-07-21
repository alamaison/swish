/*  Manage creation and manipulation of PIDLs representing SFTP connections.

    Copyright (C) 2007, 2008  Alexander Lamaison <awl03@doc.ic.ac.uk>

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
#include "HostPidlManager.h"

/*------------------------------------------------------------------------------
 * CHostPidlManager::Create
 * Create a new terminated PIDL using the passed-in connection information
 *----------------------------------------------------------------------------*/
HRESULT CHostPidlManager::Create(__in LPCWSTR pwszLabel, __in LPCWSTR pwszUser, 
								 __in LPCWSTR pwszHost, __in LPCWSTR pwszPath, 
								 __in USHORT uPort, 
								 __deref_out PITEMID_CHILD *ppidlOut )
{
	ATLASSERT(sizeof(HOSTPIDL) % sizeof(DWORD) == 0); // DWORD-aligned

	PITEMID_CHILD pidl = NULL;

	// Allocate enough memory to hold a HOSTPIDL structure plus terminator
	static SIZE_T uTerminatedSize = sizeof(HOSTPIDL) + sizeof(USHORT);
    pidl = (PITEMID_CHILD)CoTaskMemAlloc( uTerminatedSize );
    if(!pidl)
		return E_OUTOFMEMORY;
	ZeroMemory(pidl, uTerminatedSize);

	// Use first PIDL member as a HOSTPIDL structure
	PHOSTPIDL pidlHost = (PHOSTPIDL)pidl;

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
	ATLASSERT(ILGetSize(*ppidlOut) == uTerminatedSize);

    return S_OK;
}

/*------------------------------------------------------------------------------
 * CHostPidlManager::Validate
 * Validate the fingerprint stored in the PIDL.
 * If fingerprint matches HOSTPIDL return true/PIDL else return false/NULL
 *----------------------------------------------------------------------------*/
PHOSTPIDL CHostPidlManager::Validate( LPCITEMIDLIST pidl )
{
	PHOSTPIDL pHostPidl = NULL;

    if (pidl)
    {
        pHostPidl = (PHOSTPIDL)pidl;
		if (pHostPidl->cb && pHostPidl->dwFingerprint == HOSTPIDL_FINGERPRINT)
			return pHostPidl; // equal to true
	}

    return NULL; // equal to false
}

/**
 * Check if the fingerprint stored in the PIDL corresponds to a HOSTPIDL.
 * 
 * @param pidl PIDL to be validated
 * 
 * @returns Whether @a pidl is a valid HOSTPIDL
 *     @retval S_OK if @a pidl is of type HOSTPIDL
 *     @retval E_INVALIDARG otherwise
 * 
 * This function is very similar to Validate() except that a standard
 * COM success code is returned rather than a PIDL or a boolean.
 * 
 * @see Validate() | HOSTPIDL
 */
HRESULT CHostPidlManager::IsValid(
	PCUIDLIST_RELATIVE pidl, PM_VALIDMODE mode )
{
	PHOSTPIDL pHostPidl;

	if (mode == PM_LAST_PIDL)
		pHostPidl = Validate(GetLastItem(pidl));
	else
		pHostPidl = Validate(pidl);

    return pHostPidl ? S_OK : E_INVALIDARG;
}

/**
 * Search a multi-level PIDL to find the HOSTPIDL section.
 *
 * In any PIDL there should only be one HOSTPIDL as it doesn't make sense for
 * a file to be under more than one host.
 *
 * @returns The address of the HOSTPIDL segment of the original PIDL, @p pidl.
 */
PCUIDLIST_RELATIVE CHostPidlManager::FindHostPidl( PCIDLIST_ABSOLUTE pidl )
{
	PCUIDLIST_RELATIVE pidlCurrent = pidl;
	PHOSTPIDL pHostPidl = NULL;

	// Search along pidl until we find one that matched our fingerprint or
	// we run off the end
	while (pidlCurrent != NULL)
	{
		pHostPidl = (PHOSTPIDL)pidlCurrent;
		if (pHostPidl->cb && pHostPidl->dwFingerprint == HOSTPIDL_FINGERPRINT)
			break;

		pidlCurrent = ::ILGetNext(pidlCurrent);
	}

	return pidlCurrent;
}


/**
 * Returns the connection's friendly display name from the HOSTPIDL.
 */
CString CHostPidlManager::GetLabel( PCUIDLIST_RELATIVE pidl )
{
	if (pidl == NULL) return _T("");

	return GetDataSegment(pidl)->wszLabel;
}

/**
 * Returns the username from the HOSTPIDL.
 */
CString CHostPidlManager::GetUser( PCUIDLIST_RELATIVE pidl )
{
	if (pidl == NULL) return _T("");

	return GetDataSegment(pidl)->wszUser;
}

/**
 * Returns the hostname from the HOSTPIDL.
 */
CString CHostPidlManager::GetHost( PCUIDLIST_RELATIVE pidl )
{
	if (pidl == NULL) return _T("");

	return GetDataSegment(pidl)->wszHost;
}

/**
 * Returns the remote directory path from the HOSTPIDL.
 */
CString CHostPidlManager::GetPath( PCUIDLIST_RELATIVE pidl )
{
	if (pidl == NULL) return _T("");

	return GetDataSegment(pidl)->wszPath;
}

/**
 * Returns the SFTP port number from the HOSTPIDL.
 */
USHORT CHostPidlManager::GetPort( PCUIDLIST_RELATIVE pidl )
{
	if (pidl == NULL) return 0;

	return GetDataSegment(pidl)->uPort;
}

/**
 * Returns the SFTP port number from the HOSTPIDL as a CString.
 */
CString CHostPidlManager::GetPortStr( PCUIDLIST_RELATIVE pidl )
{
	if (pidl == NULL) return _T("");

	CString strPort;
	strPort.Format(_T("%u"), GetDataSegment(pidl)->uPort);
	return strPort;
}

/**
 * Returns the relative PIDL as an HOSTPIDL.
 * 
 * @param pidl The PIDL to retrieve the HOSTPIDL data segment from.
 * 
 * @returns The @a pidl (as a HOSTPIDL) if it is a valid HOSTPIDL or
 *          @c NULL otherwise.
 * 
 * @pre Assumes that the pidl is a HOSTPIDL.
 */
PHOSTPIDL CHostPidlManager::GetDataSegment( PCUIDLIST_RELATIVE pidl )
{
	ATLASSERT(SUCCEEDED(IsValid( pidl )));
	// If not, this is unexpected behviour. Why were we passed this PIDL?

	return Validate( pidl );
}
