/*  PIDL wrapper class with accessors for remote folder PIDL fields.

    Copyright (C) 2008  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#pragma once

#include "Pidl.h"
#include "RemotePidlManager.h"

#include <ATLComTime.h>

#include <pshpack1.h>
/** 
 * Duplicate of REMOTEPIDL defined in RemotePidlManager.cpp.
 * These must be kept in sync.  Eventually this file will replace 
 * RemotePidlManager.cpp
 */
struct RemoteItemId
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

	static const DWORD REMOTEITEMID_FINGERPRINT = 0x533aaf69;
};
#include <poppack.h>


template <typename IdListType>
class CRemotePidl : public CPidl<IdListType>
{
public:
	CRemotePidl() : CPidl() {}
	CRemotePidl( __in_opt const IdListType *pidl ) throw(...) : CPidl(pidl) {}
	CRemotePidl( __in const CRemotePidl& pidl ) throw(...) : CPidl(pidl) {}
	CRemotePidl& operator=( __in const CRemotePidl& pidl ) throw(...)
	{
		return CPidl(pidl);
	}

	class InvalidPidlException {};

	bool IsValid() const
	{
		const RemoteItemId *pRemoteId = Get();

		return (
			pRemoteId->cb == sizeof(RemoteItemId) && 
			pRemoteId->dwFingerprint == RemoteItemId::REMOTEITEMID_FINGERPRINT
		);
	}

	bool IsFolder() const throw(...)
	{
		if (!IsValid())
			throw InvalidPidlException();

		return Get()->fIsFolder;
	}

	bool IsLink() const throw(...)
	{
		if (!IsValid())
			throw InvalidPidlException();

		return Get()->fIsLink;
	}

	CString GetFilename() const throw(...)
	{
		if (m_pidl == NULL) return L"";
		if (!IsValid()) throw InvalidPidlException();

		CString strName = Get()->wszFilename;
		ATLASSERT(strName.GetLength() <= MAX_PATH_LEN);

		return strName;
	}

	CString GetOwner() const throw(...)
	{
		if (!IsValid())
			throw InvalidPidlException();

		return Get()->wszOwner;
	}

	CString GetGroup() const throw(...)
	{
		if (!IsValid())
			throw InvalidPidlException();

		return Get()->wszGroup;
	}

	ULONGLONG GetFileSize() const throw(...)
	{
		if (!IsValid())
			throw InvalidPidlException();

		return Get()->uSize;
	}

	COleDateTime GetDateModified() const throw(...)
	{
		if (!IsValid())
			throw InvalidPidlException();

		return Get()->dateModified;
	}

	const RemoteItemId *Get() const
	{
		return reinterpret_cast<RemoteItemId *>(m_pidl);
	}
};

typedef CRemotePidl<ITEMID_CHILD> CRemoteChildPidl;
typedef CRemotePidl<ITEMIDLIST_RELATIVE> CRemoteRelativePidl;
