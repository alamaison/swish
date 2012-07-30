/**
    @file

    PIDL access particular to host folder PIDLs.

    @if license

    Copyright (C) 2011, 2012  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#ifndef SWISH_HOST_FOLDER_HOST_PIDL_HPP
#define SWISH_HOST_FOLDER_HOST_PIDL_HPP
#pragma once

#include "swish/remotelimits.h"  // Text field limits

#include <winapi/shell/pidl.hpp> // pidl_t, apidl_t, cpidl_t
#include <winapi/shell/pidl_iterator.hpp> // raw_pidl_iterator

#include <boost/filesystem/path.hpp> // wpath
#include <boost/format.hpp> // wformat
#include <boost/numeric/conversion/cast.hpp> // numeric_cast
#include <boost/static_assert.hpp> // BOOST_STATIC_ASSERT
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#ifndef STRICT_TYPED_ITEMIDS
#error Currently, swish requires strict PIDL types: define STRICT_TYPED_ITEMIDS
#endif
#include <ShTypes.h> // Raw PIDL types
#include <StrAlign.h> // ua_wcslen, ua_wcscpy_s

#include <algorithm> // find_if
#include <cstring> // memset
#include <exception>
#include <stdexcept> // runtime_error
#include <string>
#include <vector>

namespace swish {
namespace host_folder {

namespace detail {

#include <pshpack1.h>
struct host_item_id
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

BOOST_STATIC_ASSERT((sizeof(host_item_id) % sizeof(DWORD)) == 0);

inline std::wstring copy_unaligned_string(const wchar_t __unaligned* source)
{
    std::vector<wchar_t> buffer(::ua_wcslen(source) + 1);
    ::ua_wcscpy_s(&buffer[0], buffer.size(), source);
    return std::wstring(&buffer[0]);
}

}

/**
 * View internal fields of host folder PIDLs.
 *
 * The viewer doesn't take ownership of the PIDL it's passed so it must remain
 * valid for the duration of the viewer's use.
 */
class host_itemid_view
{
public:
    // We have to take the PIDL as a template, rather than that as a pidl_t
    // as the PIDL passed might be a cpidl_t or an apidl_t.  In this case
    // the pidl would be converted to a pidl_t using a temporary which is
    // destroyed immediately after the constructor returns, thereby
    // invalidating the PIDL we've stored a reference to.
    template<typename T, typename Alloc>
    explicit host_itemid_view(
        const winapi::shell::pidl::basic_pidl<T, Alloc>& pidl)
        : m_itemid(reinterpret_cast<const detail::host_item_id*>(pidl.get())) {}

    explicit host_itemid_view(PCUIDLIST_RELATIVE pidl)
        : m_itemid(reinterpret_cast<const detail::host_item_id*>(pidl)) {}

    bool valid() const
    {
        if (m_itemid == NULL)
            return false;

        return ((m_itemid->cb == sizeof(detail::host_item_id)) &&
            (m_itemid->dwFingerprint == detail::host_item_id::FINGERPRINT));
    }

    std::wstring host() const
    {
        if (!valid())
            BOOST_THROW_EXCEPTION(std::exception("PIDL is not a host item"));
        return detail::copy_unaligned_string(m_itemid->wszHost);
    }

    std::wstring user() const
    {
        if (!valid())
            BOOST_THROW_EXCEPTION(std::exception("PIDL is not a host item"));
        return detail::copy_unaligned_string(m_itemid->wszUser);
    }

    std::wstring label() const
    {
        if (!valid())
            BOOST_THROW_EXCEPTION(std::exception("PIDL is not a host item"));
        return detail::copy_unaligned_string(m_itemid->wszLabel);
    }

    boost::filesystem::wpath path() const
    {
        if (!valid())
            BOOST_THROW_EXCEPTION(std::exception("PIDL is not a host item"));
        return detail::copy_unaligned_string(m_itemid->wszPath);
    }

    int port() const
    {
        if (!valid())
            BOOST_THROW_EXCEPTION(std::exception("PIDL is not a host item"));
        return m_itemid->uPort;
    }

private:
    const detail::host_item_id __unaligned* m_itemid;
};

