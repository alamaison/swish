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

#include "swish/provider/Provider.hpp" // CProvider

#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <stdexcept> // invalid_argument

using swish::provider::CProvider;
using swish::provider::sftp_provider;

using boost::shared_ptr;

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

shared_ptr<sftp_provider> connection_spec::create_session() const
{
    return shared_ptr<sftp_provider>(new CProvider(m_user, m_host, m_port));
}

bool connection_spec::operator<(const connection_spec& other) const
{
    if (m_host < other.m_host)
        return true;
    else if (m_user < other.m_user)
        return true;
    else if (m_port < other.m_port)
        return true;
    else
        return false;
}

}} // namespace swish::connection
