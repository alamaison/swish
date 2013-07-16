/**
    @file

    SSH session object.

    @if license

    Copyright (C) 2010, 2012, 2013  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#ifndef SSH_SESSION_HPP
#define SSH_SESSION_HPP
#pragma once

#include "agent.hpp"
#include "exception.hpp" // ssh_error
#include "host_key.hpp" // host_key

#include <boost/exception/errinfo_api_function.hpp> // errinfo_api_function
#include <boost/exception/info.hpp> // errinfo_api_function
#include <boost/bind/bind.hpp>
#include <boost/filesystem/path.hpp> // path, used for key paths
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp> // shared_ptr
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <exception> // bad_alloc
#include <string>

#include <libssh2.h>

namespace ssh {

namespace detail {
    
    /**
     * Last error encountered by the session as an exception.
     */
    inline exception::ssh_error last_error(LIBSSH2_SESSION* session)
    {
        char* message_buf = NULL; // read-only reference
        int message_len = 0; // len not including NULL-term
        int err = libssh2_session_last_error(
            session, &message_buf, &message_len, false);

        assert(err && "throwing success!");

        return exception::ssh_error(message_buf, message_len, err);
    }

    namespace libssh2 {
    namespace session {

        /**
         * Thin exception wrapper around libssh2_session_init.
         */
        inline LIBSSH2_SESSION* init()
        {
            LIBSSH2_SESSION* session = libssh2_session_init_ex(
                NULL, NULL, NULL, NULL);
            if (!session)
                BOOST_THROW_EXCEPTION(
                    std::bad_alloc("Failed to allocate new ssh session"));

            return session;
        }

        /**
         * Thin exception wrapper around libssh2_session_startup.
         */
        inline void startup(LIBSSH2_SESSION* session, int socket)
        {
            int rc = libssh2_session_startup(session, socket);
            if (rc != 0)
            {
                // using low level (raw pointer) version of last_error because
                // the higher-level session object doesn't exist yet
                BOOST_THROW_EXCEPTION(
                    last_error(session) <<
                    boost::errinfo_api_function("libssh2_session_startup"));
            }
        }

        /**
         * Thin exception wrapper around libssh2_session_disconnect.
         */
        inline void disconnect(
            LIBSSH2_SESSION* session, const char* description)
        {
            int rc = libssh2_session_disconnect(session, description);
            if (rc != 0)
            {
                BOOST_THROW_EXCEPTION(
                    last_error(session) <<
                    boost::errinfo_api_function("libssh2_session_disconnect"));
            }
        }

    }

    namespace userauth {

        /**
         * Thin exception wrapper around libssh2_userauth_password_ex.
         */
        inline void password(
            boost::shared_ptr<LIBSSH2_SESSION> session, const char* username,
            size_t username_len, const char* password, size_t password_len,
            LIBSSH2_PASSWD_CHANGEREQ_FUNC((*passwd_change_cb)))
        {
            int rc = libssh2_userauth_password_ex(
                session.get(), username, username_len, password, password_len,
                passwd_change_cb);
            if (rc != 0)
                BOOST_THROW_EXCEPTION(
                    ssh::exception::last_error(session) <<
                    boost::errinfo_api_function(
                        "libssh2_userauth_password_ex"));
        }

        /**
         * Thin exception wrapper around libssh2_userauth_publickey_fromfile_ex.
         */
        inline void public_key_from_file(
            boost::shared_ptr<LIBSSH2_SESSION> session, const char* username,
            size_t username_len, const char* public_key_path,
            const char* private_key_path, const char* passphrase)
        {
            int rc = libssh2_userauth_publickey_fromfile_ex(
                session.get(), username, username_len, public_key_path,
                private_key_path, passphrase);
            if (rc != 0)
                BOOST_THROW_EXCEPTION(
                    ssh::exception::last_error(session) <<
                    boost::errinfo_api_function(
                        "libssh2_userauth_publickey_fromfile_ex"));
        }

    }}

    inline void disconnect_and_free(
        LIBSSH2_SESSION* session, const std::string& disconnection_message)
    {
        try
        {
            libssh2::session::disconnect(
                session, disconnection_message.c_str());
        }
        catch (const std::exception&)
        {
            // Ignore errors for two reasons:
            //  - this is used as a shared_ptr deallocater so may not throw.
            //  - even if there is a problem disconnecting the session,
            //    we must still free it as everything else is going away

            // TODO: introduce some way of logging them
        }

        libssh2_session_free(session);
    }


    inline boost::shared_ptr<LIBSSH2_SESSION> allocate_and_connect_session(
        int socket, const std::string& disconnection_message)
    {
        // This function is unlike most of the others in that we do not
        // immediately wrap the created resource (the LIBSSH2_SESSION*) in a
        // shared_ptr.  We only ever want to deal in terms of sessions that have
        // been allocated *and* connected so we wait until starting succeeds and
        // then wrap it in a shared_ptr that disconnects before it frees.
        //
        // This means that we have to be careful of the lifetime of
        // the unstarted session in the code below.  The session may fail
        // to start but must still be freed.

        LIBSSH2_SESSION* session = libssh2::session::init();

        // Session is 'alive' from this point onwards.  All paths must
        // eventually free it.

        try
        {
            libssh2::session::startup(session, socket);
        }
        catch (...)
        {
            libssh2_session_free(session);
            throw;
        }

        return boost::shared_ptr<LIBSSH2_SESSION>(
            session,
            boost::bind(disconnect_and_free, _1, disconnection_message));
    }

}

