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

#include <ssh/ssh_error.hpp> // last_error_code, SSH_DETAIL_THROW_API_ERROR_CODE

#include <boost/exception/info.hpp> // errinfo_api_function
#include <boost/optional/optional.hpp>
#include <boost/system/error_code.hpp>
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <libssh2.h> // LIBSSH2_SESSION, libssh2_userauth_*

// See ssh/detail/libssh2/libssh2.hpp for rules governing functions in this
// namespace

namespace ssh
{
namespace detail
{
namespace libssh2
{
namespace userauth
{

/**
 * Error-fetching wrapper around libssh2_userauth_list.
 *
 * May return NULL if authentication succeeded with 'none' method.  In this
 * case 'ec == false'.
 */
inline const char*
list(LIBSSH2_SESSION* session, const char* username, unsigned int username_len,
     boost::system::error_code& ec,
     boost::optional<std::string&> e_msg = boost::optional<std::string&>())
{
    const char* method_list =
        ::libssh2_userauth_list(session, username, username_len);

    if (!method_list)
    {
        ec = ssh::detail::last_error_code(session, e_msg);
    }

    return method_list;
}

/**
 * Exception wrapper around libssh2_userauth_list.
 *
 * Returns NULL if authentication succeeded with 'none' method.
 */
inline const char* list(LIBSSH2_SESSION* session, const char* username,
                        unsigned int username_len)
{
    boost::system::error_code ec;
    std::string message;

    const char* method_list =
        list(session, username, username_len, ec, message);

    if (ec)
    {
        SSH_DETAIL_THROW_API_ERROR_CODE(ec, message, "libssh2_userauth_list");
    }

    return method_list;
}

/**
 * Error-fetching wrapper around libssh2_userauth_password_ex.
 */
inline void
password(LIBSSH2_SESSION* session, const char* username, size_t username_len,
         const char* password, size_t password_len,
         LIBSSH2_PASSWD_CHANGEREQ_FUNC((*passwd_change_cb)),
         boost::system::error_code& ec,
         boost::optional<std::string&> e_msg = boost::optional<std::string&>())
{
    int rc = ::libssh2_userauth_password_ex(session, username, username_len,
                                            password, password_len,
                                            passwd_change_cb);

    if (rc != 0)
    {
        ec = ssh::detail::last_error_code(session, e_msg);
    }
}

/**
 * Exception wrapper around libssh2_userauth_password_ex.
 */
inline void password(LIBSSH2_SESSION* session, const char* username,
                     size_t username_len, const char* password_string,
                     size_t password_len,
                     LIBSSH2_PASSWD_CHANGEREQ_FUNC((*passwd_change_cb)))
{
    boost::system::error_code ec;
    std::string message;

    password(session, username, username_len, password_string, password_len,
             passwd_change_cb, ec, message);

    if (ec)
    {
        SSH_DETAIL_THROW_API_ERROR_CODE(ec, message,
                                        "libssh2_userauth_password_ex");
    }
}

/**
 * Error-fetching wrapper around libssh2_userauth_keyboard_interactive_ex.
 */
inline void keyboard_interactive_ex(
    LIBSSH2_SESSION* session, const char* username, unsigned int username_len,
    LIBSSH2_USERAUTH_KBDINT_RESPONSE_FUNC((*response_callback)),
    boost::system::error_code& ec,
    boost::optional<std::string&> e_msg = boost::optional<std::string&>())
{
    int rc = ::libssh2_userauth_keyboard_interactive_ex(
        session, username, username_len, response_callback);

    if (rc != 0)
    {
        ec = ssh::detail::last_error_code(session, e_msg);
    }
}

/**
 * Exception wrapper around libssh2_userauth_keyboard_interactive_ex.
 */
inline void keyboard_interactive_ex(
    LIBSSH2_SESSION* session, const char* username, unsigned int username_len,
    LIBSSH2_USERAUTH_KBDINT_RESPONSE_FUNC((*response_callback)))
{
    boost::system::error_code ec;
    std::string message;

    keyboard_interactive_ex(session, username, username_len, response_callback,
                            ec, message);

    if (ec)
    {
        SSH_DETAIL_THROW_API_ERROR_CODE(
            ec, message, "libssh2_userauth_keyboard_interactive_ex");
    }
}

/**
 * Error-fetching wrapper around libssh2_userauth_publickey_fromfile_ex.
 */
inline void public_key_from_file(
    LIBSSH2_SESSION* session, const char* username, size_t username_len,
    const char* public_key_path, const char* private_key_path,
    const char* passphrase, boost::system::error_code& ec,
    boost::optional<std::string&> e_msg = boost::optional<std::string&>())
{
    int rc = libssh2_userauth_publickey_fromfile_ex(
        session, username, username_len, public_key_path, private_key_path,
        passphrase);

    if (rc != 0)
    {
        ec = ssh::detail::last_error_code(session, e_msg);
    }
}

/**
 * Exception wrapper around libssh2_userauth_publickey_fromfile_ex.
 */
inline void public_key_from_file(LIBSSH2_SESSION* session, const char* username,
                                 size_t username_len,
                                 const char* public_key_path,
                                 const char* private_key_path,
                                 const char* passphrase)
{
    boost::system::error_code ec;
    std::string message;

    public_key_from_file(session, username, username_len, public_key_path,
                         private_key_path, passphrase, ec, message);

    if (ec)
    {
        SSH_DETAIL_THROW_API_ERROR_CODE(
            ec, message, "libssh2_userauth_publickey_fromfile_ex");
    }
}
}
}
}
} // namespace ssh::detail::libssh2::userauth

#endif
