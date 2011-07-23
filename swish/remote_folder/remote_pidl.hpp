/**
    @file

    PIDL access particular to remote folder PIDLs.

    @if license

    Copyright (C) 2011  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#ifndef SWISH_REMOTE_FOLDER_REMOTE_PIDL_HPP
#define SWISH_REMOTE_FOLDER_REMOTE_PIDL_HPP
#pragma once

#include "swish/remotelimits.h"  // Text field limits

#include <comet/datetime.h> // datetime_t

#include <winapi/shell/pidl.hpp> // pidl_t
#include <winapi/shell/pidl_iterator.hpp> // raw_pidl_iterator

#include <boost/filesystem/path.hpp> // wpath
#include <boost/static_assert.hpp> // BOOST_STATIC_ASSERT
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#define STRICT_TYPED_ITEMIDS ///< Better type safety for PIDLs (must be 
                             ///< before <shtypes.h> or <shlobj.h>).
#include <ShTypes.h> // Raw PIDL types

#include <cstring> // memset
#include <exception>
#include <string>

namespace swish {
namespace remote_folder {

namespace detail {

#include <pshpack1.h>
/** 
 * Internal structure of the PIDLs representing items on the remote filesystem.
 */
struct remote_item_id
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

BOOST_STATIC_ASSERT((sizeof(remote_item_id) % sizeof(DWORD)) == 0);

inline const remote_item_id& as_remote_item_id(PCUIDLIST_RELATIVE pidl) 
{
	return *reinterpret_cast<const remote_item_id*>(pidl);
}

}

/**
 * View internal fields of remote folder PIDLs.
 *
 * The viewer doesn't take ownership of the PIDL it's passed so it must remain
 * valid for the duration of the viewer's use.
 */
class remote_itemid_view
{
public:
	// We have to take the PIDL as a template, rather than that as a pidl_t
	// as the PIDL passed might be a cpidl_t or an apidl_t.  In this case
	// the pidl would be converted to a pidl_t using a temporary which is
	// destroyed immediately after the constructor returns, thereby
	// invalidating the PIDL we've stored a reference to.
	template<typename T, typename Alloc>
	explicit remote_itemid_view(
		const winapi::shell::pidl::basic_pidl<T, Alloc>& pidl)
		: m_pidl(pidl.get()) {}

	explicit remote_itemid_view(PCUIDLIST_RELATIVE pidl) : m_pidl(pidl) {}

	bool valid() const
	{
		if (m_pidl == NULL)
			return false;

		const detail::remote_item_id& id = detail::as_remote_item_id(m_pidl);
		return ((id.cb == sizeof(detail::remote_item_id)) &&
			(id.dwFingerprint == detail::remote_item_id::FINGERPRINT));
	}

	std::wstring filename() const
	{
		if (!valid())
			BOOST_THROW_EXCEPTION(std::exception("PIDL is not a remote item"));
		return detail::as_remote_item_id(m_pidl).wszFilename;
	}

	std::wstring owner() const
	{
		if (!valid())
			BOOST_THROW_EXCEPTION(std::exception("PIDL is not a remote item"));
		return detail::as_remote_item_id(m_pidl).wszOwner;
	}

	std::wstring group() const
	{
		if (!valid())
			BOOST_THROW_EXCEPTION(std::exception("PIDL is not a remote item"));
		return detail::as_remote_item_id(m_pidl).wszGroup;
	}

	ULONG owner_id() const
	{
		if (!valid())
			BOOST_THROW_EXCEPTION(std::exception("PIDL is not a remote item"));
		return detail::as_remote_item_id(m_pidl).uUid;
	}

	ULONG group_id() const
	{
		if (!valid())
			BOOST_THROW_EXCEPTION(std::exception("PIDL is not a remote item"));
		return detail::as_remote_item_id(m_pidl).uGid;
	}

	bool is_folder() const
	{
		if (!valid())
			BOOST_THROW_EXCEPTION(std::exception("PIDL is not a remote item"));
		return detail::as_remote_item_id(m_pidl).fIsFolder;
	}

	bool is_link() const
	{
		if (!valid())
			BOOST_THROW_EXCEPTION(std::exception("PIDL is not a remote item"));
		return detail::as_remote_item_id(m_pidl).fIsLink;
	}

	DWORD permissions() const
	{
		if (!valid())
			BOOST_THROW_EXCEPTION(std::exception("PIDL is not a remote item"));
		return detail::as_remote_item_id(m_pidl).dwPermissions;
	}

