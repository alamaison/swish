/**
    @file

    Connected session fixture.

    @if license

    Copyright (C) 2010, 2011  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#ifndef SSH_SESSION_FIXTURE_HPP
#define SSH_SESSION_FIXTURE_HPP
#pragma once

#include "openssh_fixture.hpp" // openssh_fixture

#include <ssh/session.hpp> // session

#include <boost/asio/ip/tcp.hpp> // Boost sockets
#include <boost/system/system_error.hpp> // system_error
#include <boost/test/unit_test.hpp>
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <locale> // locale::classic
#include <sstream> // basic_ostringstream
#include <stdexcept> // logic_error
#include <string>

namespace test {
namespace ssh {

namespace detail {

/**
 * Locale-independent port number to port string conversion.
 */
inline std::string port_to_string(long port)
{
	std::ostringstream stream;
	stream.imbue(std::locale::classic()); // force locale-independence
	stream << port;
	if (!stream)
		BOOST_THROW_EXCEPTION(
			std::logic_error("Unable to convert port number to string"));

	return stream.str();
}

}

/**
 * Fixture serving ssh::session objects connected to a running server.
 */
class session_fixture : public openssh_fixture
{
public:
	session_fixture() : m_io(0), m_socket(m_io) {}

	boost::asio::ip::tcp::socket& open_socket(
		const std::string host_name, int port)
	{
		using boost::asio::ip::tcp;

		tcp::resolver resolver(m_io);
		typedef tcp::resolver::query Lookup;
		Lookup query(host_name, detail::port_to_string(port));

		tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
		tcp::resolver::iterator end;

		boost::system::error_code error = boost::asio::error::host_not_found;
		while (error && endpoint_iterator != end)
		{
			m_socket.close();
			m_socket.connect(*endpoint_iterator++, error);
		}
		if (error)
			BOOST_THROW_EXCEPTION(boost::system::system_error(error));

		return m_socket;
	}

	::ssh::session test_session()
	{
		boost::asio::ip::tcp::socket& sock = open_socket(host(), port());
		return ::ssh::session(sock.native());
	}

private:
	boost::asio::io_service m_io; ///< Boost IO system
	boost::asio::ip::tcp::socket m_socket;
};

}} // namespace test::ssh

#endif
