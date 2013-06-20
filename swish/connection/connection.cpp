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

#include "swish/provider/Provider.hpp" // CProvider

#include <boost/thread/mutex.hpp>
#include <boost/thread/once.hpp> // call_once
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <map>
#include <memory> // auto_ptr
#include <stdexcept> // invalid_argument

using swish::provider::CProvider;
using swish::provider::sftp_provider;

using boost::call_once;
using boost::mutex;
using boost::once_flag;
using boost::shared_ptr;

using std::auto_ptr;
using std::invalid_argument;
using std::map;
using std::wstring;


namespace swish {
namespace connection {

namespace {

class session_pool
{
    typedef map<connection_spec, shared_ptr<sftp_provider>> pool_mapping;

public:

    static session_pool& get()
    {
        call_once(m_initialise_once, do_init);
        return *m_instance;
    }

    shared_ptr<sftp_provider> GetSession(const connection_spec& specification)
    {
        mutex::scoped_lock lock(m_session_pool_guard);

        pool_mapping::iterator session = m_sessions.find(specification);

        if (session != m_sessions.end())
            return session->second;

        shared_ptr<sftp_provider> provider(
            new CProvider(
                specification.user(), specification.host(),
                specification.port()));

        m_sessions[specification] = provider;
        return provider;
    }

    bool has_session(const connection_spec& specification) const
    {
        mutex::scoped_lock lock(m_session_pool_guard);

        return m_sessions.find(specification) != m_sessions.end();
    }

private:

    session_pool() {};

    static void do_init()
    {
        m_instance.reset(new session_pool);
    }

    static once_flag m_initialise_once;
    static auto_ptr<session_pool> m_instance;

    mutable mutex m_session_pool_guard;
    pool_mapping m_sessions;
};


once_flag session_pool::m_initialise_once;
auto_ptr<session_pool> session_pool::m_instance;

}

connection_spec::connection_spec(
    const wstring& host, const wstring& user, int port)
: m_host(host), m_user(user), m_port(port)
{
    if (host.empty())
        BOOST_THROW_EXCEPTION(invalid_argument("Host name required"));
    if (user.empty())
        BOOST_THROW_EXCEPTION(invalid_argument("User name required"));
}

shared_ptr<sftp_provider> connection_spec::pooled_session() const
{
    return session_pool::get().GetSession(*this);
}

BOOST_SCOPED_ENUM(connection_spec::session_status)
connection_spec::session_status() const
{
    if(session_pool::get().has_session(*this))
       return session_status::running;
   else
       return session_status::not_running;
}

bool connection_spec::operator<(const connection_spec& other) const
{
    if (m_host < other.m_host)
        return true;
    else if (m_user < other.m_user)
        return true;
    else if (m_port < other.m_port)
        return true;
    else
        return false;
}

}} // namespace swish::connection
