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

#include "session_pool.hpp"

#include <boost/thread/mutex.hpp>
#include <boost/thread/once.hpp> // call_once

#include <map>
#include <memory> // auto_ptr

using swish::provider::sftp_provider;

using comet::com_ptr;

using boost::call_once;
using boost::mutex;
using boost::once_flag;
using boost::shared_ptr;

using std::auto_ptr;
using std::map;


namespace swish {
namespace connection {

namespace {

/**
 * Hides the implementation details from the session_pool.hpp file.
 */
class session_pool_impl
{
    typedef map<connection_spec, shared_ptr<authenticated_session>> pool_mapping;

public:

    static session_pool_impl& get()
    {
        call_once(m_initialise_once, do_init);
        return *m_instance;
    }

    shared_ptr<authenticated_session> pooled_session(
        const connection_spec& specification, com_ptr<ISftpConsumer> consumer)
    {
        mutex::scoped_lock lock(m_session_pool_guard);

        pool_mapping::iterator session = m_sessions.find(specification);

        if (session != m_sessions.end())
        {
            return session->second;
        }
        else
        {
            shared_ptr<authenticated_session> provider =
                specification.create_session(consumer);
            m_sessions[specification] = provider;
            return provider;
        }
    }

    bool has_session(const connection_spec& specification) const
    {
        mutex::scoped_lock lock(m_session_pool_guard);

        return m_sessions.find(specification) != m_sessions.end();
    }

    void remove_session(const connection_spec& specification)
    {
        mutex::scoped_lock lock(m_session_pool_guard);

        m_sessions.erase(specification);
    }


private:

    session_pool_impl() {};

    static void do_init()
    {
        m_instance.reset(new session_pool_impl);
    }

    static once_flag m_initialise_once;
    static auto_ptr<session_pool_impl> m_instance;

    mutable mutex m_session_pool_guard;
    pool_mapping m_sessions;
};


once_flag session_pool_impl::m_initialise_once;
auto_ptr<session_pool_impl> session_pool_impl::m_instance;

}


shared_ptr<authenticated_session> session_pool::pooled_session(
    const connection_spec& specification, com_ptr<ISftpConsumer> consumer)
{
    return session_pool_impl::get().pooled_session(specification, consumer);
}

bool session_pool::has_session(const connection_spec& specification) const
{
    return session_pool_impl::get().has_session(specification);
}

void session_pool::remove_session(const connection_spec& specification)
{
    return session_pool_impl::get().remove_session(specification);
}

}} // namespace swish::connection
