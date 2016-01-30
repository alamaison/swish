// Copyright 2010, 2011, 2016 Alexander Lamaison

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

#include "session_fixture.hpp"

#include <boost/system/system_error.hpp> // system_error
#include <boost/throw_exception.hpp>     // BOOST_THROW_EXCEPTION

#include <locale>
#include <sstream>   // basic_ostringstream
#include <stdexcept> // logic_error
#include <string>

using ssh::session;

using boost::system::system_error;

using std::auto_ptr;
using std::locale;
using std::string;
using std::ostringstream;
using std::logic_error;

namespace test
{
namespace ssh
{

namespace
{

/**
 * Locale-independent port number to port string conversion.
 */
string port_to_string(long port)
{
    ostringstream stream;
    stream.imbue(locale::classic()); // force locale-independence
    stream << port;
    if (!stream)
        BOOST_THROW_EXCEPTION(
            logic_error("Unable to convert port number to string"));

    return stream.str();
}
}

void open_socket_to_host(boost::asio::io_service& io,
                         boost::asio::ip::tcp::socket& socket,
                         const string& host_name, int port)
{
    using boost::asio::ip::tcp;

    tcp::resolver resolver(io);
    typedef tcp::resolver::query Lookup;
    Lookup query(host_name, port_to_string(port));

    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
    tcp::resolver::iterator end;

    boost::system::error_code error = boost::asio::error::host_not_found;
    while (error && endpoint_iterator != end)
    {
        socket.close();
        socket.connect(*endpoint_iterator++, error);
    }
    if (error)
        BOOST_THROW_EXCEPTION(system_error(error));
}

session_fixture::session_fixture()
    : m_io(0),
      m_socket(m_io),
      m_session(session(open_socket(host(), port()).native()))
{
}

session& session_fixture::test_session()
{
    return m_session;
}

auto_ptr<boost::asio::ip::tcp::socket>
session_fixture::connect_additional_socket()
{
    auto_ptr<boost::asio::ip::tcp::socket> socket(
        new boost::asio::ip::tcp::socket(m_io));

    open_socket_to_host(m_io, *socket, host(), port());

    return socket;
}

boost::asio::ip::tcp::socket&
session_fixture::open_socket(const string& host_name, int port)
{
    open_socket_to_host(m_io, m_socket, host_name, port);

    return m_socket;
}
}
} // namespace test::ssh
