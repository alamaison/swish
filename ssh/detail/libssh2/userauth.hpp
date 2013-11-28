/**
    @file

    Exception wrapper round raw libssh2 userauth functions.

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

#ifndef SSH_DETAIL_LIBSSH2_USERAUTH_HPP
#define SSH_DETAIL_LIBSSH2_USERAUTH_HPP

#include <ssh/ssh_error.hpp> // last_error

#include <boost/exception/info.hpp> // errinfo_api_function
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <libssh2.h> // LIBSSH2_SESSION, libssh2_userauth_*

namespace ssh {
namespace detail {
namespace libssh2 {
namespace userauth {

/**
 * Thin exception wrapper around libssh2_userauth_password_ex.
 *
 * The incorrect password failure is not reported as an exception
 * because it is expected.
 */
inline int password(
    LIBSSH2_SESSION* session, const char* username,
    size_t username_len, const char* password, size_t password_len,
    LIBSSH2_PASSWD_CHANGEREQ_FUNC((*passwd_change_cb)))
{
    int rc = ::libssh2_userauth_password_ex(
        session, username, username_len, password, password_len,
        passwd_change_cb);

    if (rc != 0 && rc != LIBSSH2_ERROR_AUTHENTICATION_FAILED)
    {
        BOOST_THROW_EXCEPTION(
            last_error(session) <<
            boost::errinfo_api_function("libssh2_userauth_password_ex"));
    }
    else
    {
        return rc;
    }
}

/**
 * Thin exception wrapper around libssh2_userauth_publickey_fromfile_ex.
 */
inline void public_key_from_file(
    LIBSSH2_SESSION* session, const char* username,
    size_t username_len, const char* public_key_path,
    const char* private_key_path, const char* passphrase)
{
    int rc = libssh2_userauth_publickey_fromfile_ex(
        session, username, username_len, public_key_path, private_key_path,
        passphrase);
    if (rc != 0)
        BOOST_THROW_EXCEPTION(
            last_error(session) <<
            boost::errinfo_api_function(
                "libssh2_userauth_publickey_fromfile_ex"));
}

}}}} // namespace ssh::detail::libssh2::userauth

#endif
