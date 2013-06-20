/**
    @file

    Pool of reusuable SFTP connections.

    @if license

    Copyright (C) 2007, 2008, 2009, 2010, 2011, 2012
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

    @endif
*/

#ifndef SWISH_CONNECTION_CONNECTION_HPP
#define SWISH_CONNECTION_CONNECTION_HPP
#pragma once

#include "swish/provider/sftp_provider.hpp" // sftp_provider

#include <boost/detail/scoped_enum_emulation.hpp> // BOOST_SCOPED_ENUM
#include <boost/shared_ptr.hpp>

#include <string>

namespace swish {
namespace connection {


/**
 * Represents specification for a connection to an SFTP server.
 *
 * Instances of this class are just recipes for connecting, they are *not*
 * the running connections themselves.  Running connections are called
 * sessions and can be created and queried via this class.
 */
class connection_spec
{
public:

    connection_spec(
        const std::wstring& host, const std::wstring& user, int port);

    /**
     * Returns a running SFTP session based on this specification.
     * 
     * If an appropriate SFTP session already exists in the global pool,
     * the connection is reused.  Otherwise a new one is created, and added
     * to the pool.
     */
    boost::shared_ptr<swish::provider::sftp_provider> pooled_session() const;

    BOOST_SCOPED_ENUM_START(session_status)
    {
        running,
        not_running
    };
    BOOST_SCOPED_ENUM_END;

    /**
     * The status of a connection with this specification.
     *
     * Indicates whether the session matches one already running or whether
     * the session would need to to be created anew, should the caller decide to
     * call pooled_session().
     */
    BOOST_SCOPED_ENUM(session_status) session_status() const;

private:
    std::wstring m_host;
    std::wstring m_user;
    int m_port;
};

/**
 * Interface for connection making logic.
 *
 * Connection strategy is not uniform.  Sometime we want to establish a running
 * connection and pass that into an object so that it can use it at will.
 * Other times we want to the connection to be established---an activity that
 * may disturb the user with dialogues---as late as possible just before it
 * will be used.
 *
 * This interface abstracts such decisions behind a uniform way to request a
 * connection.
 */
/*class connection_maker
{
public:
    virtual ~connection_maker() = 0;
    virtual boost::shared_ptr<swish::provider::sftp_provider> provider() = 0;
    virtual comet::com_ptr<ISftpConsumer> consumer() = 0;
};*/

}} // namespace swish::remote_folder

#endif
