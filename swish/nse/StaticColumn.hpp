/**
    @file

    NSE folder columns.

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

#ifndef SWISH_NSE_STATIC_COLUMN_HPP
#define SWISH_NSE_STATIC_COLUMN_HPP
#pragma once

#include <winapi/shell/pidl.hpp> // cpidl_t

#include <string>

#include <ShTypes.h> // SHCOLSTATEF

namespace swish {
namespace nse {

template<typename Base>
class StaticColumn : Base
{
public:
    /**
     * Create a column manager for the indexth column.
     *
     * @throws  std::range_error if the column index is out of range.
     */
    StaticColumn(size_t index) : m_index(index) {}
    virtual ~StaticColumn() {}

    /**
     * Localised heading of the column.
     */
    std::wstring header() const
    {
        return entry(m_index).title();
    }

    /**
     * Get the contents corresponding to this column for the given PIDL.
     *
     * Regardless of the type of the underlying data, this function must always
     * returns the data as a string.  If any formatting is required, it must
     * be done in this function.
     */
    std::wstring detail(const winapi::shell::pidl::cpidl_t& pidl) const
    {
        return entry(m_index).detail(pidl);
    }

    /**
     * The number of 'x' characters an average item in the column will occupy.
     */
    int average_width_in_chars() const
    {
        return entry(m_index).avg_char_width();
    }

    /**
     * Returns the state (data type and whether to display by default) for the
     * column.
     */
    SHCOLSTATEF state() const
    {
        return entry(m_index).flags();
    }

    /**
     * How to display the data in the column (e.g. alignment).
     */
    int format() const
    {
        return entry(m_index).format();
    }

    /**
     * Compare two PIDLs by this column's detail.
     *
     * @retval -1 if lhs < rhs
     * @retval  0 if lhs == rhs
     * @retval  1 if lhs > rhs
     */
    int compare(
        const winapi::shell::pidl::cpidl_t& lhs,
        const winapi::shell::pidl::cpidl_t& rhs) const
    {
        return entry(m_index).compare(lhs, rhs);
    }

private:
    const size_t m_index;
};

}} // swish::nse

#endif
