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
#include "RemotePidl.h" // For GetFullPath() implementation
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
			PidlT::operator=(pidl);
		return *this;
	}

	/**
	 * Concatenation constructor only implemented for non-const PidlT.
	 */
	explicit CHostPidlBase(
		__in_opt ConstPidlType pidl1, __in_opt PCUIDLIST_RELATIVE pidl2 )
	throw(...);

	/**
	 * Does fingerprint stored in this PIDL correspond to a HostItemId?
	 */
	inline bool IsValid() const
	{
		return (
			!IsEmpty() &&
			Get()->cb == sizeof(HostItemId) && 
			Get()->dwFingerprint == HostItemId::FINGERPRINT
		);
	}

	/**
	 * Does fingerprint stored in @p pidl correspond to a HostItemId?
	 */
	static bool IsValid(ConstPidlType pidl)
	{
		CHostPidlBase<PidlT> hpidl(pidl);

		return hpidl.IsValid();
	}

	CString GetLabel() const throw(...)
	{
		ATLENSURE_THROW(IsValid(), E_UNEXPECTED);
		return Get()->wszLabel;
	}

	CString GetUser() const throw(...)
	{
		ATLENSURE_THROW(IsValid(), E_UNEXPECTED);
		return Get()->wszUser;
	}

	CString GetHost() const throw(...)
	{
		ATLENSURE_THROW(IsValid(), E_UNEXPECTED);
		return Get()->wszHost;
	}

	CString GetPath() const throw(...)
	{
		ATLENSURE_THROW(IsValid(), E_UNEXPECTED);
		return Get()->wszPath;
	}

	USHORT GetPort() const throw(...)
	{
		ATLENSURE_THROW(IsValid(), E_UNEXPECTED);
		return Get()->uPort;
	}

	CString GetPortStr() const throw(...)
	{
		ATLENSURE_THROW(IsValid(), E_UNEXPECTED);

		CString strPort;
		strPort.Format(L"%u", Get()->uPort);
		return strPort;
	}
	
	/**
	 * Retrieve the long name of the host connection from the PIDL.
	 *
	 * The long name is either the canonical form if fCanonical is set:
	 *     sftp://username\@hostname:port/path
	 * or, if not set and if the port is the default port, the reduced form:
	 *     sftp://username\@hostname/path
	 */
	CString GetLongName(bool fCanonical) const throw(...)
	{
		ATLENSURE_THROW(IsValid(), E_UNEXPECTED);

		CString strName;

		// Construct string from info in PIDL
		strName.Format(
			L"sftp://%ws@%ws", Get()->wszUser, Get()->wszHost);

		if (fCanonical || (Get()->uPort != SFTP_DEFAULT_PORT))
			strName.AppendFormat(L":%u", Get()->uPort);

		strName.AppendFormat(L"/%ws", Get()->wszPath);

		ATLASSERT(strName.GetLength() <= MAX_CANONICAL_LEN);
		return strName;
	}

	/**
	 * Return the absolute path made by the items in this PIDL.
	 * e.g.
	 * - A child PIDL returns:     "/path"
	 * - A relative PIDL returns:  "/path/dir2/dir2/dir3/filename.ext"
	 * - An absolute PIDL returns: "/path/dir2/dir2/dir3/filename.ext"
	 *
	 * This method is in contrast to GetPath() which just returns the
	 * path information for the current HostItemId.
	 */
	CString GetFullPath() const throw(...)
	{
		CHostItemListHandle pidlHost = FindHostPidl();
		ATLENSURE_THROW(pidlHost, E_UNEXPECTED);

		CString strPath = pidlHost.GetPath();
		CRemoteItemListHandle pidlNext = pidlHost.GetNext();

		if (pidlNext.IsValid())
		{
			(strPath == L"/") || (strPath += L"/");
			strPath += pidlNext.GetFilePath();
		}

		ATLASSERT( strPath.GetLength() <= MAX_PATH_LEN );
		return strPath;
	}

	inline const HostItemId *Get() const throw()
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
	__checkReturn CHostPidlBase<CRelativePidlHandle> FindHostPidl()
	const throw()
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
 * Concatenation constructor only implemented for non-const base type.
 * Also, the only non-const bases that make sense for concatentation are
 * those derived from absolute and relative PIDLs.
 */
template <>
inline CHostPidlBase< CPidl<ITEMIDLIST_ABSOLUTE> >::CHostPidlBase(
	__in_opt CPidl<ITEMIDLIST_ABSOLUTE>::ConstPidlType pidl1,
	__in_opt PCUIDLIST_RELATIVE pidl2 )
	throw(...) : CPidl<ITEMIDLIST_ABSOLUTE>(pidl1, pidl2) {}

template <>
inline CHostPidlBase< CPidl<ITEMIDLIST_RELATIVE> >::CHostPidlBase(
	__in_opt CPidl<ITEMIDLIST_RELATIVE>::ConstPidlType pidl1,
	__in_opt PCUIDLIST_RELATIVE pidl2 )
	throw(...) : CPidl<ITEMIDLIST_RELATIVE>(pidl1, pidl2) {}

/**
 * Unmanaged-lifetime child PIDL for read-only HostItemId operations.
 */
typedef CHostPidlBase<CChildPidlHandle> CHostItemHandle;

/**
 * Unmanaged-lifetime relative PIDL for read-only HostItemId operations.
 */
typedef CHostPidlBase<CRelativePidlHandle> CHostItemListHandle;

/**
 * Unmanaged-lifetime relative PIDL for read-only HostItemId operations.
 */
typedef CHostPidlBase<CAbsolutePidlHandle> CHostItemAbsoluteHandle;

/**
 * Managed-lifetime PIDL for HostItemId operations.
 */
template <typename IdListType>
class CHostPidl : public CHostPidlBase< CPidl<IdListType> >
{
public:
	/**
	 * Create a new wrapped PIDL holding a HostItemId with given parameters.
	 * 
	 * @param[in] pwszUser   User to log in as.
	 * @param[in] pwszHost   Host to connect to.
	 * @param[in] uPort      Port to connect to on host (default 22)
	 * @param[in] pwszPath   Path on host to use as starting directory.
	 * @param[in] pwszLabel  Friendly name of connection.
	 * 
	 * @throws CAtlException if error.
	 */
	explicit CHostPidl(
		PCWSTR pwszUser, PCWSTR pwszHost, PCWSTR pwszPath, 
		USHORT uPort=SFTP_DEFAULT_PORT, PCWSTR pwszLabel=L"")
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
			CHostPidlBase::operator=(pidl);
		return *this;
	}

	/**
	 * Concatenation constructor.
	 */
	explicit CHostPidl(
		__in_opt ConstPidlType pidl1, __in_opt PCUIDLIST_RELATIVE pidl2 )
		throw(...) : CHostPidlBase(pidl1, pidl2) {}

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

/**
 * Managed-lifetime absolute PIDL for HostItemId operations.
 */
typedef CHostPidl<ITEMIDLIST_ABSOLUTE> CHostItemAbsolute;