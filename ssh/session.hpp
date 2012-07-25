/**
    @file

    SSH session object.

    @if license

    Copyright (C) 2010, 2012  Alexander Lamaison <awl03@doc.ic.ac.uk>

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
#include "exception.hpp" // last_error
#include "host_key.hpp" // host_key

#include <boost/exception/errinfo_api_function.hpp> // errinfo_api_function
#include <boost/exception/info.hpp> // errinfo_api_function
#include <boost/filesystem/path.hpp> // path, used for key paths
#include <boost/shared_ptr.hpp> // shared_ptr
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <exception> // bad_alloc
#include <string>

#include <libssh2.h>

namespace ssh {

namespace detail {
	namespace libssh2 {
	namespace session {

		/**
		 * Thin exception wrapper around libssh2_session_init.
		 */
		inline boost::shared_ptr<LIBSSH2_SESSION> init()
		{
			boost::shared_ptr<LIBSSH2_SESSION> session(
				libssh2_session_init_ex(NULL, NULL, NULL, NULL),
				libssh2_session_free);
			if (!session)
				BOOST_THROW_EXCEPTION(
					std::bad_alloc("Failed to allocate new ssh session"));

			return session;
		}

		/**
		 * Thin exception wrapper around libssh2_session_startup.
		 */
		inline void startup(
			boost::shared_ptr<LIBSSH2_SESSION> session, int socket)
		{
			int rc = libssh2_session_startup(session.get(), socket);
			if (rc != 0)
				BOOST_THROW_EXCEPTION(
					ssh::exception::last_error(session) <<
					boost::errinfo_api_function("libssh2_session_startup"));
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
}

class session
{
public:
	session(int socket) : m_session(detail::libssh2::session::init())
	{
		detail::libssh2::session::startup(m_session, socket);
	}

	session(boost::shared_ptr<LIBSSH2_SESSION> session) : m_session(session) {}

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
