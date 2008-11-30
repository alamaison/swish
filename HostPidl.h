/*  PIDL wrapper class with accessors for host folder PIDL fields.

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
#include "remotelimits.h"

#include <pshpack1.h>
/** 
 * Duplicate of HOSTPIDL defined in HostPidlManager.h.
 * These must be kept in sync.  Eventually this file will replace 
 * HostPidlManager.h/cpp
 */
struct HostItemId
{
	USHORT cb;
	DWORD dwFingerprint;
	WCHAR wszLabel[MAX_LABEL_LENZ];
	WCHAR wszUser[MAX_USERNAME_LENZ];
	WCHAR wszHost[MAX_HOSTNAME_LENZ];
	WCHAR wszPath[MAX_PATH_LENZ];
	USHORT uPort;
	
	static const DWORD HOSTITEMID_FINGERPRINT = 0x496c1066;
};
#include <poppack.h>

template <typename IdListType>
class CHostPidl : public CPidl<IdListType>
{
public:
	CHostPidl() : CPidl() {}
	CHostPidl( __in_opt const IdListType *pidl ) throw(...) : CPidl(pidl) {}
	CHostPidl( __in const CHostPidl& pidl ) throw(...) : CPidl(pidl) {}
	CHostPidl& operator=( __in const CHostPidl& pidl ) throw(...)
	{
		if (this != &pidl)
			CPidl::operator=(pidl);

		return *this;
	}

	class InvalidPidlException {};

	bool IsValid() const
	{
		const HostItemId *pHostId = Get();

		return (
			pHostId->cb == sizeof(HostItemId) && 
			pHostId->dwFingerprint == HostItemId::HOSTITEMID_FINGERPRINT
		);
	}

	CString GetLabel() const throw(...)
	{
		if (!IsValid())
			throw InvalidPidlException();

		return Get()->wszLabel;
	}

	CString GetUser() const throw(...)
	{
		if (!IsValid())
			throw InvalidPidlException();

		return Get()->wszUser;
	}

	CString GetHost() const throw(...)
	{
		if (!IsValid())
			throw InvalidPidlException();

		return Get()->wszHost;
	}

	CString GetPath() const throw(...)
	{
		if (!IsValid())
			throw InvalidPidlException();

		return Get()->wszPath;
	}

	USHORT GetPort() const throw(...)
	{
		if (!IsValid())
			throw InvalidPidlException();

		return Get()->uPort;
	}

	const HostItemId *Get() const
	{
		return reinterpret_cast<HostItemId *>(m_pidl);
	}

	/**
	 * Search this (multi-level) PIDL to find the HostItemId section.
	 *
	 * In any PIDL there should be at most one HostItemId as it doesn't make 
	 * sense for a file to be under more than one host.
	 *
	 * @returns The address of the HostItemId segment this wrapped PIDL.
	 */
	__checkReturn PCUIDLIST_RELATIVE FindHostPidl()
	{
		PCUIDLIST_RELATIVE pidlCurrent = m_pidl;
		
		// Search along pidl until we find one that matched our fingerprint or
		// we run off the end
		while (pidlCurrent != NULL)
		{
			CHostPidl pidl(pidlCurrent);
			if (pidl.IsValid())
				return pidlCurrent;

			pidlCurrent = ::ILGetNext(pidlCurrent);
		}

		return NULL;
	}
};

typedef CHostPidl<ITEMID_CHILD> CHostChildPidl;
typedef CHostPidl<ITEMIDLIST_RELATIVE> CHostRelativePidl;
