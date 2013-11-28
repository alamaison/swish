/**
    @file

    Exception wrapper round raw libssh2 agent functions.

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

#ifndef SSH_DETAIL_LIBSSH2_AGENT_HPP
#define SSH_DETAIL_LIBSSH2_AGENT_HPP

#include <ssh/ssh_error.hpp> // last_error

#include <boost/exception/info.hpp> // errinfo_api_function
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <cassert> // assert

#include <libssh2.h> // LIBSSH2_SESSION, LIBSSH2_AGENT, libssh2_agent_*

// See ssh/detail/libssh2/libssh2.hpp for rules governing functions in this
// namespace

namespace ssh {
namespace detail {
namespace libssh2 {
namespace agent {

/**
 * Thin wrapper around libssh2_agent_init.
 */
inline LIBSSH2_AGENT* init(LIBSSH2_SESSION* session)
{
    LIBSSH2_AGENT* agent = ::libssh2_agent_init(session);
    if (!agent)
        BOOST_THROW_EXCEPTION(
            ssh::detail::last_error(session) <<
            boost::errinfo_api_function("libssh2_agent_init"));

    return agent;
}

/**
 * Thin wrapper around libssh2_agent_connect.
 */
inline void connect(LIBSSH2_AGENT* agent, LIBSSH2_SESSION* session)
{
    int rc = ::libssh2_agent_connect(agent);
    if (rc < 0)
        BOOST_THROW_EXCEPTION(
            ssh::detail::last_error(session) <<
            boost::errinfo_api_function("libssh2_agent_connect"));
}

/**
 * Thin wrapper around libssh2_agent_get_identity.
 */
inline bool get_identity(
    LIBSSH2_AGENT* agent, LIBSSH2_SESSION* session,
    libssh2_agent_publickey** out, libssh2_agent_publickey* previous)
{
    int rc = ::libssh2_agent_get_identity(agent, out, previous);
    if (rc < 0)
        BOOST_THROW_EXCEPTION(
            ssh::detail::last_error(session) <<
            boost::errinfo_api_function("libssh2_agent_get_identity"));

    return rc != 0;
}

/**
 * Thin wrapper around libssh2_agent_list_identities.
 */
inline void list_identities(LIBSSH2_AGENT* agent, LIBSSH2_SESSION* session)
{
    int rc = ::libssh2_agent_list_identities(agent);
    if (rc < 0)
        BOOST_THROW_EXCEPTION(
            ssh::detail::last_error(session) <<
            boost::errinfo_api_function("libssh2_agent_list_identities"));
}

/**
 * Thin wrapper around libssh2_agent_userauth.
 */
inline void userauth(
    LIBSSH2_AGENT* agent, LIBSSH2_SESSION* session,
    const std::string& user_name, libssh2_agent_publickey* identity)
{
    int rc = ::libssh2_agent_userauth(agent, user_name.c_str(), identity);
    if (rc < 0)
        BOOST_THROW_EXCEPTION(
            ssh::detail::last_error(session) <<
            boost::errinfo_api_function("libssh2_agent_userauth"));
}

}}}} // namespace ssh::detail::libssh2::agent

#endif
