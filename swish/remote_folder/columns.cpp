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

#include "columns.hpp"

#include "pkeys.hpp" // PKEY_* constants

#include <winapi/shell/format.hpp> // format_date_time,
                                   // format_filesize_kilobytes
#include <winapi/shell/property_key.hpp> // property_key

#pragma warning(push)
#pragma warning(disable: 4510 4610) // Cannot generate default constructor
#include <boost/array.hpp> // array
#pragma warning(pop)
#include <boost/locale.hpp> // translate

#include <string>

using winapi::shell::format_date_time;
using winapi::shell::format_filesize_kilobytes;
using winapi::shell::property_key;

using comet::variant_t;

using boost::array;
using boost::locale::translate;

using std::wstring;

namespace swish {
namespace remote_folder {

namespace {

	/**
	 * Convert the variant to a date string in the format normal for the shell.
	 */
	wstring date_formatter(const variant_t& val)
	{
		return format_date_time<wchar_t>(val);
	}

	/**
	 * Format the number in the variant as a file size in KB.
	 */
	wstring size_formatter(const variant_t& val)
	{
		return format_filesize_kilobytes<wchar_t>(val);
	}

	/**
	 * Static column information.
	 * Order of entries must correspond to the indices in columnIndices.
	 */
	const boost::array<column_entry, 10> column_key_index = { {

		{ PKEY_ItemNameDisplay, translate("#Property (filename/label)#Name"),
		  SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT, 30 },

		{ PKEY_Size, translate("#Property#Size"),
		  SHCOLSTATE_TYPE_INT | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 15,
		  size_formatter },

		{ PKEY_ItemTypeText, translate("#Property#Type"),
		  SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT, 20 },

		{ PKEY_DateModified, translate("#Property#Date Modified"),
		  SHCOLSTATE_TYPE_DATE | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT, 20,
		  date_formatter },

		{ PKEY_DateAccessed, translate("#Property#Date Accessed"),
		  SHCOLSTATE_TYPE_DATE, LVCFMT_LEFT, 20, date_formatter },

		{ PKEY_Permissions, translate("#Property#Permissions"),
		  SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT, 12 },

		{ PKEY_FileOwner, translate("#Property#Owner"),
		  SHCOLSTATE_TYPE_STR, LVCFMT_LEFT, 12 },

		{ PKEY_Group, translate("#Property#Group"),
		  SHCOLSTATE_TYPE_STR, LVCFMT_LEFT, 12 },

		{ PKEY_OwnerId, translate("#Property#Owner ID"),
		  SHCOLSTATE_TYPE_INT, LVCFMT_LEFT, 10 },

		{ PKEY_GroupId, translate("#Property#Group ID"),
		  SHCOLSTATE_TYPE_INT, LVCFMT_LEFT, 10 }
	} };

}

/**
 * Return a column entry.
 */
const column_entry& RemoteColumnEntries::entry(size_t index) const
{
	return column_key_index.at(index);
}

/**
 * Convert index to a corresponding PROPERTYKEY.
 */
const property_key& property_key_from_column_index(size_t index)
{
	return column_key_index.at(index).m_key;
}

}} // namespace swish::remote_folder
