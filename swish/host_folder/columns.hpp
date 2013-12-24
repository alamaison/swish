/**
    @file

    Host folder detail columns.

    @if license

    Copyright (C) 2010  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#ifndef SWISH_HOST_FOLDER_COLUMNS_HPP
#define SWISH_HOST_FOLDER_COLUMNS_HPP
#pragma once

#include "properties.hpp" // property_from_pidl, compare_pidls_by_property

#include "swish/nse/StaticColumn.hpp" // StaticColumn

#include <winapi/shell/pidl.hpp> // cpidl_t
#include <winapi/shell/property_key.hpp> // property_key

#include <boost/locale/encoding_utf.hpp> // utf_to_utf
#include <boost/locale.hpp> // message

#include <ShTypes.h> // SHCOLSTATEF

#include <string>

namespace swish {
namespace host_folder {

#pragma warning(push)
#pragma warning(disable: 4510 4610) // Cannot generate default constructor

struct column_entry
{
    /**
     * @name Fields.
     *
     * These can be set using an aggregate initialiser: { val1, val2, ... }
     */
    // @{
    winapi::shell::property_key m_key;
    boost::locale::message m_title;
    SHCOLSTATEF m_flags;
    int m_format;
    int m_avg_char_width;
    // @}

    std::wstring title() const
    {
        return boost::locale::conv::utf_to_utf<wchar_t>(m_title.str());
    }

    SHCOLSTATEF flags() const { return m_flags; }
    int format() const { return m_format; }
    int avg_char_width() const { return m_avg_char_width; }

    /**
     * Convert the column's property variant to a string.
     *
     * Transforms the output using m_stringifier, if any, otherwise performs
     * simple wstring conversion.
     */
    std::wstring detail(const winapi::shell::pidl::cpidl_t& pidl) const
    {
        return property_from_pidl(pidl, m_key);
    }

    int compare(
        const winapi::shell::pidl::cpidl_t& lhs,
        const winapi::shell::pidl::cpidl_t& rhs) const
    {
        return compare_pidls_by_property(lhs, rhs, m_key);
    }
};

#pragma warning(pop)

/**
 * StaticColumn-compatible interface to the static column data.
 */
class HostColumnEntries
{
protected:
    const column_entry& entry(size_t index) const;
};

typedef swish::nse::StaticColumn<HostColumnEntries> Column;

const winapi::shell::property_key& property_key_from_column_index(
    size_t index);

}} // namespace swish::host_folder

#endif
