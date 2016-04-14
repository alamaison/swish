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

#ifndef SWISH_CONNECTION_AUTHENTICATED_SESSION_HPP
#define SWISH_CONNECTION_AUTHENTICATED_SESSION_HPP

#include "swish/connection/running_session.hpp"
#include "swish/provider/sftp_provider.hpp" // ISftpConsumer

#include <ssh/filesystem.hpp>
#include <ssh/session.hpp>

#include <comet/ptr.h> // com_ptr

#include <boost/thread/mutex.hpp>

#include <string>

namespace swish
{
namespace connection
{

/**
 * SSH session authenticated with the server.
 *
 * The point of this class is remove uncertainty as to whether the session
 * is usable.  Every instance is successfully authenticated with the server
 * and has a running SFTP channel.
 *
 * XXX: Maybe the SFTP channel part should be separated.  It's unclear if Swish
 * ever needs the two concepts separately.
 */
class authenticated_session
{
public:
    /*
    template<typename Authentication>
    authenticated_session(
        const std::wstring& host, const std::wstring& user,
        unsigned int port, Authentication& authentication) :
        m_session(host, port)
    {
        boost::mutex::scoped_lock(m_session.aquire_lock());

        ssh::session session = m_session.get_session();
        authentication.approve_host_key(session.host_key());

    }
    */

    /**
     * Creates and authenticates an SSH session and start SFTP channel.
     *
     * @param host
     *     Name of the remote host to connect the session to.
     * @param port
     *    Port on the remote host to connect to.
     * @param user
     *     User to authenticate as.
     * @param consumer
     *    Callback used for user-interaction needed to authenticate, such as
     *    requesting a password.
     *
     * @throws com_error if any part of this process fails:
     * - E_ABORT if user cancelled the operation (via ISftpConsumer)
     * - E_FAIL otherwise
     */
    authenticated_session(const std::wstring& host, unsigned int port,
                          const std::wstring& user,
                          comet::com_ptr<ISftpConsumer> consumer);

    bool is_dead();

    // This class really represents an SFTP channel rather than an
    // authenticated session.  Clients only use the session accessors
    // below to report errors and this will be replaced by the wrapper
    // sftp code which handles this internally.  Therefore we will be able
    // to remove these accessors from the public interface.
    ssh::session& get_session();

    ssh::filesystem::sftp_filesystem& get_sftp_filesystem();

    friend void swap(authenticated_session& lhs, authenticated_session& rhs);

private:
    running_session m_session;
    ssh::filesystem::sftp_filesystem m_filesystem;
};
}
} // namespace swish::connection

#endif
