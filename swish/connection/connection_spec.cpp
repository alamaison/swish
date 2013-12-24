/**
    @file

    Specify a connection.

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

    @endif
*/

#include "connection_spec.hpp"

#include "swish/connection/authenticated_session.hpp"

#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION
#include <boost/tuple/tuple.hpp> // tie
#include <boost/tuple/tuple_comparison.hpp> // <

#include <stdexcept> // invalid_argument

using comet::com_ptr;

using boost::tie;

using std::invalid_argument;
using std::wstring;


namespace swish {
namespace connection {

connection_spec::connection_spec(
    const wstring& host, const wstring& user, const int port)
: m_host(host), m_user(user), m_port(port)
{
    if (host.empty())
        BOOST_THROW_EXCEPTION(invalid_argument("Host name required"));
    if (user.empty())
        BOOST_THROW_EXCEPTION(invalid_argument("User name required"));
}

authenticated_session connection_spec::create_session(
    com_ptr<ISftpConsumer> consumer) const
{
    return authenticated_session(m_host, m_port, m_user, consumer);
}

bool connection_spec::operator<(const connection_spec& other) const
{
    // Reusing comparison from tuples - no point reinventing the wheel
    // See: http://stackoverflow.com/q/6218812/67013
    return tie(m_host, m_user, m_port) <
        tie(other.m_host, other.m_user, other.m_port);
}

}} // namespace swish::connection
