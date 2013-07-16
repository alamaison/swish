/**
    @file

    Key-agent protocol.

    @if license

    Copyright (C) 2012, 2013  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

    In addition, as a special exception, the the copyright holders give you
    permission to combine this program with free software programs or the 
    OpenSSL project's "OpenSSL" library (or with modified versions of it, 
    with unchanged license). You may copy and distribute such a system 
    following the terms of the GNU GPL for this program and the licenses 
    of the other code concerned. The GNU General Public License gives 
    permission to release a modified version without this exception; this 
    exception also makes it possible to release a modified version which 
    carries forward this exception.

    @endif
*/

#ifndef SSH_AGENT_HPP
#define SSH_AGENT_HPP
#pragma once

#include <ssh/ssh_error.hpp>

#include <boost/exception/errinfo_api_function.hpp> // errinfo_api_function
#include <boost/exception/info.hpp> // errinfo_api_function
#include <boost/iterator/iterator_facade.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <string>

#include <libssh2.h>

namespace ssh {
namespace agent {

namespace detail {
    namespace libssh2 {
    namespace agent {

        /**
         * Thin wrapper around libssh2_agent_init.
         */
        inline boost::shared_ptr<LIBSSH2_AGENT> init(
            boost::shared_ptr<LIBSSH2_SESSION> session)
        {
            boost::shared_ptr<LIBSSH2_AGENT> agent(
                ::libssh2_agent_init(session.get()),
                ::libssh2_agent_free);
            if (!agent)
                BOOST_THROW_EXCEPTION(
                    ssh::last_error(session) <<
                    boost::errinfo_api_function("libssh2_agent_init"));

            return agent;
        }

        /**
         * Thin wrapper around libssh2_agent_connect.
         */
        inline boost::shared_ptr<LIBSSH2_AGENT> connect(
            boost::shared_ptr<LIBSSH2_AGENT> agent,
            boost::shared_ptr<LIBSSH2_SESSION> session)
        {
            int rc = ::libssh2_agent_connect(agent.get());
            if (rc < 0)
                BOOST_THROW_EXCEPTION(
                    ssh::last_error(session) <<
                    boost::errinfo_api_function("libssh2_agent_connect"));

            // This second shared pointer to the same object only exists to
            // manage the lifetime of the connection
            return boost::shared_ptr<LIBSSH2_AGENT>(
                agent.get(), ::libssh2_agent_disconnect);
        }

        /**
         * Thin wrapper around libssh2_agent_get_identity.
         */
        inline bool get_identity(
            boost::shared_ptr<LIBSSH2_AGENT> agent,
            boost::shared_ptr<LIBSSH2_SESSION> session,
            libssh2_agent_publickey** out, libssh2_agent_publickey* previous)
        {
            int rc = ::libssh2_agent_get_identity(agent.get(), out, previous);
            if (rc < 0)
                BOOST_THROW_EXCEPTION(
                    ssh::last_error(session) <<
                    boost::errinfo_api_function("libssh2_agent_get_identity"));

            return rc != 0;
        }

        /**
         * Thin wrapper around libssh2_agent_list_identities.
         */
        inline void list_identities(
            boost::shared_ptr<LIBSSH2_AGENT> agent,
            boost::shared_ptr<LIBSSH2_SESSION> session)
        {
            int rc = ::libssh2_agent_list_identities(agent.get());
            if (rc < 0)
                BOOST_THROW_EXCEPTION(
                    ssh::last_error(session) <<
                    boost::errinfo_api_function(
                        "libssh2_agent_list_identities"));
        }

        /**
         * Thin wrapper around libssh2_agent_userauth.
         */
        inline void userauth(
            boost::shared_ptr<LIBSSH2_AGENT> agent,
            boost::shared_ptr<LIBSSH2_SESSION> session,
            const std::string& user_name,
            libssh2_agent_publickey* identity)
        {
            int rc = ::libssh2_agent_userauth(
                agent.get(), user_name.c_str(), identity);
            if (rc < 0)
                BOOST_THROW_EXCEPTION(
                    ssh::last_error(session) <<
                    boost::errinfo_api_function("libssh2_agent_userauth"));
        }

    }}
}

class identity
{
public:

