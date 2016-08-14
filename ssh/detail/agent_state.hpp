// Copyright 2013, 2016 Alexander Lamaison

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef SSH_DETAIL_AGENT_STATE_HPP
#define SSH_DETAIL_AGENT_STATE_HPP

#include <ssh/detail/libssh2/agent.hpp>
#include <ssh/detail/session_state.hpp>

#include <libssh2.h> // LIBSSH2_AGENT

namespace ssh
{
namespace detail
{

inline LIBSSH2_AGENT* do_agent_init(session_state& session)
{
    auto lock = session.aquire_lock();

    return detail::libssh2::agent::init(session.session_ptr());
}

/**
 * RAII object managing agent state that must be maintained together.
 *
 * Manages the graceful startup/shutdown the agent collection and does so in
 * a thread-safe manner.
 */
class agent_state
{
    //
    // Intentionally not movable to prevent the public classes that own
    // this object moving it when they are themselves moved.  This object
    // is referenced by other classes that don't own it so the owning classes
    // need to leave it where it is when they move so as not to invalidate
    // the other references.  Making this non-copyable, non-movable enforces
    // that.
    //
    agent_state(const agent_state&) = delete;
    agent_state& operator=(const agent_state&) = delete;
    agent_state(agent_state&&) = delete;
    agent_state& operator=(agent_state&&) = delete;

public:
    /**
     * Creates agent collection that closes itself in a thread-safe manner
     * when it goes out of scope.
     */
    agent_state(session_state& session)
        : m_session(session), m_agent(do_agent_init(session_ref()))
    {
        auto lock = session_ref().aquire_lock();

        try
        {
            detail::libssh2::agent::connect(m_agent,
                                            session_ref().session_ptr());
        }
        catch (...)
        {
            // If connection fails, the destructor won't be called so we have
            // to free the agent here as well
            ::libssh2_agent_disconnect(m_agent);
        }
    }

    ~agent_state() throw()
    {
        auto lock = session_ref().aquire_lock();

        ::libssh2_agent_disconnect(m_agent);
        ::libssh2_agent_free(m_agent);
    }

    auto aquire_lock()
    {
        return session_ref().aquire_lock();
    }

    LIBSSH2_SESSION* session_ptr()
    {
        return session_ref().session_ptr();
    }

    LIBSSH2_AGENT* agent_ptr()
    {
        return m_agent;
    }

private:
    session_state& session_ref()
    {
        return m_session;
    }

    session_state& m_session;
    LIBSSH2_AGENT* m_agent;
};
}
} // namespace ssh::detail

#endif
