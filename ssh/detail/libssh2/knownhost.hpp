/**
    @file

    Exception wrapper round raw libssh2 knownhost functions.

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

#ifndef SSH_DETAIL_LIBSSH2_KNOWNHOST_HPP
#define SSH_DETAIL_LIBSSH2_KNOWNHOST_HPP

#include <ssh/ssh_error.hpp> // last_error_code, SSH_DETAIL_THROW_API_ERROR_CODE

#include <boost/exception/info.hpp>  // errinfo_api_function
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <libssh2.h> // LIBSSH2_SESSION, LIBSSH2_KNOWNHOSTS, libssh2_knownhost_*

// See ssh/detail/libssh2/libssh2.hpp for rules governing functions in this
// namespace

namespace ssh
{
namespace detail
{
namespace libssh2
{
namespace knownhost
{

/**
 * Error-fetching wrapper around libssh2_knownhost_init.
 */
inline LIBSSH2_KNOWNHOSTS*
init(LIBSSH2_SESSION* session, boost::system::error_code& ec,
     boost::optional<std::string&> e_msg = boost::optional<std::string&>())
{
    LIBSSH2_KNOWNHOSTS* hosts = ::libssh2_knownhost_init(session);

    if (!hosts)
    {
        ec = ssh::detail::last_error_code(session, e_msg);
    }

    return hosts;
}

/**
 * Exception wrapper around libssh2_knownhost_init.
 */
inline LIBSSH2_KNOWNHOSTS* init(LIBSSH2_SESSION* session)
{
    boost::system::error_code ec;
    std::string message;

    LIBSSH2_KNOWNHOSTS* hosts = init(session, ec, message);

    if (ec)
    {
        SSH_DETAIL_THROW_API_ERROR_CODE(ec, message, "libssh2_knownhost_init");
    }

    return hosts;
}

/**
 * Error-fetching wrapper around libssh2_knownhost_readline.
 */
inline void
readline(LIBSSH2_SESSION* session, LIBSSH2_KNOWNHOSTS* hosts, const char* line,
         size_t line_length, int type, boost::system::error_code& ec,
         boost::optional<std::string&> e_msg = boost::optional<std::string&>())
{
    int rc = ::libssh2_knownhost_readline(hosts, line, line_length, type);

    if (rc < 0)
    {
        ec = ssh::detail::last_error_code(session, e_msg);
    }
}

/**
 * Exception wrapper around libssh2_knownhost_readline.
 */
inline void readline(LIBSSH2_SESSION* session, LIBSSH2_KNOWNHOSTS* hosts,
                     const char* line, size_t line_length, int type)
{
    boost::system::error_code ec;
    std::string message;

    readline(session, hosts, line, line_length, type, ec, message);

    if (ec)
    {
        SSH_DETAIL_THROW_API_ERROR_CODE(ec, message,
                                        "libssh2_knownhost_readline");
    }
}

/**
 * Error-fetching wrapper around libssh2_knownhost_writeline.
 */
inline void
writeline(LIBSSH2_SESSION* session, LIBSSH2_KNOWNHOSTS* hosts,
          libssh2_knownhost* host, char* buffer, size_t buffer_length,
          size_t* written_length_out, int type, boost::system::error_code& ec,
          boost::optional<std::string&> e_msg = boost::optional<std::string&>())
{
    int rc = ::libssh2_knownhost_writeline(hosts, host, buffer, buffer_length,
                                           written_length_out, type);

    if (rc < 0)
    {
        ec = ssh::detail::last_error_code(session, e_msg);
    }
}

/**
 * Exception wrapper around libssh2_knownhost_writeline.
 */
inline void writeline(LIBSSH2_SESSION* session, LIBSSH2_KNOWNHOSTS* hosts,
                      libssh2_knownhost* host, char* buffer,
                      size_t buffer_length, size_t* written_length_out,
                      int type)
{
    boost::system::error_code ec;
    std::string message;

    writeline(session, hosts, host, buffer, buffer_length, written_length_out,
              type, ec, message);

    if (ec)
    {
        SSH_DETAIL_THROW_API_ERROR_CODE(ec, message,
                                        "libssh2_knownhost_writeline");
    }
}

/**
 * Error-fetching wrapper around libssh2_knownhost_get.
 *
 * @returns 1 if finished.  The return code has no meaning if `ec == false`.
 */
inline int
get(LIBSSH2_SESSION* session, LIBSSH2_KNOWNHOSTS* hosts,
    libssh2_knownhost** store, libssh2_knownhost* current_position,
    boost::system::error_code& ec,
    boost::optional<std::string&> e_msg = boost::optional<std::string&>())
{
    int rc = ::libssh2_knownhost_get(hosts, store, current_position);

    if (rc < 0)
    {
        ec = ssh::detail::last_error_code(session, e_msg);
    }

    return rc;
}

