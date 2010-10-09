/**
    @file

    Host folder detail columns.

    @if licence

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

#include "columns.hpp"

#pragma warning(push)
#pragma warning(disable: 4510 4610) // Cannot generate default constructor
#include <boost/array.hpp> // array
#pragma warning(pop)
#include <boost/locale.hpp> // translate

#include <string>

#include <Propkey.h> // PKEY_ *

using winapi::shell::property_key;

using boost::array;
using boost::locale::translate;

using std::wstring;

namespace swish {
namespace host_folder {

namespace {

	/**
	 * Static column information.
	 * Order of entries must correspond to the indices in columnIndices.
	 */
	const boost::array<column_entry, 6> column_key_index = { {

		{ PKEY_ItemNameDisplay, translate("Property (filename/label)", "Name"),
		  SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT, 30 },

		{ PKEY_ComputerName, translate("Property", "Host"),
		  SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT, 30 },

		{ PKEY_SwishHostUser, translate("Property", "Username"),
		  SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT, 30 },

		{ PKEY_SwishHostPort, translate("Property", "Port"),
		  SHCOLSTATE_TYPE_INT | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT, 20 },

		{ PKEY_ItemPathDisplay, translate("Property", "Remote path"),
		  SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT, 30 },

		{ PKEY_ItemType, translate("Property", "Type"),
		  SHCOLSTATE_TYPE_STR | SHCOLSTATE_SECONDARYUI, LVCFMT_LEFT, 30 }
	} };

}

/**
 * Return a column entry.
 */
const column_entry& HostColumnEntries::entry(size_t index) const
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

}} // namespace swish::host_folder
