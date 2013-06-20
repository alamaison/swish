/**
    @file

    Pool of reusuable SFTP connections.

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

#include "connection.hpp"

#include "swish/port_conversion.hpp" // port_to_wstring
#include "swish/provider/Provider.hpp" // CProvider
#include "swish/remotelimits.h" // Text field limits

#include <comet/error.h> // com_error
#include <comet/threading.h> // critical_section

#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <map>
#include <stdexcept> // invalid_argument

using swish::provider::CProvider;
using swish::provider::sftp_provider;

using comet::critical_section;
using comet::auto_cs;

using boost::shared_ptr;

using std::invalid_argument;
using std::map;
using std::wstring;


namespace swish {
namespace connection {

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

class CPool
{
public:

    shared_ptr<sftp_provider> GetSession(
        const wstring& host, const wstring& user, int port)
    {
        if (host.empty())
            BOOST_THROW_EXCEPTION(invalid_argument("Host name required"));
        if (user.empty())
            BOOST_THROW_EXCEPTION(invalid_argument("User name required"));


        // Try to get the session from the global pool
        wstring display_name = provider_moniker_name(user, host, port);

        auto_cs lock(m_cs);
        map<wstring, shared_ptr<sftp_provider>>::iterator session
            = m_sessions.find(display_name);

        if (session != m_sessions.end())
            return session->second;

        shared_ptr<sftp_provider> provider(new CProvider(user, host, port));

        m_sessions[display_name] = provider;
        return provider;
    }

private:
    static critical_section m_cs;
    static map<wstring, shared_ptr<swish::provider::sftp_provider>>
        m_sessions;
};

critical_section CPool::m_cs;
std::map<std::wstring, shared_ptr<sftp_provider> > CPool::m_sessions;



connection_spec::connection_spec(
    const wstring& host, const wstring& user, int port)
: m_host(host), m_user(user), m_port(port) {}

shared_ptr<sftp_provider> connection_spec::pooled_session() const
{
    CPool session_pool;
    return session_pool.GetSession(m_host, m_user, m_port);
}

}} // namespace swish::connection