namespace detail {
    struct is_valid_host_item
    {
        bool operator()(const winapi::shell::pidl::pidl_t& pidl)
        {
            return host_itemid_view(pidl).valid();
        }
    };
}

/**
 * Search a (multi-level) PIDL to find the host folder ITEMID.
 *
 * In any Swish PIDL there should be at most one as it doesn't make sense for
 * a file to be under more than one host.
 *
 * @returns an iterator pointing to the position of the host ITEMID in the
 *          original PIDL.
 * @throws if no host ITEMID is found in the PIDL.
 */
inline winapi::shell::pidl::raw_pidl_iterator find_host_itemid(
    PCIDLIST_ABSOLUTE pidl)
{
    winapi::shell::pidl::raw_pidl_iterator begin(pidl);
    winapi::shell::pidl::raw_pidl_iterator end;
    
    // Search along pidl until we find one that matches our fingerprint or
    // we run off the end
    winapi::shell::pidl::raw_pidl_iterator pos = std::find_if(
        begin, end, detail::is_valid_host_item());
    if (pos != end)
        return pos;
    else
        BOOST_THROW_EXCEPTION(
            std::runtime_error("PIDL doesn't contain host ITEMID"));
}

inline winapi::shell::pidl::raw_pidl_iterator find_host_itemid(
    const winapi::shell::pidl::apidl_t& pidl)
{
    return swish::host_folder::find_host_itemid(pidl.get());
}

namespace detail {

#include <pshpack1.h>
    struct host_item_template
    {
        host_item_id id;
        SHITEMID terminator;
    };
#include <poppack.h>

}

/**
 * Construct a new host folder PIDL with the fields initialised.
 */
inline winapi::shell::pidl::cpidl_t create_host_itemid(
    const std::wstring& host, const std::wstring& user, 
    const boost::filesystem::wpath& path, int port,
    const std::wstring& label=std::wstring())
{
    // We create the item on the stack and then clone it into 
    // a CoTaskMemAllocated pidl when we return it as a cpidl_t
    detail::host_item_template item;
    std::memset(&item, 0, sizeof(item));

    item.id.cb = sizeof(item.id);
    item.id.dwFingerprint = detail::host_item_id::FINGERPRINT;

#pragma warning(push)
#pragma warning(disable:4996)
    host.copy(item.id.wszHost, MAX_HOSTNAME_LENZ);
    item.id.wszHost[MAX_HOSTNAME_LENZ - 1] = wchar_t();

    user.copy(item.id.wszUser, MAX_USERNAME_LENZ);
    item.id.wszUser[MAX_USERNAME_LENZ - 1] = wchar_t();

    path.string().copy(item.id.wszPath, MAX_PATH_LENZ);
    item.id.wszPath[MAX_PATH_LENZ - 1] = wchar_t();

    label.copy(item.id.wszLabel, MAX_LABEL_LENZ);
    item.id.wszLabel[MAX_LABEL_LENZ - 1] = wchar_t();
#pragma warning(pop)

    item.id.uPort = boost::numeric_cast<USHORT>(port);

    assert(item.terminator.cb == 0);

    return winapi::shell::pidl::cpidl_t(
        reinterpret_cast<PCITEMID_CHILD>(&item));
}
    
/**
 * Retrieve the long name of the host connection from the PIDL.
 *
 * The long name is either the canonical form if @a canonical is set:
 *     sftp://username\@hostname:port/path
 * or, if not set and if the port is the default port, the reduced form:
 *     sftp://username\@hostname/path
 */
inline std::wstring url_from_host_itemid(
    const winapi::shell::pidl::cpidl_t itemid, bool canonical)
{
    host_itemid_view host_pidl(itemid);

    if (canonical || host_pidl.port() != SFTP_DEFAULT_PORT)
    {
        return str(
            boost::wformat(L"sftp://%s@%s:%u/%s")
            % host_pidl.user() % host_pidl.host() % host_pidl.port()
            % host_pidl.path());
    }
    else
    {
        return str(
            boost::wformat(L"sftp://%s@%s/%s")
            % host_pidl.user() % host_pidl.host() % host_pidl.path());
    }
}

}} // namespace swish::host_folder

#endif
