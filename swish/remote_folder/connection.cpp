/**
    @file

    Pool of reusuable SFTP connections.

    @if license

    Copyright (C) 2007, 2008, 2009, 2010, 2011
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

#include "connection.hpp"

#include "swish/provider/SftpProvider.h" // ISftpProvider/Consumer
#include "swish/port_conversion.hpp" // port_to_wstring
#include "swish/host_folder/host_pidl.hpp" // find_host_itemid, host_item_view
#include "swish/provider/Provider.hpp"
#include "swish/remotelimits.h" // Text field limits

#include <comet/bstr.h> // bstr_t
#include <comet/error.h> // com_error
#include <comet/interface.h> // uuidof, comtype

#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <cstring> // memset

using swish::host_folder::find_host_itemid;
using swish::host_folder::host_itemid_view;

using winapi::shell::pidl::apidl_t;

using comet::bstr_t;
using comet::com_error;
using comet::com_ptr;
using comet::critical_section;
using comet::auto_cs;
using comet::uuidof;

using std::wstring;


namespace swish {
namespace remote_folder {

namespace {

    /**
     * Create an moniker string for the session with the given parameters.
     *
     * e.g. clsid:b816a864-5022-11dc-9153-0090f5284f85:!user@host:port
     */
    wstring provider_moniker_name(
        const wstring& user, const wstring& host, int port)
    {
        wstring item_name = 
            L"clsid:b816a864-5022-11dc-9153-0090f5284f85:!" + user + L'@' + 
            host + L':' + port_to_wstring(port);
        return item_name;
    }

}

critical_section CPool::m_cs;
std::map<std::wstring, comet::com_ptr<ISftpProvider> > CPool::m_connections;

/**
 * Retrieves an SFTP session for a global pool or creates it if none exists.
 *
 * Pointers to the session objects are stored in the Running Object Table (ROT)
 * making them available to any client that needs one under the same 
 * Winstation (login).  They are identified by item monikers of the form 
 * "!username@hostname:port".
 *
 * If an existing session can't be found in the ROT (as will happen the first
 * a connection is made) this function creates a new (Provider) 
 * connection with the given parameters.  In the future this may be extended to
 * give a choice of the type of connection to make.
 *
 * @param hwnd  Isn't used but could be in future to correctly parent any
 *              elevation window.
 *
 * @returns pointer to the session (ISftpProvider).
 */
com_ptr<ISftpProvider> CPool::GetSession(
    const wstring& host, const wstring& user, int port, HWND /*hwnd*/)
{
    if (host.empty()) BOOST_THROW_EXCEPTION(com_error(E_INVALIDARG));
    if (host.empty()) BOOST_THROW_EXCEPTION(com_error(E_INVALIDARG));
    if (port > MAX_PORT) BOOST_THROW_EXCEPTION(com_error(E_INVALIDARG));

    auto_cs lock(m_cs);

    // Try to get the session from the global pool
    wstring display_name = provider_moniker_name(user, host, port);

    std::map<std::wstring, comet::com_ptr<ISftpProvider> >::iterator connection
        = m_connections.find(display_name);

    if (connection != m_connections.end())
        return connection->second;

    com_ptr<ISftpProvider> provider = new swish::provider::CProvider();
    HRESULT hr = provider->Initialize(bstr_t(user).in(), bstr_t(host).in(),
        port);
    if (FAILED(hr))
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(comet::com_error(hr)));

    m_connections[display_name] = provider;
    return provider;
}

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

    /**
     * Gets connection for given SFTP session parameters.
     */
    com_ptr<ISftpProvider> connection(
        const wstring& host, const wstring& user, int port, HWND hwnd)
    {
        CPool pool;
        return pool.GetSession(host, user, port, hwnd);
    }
}

com_ptr<ISftpProvider> connection_from_pidl(const apidl_t& pidl, HWND hwnd)
{
    // Extract connection info from PIDL
    wstring user, host, path;
    int port;
    params_from_pidl(pidl, user, host, port);

    return connection(host, user, port, hwnd);
}

}} // namespace swish::remote_folder
