/**
    @file

    Custom PIDL functions for use only by tests.

    @if license

    Copyright (C) 2009  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "pidl.hpp"

namespace { // private

	#include <pshpack1.h>
	/** 
	 * Duplicate of HostItemId defined in HostPidl.h.
	 * These must be kept in sync.
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

	/** 
	 * Duplicate of RemoteItemId defined in RemotePidl.h.
	 * These must be kept in sync.
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
}

namespace test {
namespace swish {
namespace com_dll {
namespace pidl {

PITEMID_CHILD MakeHostPidl(
	PCWSTR user, PCWSTR host, PCWSTR path, USHORT port, PCWSTR label)
{
	// Allocate enough memory to hold HostItemId structure & terminator
	static size_t cbItem = sizeof(HostItemId) + sizeof(USHORT);
	HostItemId* item = static_cast<HostItemId*>(::CoTaskMemAlloc(cbItem));
	::ZeroMemory(item, cbItem);

	// Fill members of the PIDL with data
	item->cb = sizeof(*item);
	item->dwFingerprint = HostItemId::FINGERPRINT; // Sign with fingerprint
	::wcscpy_s(item->wszUser, ARRAYSIZE(item->wszUser), user);
	::wcscpy_s(item->wszHost, ARRAYSIZE(item->wszHost), host);
	item->uPort = port;
	::wcscpy_s(item->wszPath, ARRAYSIZE(item->wszPath), path);
	::wcscpy_s(item->wszLabel, ARRAYSIZE(item->wszLabel), label);

	return reinterpret_cast<PITEMID_CHILD>(item);
}

PITEMID_CHILD MakeRemotePidl(
	PCWSTR filename, bool fIsFolder, PCWSTR owner, PCWSTR group, ULONG uUid,
	ULONG uGid, bool fIsLink, DWORD dwPermissions, ULONGLONG uSize,
	DATE dateModified, DATE dateAccessed)
{
	// Allocate enough memory to hold RemoteItemId structure & terminator
	static size_t cbItem = sizeof RemoteItemId + sizeof USHORT;
	RemoteItemId* item = static_cast<RemoteItemId*>(
		::CoTaskMemAlloc(cbItem));
	::ZeroMemory(item, cbItem);

	// Fill members of the PIDL with data
	item->cb = sizeof(*item);
	item->dwFingerprint = RemoteItemId::FINGERPRINT; // Sign with fprint
	::wcscpy_s(item->wszFilename, ARRAYSIZE(item->wszFilename), filename);
	::wcscpy_s(item->wszOwner, ARRAYSIZE(item->wszOwner), owner);
	::wcscpy_s(item->wszGroup, ARRAYSIZE(item->wszGroup), group);
	item->uUid = uUid;
	item->uGid = uGid;
	item->dwPermissions = dwPermissions;
	item->uSize = uSize;
	item->dateModified = dateModified;
	item->dateAccessed = dateAccessed;
	item->fIsFolder = fIsFolder;
	item->fIsLink = fIsLink;

	return reinterpret_cast<PITEMID_CHILD>(item);
}

}}}} // namespace test::swish::com_dll::pidl