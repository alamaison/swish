/*  Declaration of the PIDL types and PIDL manager class

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

#ifndef PIDLMANAGER_H
#define PIDLMANAGER_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <string.h>
#include "remotelimits.h"

// PIDL for storing connection data object details
#define HOSTPIDL_FINGERPRINT 0x496c1066
#pragma pack(1)
struct HOSTPIDL
{
	USHORT cb;
	DWORD dwFingerprint;
	WCHAR wszLabel[MAX_LABEL_LENZ];
	WCHAR wszUser[MAX_USERNAME_LENZ];
	WCHAR wszHost[MAX_HOSTNAME_LENZ];
	WCHAR wszPath[MAX_PATH_LENZ];
	USHORT uPort;
};
#pragma pack()
typedef UNALIGNED HOSTPIDL *LPHOSTPIDL;
typedef const UNALIGNED HOSTPIDL *LPCHOSTPIDL;

// Class that creates/destroyes PIDLs and gets data from PIDLs.
class CPidlManager  
{
public:
	CPidlManager();
	~CPidlManager();

	HRESULT Create( LPCWSTR pwszLabel, LPCWSTR pwszUser, LPCWSTR pwszHost,
					LPCWSTR pwszPath, USHORT uPort, PITEMID_CHILD *ppidlOut );

   LPITEMIDLIST Copy ( LPCITEMIDLIST );
   void Delete ( LPITEMIDLIST );
   UINT GetSize ( LPCITEMIDLIST );

   LPITEMIDLIST GetNextItem( LPCITEMIDLIST );
   LPITEMIDLIST GetLastItem( LPCITEMIDLIST );

   LPHOSTPIDL Validate ( LPCITEMIDLIST );
   HRESULT IsValid ( LPCITEMIDLIST );

   // All accessors take a LPCITEMIDLIST as they may be multilevel
   // where only the last SHITEMID is of specific type
   CString GetLabel( LPCITEMIDLIST pidl );
   CString GetUser( LPCITEMIDLIST pidl );
   CString GetHost( LPCITEMIDLIST pidl );
   CString GetPath( LPCITEMIDLIST pidl );
   USHORT GetPort( LPCITEMIDLIST pidl );

private:
	LPHOSTPIDL GetData ( LPCITEMIDLIST pidl );
	HRESULT CopyWSZString( PWSTR pwszDest, USHORT cchDest, PCWSTR pwszSrc );
};

#endif // PIDLMANAGER_H
