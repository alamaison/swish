/**
    @file

    Source PIDL with common root.

    @if license

    Copyright (C) 2012  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#ifndef SWISH_DROP_TARGET_ROOTEDSOURCE_HPP
#define SWISH_DROP_TARGET_ROOTEDSOURCE_HPP
#pragma once

#include <washer/shell/pidl.hpp> // apidl_t, pidl_t
#include <washer/shell/pidl_iterator.hpp>
#include <washer/shell/shell_item.hpp> // pidl_shell_item

#include <string>

namespace swish {
namespace drop_target {

namespace detail {

    inline std::wstring display_name_of_item(
        const washer::shell::pidl::apidl_t& pidl)
    {
        using washer::shell::pidl_shell_item;

        return pidl_shell_item(pidl).friendly_name(
            pidl_shell_item::friendly_name_type::relative);
    }

    /**
     * Return the parsing path name for a PIDL relative the the given parent.
     */
    inline std::wstring relative_name_for_pidl(
        const washer::shell::pidl::apidl_t& parent,
        const washer::shell::pidl::pidl_t& pidl)
    {
        std::wstring name;
        washer::shell::pidl::apidl_t abs = parent;

        washer::shell::pidl::pidl_iterator it(pidl);
        while (it != washer::shell::pidl::pidl_iterator())
        {
            if (!name.empty())
                name += L"\\";

            abs += *it++;
            name += display_name_of_item(abs);
        }

        return name;
    }
}

/**
 * Shell-based source relative to a root.
 *
 * Purpose: to maintain the connection between a particular source item in a
 * multi-item transfer and the common root of all the items.
 *
 * To the user, a given source item in a file transfer does not exist in
 * isolation.  All the items in the transfer are with respect to a particular
 * root.  Paths shown as progress information, for example, are typically given
 * with respect rather than as absolute paths.  This class exists to maintain
 * that relationship.
 */
class RootedSource
{
public:
    RootedSource(
        const washer::shell::pidl::apidl_t& common_root,
        const washer::shell::pidl::pidl_t& relative_branch)
        : m_root(common_root), m_branch(relative_branch) {}

    const washer::shell::pidl::apidl_t& common_root() const
    {
        return m_root;
    }

    washer::shell::pidl::apidl_t pidl() const
    {
        return m_root + m_branch;
    }

    std::wstring relative_name() const
    {
        return detail::relative_name_for_pidl(m_root, m_branch);
    }

    RootedSource operator/(const washer::shell::pidl::pidl_t& pidl) const
    {
        return RootedSource(m_root, m_branch + pidl);
    }

private:
    const washer::shell::pidl::apidl_t m_root;
    const washer::shell::pidl::pidl_t m_branch;
};

}}

#endif
