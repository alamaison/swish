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
#include "remotelimits.h"

#include <ATLComTime.h> // For COleDateTime

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

	static const DWORD FINGERPRINT = 0x533aaf69;
};
#include <poppack.h>

/**
 * PIDL class augmenting CPidl with methods specific to RemoteItemIds.
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
class CRemotePidlBase : public PidlT
{
protected:
	typedef typename PidlT::PidlType PidlType;
	typedef typename PidlT::ConstPidlType ConstPidlType;

public:
	CRemotePidlBase() throw() {}
	CRemotePidlBase( __in_opt ConstPidlType pidl ) throw(...) : PidlT(pidl) {}
	CRemotePidlBase( __in const CRemotePidlBase& pidl ) throw(...) : 
		PidlT(pidl) {}
	CRemotePidlBase& operator=( __in const CRemotePidlBase& pidl ) throw(...)
	{
		if (this != &pidl)
			PidlT::operator=(pidl);
		return *this;
	}

	class InvalidPidlException {};

	bool IsValid() const
	{
		const RemoteItemId *pRemoteId = Get();

		return (
			pRemoteId->cb == sizeof(RemoteItemId) && 
			pRemoteId->dwFingerprint == RemoteItemId::FINGERPRINT
		);
	}

	static bool IsValid(ConstPidlType pidl)
	{
		CRemotePidlBase<PidlT> rpidl(pidl);

		return rpidl.IsValid();
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
		if (!IsValid())
			throw InvalidPidlException();

		return Get()->wszFilename;
	}

	CString GetFilename(bool fIncludeExtension) const throw(...)
	{
		if (m_pidl == NULL) return L"";
		if (!IsValid())
			throw InvalidPidlException();

		CString strName = GetFilename();

		if (!fIncludeExtension && strName[0] != L'.')
		{
			int nLimit = strName.ReverseFind(L'.');
			if (nLimit < 0)
				nLimit = strName.GetLength();

			strName.Truncate(nLimit);
		}

		ATLASSERT(strName.GetLength() <= MAX_PATH_LEN);
		return strName;
	}

	/**
	 * Extract the extension part of the filename.
	 * The extension does not include the dot.  If the filename has no extension
	 * an empty string is returned.
	 */
	CString GetExtension()
	{
		if (m_pidl == NULL) return L"";
		if (!IsValid())
			throw InvalidPidlException();

		const wchar_t *pwszExtStart = ::PathFindExtension(GetFilename());
		ATLASSERT(pwszExtStart);

		if (*pwszExtStart != L'\0')
		{
			ATLASSERT(*pwszExtStart == L'.');
			pwszExtStart++; // Remove dot
		}
		
		return pwszExtStart;
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

	DWORD GetPermissions() const throw(...)
	{
		if (!IsValid())
			throw InvalidPidlException();

		return Get()->dwPermissions;
	}

	COleDateTime GetDateModified() const throw(...)
	{
		if (!IsValid())
			throw InvalidPidlException();

		return Get()->dateModified;
	}

	inline const RemoteItemId *Get() const
	{
		return reinterpret_cast<const RemoteItemId *>(m_pidl);
	}
};

/**
 * Unmanaged-lifetime child PIDL for read-only RemoteItemId operations.
 */
typedef CRemotePidlBase<CChildPidlHandle> CRemoteItemHandle;

/**
 * Unmanaged-lifetime relative PIDL for read-only RemoteItemId operations.
 */
typedef CRemotePidlBase<CRelativePidlHandle> CRemoteItemListHandle;

/**
 * Managed-lifetime PIDL for RemoteItemId operations.
 */
template <typename IdListType>
class CRemotePidl : public CRemotePidlBase< CPidl<IdListType> >
{
public:
	CRemotePidl(
		PCWSTR pwszFilename, PCWSTR pwszOwner=L"", PCWSTR pwszGroup=L"",
		bool fIsFolder=false, bool fIsLink=false, DWORD dwPermissions=0,
		ULONGLONG uSize=0, DATE dateModified=0)
	throw(...)
	{
		ATLASSERT(sizeof(RemoteItemId) % sizeof(DWORD) == 0); // DWORD-aligned

		// Allocate enough memory to hold RemoteItemId structure & terminator
		static size_t cbItem = sizeof RemoteItemId + sizeof USHORT;
		RemoteItemId *item = static_cast<RemoteItemId *>(
			::CoTaskMemAlloc(cbItem));
		if(item == NULL)
			AtlThrow(E_OUTOFMEMORY);
		::ZeroMemory(item, cbItem);

		// Fill members of the PIDL with data
		item->cb = sizeof RemoteItemId;
		item->dwFingerprint = RemoteItemId::FINGERPRINT; // Sign with fprint
		CopyWSZString(item->wszFilename, 
			ARRAYSIZE(item->wszFilename), pwszFilename);
		CopyWSZString(item->wszOwner, ARRAYSIZE(item->wszOwner), pwszOwner);
		CopyWSZString(item->wszGroup, ARRAYSIZE(item->wszGroup), pwszGroup);
		item->dwPermissions = dwPermissions;
		item->uSize = uSize;
		item->dateModified = dateModified;
		item->fIsFolder = fIsFolder;
		item->fIsLink = fIsLink;

		m_pidl = reinterpret_cast<PidlType>(item);
		ATLASSERT(IsValid());
		ATLASSERT(GetNext() == NULL); // PIDL is terminated
	}

	CRemotePidl() throw() {}
	CRemotePidl( __in_opt ConstPidlType pidl ) throw(...) :
		CRemotePidlBase(pidl) {}
	CRemotePidl( __in const CRemotePidl& pidl ) throw(...) :
		CRemotePidlBase(pidl) {}
	CRemotePidl& operator=( __in const CRemotePidl& pidl ) throw(...)
	{
		if (this != &pidl)
			CRemotePidlBase::operator=(pidl);
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
 * Managed-lifetime child PIDL for RemoteItemId operations.
 */
typedef CRemotePidl<ITEMID_CHILD> CRemoteItem;

/**
 * Managed-lifetime relative PIDL for RemoteItemId operations.
 */
typedef CRemotePidl<ITEMIDLIST_RELATIVE> CRemoteItemList;