    identity(
        boost::shared_ptr<LIBSSH2_SESSION> session,
        boost::shared_ptr<LIBSSH2_AGENT> agent,
        libssh2_agent_publickey* identity) :
    m_session(session), m_agent(agent), m_identity(identity) {}

    void authenticate(const std::string& user_name)
    {
        detail::libssh2::agent::userauth(
            m_agent, m_session, user_name, m_identity);
    }
        
private:

    boost::shared_ptr<LIBSSH2_SESSION> m_session;
    boost::shared_ptr<LIBSSH2_AGENT> m_agent;
    libssh2_agent_publickey* m_identity;
};

namespace detail {

template<typename IdentityType>
class identity_iterator_base :
    public boost::iterator_facade<
        identity_iterator_base<IdentityType>,
        IdentityType,
        boost::forward_traversal_tag, // this tag allows value return
        IdentityType>
{
    friend class boost::iterator_core_access;

    // Enables conversion constructor to work:
    template<typename> friend class identity_iterator_base;

public:

    identity_iterator_base(
        boost::shared_ptr<LIBSSH2_SESSION> session,
        boost::shared_ptr<LIBSSH2_AGENT> agent) :
    m_session(session), m_agent(agent), m_pos(NULL)
    {
        increment();
    }

    /**
     * End iterator.
     */
    identity_iterator_base()  : m_pos(NULL) {}

    /**
     * Copy conversion constructor.
     *
     * Purpose: to allow mutable iterators to be converted to const iterators.
     */
    template<typename OtherValue>
    identity_iterator_base(const identity_iterator_base<OtherValue>& other)
        : m_session(other.m_session), m_agent(other.m_agent),
        m_pos(other.m_pos) {}

private:

    void increment()
    {
        if (!m_agent)
            BOOST_THROW_EXCEPTION(
                std::logic_error(
                    "Can't increment past the end of a collection"));

        bool no_more_identities = detail::libssh2::agent::get_identity(
            m_agent, m_session, &m_pos, m_pos);

        if (no_more_identities)
        {
            // Use m_agent as the end marker as a NULL m_pos means start again
            m_agent.reset();
            m_pos = NULL; // To keep equality with the end iterator happy
        }
    }

    bool equal(const identity_iterator_base& other) const
    {
        return m_agent == other.m_agent && m_pos == other.m_pos;
    }

    value_type dereference() const
    {
        if (!m_agent)
            BOOST_THROW_EXCEPTION(
                std::logic_error("Can't dereference the end of a collection"));

        return identity(m_session, m_agent, m_pos);
    }

    boost::shared_ptr<LIBSSH2_SESSION> m_session;
    boost::shared_ptr<LIBSSH2_AGENT> m_agent;
    libssh2_agent_publickey* m_pos;
};

}

/**
 * A connection to an SSH key agent.
 *
 * When this object is created, all the identities currently stored in it are
 * copied out.  If you need a fresh list, request a new agent instance.
 */
class agent_identities
{
public:

    typedef detail::identity_iterator_base<identity> iterator;
    typedef detail::identity_iterator_base<const identity> const_iterator;

    explicit agent_identities(boost::shared_ptr<LIBSSH2_SESSION> session)
        : m_session(session),
          m_agent(detail::libssh2::agent::init(session)),
          m_agent_connection(
              detail::libssh2::agent::connect(m_agent, m_session))
    {
        // We pull the identities out here (AND ONLY HERE) so that all copies
        // of the agent, iterators and identity objects refer to valid data.
        // If we called this when creating the iterator it would wipe out all
        // other iterators.
        detail::libssh2::agent::list_identities(m_agent, m_session);
    }

    iterator begin() const
    {
        return iterator(m_session, m_agent);
    }

    iterator end() const
    {
        return iterator();
    }

private:

    boost::shared_ptr<LIBSSH2_SESSION> m_session;
    boost::shared_ptr<LIBSSH2_AGENT> m_agent;
    // This must remain after m_agent so it is destroyed first as it points to
    // the same agent struct.  It only exists to ensure the connection is shared
    // between copies of this instance in the same way as the agent itself.
    boost::shared_ptr<LIBSSH2_AGENT> m_agent_connection;
};

}} // namespace ssh::agent

#endif
