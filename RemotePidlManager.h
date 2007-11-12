/*  Declaration of PIDL for remote system directory listing and manager class

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

#ifndef REMOTEPIDLMANAGER_H
#define REMOTEPIDLMANAGER_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <string.h>
#include "remotelimits.h"
#include "PidlManager.h"

// PIDL for storing connection data object details
static const DWORD REMOTEPIDL_FINGERPRINT = 0x533aaf69;
#pragma pack(1)
struct REMOTEPIDL
{
	USHORT cb;
	DWORD dwFingerprint;
	BOOL fIsFolder;
	WCHAR wszPath[MAX_PATH_LENZ];
	WCHAR wszOwner[MAX_USERNAME_LENZ];
	WCHAR wszGroup[MAX_USERNAME_LENZ];
	DWORD dwPermissions;
	WORD wPadding;
	ULONGLONG uSize;
	time_t dtModified;
};
#pragma pack()
typedef UNALIGNED REMOTEPIDL *PREMOTEPIDL;
typedef const UNALIGNED REMOTEPIDL *PCREMOTEPIDL;

class CRemotePidlManager : public CPidlManager
{
public:
	HRESULT Create( LPCWSTR pwszPath, LPCWSTR pwszOwner, LPCWSTR pwszGroup,
					DWORD dwPermissions, ULONGLONG uSize, time_t dtModified,
					BOOL fIsFolder, PITEMID_CHILD *ppidlOut );

	PREMOTEPIDL Validate( PCIDLIST_RELATIVE );
	HRESULT IsValid( PCIDLIST_RELATIVE );

	// All accessors take a LPCITEMIDLIST as they may be multilevel
	// where only the last SHITEMID is of specific type
	CString GetPath( LPCITEMIDLIST pidl );
	CString GetOwner( LPCITEMIDLIST pidl );
	CString GetGroup( LPCITEMIDLIST pidl );
	DWORD GetPermissions( LPCITEMIDLIST pidl );
	CString GetPermissionsStr( LPCITEMIDLIST pidl );
	ULONGLONG GetFileSize( LPCITEMIDLIST pidl );
	CTime GetLastModified( LPCITEMIDLIST pidl );
	BOOL IsFolder( LPCITEMIDLIST pidl );
private:
	PREMOTEPIDL GetDataSegment( LPCITEMIDLIST pidl );
};

#endif // REMOTEPIDLMANAGER_H
