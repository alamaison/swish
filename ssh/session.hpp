/**
    @file

    SSH session object.

    @if licence

    Copyright (C) 2010  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "exception.hpp" // last_error
#include "host_key.hpp" // host_key

#include <boost/exception/errinfo_api_function.hpp> // errinfo_api_function
#include <boost/exception/info.hpp> // errinfo_api_function
#include <boost/shared_ptr.hpp> // shared_ptr
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <exception> // bad_alloc
#include <string>

#include <libssh2.h>

namespace ssh {
namespace session {

namespace detail {
	namespace libssh2 {
	namespace session {

		/**
		 * Thin exception wrapper around libssh2_session_init.
		 */
		boost::shared_ptr<LIBSSH2_SESSION> init()
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
		void startup(
			boost::shared_ptr<LIBSSH2_SESSION> session, int socket)
		{
			int rc = libssh2_session_startup(session.get(), socket);
			if (rc)
				BOOST_THROW_EXCEPTION(
					ssh::exception::last_error(session) <<
					boost::errinfo_api_function("libssh2_session_startup"));
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

private:
	boost::shared_ptr<LIBSSH2_SESSION> m_session;
};

}} // namespace ssh::session

#endif
