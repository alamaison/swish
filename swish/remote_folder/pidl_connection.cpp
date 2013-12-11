/**
    @file

    Relates PIDLs to SFTP connections.

    @if license

    Copyright (C) 2007, 2008, 2009, 2010, 2011, 2013
    Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "pidl_connection.hpp"

#include "swish/host_folder/host_pidl.hpp" // find_host_itemid, host_itemid_view
#include "swish/provider/Provider.hpp" // CProvider

#include <boost/make_shared.hpp>

#include <string>

using swish::connection::connection_spec;
using swish::host_folder::find_host_itemid;
using swish::host_folder::host_itemid_view;
using swish::provider::CProvider;
using swish::provider::sftp_provider;

using winapi::shell::pidl::apidl_t;

using comet::com_ptr;

using boost::make_shared;
using boost::shared_ptr;

using std::wstring;


namespace swish {
namespace remote_folder {

namespace {

    void params_from_pidl(
        const apidl_t& pidl, wstring& user, wstring& host, int& port)
    {
        // Find HOSTPIDL part of this folder's absolute pidl to extract server
        // info
        host_itemid_view host_itemid(*find_host_itemid(pidl));
        assert(host_itemid.valid());

        user = host_itemid.user();
        host = host_itemid.host();
        port = host_itemid.port();
        assert(!user.empty());
        assert(!host.empty());
    }

}

connection_spec connection_from_pidl(const apidl_t& pidl)
{
    // Extract connection info from PIDL
    wstring user, host, path;
    int port;
    params_from_pidl(pidl, user, host, port);

    return connection_spec(host, user, port);
}

shared_ptr<sftp_provider> provider_from_pidl(
    const apidl_t& pidl, com_ptr<ISftpConsumer> consumer)
{
    connection_spec specification = connection_from_pidl(pidl);

    return make_shared<CProvider>(specification, consumer);
}

}} // namespace swish::remote_folder
