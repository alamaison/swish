/**
    @file

    Host folder detail columns.

    @if licence

    Copyright (C) 2009, 2010  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#ifndef SWISH_REMOTE_FOLDER_COLUMNS_HPP
#define SWISH_REMOTE_FOLDER_COLUMNS_HPP
#pragma once

#include "properties.hpp" // property_from_pidl, compare_pidls_by_property

#include "swish/nse/StaticColumn.hpp" // StaticColumn

#include <winapi/shell/pidl.hpp> // cpidl_t
#include <winapi/shell/property_key.hpp> // property_key

#include <comet/variant.h> // variant_t

#include <boost/locale.hpp> // message
#include <boost/function.hpp> // function

#include <ShTypes.h> // SHCOLSTATEF

#include <string>

namespace swish {
namespace remote_folder {

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
	boost::function<std::wstring (const comet::variant_t&)> m_stringifier;
	// @}

	std::wstring title() const { return m_title; }
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
		comet::variant_t var = property_from_pidl(pidl, m_key);
		if (m_stringifier)
			return m_stringifier(var);
		else
			return var;
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
class RemoteColumnEntries
{
protected:
	const column_entry& entry(size_t index) const;
};

typedef swish::nse::StaticColumn<RemoteColumnEntries> Column;

const winapi::shell::property_key& property_key_from_column_index(
	size_t index);

}} // namespace swish::remote_folder

#endif