/**
 * An SSH session connected to a host.
 */
// The underlying session is both allocated and connected to the host by
// allocate_and_connect_session.  This function also makes sure it will be
// first disconnected before it is freed.  The implementation of `class session`
// need to worry about these details, they are already taken care of.
class session
{
public:
    
    /**
     * Start a new SSH session with a host.
     *
     * The host is listening on the other end of the given socket.
     *
     * The constructor will throw an exception if it cannot connect to the host
     * or negotiate an SSH session.  Therefore any instance of this class
     * begins life successfully connected to the host.  Of course, the
     * connection may break subsequently and the server is free to terminate
     * the session at any time.
     *
     * @param socket
     *     The socket through which to communicate with the listening server.
     * @param disconnection_message
     *     An optional message sent to the server when the session is
     *     destroyed.
     */
    session(
        int socket,
        const std::string& disconnection_message=
            "libssh2 C++ bindings session destructor") :
    m_session(
        detail::allocate_and_connect_session(socket, disconnection_message))
    {}

    boost::shared_ptr<LIBSSH2_SESSION> get() { return m_session; }

    /**
     * Hostkey sent by the server to identify itself.
     */
    ssh::host_key::host_key hostkey() const
    {
        return ssh::host_key::host_key(m_session);
    }

    bool authenticated() const
    {
        return libssh2_userauth_authenticated(m_session.get()) != 0;
    }

    /**
     * Simple password authentication.
     *
     * @todo  Handle password change callback.
     */
    void authenticate_by_password(
        const std::string& username, const std::string& password)
    {
        detail::libssh2::userauth::password(
            m_session, username.data(), username.size(), password.data(),
            password.size(), NULL);
    }

    /**
     * Public-key authentication.
     *
     * This method requires a path to both the public and private keys because
     * libssh2 does.  It should be possible to derive one from the other so
     * when libssh2 supports this the method will take one fewer argument.
     */
    void authenticate_by_key(
        const std::string& username, const boost::filesystem::path& public_key,
        const boost::filesystem::path& private_key,
        const std::string& passphrase)
    {
        detail::libssh2::userauth::public_key_from_file(
            m_session, username.data(), username.size(),
            public_key.external_file_string().c_str(),
            private_key.external_file_string().c_str(), passphrase.c_str());
    }

    /**
     * Connect to any agent running on the system and return object to
     * authenticate using its identities.
     */
    agent::agent_identities agent_identities()
    {
        return agent::agent_identities(m_session);
    }

private:
    boost::shared_ptr<LIBSSH2_SESSION> m_session;
};

} // namespace ssh

#endif
