/**
    @file

    Tests for session existence.

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

#include "openssh_fixture.hpp"
#include "session_fixture.hpp" // open_socket_to_host

#include <ssh/session.hpp> // test subject

#include <boost/move/move.hpp>
#include <boost/test/unit_test.hpp>

using ssh::session;

using test::ssh::openssh_fixture;
using test::ssh::open_socket_to_host;

using boost::asio::io_service;
using boost::asio::ip::tcp;
using boost::move;

BOOST_FIXTURE_TEST_SUITE(session_tests, openssh_fixture)

BOOST_AUTO_TEST_CASE(default_message)
{
    io_service io;
    tcp::socket socket(io);
    open_socket_to_host(io, socket, host(), port());
    session s(socket.native());
}

BOOST_AUTO_TEST_CASE(custom_message)
{
    io_service io;
    tcp::socket socket(io);
    open_socket_to_host(io, socket, host(), port());
    session s(socket.native(), "blah");
}

BOOST_AUTO_TEST_CASE(swap)
{
    io_service io;

    // BOTH sockets must be created before first session.  Once swapped,
    // second socket is used by first session, so must outlive it.
    tcp::socket socket1(io);
    tcp::socket socket2(io);

    open_socket_to_host(io, socket1, host(), port());
    session s1(socket1.native());

    open_socket_to_host(io, socket2, host(), port());
    session s2(socket2.native());

    boost::swap(s1, s2);
}

BOOST_AUTO_TEST_CASE(move_construct)
{
    io_service io;
    tcp::socket socket(io);
    open_socket_to_host(io, socket, host(), port());
    session s1(socket.native());

    session s2(move(s1));
}

BOOST_AUTO_TEST_CASE(move_assign)
{
    io_service io;

    tcp::socket socket1(io);
    open_socket_to_host(io, socket1, host(), port());
    session s1(socket1.native());

    tcp::socket socket2(io);
    open_socket_to_host(io, socket2, host(), port());
    session s2(socket2.native());

    s2 = move(s1);
}

BOOST_AUTO_TEST_SUITE_END();