	ULONGLONG size() const
	{
		if (!valid())
			BOOST_THROW_EXCEPTION(std::exception("PIDL is not a remote item"));
		return detail::as_remote_item_id(m_pidl).uSize;
	}

	comet::datetime_t date_modified() const
	{
		if (!valid())
			BOOST_THROW_EXCEPTION(std::exception("PIDL is not a remote item"));
		return comet::datetime_t(
			detail::as_remote_item_id(m_pidl).dateModified);
	}

	comet::datetime_t date_accessed() const
	{
		if (!valid())
			BOOST_THROW_EXCEPTION(std::exception("PIDL is not a remote item"));
		return comet::datetime_t(
			detail::as_remote_item_id(m_pidl).dateAccessed);
	}

private:
	PCUIDLIST_RELATIVE m_pidl;
};

namespace detail {

#include <pshpack1.h>
	struct remote_item_template
	{
		remote_item_id id;
		SHITEMID terminator;
	};
#include <poppack.h>

}

/**
 * Create a new wrapped PIDL holding a remote_item_id with given parameters.
 * 
 * @param filename       Name of file or directory on the remote filesystem.
 * @param is_folder      Is file a folder?
 * @param is_link        Is file a symlink?
 * @param owner          Name of file owner on remote system.
 * @param group          Name of file group on remote system.
 * @param owner_id       UID of file owner on remote system.
 * @param group_id       GID of file group on remote system.
 * @param permissions    The file's Unix permissions bits.
 * @param size           Size of file in bytes.
 * @param date_modified  Date that file was last modified.
 * @param date_accessed  Date that file was last accessed.
 */
inline winapi::shell::pidl::cpidl_t create_remote_itemid(
	const std::wstring& filename, bool is_folder, bool is_link,
	const std::wstring& owner, const std::wstring& group, ULONG owner_id,
	ULONG group_id, DWORD permissions, ULONGLONG size,
	const comet::datetime_t date_modified,
	const comet::datetime_t date_accessed)
{
	// We create the item on the stack and then clone it into 
	// a CoTaskMemAllocated pidl when we return it as a cpidl_t
	detail::remote_item_template item;
	std::memset(&item, 0, sizeof(item));

	item.id.cb = sizeof(item.id);
	item.id.dwFingerprint = detail::remote_item_id::FINGERPRINT;

#pragma warning(push)
#pragma warning(disable:4996)
	filename.copy(item.id.wszFilename, MAX_FILENAME_LENZ);
	item.id.wszFilename[MAX_HOSTNAME_LENZ - 1] = wchar_t();

	owner.copy(item.id.wszOwner, MAX_USERNAME_LENZ);
	item.id.wszOwner[MAX_USERNAME_LENZ - 1] = wchar_t();

	group.copy(item.id.wszGroup, MAX_USERNAME_LENZ);
	item.id.wszGroup[MAX_USERNAME_LENZ - 1] = wchar_t();
#pragma warning(pop)

	item.id.fIsFolder = is_folder;
	item.id.fIsLink = is_link;

	item.id.uUid = owner_id;
	item.id.uGid = group_id;
	item.id.dwPermissions = permissions;
	item.id.uSize = size;

	item.id.dateModified = date_modified.get();
	item.id.dateAccessed = date_accessed.get();

	assert(item.terminator.cb == 0);

	return winapi::shell::pidl::cpidl_t(
		reinterpret_cast<PCITEMID_CHILD>(&item));
}

/**
 * Return the relative path made by the items in this PIDL.
 * e.g.
 * - A child PIDL returns:     "filename.ext"
 * - A relative PIDL returns:  "dir2/dir2/dir3/filename.ext"
 * - An absolute PIDL returns: "dir2/dir2/dir3/filename.ext"
 */
inline boost::filesystem::wpath path_from_remote_pidl(
	const winapi::shell::pidl::pidl_t& remote_pidl)
{
	// Walk over RemoteItemIds and append each filename to form the path
	winapi::shell::pidl::raw_pidl_iterator it(remote_pidl.get());

	boost::filesystem::wpath path;
	while (it != winapi::shell::pidl::raw_pidl_iterator())
	{
		remote_itemid_view itemid(*it);
		if (!itemid.valid())
			break; // should never happen

		path /= itemid.filename();

		++it;
	}

	return path;
}

}} // namespace swish::remote_folder

#endif
