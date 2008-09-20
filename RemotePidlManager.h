/*  Declaration of PIDL for remote system directory listing and manager class

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
	bool fIsFolder;
	bool fIsLink;
	WCHAR wszFilename[MAX_PATH_LENZ];
	WCHAR wszOwner[MAX_USERNAME_LENZ];
	WCHAR wszGroup[MAX_USERNAME_LENZ];
	DWORD dwPermissions;
	//WORD wPadding;
	ULONGLONG uSize;
	DATE dateModified;
};
#pragma pack()
typedef UNALIGNED REMOTEPIDL *PREMOTEPIDL;
typedef const UNALIGNED REMOTEPIDL *PCREMOTEPIDL;

class CRemotePidlManager : public CPidlManager
{
public:
	HRESULT Create( LPCWSTR pwszPath, LPCWSTR pwszOwner, LPCWSTR pwszGroup,
					DWORD dwPermissions, ULONGLONG uSize, DATE dateModified,
					bool fIsFolder, PITEMID_CHILD *ppidlOut );

	PREMOTEPIDL Validate( PCUIDLIST_RELATIVE );
	HRESULT IsValid(
	   PCUIDLIST_RELATIVE, PM_VALIDMODE mode = PM_THIS_PIDL);

	// All accessors take a PCUIDLIST_RELATIVE as they may be multilevel
	// where only the last SHITEMID is of specific type
	CString GetFilename( PCUIDLIST_RELATIVE pidl );
	CString GetOwner( PCUIDLIST_RELATIVE pidl );
	CString GetGroup( PCUIDLIST_RELATIVE pidl );
	DWORD GetPermissions( PCUIDLIST_RELATIVE pidl );
	CString GetPermissionsStr( PCUIDLIST_RELATIVE pidl );
	ULONGLONG GetFileSize( PCUIDLIST_RELATIVE pidl );
	DATE GetLastModified( PCUIDLIST_RELATIVE pidl );
	bool IsFolder( PCUIDLIST_RELATIVE pidl );
private:
	PREMOTEPIDL GetDataSegment( PCUIDLIST_RELATIVE pidl );
};

#endif // REMOTEPIDLMANAGER_H
