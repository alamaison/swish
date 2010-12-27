/**
    @file

	PIDL wrapper class with accessors for remote folder PIDL fields.

    @if license

    Copyright (C) 2008, 2009  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

    @endif
*/

#pragma once

#include "Pidl.h"                // Superclass

#include "swish/remotelimits.h" // Text field limits

#include <atlstr.h>              // CString
#include <ATLComTime.h>          // COleDateTime

#include <pshpack1.h>
/** 
 * Internal structure of the PIDLs representing items on the remote filesystem.
 */
struct RemoteItemId
{
	USHORT cb;
	DWORD dwFingerprint;
	bool fIsFolder;
	bool fIsLink;
	WCHAR wszFilename[MAX_FILENAME_LENZ];
	WCHAR wszOwner[MAX_USERNAME_LENZ];
	WCHAR wszGroup[MAX_USERNAME_LENZ];
	ULONG uUid;
	ULONG uGid;
	DWORD dwPermissions;
	//WORD wPadding;
	ULONGLONG uSize;
	DATE dateModified;
	DATE dateAccessed;

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

	/**
	 * Concatenation constructor only implemented for non-const PidlT.
	 */
	explicit CRemotePidlBase(
		__in_opt ConstPidlType pidl1, __in_opt PCUIDLIST_RELATIVE pidl2 )
	throw(...);

	/**
	 * Does fingerprint stored in this PIDL correspond to a RemoteItemId?
	 */
	inline bool IsValid() const
	{
		return (
			!IsEmpty() &&
			Get()->cb == sizeof(RemoteItemId) && 
			Get()->dwFingerprint == RemoteItemId::FINGERPRINT
		);
	}

	/**
	 * Throw an exception if the PIDL is invalid.
	 */
	inline void CheckValidity() const
	{
		if (!IsValid())
			AtlThrow(E_UNEXPECTED);
	}

	/**
	 * Does fingerprint stored in @p pidl correspond to a RemoteItemId?
	 */
	static bool IsValid(ConstPidlType pidl)
	{
		CRemotePidlBase<PidlT> rpidl(pidl);

		return rpidl.IsValid();
	}

	bool IsFolder() const throw(...)
	{
		CheckValidity();
		return Get()->fIsFolder;
	}

	bool IsLink() const throw(...)
	{
		CheckValidity();
		return Get()->fIsLink;
	}

	ATL::CString GetFilename() const throw(...)
	{
		CheckValidity();
		return Get()->wszFilename;
	}