/**
 * Exception wrapper around libssh2_knownhost_get.
 *
 * @returns 1 if finished.
 */
inline int get(LIBSSH2_SESSION* session, LIBSSH2_KNOWNHOSTS* hosts,
               libssh2_knownhost** store, libssh2_knownhost* current_position)
{
    boost::system::error_code ec;
    std::string message;

    int rc = get(session, hosts, store, current_position, ec, message);

    if (ec)
    {
        SSH_DETAIL_THROW_API_ERROR_CODE(ec, message, "libssh2_knownhost_get");
    }

    return rc;
}

/**
 * Error-fetching wrapper around libssh2_knownhost_add.
 */
inline void
add(LIBSSH2_SESSION* session, LIBSSH2_KNOWNHOSTS* hosts, const char* host,
    const char* salt, const char* key, size_t key_length, int typemask,
    libssh2_knownhost** store, boost::system::error_code& ec,
    boost::optional<std::string&> e_msg = boost::optional<std::string&>())
{
    int rc = ::libssh2_knownhost_add(hosts, host, salt, key, key_length,
                                     typemask, store);

    if (rc < 0)
    {
        ec = ssh::detail::last_error_code(session, e_msg);
    }
}

/**
 * Exception wrapper around libssh2_knownhost_add.
 */
inline void add(LIBSSH2_SESSION* session, LIBSSH2_KNOWNHOSTS* hosts,
                const char* host, const char* salt, const char* key,
                size_t key_length, int typemask, libssh2_knownhost** store)
{
    boost::system::error_code ec;
    std::string message;

    add(session, hosts, host, salt, key, key_length, typemask, store, ec,
        message);

    if (ec)
    {
        SSH_DETAIL_THROW_API_ERROR_CODE(ec, message, "libssh2_knownhost_add");
    }
}

/**
 * Error-fetching wrapper around libssh2_knownhost_del.
 */
inline void
del(LIBSSH2_SESSION* session, LIBSSH2_KNOWNHOSTS* hosts,
    libssh2_knownhost* entry, boost::system::error_code& ec,
    boost::optional<std::string&> e_msg = boost::optional<std::string&>())
{
    int rc = ::libssh2_knownhost_del(hosts, entry);

    if (rc < 0)
    {
        ec = ssh::detail::last_error_code(session, e_msg);
    }
}

/**
 * Exception wrapper around libssh2_knownhost_del.
 */
inline void del(LIBSSH2_SESSION* session, LIBSSH2_KNOWNHOSTS* hosts,
                libssh2_knownhost* entry)
{
    boost::system::error_code ec;
    std::string message;

    del(session, hosts, entry, ec, message);

    if (ec)
    {
        SSH_DETAIL_THROW_API_ERROR_CODE(ec, message, "libssh2_knownhost_del");
    }
}

/**
 * Error-fetching wrapper around libssh2_knownhost_check.
 */
inline int
check(LIBSSH2_SESSION* session, LIBSSH2_KNOWNHOSTS* hosts, const char* host,
      const char* key, size_t key_length, int typemask,
      libssh2_knownhost** knownhost, boost::system::error_code& ec,
      boost::optional<std::string&> e_msg = boost::optional<std::string&>())
{
    int rc = ::libssh2_knownhost_check(hosts, host, key, key_length, typemask,
                                       knownhost);

    switch (rc)
    {
    case LIBSSH2_KNOWNHOST_CHECK_MATCH:
    case LIBSSH2_KNOWNHOST_CHECK_MISMATCH:
    case LIBSSH2_KNOWNHOST_CHECK_NOTFOUND:
        // Positive values, NOT ERROR CODES, won't appear in last_error
        return rc;

    case LIBSSH2_KNOWNHOST_CHECK_FAILURE:
    // This value, also positive, also not an error code, means there
    // was an error so check last_error to find the error code

    default:

        ec = ssh::detail::last_error_code(session, e_msg);
        return LIBSSH2_KNOWNHOST_CHECK_FAILURE;
    }
}

/**
 * Exception wrapper around libssh2_knownhost_check.
 */
inline int check(LIBSSH2_SESSION* session, LIBSSH2_KNOWNHOSTS* hosts,
                 const char* host, const char* key, size_t key_length,
                 int typemask, libssh2_knownhost** knownhost)
{
    boost::system::error_code ec;
    std::string message;

    int rc = check(session, hosts, host, key, key_length, typemask, knownhost,
                   ec, message);

    if (ec)
    {
        SSH_DETAIL_THROW_API_ERROR_CODE(ec, message, "libssh2_knownhost_check");
    }

    return rc;
}
}
}
}
} // namespace ssh::detail::libssh2::knownhost

#endif
