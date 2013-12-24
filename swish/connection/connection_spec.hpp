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

#ifndef SWISH_CONNECTION_CONNECTION_SPEC_HPP
#define SWISH_CONNECTION_CONNECTION_SPEC_HPP
#pragma once

#include "swish/provider/sftp_provider.hpp" // ISftpConsumer

#include <comet/ptr.h> // com_ptr

#include <string>

namespace swish {
namespace connection {

// Forward-declaring this here, rather than including the header, prevents
// test/remote_folder/remote_commands_test.cpp from crashing the compiler by
// letting that file not include the ssh_error.hpp file that uses lexical_cast
// and through there.  I don't know why.  Just go with it.
class authenticated_session;

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
     * Returns a new SFTP session based on this specification.
     *
     * The returned session is authenticated ready for use.  Any
     * interaction needed to authenticate is performed via the `consumer`
     * callback.
     */
    authenticated_session create_session(
        comet::com_ptr<ISftpConsumer> consumer) const;

    bool operator<(const connection_spec& other) const;

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
