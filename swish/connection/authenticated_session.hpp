/**
    @file

    SSH session authentication.

    @if license

    Copyright (C) 2013  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#ifndef SWISH_CONNECTION_AUTHENTICATED_SESSION_HPP
#define SWISH_CONNECTION_AUTHENTICATED_SESSION_HPP

#include "swish/connection/running_session.hpp"
#include "swish/provider/sftp_provider.hpp" // ISftpConsumer

#include <ssh/session.hpp>
#include <ssh/sftp.hpp>

#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>

#include <memory> // auto_ptr
#include <string>

namespace swish {
namespace connection {

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
class authenticated_session : private boost::noncopyable
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
    authenticated_session(
        const std::wstring& host, unsigned int port, const std::wstring& user,
        ISftpConsumer* consumer);

    boost::mutex::scoped_lock aquire_lock();

    bool is_dead();

    // This class really represents an SFTP channel rather than an
    // authenticated session.  Clients only use the session accessors
    // below to report errors and this will be replaced by the wrapper
    // sftp code which handles this internally.  Therefore we will be able
    // to remove these accessors from the public interface.
    ssh::session get_session() const;
    LIBSSH2_SESSION* get_raw_session();

    ssh::sftp::sftp_channel get_sftp_channel() const;
    LIBSSH2_SFTP* get_raw_sftp_channel()
    { return get_sftp_channel().get().get(); }

private:
    std::auto_ptr<running_session> m_session;
    ssh::sftp::sftp_channel m_sftp_channel;
};

}} // namespace swish::connection

#endif
