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
	
	static const DWORD FINGERPRINT = 0x496c1066;
};
#include <poppack.h>

/**
 * PIDL class augmenting CPidl with methods specific to HostItemIds.
 *
 * The methods added by this class are common to both const PIDLs (PIDL 
 * handles) and non-const PIDLs.  This class derives from an implementation
 * of either type of PIDL (provided by CPidl<> or CXPidlHandle) passed as
 * the template argument.
 *
 * @param PidlT  Type (const on non-const) of PIDL to use as base.  Either
 *               CPidl or one of the CXPidlHandle types.
 */
template <typename PidlT>
class CHostPidlBase : public PidlT
{
protected:
	typedef typename PidlT::PidlType PidlType;
	typedef typename PidlT::ConstPidlType ConstPidlType;
public:
	CHostPidlBase() throw() {}
	CHostPidlBase( __in_opt ConstPidlType pidl ) throw(...) : PidlT(pidl) {}
	CHostPidlBase( __in const CHostPidlBase& pidl ) throw(...) : PidlT(pidl) {}
	CHostPidlBase& operator=( __in const CHostPidlBase& pidl ) throw(...)
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
			pHostId->dwFingerprint == HostItemId::FINGERPRINT
		);
	}

	static bool IsValid(ConstPidlType pidl)
	{
		CHostPidlBase<PidlT> hpidl(pidl);

		return hpidl.IsValid();
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

	inline const HostItemId *Get() const
	{
		return reinterpret_cast<const HostItemId *>(m_pidl);
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
		CHostItemListHandle pidlCurrent(m_pidl);
		
		// Search along pidl until we find one that matched our fingerprint or
		// we run off the end
		while (!pidlCurrent.IsEmpty())
		{
			if (pidlCurrent.IsValid())
				return pidlCurrent;

			pidlCurrent = pidlCurrent.GetNext();
		}

		return NULL;
	}
};


/**
 * Unmanaged-lifetime child PIDL for read-only HostItemId operations.
 */
typedef CHostPidlBase<CChildPidlHandle> CHostItemHandle;

/**
 * Unmanaged-lifetime relative PIDL for read-only HostItemId operations.
 */
typedef CHostPidlBase<CRelativePidlHandle> CHostItemListHandle;

/**
 * Managed-lifetime PIDL for HostItemId operations.
 */
template <typename IdListType>
class CHostPidl : public CHostPidlBase< CPidl<IdListType> >
{
public:
	CHostPidl(
		PCWSTR pwszUser, PCWSTR pwszHost, USHORT uPort=SFTP_DEFAULT_PORT,
		PCWSTR pwszPath=L"", PCWSTR pwszLabel=L"")
	throw(...)
	{
		ATLASSERT(sizeof(HostItemId) % sizeof(DWORD) == 0); // DWORD-aligned

		// Allocate enough memory to hold HostItemId structure & terminator
		static size_t cbItem = sizeof HostItemId + sizeof USHORT;
		HostItemId *item = static_cast<HostItemId *>(::CoTaskMemAlloc(cbItem));
		if(item == NULL)
			AtlThrow(E_OUTOFMEMORY);
		::ZeroMemory(item, cbItem);

		// Fill members of the PIDL with data
		item->cb = sizeof HostItemId;
		item->dwFingerprint = HostItemId::FINGERPRINT; // Sign with fingerprint
		CopyWSZString(item->wszUser, ARRAYSIZE(item->wszUser), pwszUser);
		CopyWSZString(item->wszHost, ARRAYSIZE(item->wszHost), pwszHost);
		item->uPort = uPort;
		CopyWSZString(item->wszPath, ARRAYSIZE(item->wszPath), pwszPath);
		CopyWSZString(item->wszLabel, ARRAYSIZE(item->wszLabel), pwszLabel);

		m_pidl = reinterpret_cast<PidlType>(item);
		ATLASSERT(IsValid());
		ATLASSERT(GetNext() == NULL); // PIDL is terminated
	}

	CHostPidl() throw() {}
	CHostPidl( __in_opt ConstPidlType pidl ) throw(...) :
		CHostPidlBase(pidl) {}
	CHostPidl( __in const CHostPidl& pidl ) throw(...) : CHostPidlBase(pidl) {}
	CHostPidl& operator=( __in const CHostPidl& pidl ) throw(...)
	{
		if (this != &pidl)
			CHostPidl::operator=(pidl);
		return *this;
	}

private:

	/**
	 * Copy a wide string into provided buffer.
	 *
	 * @param[out] pwszDest  Destination wide-char string buffer.
	 * @param[in]  cchDest   Length of destination buffer in @b wide-chars.
	 * @param[in]  pwszSrc   Source string.  Must be NULL-terminated.
	 *
	 * @throws CAtlException if an error occurs while copying.
	 */
	static void CopyWSZString(
		__out_ecount(cchDest) PWSTR pwszDest, __in size_t cchDest,
		__in PCWSTR pwszSrc)
	throw(...)
	{
		// Neither source nor destination of StringCbCopyW can be NULL
		ATLASSERT(pwszSrc && pwszDest);

		HRESULT hr = StringCchCopyW(pwszDest, cchDest, pwszSrc);
		ATLENSURE_SUCCEEDED(hr);
	}
};

/**
 * Managed-lifetime child PIDL for HostItemId operations.
 */
typedef CHostPidl<ITEMID_CHILD> CHostItem;

/**
 * Managed-lifetime relative PIDL for HostItemId operations.
 */
typedef CHostPidl<ITEMIDLIST_RELATIVE> CHostItemList;