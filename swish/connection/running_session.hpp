// Copyright 2008, 2009, 2010, 2013, 2016 Alexander Lamaison

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

#ifndef SWISH_CONNECTION_RUNNING_SESSION_HPP
#define SWISH_CONNECTION_RUNNING_SESSION_HPP

#include <ssh/session.hpp>

#include <boost/asio/ip/tcp.hpp> // Boost sockets

#include <memory>
#include <string>

namespace swish
{
namespace connection
{

/**
 * An SSH session connected to a port on a server.
 *
 * The session may or may not be authenticated.
 *
 * The point of this class is to add host resolution and death detection
 * to the existing libssh2 C++ binding session object.
 */
class running_session
{
public:
    /**
     * Connect to host server and start new SSH connection on given port.
     */
    running_session(const std::wstring& host, unsigned int port);

    /**
     * Has the connection broken since we connected?
     *
     * This only gives the correct answer as long as we're not expecting data
     * to arrive on the socket. select()ing a silent socket should return 0.
     * If it doesn't, it indicates that the connection is broken.
     *
     * XXX: we could double-check this by reading from the socket.  It would
     * return
     *      0 if the socket is closed.
     *
     * @see http://www.libssh2.org/mail/libssh2-devel-archive-2010-07/0050.shtml
     */
    bool is_dead();

    ssh::session& get_session();

    friend void swap(running_session& lhs, running_session& rhs);

private:
    // Must use unique_ptr for these members to make our class movable because
    // Boost.ASIO doesn't support move emulation

    std::unique_ptr<boost::asio::io_service> m_io;
    ///< Boost IO system

    std::unique_ptr<boost::asio::ip::tcp::socket> m_socket;
    ///< TCP/IP socket to remote host

    ssh::session m_session;
    ///< libssh2 session
};
}
} // namespace swish::connection

#endif
