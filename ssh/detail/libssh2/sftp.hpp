/**
    @file

    Exception wrapper round raw libssh2 SFTP functions.

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

#ifndef SSH_DETAIL_LIBSSH2_SFTP_HPP
#define SSH_DETAIL_LIBSSH2_SFTP_HPP

#include <ssh/sftp_error.hpp> // throw_last_error
#include <ssh/ssh_error.hpp> // last_error

#include <boost/exception/info.hpp> // errinfo_api_function
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <cassert> // assert

#include <libssh2.h> // LIBSSH2_SESSION
#include <libssh2_sftp.h> // libssh2_sftp_*

#define SSH_THROW_LAST_SFTP_ERROR(session, sftp_session, api_function) \
    ::ssh::sftp::detail::throw_last_error( \
        session,sftp_session,BOOST_CURRENT_FUNCTION,__FILE__,__LINE__, \
        api_function)

#define SSH_THROW_LAST_SFTP_ERROR_WITH_PATH( \
    session, sftp_session, api_function, path, path_len) \
    ::ssh::sftp::detail::throw_last_error( \
        session,sftp_session,BOOST_CURRENT_FUNCTION,__FILE__,__LINE__, \
        api_function, path, path_len)


// See ssh/detail/libssh2/libssh2.hpp for rules governing functions in this
// namespace

namespace ssh {
namespace detail {
namespace libssh2 {
namespace sftp {

/**
 * Thin exception wrapper around libssh2_sftp_init.
 */
inline LIBSSH2_SFTP* init(LIBSSH2_SESSION* session)
{
    LIBSSH2_SFTP* sftp = ::libssh2_sftp_init(session);
    if (!sftp)
        BOOST_THROW_EXCEPTION(
            ssh::detail::last_error(session) <<
            boost::errinfo_api_function("libssh2_sftp_init"));

    return sftp;
}

/**
 * Thin exception wrapper around libssh2_sftp_open_ex
 */
inline LIBSSH2_SFTP_HANDLE* open(
    LIBSSH2_SESSION* session, LIBSSH2_SFTP* sftp, const char* filename,
    unsigned int filename_len, unsigned long flags, long mode, int open_type)
{
    LIBSSH2_SFTP_HANDLE* handle = ::libssh2_sftp_open_ex(
        sftp, filename, filename_len, flags, mode, open_type);
    if (!handle)
        SSH_THROW_LAST_SFTP_ERROR_WITH_PATH(
            session, sftp, "libssh2_sftp_open_ex", filename, filename_len);

    return handle;
}

/**
 * Thin exception wrapper around libssh2_sftp_symlink_ex.
 *
 * Slightly odd treatment of the return value because it's overloaded.
 * Callers of this function never need to check it for success but do
 * need to check if for buffer validity in the @c LIBSSH2_SFTP_READLINK
 * and @c LIBSSH2_SFTP_REALPATH cases.
 */
inline int symlink_ex(
    LIBSSH2_SESSION* session, LIBSSH2_SFTP* sftp,
    const char* path, unsigned int path_len,
    char* target, unsigned int target_len, int resolve_action)
{
    int rc = ::libssh2_sftp_symlink_ex(
        sftp, path, path_len, target, target_len, resolve_action);
    switch (resolve_action)
    {
    case LIBSSH2_SFTP_READLINK:
    case LIBSSH2_SFTP_REALPATH:
        if (rc < 0)
            SSH_THROW_LAST_SFTP_ERROR(
                session, sftp, "libssh2_sftp_symlink_ex");
        break;

    case LIBSSH2_SFTP_SYMLINK:
    default:
        if (rc != 0)
        {
            assert(rc < 0);
            SSH_THROW_LAST_SFTP_ERROR(
                session, sftp, "libssh2_sftp_symlink_ex");
        }

        break;
    }

    return rc;
}

/**
 * Thin exception wrapper around libssh2_sftp_symlink.
 *
 * Uses the raw libssh2_sftp_symlink_ex function so we aren't forced
 * to use strlen.
 */
inline void symlink(
    LIBSSH2_SESSION* session, LIBSSH2_SFTP* sftp,
    const char* path, unsigned int path_len,
    const char* target, unsigned int target_len)
{
    symlink_ex(
        session, sftp, path, path_len, const_cast<char*>(target), target_len,
        LIBSSH2_SFTP_SYMLINK);
}

