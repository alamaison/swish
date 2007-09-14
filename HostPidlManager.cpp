/*  Manage creation and manipulation of PIDLs representing sftp host connections

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
    pidl = (PITEMID_CHILD)CoTaskMemAlloc(sizeof(HOSTPIDL) + sizeof(USHORT));
    if(!pidl)
		return E_OUTOFMEMORY;

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
	ATLASSERT(ILGetSize(*ppidlOut) == sizeof(HOSTPIDL) + sizeof(USHORT));

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

/*------------------------------------------------------------------------------
 * CHostPidlManager::IsValid
 * Check if the fingerprint stored in the PIDL corresponds to a HOSTPIDL.
 *----------------------------------------------------------------------------*/
HRESULT CHostPidlManager::IsValid( LPCITEMIDLIST pidl )
{
    PHOSTPIDL pHostPidl = Validate(pidl);
    return pHostPidl ? S_OK : E_INVALIDARG;
}

/*------------------------------------------------------------------------------
 * CHostPidlManager::GetLabel
 * Returns the friendly display name from the (possibly multilevel) PIDL.
 *----------------------------------------------------------------------------*/
CString CHostPidlManager::GetLabel( LPCITEMIDLIST pidl )
{
	if (pidl == NULL) return _T("");

	return GetDataSegment(pidl)->wszLabel;
}

/*------------------------------------------------------------------------------
 * CHostPidlManager::GetUser
 * Returns the username from the (possibly multilevel) PIDL.
 *----------------------------------------------------------------------------*/
CString CHostPidlManager::GetUser( LPCITEMIDLIST pidl )
{
	if (pidl == NULL) return _T("");

	return GetDataSegment(pidl)->wszUser;
}

/*------------------------------------------------------------------------------
 * CHostPidlManager::GetHost
 * Returns the hostname from the (possibly multilevel) PIDL.
 *----------------------------------------------------------------------------*/
CString CHostPidlManager::GetHost( LPCITEMIDLIST pidl )
{
	if (pidl == NULL) return _T("");

	return GetDataSegment(pidl)->wszHost;
}

/*------------------------------------------------------------------------------
 * CHostPidlManager::GetPath
 * Returns the remote directory path from the (possibly multilevel) PIDL.
 *----------------------------------------------------------------------------*/
CString CHostPidlManager::GetPath( LPCITEMIDLIST pidl )
{
	if (pidl == NULL) return _T("");

	return GetDataSegment(pidl)->wszPath;
}

/*------------------------------------------------------------------------------
 * CHostPidlManager::GetPort
 * Returns the SFTP port number from the (possibly multilevel) PIDL.
 *----------------------------------------------------------------------------*/
USHORT CHostPidlManager::GetPort( LPCITEMIDLIST pidl )
{
	if (pidl == NULL) return 0;

	return GetDataSegment(pidl)->uPort;
}

/*------------------------------------------------------------------------------
 * CHostPidlManager::GetPortStr
 * Returns the SFTP port number from the (possibly multilevel) PIDL as a CString
 *----------------------------------------------------------------------------*/
CString CHostPidlManager::GetPortStr( LPCITEMIDLIST pidl )
{
	if (pidl == NULL) return _T("");

	CString strPort;
	strPort.Format(_T("%u"), GetDataSegment(pidl)->uPort);
	return strPort;
}

/*------------------------------------------------------------------------------
 * CHostPidlManager::GetDataSegment
 * Walk to last item in PIDL (if multilevel) and returns item as a HOSTPIDL.
 * If the item was not a valid HOSTPIDL then we return NULL.
 *----------------------------------------------------------------------------*/
PHOSTPIDL CHostPidlManager::GetDataSegment( LPCITEMIDLIST pidl )
{
    // Get the last item of the PIDL to make sure we get the right value
    // in case of multiple nesting levels
	PITEMID_CHILD pidlHost = CPidlManager::GetDataSegment( pidl );

	ATLASSERT(SUCCEEDED(IsValid( pidlHost )));
	// If not, this is unexpected behviour. Why were we passed this PIDL?

	return Validate( pidlHost );
}
