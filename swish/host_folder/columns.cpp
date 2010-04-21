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

#include "properties.hpp" // property_from_pidl

#include <winapi/shell/shell.hpp> // string_to_strret

#include <boost/array.hpp> // array
#include <boost/locale.hpp> // translate

#include <cstring> // memset
#include <string>
#include <utility> // pair

#include <Propkey.h> // PKEY_ *

using swish::host_folder::property_from_pidl;

using winapi::shell::pidl::cpidl_t;
using winapi::shell::property_key;
using winapi::shell::string_to_strret;

using boost::array;
using boost::locale::message;
using boost::locale::translate;

using std::pair;
using std::wstring;

namespace swish {
namespace host_folder {

namespace {

	struct column {
		property_key key;
		message title;
		SHCOLSTATEF flags;
		int format;
		int avg_char_width;
	};

	const boost::array<column, 6> column_key_index = { {
		{ PKEY_ItemNameDisplay, translate("#Property (filename/label)#Name"),
		  SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT, 30 },
		{ PKEY_ComputerName, translate("#Property#Host"),
		  SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT, 30 },
		{ PKEY_SwishHostUser, translate("#Property#Username"),
		  SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT, 30 },
		{ PKEY_SwishHostPort, translate("#Property#Port"),
		  SHCOLSTATE_TYPE_INT | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT, 20 },
		{ PKEY_ItemPathDisplay, translate("#Property#Remote path"),
		  SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT, 30 },
		{ PKEY_ItemType, translate("#Property#Type"),
		  SHCOLSTATE_TYPE_STR | SHCOLSTATE_SECONDARYUI, LVCFMT_LEFT, 30 }
	} };

}

const property_key& property_key_from_column_index(size_t index)
{
	return column_key_index.at(index).key;
}

SHCOLSTATEF column_state_from_column_index(size_t index)
{
	return column_key_index.at(index).flags;
}

SHELLDETAILS header_from_column_index(size_t index)
{
	SHELLDETAILS details;
	std::memset(&details, 0, sizeof(details));

	details.cxChar = column_key_index.at(index).avg_char_width;
	details.fmt = column_key_index.at(index).format;
	wstring title = column_key_index.at(index).title;
	details.str = string_to_strret(title);

	return details;
}

SHELLDETAILS detail_from_property_key(
	const property_key& key, const cpidl_t& pidl)
{
	SHELLDETAILS details;
	std::memset(&details, 0, sizeof(details));

	wstring property = property_from_pidl(pidl, key);
	details.str = string_to_strret(property);

	return details;
}

}} // namespace swish::host_folder
