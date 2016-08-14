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

#ifndef SSH_DETAIL_SFTP_CHANNEL_STATE_HPP
#define SSH_DETAIL_SFTP_CHANNEL_STATE_HPP

#include <ssh/detail/libssh2/sftp.hpp> // init
#include <ssh/detail/session_state.hpp>

#include <libssh2_sftp.h> // LIBSSH2_SFTP

namespace ssh
{
namespace detail
{

inline LIBSSH2_SFTP* do_sftp_init(session_state& session)
{
    auto lock = session.aquire_lock();

    return libssh2::sftp::init(session.session_ptr());
}

/**
 * RAII object managing SFTP channel state that must be maintained together.
 *
 * Manages the graceful startup/shutdown the SFTP channel and does so in
 * a thread-safe manner.
 */
class sftp_channel_state
{
    //
    // Intentionally not movable to prevent the public classes that own
    // this object moving it when they are themselves moved.  This object
    // is referenced by other classes that don't own it so the owning classes
    // need to leave it where it is when they move so as not to invalidate
    // the other references.  Making this non-copyable, non-movable enforces
    // that.
    //
    sftp_channel_state(const sftp_channel_state&) = delete;
    sftp_channel_state& operator=(const sftp_channel_state&) = delete;
    sftp_channel_state(sftp_channel_state&&) = delete;
    sftp_channel_state& operator=(sftp_channel_state&&) = delete;

public:
    /**
     * Creates SFTP channel that closes itself in a thread-safe manner
     * when it goes out of scope.
     */
    sftp_channel_state(session_state& session)
        : m_session(session), m_sftp(do_sftp_init(session_ref()))
    {
    }

    ~sftp_channel_state() throw()
    {
        auto lock = session_ref().aquire_lock();

        ::libssh2_sftp_shutdown(m_sftp);
    }

    auto aquire_lock()
    {
        return session_ref().aquire_lock();
    }

    LIBSSH2_SESSION* session_ptr()
    {
        return session_ref().session_ptr();
    }

    LIBSSH2_SFTP* sftp_ptr()
    {
        return m_sftp;
    }

private:
    session_state& session_ref()
    {
        return m_session;
    }

    session_state& m_session;
    LIBSSH2_SFTP* m_sftp;
};
}
} // namespace ssh::detail

#endif
