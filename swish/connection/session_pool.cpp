/**
    @file

    Pool of reusuable SFTP connections.

    @if license

    Copyright (C) 2007, 2008, 2009, 2010, 2011, 2013, 2014
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

// Using ptr_map because move-aware map isn't usable with C++03
#include <boost/ptr_container/ptr_map.hpp>
//#include <boost/container/map.hpp> // move-aware map
#include <boost/thread/mutex.hpp>
#include <boost/thread/once.hpp> // call_once

#include <memory> // auto_ptr

using swish::provider::sftp_provider;

using comet::com_ptr;

using boost::call_once;
using boost::container::map;
using boost::mutex;
using boost::once_flag;

using std::auto_ptr;


namespace swish {
namespace connection {

namespace {

/**
 * Hides the implementation details from the session_pool.hpp file.
 */
class session_pool_impl
{
    // Using ptr_map because move-aware map isn't usable with C++03
    // (http://bit.ly/1jP9BDL, https://svn.boost.org/trac/boost/ticket/6618)
    typedef boost::ptr_map<connection_spec, authenticated_session>
        pool_mapping;
    //typedef boost::container::map<connection_spec, authenticated_session>
    //    pool_mapping;

public:

    static session_pool_impl& get()
    {
        call_once(m_initialise_once, do_init);
        return *m_instance;
    }

    static void destroy()
    {
        m_instance.reset();
    }

    authenticated_session& pooled_session(
        connection_spec specification, com_ptr<ISftpConsumer> consumer)
    {
        mutex::scoped_lock lock(m_session_pool_guard);

        pool_mapping::iterator session = m_sessions.find(specification);

        if (session != m_sessions.end())
        {
            // Dead sessions are replaced in the pool so that we always serve
            // something usable

            if (session->second->is_dead())
            {
                m_sessions.replace(
                    session,
                    new authenticated_session(
                        specification.create_session(consumer)));
            }
        }
        else
        {
            session = m_sessions.insert(
                specification,
                new authenticated_session(
                    specification.create_session(consumer))).first;
        }

        return *(session->second);
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


authenticated_session& session_pool::pooled_session(
    const connection_spec& specification, com_ptr<ISftpConsumer> consumer)
{
    return session_pool_impl::get().pooled_session(specification, consumer);
}

void session_pool::destroy()
{
    return session_pool_impl::destroy();
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