/**
 * Thin exception wrapper around libssh2_sftp_stat_ex.
 */
inline void stat(
    LIBSSH2_SESSION* session, LIBSSH2_SFTP* sftp,
    const char* path, unsigned int path_len, int stat_type,
    LIBSSH2_SFTP_ATTRIBUTES* attributes)
{
    int rc = ::libssh2_sftp_stat_ex(
        sftp, path, path_len, stat_type, attributes);
    if (rc < 0)
        SSH_THROW_LAST_SFTP_ERROR_WITH_PATH(
            session, sftp, "libssh2_sftp_stat_ex", path, path_len);
}

/**
 * Thin exception wrapper around libssh2_sftp_unlink_ex.
 */
inline void unlink_ex(
    LIBSSH2_SESSION* session, LIBSSH2_SFTP* sftp,
    const char* path, unsigned int path_len)
{
    int rc = ::libssh2_sftp_unlink_ex(sftp, path, path_len);
    if (rc < 0)
        SSH_THROW_LAST_SFTP_ERROR_WITH_PATH(
            session, sftp, "libssh2_sftp_unlink_ex", path, path_len);
}

/**
 * Thin exception wrapper around libssh2_sftp_mkdir_ex.
 */
inline void mkdir_ex(
    LIBSSH2_SESSION* session, LIBSSH2_SFTP* sftp,
    const char* path, unsigned int path_len, long mode)
{
    int rc = ::libssh2_sftp_mkdir_ex(sftp, path, path_len, mode);
    if (rc < 0)
        SSH_THROW_LAST_SFTP_ERROR_WITH_PATH(
            session, sftp, "libssh2_sftp_mkdir_ex", path, path_len);
}

/**
 * Thin exception wrapper around libssh2_sftp_rmdir_ex.
 */
inline void rmdir_ex(
    LIBSSH2_SESSION* session, LIBSSH2_SFTP* sftp,
    const char* path, unsigned int path_len)
{
    int rc = ::libssh2_sftp_rmdir_ex(sftp, path, path_len);
    if (rc < 0)
        SSH_THROW_LAST_SFTP_ERROR_WITH_PATH(
            session, sftp, "libssh2_sftp_rmdir_ex", path, path_len);
}

/**
 * Thin exception wrapper around libssh2_sftp_rename_ex
 */
inline void rename(
    LIBSSH2_SESSION* session, LIBSSH2_SFTP* sftp,
    const char* source, unsigned int source_len, const char* destination,
    unsigned int destination_len, long flags)
{
    int rc = ::libssh2_sftp_rename_ex(
        sftp, source, source_len, destination, destination_len, flags);
    if (rc)
        SSH_THROW_LAST_SFTP_ERROR_WITH_PATH(
            session, sftp, "libssh2_sftp_rename_ex", source, source_len);
}

/**
 * Thin exception wrapper around libssh2_sftp_read.
 */
inline ssize_t read(
    LIBSSH2_SESSION* session, LIBSSH2_SFTP* sftp,
    LIBSSH2_SFTP_HANDLE* file_handle, char* buffer, size_t buffer_len)
{
    ssize_t count = libssh2_sftp_read(file_handle, buffer, buffer_len);
    if (count < 0)
        SSH_THROW_LAST_SFTP_ERROR(session, sftp, "libssh2_sftp_read");

    return count;
}

/**
 * Thin exception wrapper around libssh2_sftp_write.
 */
inline ssize_t write(
    LIBSSH2_SESSION* session, LIBSSH2_SFTP* sftp,
    LIBSSH2_SFTP_HANDLE* file_handle, const char* buffer, size_t buffer_len)
{
    ssize_t count = libssh2_sftp_write(file_handle, buffer, buffer_len);
    if (count < 0)
        SSH_THROW_LAST_SFTP_ERROR(session, sftp, "libssh2_sftp_write");

    return count;
}

}}}} // namespace ssh::detail::libssh2::sftp

#undef SSH_THROW_LAST_SFTP_ERROR_WITH_PATH
#undef SSH_THROW_LAST_SFTP_ERROR

#endif
