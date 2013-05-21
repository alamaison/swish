/**
    @file

    Libssh2 SSH and SFTP session management

    @if license

    Copyright (C) 2008, 2009, 2010, 2011, 2013
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

#include "Session.hpp"

#include "swish/remotelimits.h"
#include "swish/debug.hpp"        // Debug macros
#include "swish/port_conversion.hpp" // port_to_string
#include "swish/utils.hpp" // WideStringToUtf8String

#include <libssh2.h>
#include <libssh2_sftp.h>

#include <boost/asio/ip/tcp.hpp> // Boost sockets: only used for name resolving
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <string>

using swish::port_to_string;
using swish::utils::WideStringToUtf8String;

using boost::asio::ip::tcp;
using boost::asio::error::host_not_found;
using boost::mutex;
using boost::shared_ptr;
using boost::system::get_system_category;
using boost::system::system_error;
using boost::system::error_code;

using std::string;

CSession::CSession() : 
    m_io(0), m_socket(m_io), m_bConnected(false)
{
    _CreateSession();
    ATLASSUME(m_session);
}

CSession::~CSession()
{
    _DestroySftpChannel();
    _DestroySession();
}


mutex::scoped_lock CSession::aquire_lock()
{
    return mutex::scoped_lock(m_mutex);
}

CSession::operator LIBSSH2_SESSION*() const
{
    ATLASSUME(m_session);
    return m_session.get();
}

CSession::operator LIBSSH2_SFTP*() const
{
    ATLASSUME(m_sftp_session);
    return m_sftp_session.get();
}

/**
 * Has the connection broken since we connected?
 *
 * This only gives the correct answer as long as we're not expecting data
 * to arrive on the socket. select()ing a silent socket should return 0. If it
 * doesn't, it indicates that the connection is broken.
 *
 * XXX: we could double-check this by reading from the socket.  It would return
 *      0 if the socket is closed.
 *
 * @see http://www.libssh2.org/mail/libssh2-devel-archive-2010-07/0050.shtml
 */
bool CSession::IsDead()
{
    fd_set socket_set;
    FD_ZERO(&socket_set);
    FD_SET(m_socket.native(), &socket_set);
    TIMEVAL tv = TIMEVAL();

    int rc = ::select(1, &socket_set, NULL, NULL, &tv);
    if (rc < 0)
        BOOST_THROW_EXCEPTION(
            system_error(::WSAGetLastError(), get_system_category()));
    return rc != 0;
}

void CSession::Connect(PCWSTR pwszHost, unsigned int uPort) throw(...)
{
    if (m_bConnected)
        BOOST_THROW_EXCEPTION(std::logic_error("Already connected"));
    
    // Connect to host over TCP/IP
    _OpenSocketToHost(pwszHost, uPort);

    // Start up libssh2 and trade welcome banners, exchange keys,
    // setup crypto, compression, and MAC layers
    ATLASSERT(m_socket.native() != INVALID_SOCKET);
    if (libssh2_session_startup(*this, static_cast<int>(m_socket.native())) != 0)
    {
        char *szError;
        int cchError;
        libssh2_session_last_error(*this, &szError, &cchError, false);

        _ResetSession();
        _CloseSocketToHost();
    
        BOOST_THROW_EXCEPTION(std::exception(szError));
        // Legal to fail here, e.g. server refuses banner/kex
    }
    
    // Tell libssh2 we are blocking
    libssh2_session_set_blocking(*this, 1);

    m_bConnected = true;
}

void CSession::Disconnect()
{
    if (!m_bConnected)
        return;

    libssh2_session_disconnect(m_session.get(), "Swish says goodbye.");
    m_bConnected = false;
}

void CSession::StartSftp() throw(...)
{
    _CreateSftpChannel();
}


/*----------------------------------------------------------------------------*
 * Private methods
 *----------------------------------------------------------------------------*/

/**
 * Allocate a blocking LIBSSH2_SESSION instance.
 */
void CSession::_CreateSession() throw(...)
{
    // Create a session instance
    m_session = shared_ptr<LIBSSH2_SESSION>(
        libssh2_session_init(), libssh2_session_free);
    ATLENSURE_THROW( m_session, E_FAIL );
}

/**
 * Free a LIBSSH2_SESSION instance.
 */
void CSession::_DestroySession() throw()
{
    ATLASSUME(m_session);
    if (m_session)
    {
        Disconnect();
    }
}

/**
 * Destroy and recreate a LIBSSH2_SESSION instance.
 *
 * A session instance which has been used in a libssh2_session_startup call
 * cannot be reused safely.
 */
void CSession::_ResetSession() throw(...)
{
    _DestroySession();
    _DestroySftpChannel();
    _CreateSession();
}

/**
 * Start up an SFTP channel on this SSH session.
 */
void CSession::_CreateSftpChannel() throw(...)
{
    ATLASSUME(m_sftp_session == NULL);

    if (libssh2_userauth_authenticated(*this) == 0)
        AtlThrow(E_UNEXPECTED); // We must be authenticated first

    LIBSSH2_SFTP* sftp = libssh2_sftp_init(*this); // Start up SFTP session
    if (!sftp)
    {
#ifdef _DEBUG
        char *szError;
        int cchError;
        int rc = libssh2_session_last_error(*this, &szError, &cchError, false);
        ATLTRACE("libssh2_sftp_init failed (%d): %s", rc, szError);
#endif
        AtlThrow(E_FAIL);
    }
    
    m_sftp_session = shared_ptr<LIBSSH2_SFTP>(sftp, libssh2_sftp_shutdown);
}

/**
 * Shut down the SFTP channel.
 */
void CSession::_DestroySftpChannel() throw()
{
    m_sftp_session.reset();
}

/**
 * Creates a socket and connects it to the host.
 *
 * The socket is stored as the member variable @c m_socket. The hostname 
 * and port used are passed as parameters.
 *
 * @throws  A boost::system::system_error if there is a failure.
 *
 * @remarks The socket should be cleaned up when no longer needed using
 *          @c _CloseSocketToHost()
 */
void CSession::_OpenSocketToHost(PCWSTR pwszHost, unsigned int uPort)
{
    ATLASSERT(pwszHost[0] != '\0');
    ATLASSERT(uPort >= MIN_PORT && uPort <= MAX_PORT);

    // Convert host address to a UTF-8 string
    string host_name = WideStringToUtf8String(pwszHost);

    tcp::resolver resolver(m_io);
    typedef tcp::resolver::query Lookup;
    Lookup query(host_name, port_to_string(uPort));

    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
    tcp::resolver::iterator end;

    error_code error = host_not_found;
    while (error && endpoint_iterator != end)
    {
        m_socket.close();
        m_socket.connect(*endpoint_iterator++, error);
    }
    if (error)
        BOOST_THROW_EXCEPTION(system_error(error));

    assert(m_socket.is_open());
    assert(m_socket.available() == 0);
}

/**
 * Closes the socket stored in @c m_socket and sets is to @c INVALID_SOCKET.
 */
void CSession::_CloseSocketToHost() throw()
{
    m_socket.close();
}