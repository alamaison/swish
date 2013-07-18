/**
    @file

    SSH session authentication.

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

#include "authenticated_session.hpp"

#include "swish/remotelimits.h"
#include "swish/debug.hpp"        // Debug macros
#include "swish/port_conversion.hpp" // port_to_string
#include "swish/utils.hpp" // WideStringToUtf8String

#include <ssh/session.hpp>
#include <ssh/sftp.hpp> // sftp_channel

#include <libssh2.h>
#include <libssh2_sftp.h>

#include <boost/asio/ip/tcp.hpp> // Boost sockets: only used for name resolving
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <cassert>
#include <string>

using swish::port_to_string;
using swish::utils::WideStringToUtf8String;

using ssh::session;
using ssh::sftp::sftp_channel;

using boost::asio::error::host_not_found;
using boost::asio::io_service;
using boost::asio::ip::tcp;
using boost::mutex;
using boost::shared_ptr;
using boost::system::get_system_category;
using boost::system::system_error;
using boost::system::error_code;

using std::string;
using std::wstring;


namespace swish {
namespace connection {


session authenticated_session::get_session() const
{
    return m_session->get_session();
}

LIBSSH2_SESSION* authenticated_session::get_raw_session()
{
    return m_session->get_raw_session();
}

sftp_channel authenticated_session::get_sftp_channel() const
{
    return m_sftp_channel;
}

mutex::scoped_lock authenticated_session::aquire_lock()
{
    return m_session->aquire_lock();
}

bool authenticated_session::is_dead()
{
   return m_session->is_dead();
}

}} // namespace swish::connection