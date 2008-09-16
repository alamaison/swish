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
