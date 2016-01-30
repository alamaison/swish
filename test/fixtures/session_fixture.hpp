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

#ifndef SWISH_TEST_FIXTURES_SESSION_FIXTURE_HPP
#define SWISH_TEST_FIXTURES_SESSION_FIXTURE_HPP

#include "openssh_fixture.hpp"

#include <ssh/session.hpp>

#include <boost/asio/ip/tcp.hpp> // Boost sockets

#include <string>

namespace test
{
namespace fixtures
{

/**
 * Fixture serving ssh::session objects connected to a running server.
 */
class session_fixture : virtual public openssh_fixture
{
public:
    session_fixture();

    ssh::session& test_session();

    std::auto_ptr<boost::asio::ip::tcp::socket> connect_additional_socket();

private:
    boost::asio::ip::tcp::socket& open_socket(const std::string& host_name,
                                              int port);

    boost::asio::io_service m_io; ///< Boost IO system
    boost::asio::ip::tcp::socket m_socket;
    ssh::session m_session;
};
}
} // namespace test::fixtures

#endif
