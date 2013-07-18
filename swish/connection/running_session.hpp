/**
    @file

    C++ wrapper round Libssh2 SSH and SFTP session creation.

    @if license

    Copyright (C) 2008, 2009, 2010, 2013
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

#ifndef SWISH_CONNECTION_RUNNING_SESSION_HPP
#define SWISH_CONNECTION_RUNNING_SESSION_HPP

#include <ssh/session.hpp>

#include <boost/asio/ip/tcp.hpp> // Boost sockets
#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>

#include <string>

namespace swish {
namespace connection {

/**
 * An SSH session connected to a port on a server.
 *
 * The session may or may not be authenticated.
 *
 * The point of this class is to add host resolution and death detection
 * to the existing libssh2 C++ binding session object.
 */
class running_session : private boost::noncopyable
{
    // Would like to use move-emulation here but can't because Boost.ASIO
    // doesn't support it.

public:

    /**
     * Connect to host server and start new SSH connection on given port.
     */
    running_session(const std::wstring& host, unsigned int port);

    boost::mutex::scoped_lock aquire_lock();

    /**
     * Has the connection broken since we connected?
     *
     * This only gives the correct answer as long as we're not expecting data
     * to arrive on the socket. select()ing a silent socket should return 0. 
     * If it doesn't, it indicates that the connection is broken.
     *
     * XXX: we could double-check this by reading from the socket.  It would return
     *      0 if the socket is closed.
     *
     * @see http://www.libssh2.org/mail/libssh2-devel-archive-2010-07/0050.shtml
     */
    bool is_dead();

    ssh::session get_session() const;
    LIBSSH2_SESSION* get_raw_session();

private:
    boost::mutex m_mutex;
    boost::asio::io_service m_io; ///< Boost IO system
    boost::asio::ip::tcp::socket m_socket; ///< TCP/IP socket to remote host
    ssh::session m_session;
};

}} // namespace swish::connection

#endif
