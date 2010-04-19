/**
    @file

    Host folder property columns.

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

#include <initguid.h>  // Make DEFINE_PROPERTYKEY() actually define a key
#include "properties.hpp"

#include <swish/shell_folder/HostPidl.h> // CHostItemHandle

#include <boost/assign.hpp> // map_list_of
#include <boost/function.hpp> // function
#include <boost/locale.hpp> // translate
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <map>

using winapi::shell::property_key;
using swish::shell_folder::pidl::cpidl_t;

using comet::variant_t;

using boost::assign::map_list_of;
using boost::locale::translate;

using std::map;

namespace swish {
namespace host_folder {

namespace {

	class unknown_property_error : public std::runtime_error
	{
	public:
		unknown_property_error() : std::runtime_error("Unknown property") {}
	};

	typedef map<
		property_key,
		boost::function<variant_t (const CHostItemHandle& pidl)> >
		host_property_map;

	variant_t net_drive_returner(const CHostItemHandle& /*pidl*/)
	{
		return translate("#FileType#Network Drive").str<wchar_t>();
	}

	variant_t label_getter(const CHostItemHandle& pidl)
	{ return pidl.GetLabel().GetString(); }
	variant_t host_getter(const CHostItemHandle& pidl)
	{ return pidl.GetHost().GetString(); }
	variant_t user_getter(const CHostItemHandle& pidl)
	{ return pidl.GetUser().GetString(); }
	variant_t port_getter(const CHostItemHandle& pidl)
	{ return pidl.GetPortStr().GetString(); }
	variant_t path_getter(const CHostItemHandle& pidl)
	{ return pidl.GetPath().GetString(); }

	const host_property_map host_property_getters = map_list_of
		(PKEY_ItemNameDisplay, label_getter) // Display name (Label)
		(PKEY_ComputerName, host_getter) // Hostname
		(PKEY_SwishHostUser, user_getter) // Username
		(PKEY_SwishHostPort, port_getter) // SFTP port
		(PKEY_ItemPathDisplay, path_getter) // Remote filesystem path
		(PKEY_ItemType, net_drive_returner); // Type: always 'Network Drive'
}

variant_t property_from_pidl(const cpidl_t& pidl, const property_key& key)
{
	host_property_map::const_iterator pos = host_property_getters.find(key);
	if (pos == host_property_getters.end())
		BOOST_THROW_EXCEPTION(unknown_property_error());

	return (pos->second)(pidl.get());
}

}} // namespace swish::host_folder
