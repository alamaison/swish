// Copyright 2008, 2009, 2010, 2011, 2013, 2016 Alexander Lamaison

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

#include "running_session.hpp"

#include "swish/debug.hpp"           // Debug macros
#include "swish/port_conversion.hpp" // port_to_string
#include "swish/remotelimits.h"
#include "swish/utils.hpp" // WideStringToUtf8String

#include <ssh/filesystem.hpp> // sftp_filesystem
#include <ssh/session.hpp>

#include <boost/asio/ip/tcp.hpp> // Boost sockets: only used for name resolving
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <cassert>
#include <string>

using swish::port_to_string;
using swish::utils::WideStringToUtf8String;

using ssh::session;
using ssh::filesystem::sftp_filesystem;

using boost::asio::error::host_not_found;
using boost::asio::io_service;
using boost::asio::ip::tcp;
using boost::shared_ptr;
using boost::system::get_system_category;
using boost::system::system_error;
using boost::system::error_code;

using std::string;
using std::wstring;

namespace swish
{
namespace connection
{

namespace
{

/**
 * Connect a socket to the given port on the given host.
 *
 * @throws  A boost::system::system_error if there is a failure.
 */
void connect_socket_to_host(tcp::socket& socket, const wstring& host,
                            unsigned int port, io_service& io)
{
    assert(!host.empty());
    assert(host[0] != L'\0');

    // Convert host address to a UTF-8 string
    string host_name = WideStringToUtf8String(host);

    tcp::resolver resolver(io);
    typedef tcp::resolver::query Lookup;
    Lookup query(host_name, port_to_string(port));

    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
    tcp::resolver::iterator end;

    error_code error = host_not_found;
    while (error && endpoint_iterator != end)
    {
        socket.close();
        socket.connect(*endpoint_iterator++, error);
    }
    if (error)
        BOOST_THROW_EXCEPTION(system_error(error));
}

// We have to have this weird function because Boost.ASIO doesn't
// support Boost.Move rvalue-emulation.
//
// Ideally, connect_socket_to_host would return the connected sockets
// so could be used in the running_session initialiser list below.  And
// the the initialiser for m_session would have a valid socket to use.
//
// But as we can't return the valid socket, we have to connect it *during*
// the m_session initialisation, *after* the m_socket initialisation. Yuk!
ssh::session session_on_socket(tcp::socket& socket, const wstring& host,
                               unsigned int port, io_service& io,
                               const string& disconnection_message)
{
    connect_socket_to_host(socket, host, port, io);
    return ssh::session(socket.native(), disconnection_message);
}
}

running_session::running_session(const wstring& host, unsigned int port)
    : m_io(new io_service(0)),
      m_socket(new tcp::socket(*m_io)),
      m_session(session_on_socket(*m_socket, host, port, *m_io,
                                  "Swish says goodbye."))
{
}

session& running_session::get_session()
{
    return m_session;
}

bool running_session::is_dead()
{
    fd_set socket_set;
    FD_ZERO(&socket_set);
    FD_SET(m_socket->native(), &socket_set);
    TIMEVAL tv = TIMEVAL();

    int rc = ::select(1, &socket_set, NULL, NULL, &tv);
    if (rc < 0)
        BOOST_THROW_EXCEPTION(
            system_error(::WSAGetLastError(), get_system_category()));
    return rc != 0;
}

void swap(running_session& lhs, running_session& rhs)
{
    boost::swap(lhs.m_io, rhs.m_io);
    boost::swap(lhs.m_socket, rhs.m_socket);
    boost::swap(lhs.m_session, rhs.m_session);
}
}
} // namespace swish::connection
