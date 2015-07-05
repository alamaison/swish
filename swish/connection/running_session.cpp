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

#include "running_session.hpp"

#include "swish/remotelimits.h"
#include "swish/debug.hpp"        // Debug macros
#include "swish/port_conversion.hpp" // port_to_string
#include "swish/utils.hpp" // WideStringToUtf8String

#include <ssh/session.hpp>
#include <ssh/filesystem.hpp> // sftp_filesystem

#include <boost/asio/ip/tcp.hpp> // Boost sockets: only used for name resolving
#include <boost/move/move.hpp>
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
using boost::move;
using boost::shared_ptr;
using boost::system::get_system_category;
using boost::system::system_error;
using boost::system::error_code;

using std::string;
using std::wstring;


namespace swish {
namespace connection {

namespace {
    
    /**
     * Connect a socket to the given port on the given host.
     *
     * @throws  A boost::system::system_error if there is a failure.
     */
    void connect_socket_to_host(
        tcp::socket& socket, const wstring& host, unsigned int port,
        io_service& io)
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

        assert(socket.is_open());
        assert(socket.available() == 0);
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
    ssh::session session_on_socket(
        tcp::socket& socket, const wstring& host, unsigned int port,
        io_service& io, const string& disconnection_message)
    {
        connect_socket_to_host(socket, host, port, io);
        return ssh::session(socket.native(), disconnection_message);
    }
}

running_session::running_session(const wstring& host, unsigned int port)
: 
m_io(new io_service(0)), m_socket(new tcp::socket(*m_io)),
m_session(
     session_on_socket(*m_socket, host, port, *m_io, "Swish says goodbye."))
{}

running_session::running_session(BOOST_RV_REF(running_session) other)
:
m_io(move(other.m_io)),
m_socket(move(other.m_socket)), m_session(move(other.m_session)) {}

running_session& running_session::operator=(BOOST_RV_REF(running_session) other)
{
    swap(running_session(move(other)), *this);
    return *this;
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

}} // namespace swish::connection