	ATL::CString GetFilename(bool fIncludeExtension) const throw(...)
	{
		ATL::CString strName = GetFilename();

		if (!fIncludeExtension && !Get()->fIsFolder && strName[0] != L'.')
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
	ATL::CString GetExtension() const throw(...)
	{
		CheckValidity();

		const wchar_t *pwszExtStart = ::PathFindExtension(GetFilename());
		ATLASSERT(pwszExtStart);

		if (*pwszExtStart != L'\0')
		{
			ATLASSERT(*pwszExtStart == L'.');
			pwszExtStart++; // Remove dot
		}
		
		return pwszExtStart;
	}

	/**
	 * Return the relative path made by the items in this PIDL.
	 * e.g.
	 * - A child PIDL returns:     "filename.ext"
	 * - A relative PIDL returns:  "dir2/dir2/dir3/filename.ext"
	 * - An absolute PIDL returns: "dir2/dir2/dir3/filename.ext"
	 */
	ATL::CString GetFilePath() const throw(...)
	{
		// Walk over RemoteItemIds and append each filename to form the path
		ATL::CString strPath = GetFilename();
		CRemoteItemListHandle pidlNext = GetNext();

		while (pidlNext.IsValid())
		{
			strPath += L"/";
			strPath += pidlNext.Get()->wszFilename;
			pidlNext = pidlNext.GetNext();
		}

		ATLASSERT( strPath.GetLength() <= MAX_PATH_LEN );

		return strPath;
	}

	ATL::CString GetOwner() const throw(...)
	{
		CheckValidity();
		return Get()->wszOwner;
	}

	ATL::CString GetGroup() const throw(...)
	{
		CheckValidity();
		return Get()->wszGroup;
	}

	ULONG GetOwnerId() const throw(...)
	{
		CheckValidity();
		return Get()->uUid;
	}

	ULONG GetGroupId() const throw(...)
	{
		CheckValidity();
		return Get()->uGid;
	}

	ULONGLONG GetFileSize() const throw(...)
	{
		CheckValidity();
		return Get()->uSize;
	}

	DWORD GetPermissions() const throw(...)
	{
		CheckValidity();
		return Get()->dwPermissions;
	}

	ATL::COleDateTime GetDateModified() const throw(...)
	{
		CheckValidity();
		return Get()->dateModified;
	}

	ATL::COleDateTime GetDateAccessed() const throw(...)
	{
		CheckValidity();
		return Get()->dateAccessed;
	}

	inline const RemoteItemId *Get() const throw()
	{
		return reinterpret_cast<const RemoteItemId *>(m_pidl);
	}
};

/**
 * Concatenation constructor only implemented for non-const base type.
 * Also, the only non-const bases that make sense for concatentation are
 * those derived from absolute and relative PIDLs.
 */
template <>
inline CRemotePidlBase< CPidl<ITEMIDLIST_ABSOLUTE> >::CRemotePidlBase(
	__in_opt CPidl<ITEMIDLIST_ABSOLUTE>::ConstPidlType pidl1,
	__in_opt PCUIDLIST_RELATIVE pidl2 )
	throw(...) : CPidl<ITEMIDLIST_ABSOLUTE>(pidl1, pidl2) {}

template <>
inline CRemotePidlBase< CPidl<ITEMIDLIST_RELATIVE> >::CRemotePidlBase(
	__in_opt CPidl<ITEMIDLIST_RELATIVE>::ConstPidlType pidl1,
	__in_opt PCUIDLIST_RELATIVE pidl2 )
	throw(...) : CPidl<ITEMIDLIST_RELATIVE>(pidl1, pidl2) {}

/**
 * Unmanaged-lifetime child PIDL for read-only RemoteItemId operations.
 */
typedef CRemotePidlBase<CChildPidlHandle> CRemoteItemHandle;

/**
 * Unmanaged-lifetime relative PIDL for read-only RemoteItemId operations.
 */
typedef CRemotePidlBase<CRelativePidlHandle> CRemoteItemListHandle;

/**
 * Unmanaged-lifetime relative PIDL for read-only RemoteItemId operations.
 */
typedef CRemotePidlBase<CAbsolutePidlHandle> CRemoteItemAbsoluteHandle;

/**
 * Managed-lifetime PIDL for RemoteItemId operations.
 */
template <typename IdListType>
class CRemotePidl : public CRemotePidlBase< CPidl<IdListType> >
{
public:
	/**
	 * Create a new wrapped PIDL holding a RemoteItemId with given parameters.
	 * 
	 * @param[in] pwszFilename   Name of file or directory on the remote 
	 *                           file-system.
	 * @param[in] fIsFolder      Is file a folder?
	 * @param[in] pwszOwner      Name of file owner on remote system.
	 * @param[in] pwszGroup      Name of file group on remote system.
	 * @param[in] uUID           Numeric ID of file owner on remote system.
	 * @param[in] uGID           Numeric ID of file group on remote system.
	 * @param[in] dwPermissions  Value of the file's Unix permissions bits.
	 * @param[in] uSize          Size of file in bytes.
	 * @param[in] dateModified   Date that file was last modified.
	 * @param[in] fIsLink        Is file a symlink?
	 * 
	 * @throws com_error if error.
	 */
	explicit CRemotePidl(
		PCWSTR pwszFilename, bool fIsFolder=false, PCWSTR pwszOwner=L"", 
		PCWSTR pwszGroup=L"", ULONG uUid=0, ULONG uGid=0, bool fIsLink=false,
		DWORD dwPermissions=0, ULONGLONG uSize=0,
		DATE dateModified=0, DATE dateAccessed=0)
	throw(...)
	{
		ATLASSERT(sizeof(RemoteItemId) % sizeof(DWORD) == 0); // DWORD-aligned

		// Allocate enough memory to hold RemoteItemId structure & terminator
		static size_t cbItem = sizeof RemoteItemId + sizeof USHORT;
		m_pidl = static_cast<PidlType>(::CoTaskMemAlloc(cbItem));
		ATLENSURE_THROW(m_pidl, E_OUTOFMEMORY);
		::ZeroMemory(m_pidl, cbItem);

		// Fill members of the PIDL with data
		RemoteItemId *item = reinterpret_cast<RemoteItemId *>(m_pidl);
		item->cb = sizeof RemoteItemId;
		item->dwFingerprint = RemoteItemId::FINGERPRINT; // Sign with fprint
		CopyWSZString(item->wszFilename, 
			ARRAYSIZE(item->wszFilename), pwszFilename);
		CopyWSZString(item->wszOwner, ARRAYSIZE(item->wszOwner), pwszOwner);
		CopyWSZString(item->wszGroup, ARRAYSIZE(item->wszGroup), pwszGroup);
		item->uUid = uUid;
		item->uGid = uGid;
		item->dwPermissions = dwPermissions;
		item->uSize = uSize;
		item->dateModified = dateModified;
		item->dateAccessed = dateAccessed;
		item->fIsFolder = fIsFolder;
		item->fIsLink = fIsLink;

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

	/**
	 * Concatenation constructor.
	 */
	explicit CRemotePidl(
		__in_opt ConstPidlType pidl1, __in_opt PCUIDLIST_RELATIVE pidl2 )
		throw(...) : CRemotePidlBase(pidl1, pidl2) {}

	CRemotePidl& SetFilename(__in PCWSTR pwszFilename) throw(...)
	{
		ATLENSURE_THROW(pwszFilename, E_POINTER);
		ATLENSURE_THROW(*pwszFilename != L'\0', E_INVALIDARG);
		CheckValidity();

		CopyWSZString(
			Set()->wszFilename, ARRAYSIZE(Set()->wszFilename), pwszFilename);

		return *this;
	}

protected:
	inline RemoteItemId *Set() const throw()
	{
		return reinterpret_cast<RemoteItemId *>(m_pidl);
	}

private:
	/**
	 * Copy a wide string into provided buffer.
	 *
	 * @param[out] pwszDest  Destination wide-char string buffer.
	 * @param[in]  cchDest   Length of destination buffer in @b wide-chars.
	 * @param[in]  pwszSrc   Source string.  Must be NULL-terminated.
	 *
	 * @throws com_error if an error occurs while copying.
	 */
	static void CopyWSZString(
		__out_ecount(cchDest) PWSTR pwszDest, __in size_t cchDest,
		__in PCWSTR pwszSrc)
	throw(...)
	{
		// Neither source nor destination can be NULL
		ATLASSERT(pwszSrc && pwszDest);

		ATL::Checked::wcscpy_s(pwszDest, cchDest, pwszSrc);
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

/**
 * Managed-lifetime absolute PIDL for RemoteItemId operations.
 */
typedef CRemotePidl<ITEMIDLIST_ABSOLUTE> CRemoteItemAbsolute;
