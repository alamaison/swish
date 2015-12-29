/**
    @file

    Error-reporting wrapper round raw libssh2 SFTP functions.

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

#include <ssh/sftp_error.hpp> // last_sftp_error_code
#include <ssh/ssh_error.hpp>  // last_error_code

#include <boost/optional/optional.hpp>
#include <boost/system/error_code.hpp>
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <cassert> // assert
#include <string>

#include <libssh2.h>      // LIBSSH2_SESSION, LIBSSH2_SFTP
#include <libssh2_sftp.h> // libssh2_sftp_*

// See ssh/detail/libssh2/libssh2.hpp for rules governing functions in this
// namespace

namespace ssh
{
namespace detail
{
namespace libssh2
{
namespace sftp
{

/**
 * Error-fetching wrapper around libssh2_sftp_init.
 */
inline LIBSSH2_SFTP*
init(LIBSSH2_SESSION* session, boost::system::error_code& ec,
     boost::optional<std::string&> e_msg = boost::optional<std::string&>())
{
    LIBSSH2_SFTP* sftp = ::libssh2_sftp_init(session);
    if (!sftp)
    {
        ec = ssh::detail::last_error_code(session, e_msg);
    }

    return sftp;
}

/**
 * Exception wrapper around libssh2_sftp_init.
 */
inline LIBSSH2_SFTP* init(LIBSSH2_SESSION* session)
{
    boost::system::error_code ec;
    std::string message;
    LIBSSH2_SFTP* sftp = init(session, ec, message);
    if (ec)
        SSH_DETAIL_THROW_API_ERROR_CODE(ec, message, "libssh2_sftp_init");

    return sftp;
}

/**
 * Error-fetching wrapper around libssh2_sftp_open_ex.
 */
inline LIBSSH2_SFTP_HANDLE*
open(LIBSSH2_SESSION* session, LIBSSH2_SFTP* sftp, const char* filename,
     unsigned int filename_len, unsigned long flags, long mode, int open_type,
     boost::system::error_code& ec,
     boost::optional<std::string&> e_msg = boost::optional<std::string&>())
{
    LIBSSH2_SFTP_HANDLE* handle = ::libssh2_sftp_open_ex(
        sftp, filename, filename_len, flags, mode, open_type);
    if (!handle)
    {
        ec =
            ssh::filesystem::detail::last_sftp_error_code(session, sftp, e_msg);
    }

    return handle;
}

/**
 * Exception wrapper around libssh2_sftp_open_ex.
 */
inline LIBSSH2_SFTP_HANDLE* open(LIBSSH2_SESSION* session, LIBSSH2_SFTP* sftp,
                                 const char* filename,
                                 unsigned int filename_len, unsigned long flags,
                                 long mode, int open_type)
{
    boost::system::error_code ec;
    std::string message;

    LIBSSH2_SFTP_HANDLE* handle = open(session, sftp, filename, filename_len,
                                       flags, mode, open_type, ec, message);

    if (ec)
    {
        SSH_DETAIL_THROW_API_ERROR_CODE_WITH_PATH(
            ec, message, "libssh2_sftp_open_ex", filename, filename_len);
    }

    return handle;
}

/**
 * Error-fetching wrapper around libssh2_sftp_symlink_ex.
 *
 * The return value has no meaning if `resolve_action` is
 * `LIBSSH2_SFTP_SYMLINK` or if `ec == true`.  If `resolve_action` is
 * `LIBSSH2_SFTP_READLINK` and `LIBSSH2_SFTP_REALPATH` the return code,
 * if successful, is the number of bytes written to the buffer.
 */
inline int symlink_ex(
    LIBSSH2_SESSION* session, LIBSSH2_SFTP* sftp, const char* path,
    unsigned int path_len, char* target, unsigned int target_len,
    int resolve_action, boost::system::error_code& ec,
    boost::optional<std::string&> e_msg = boost::optional<std::string&>())
{
    // Slightly odd treatment of the return value because the success value
    // is 0 for `LIBSSH2_SFTP_SYMLINK` but >= 0 for `LIBSSH2_SFTP_READLINK` or
    // `LIBSSH2_SFTP_REALPATH`.

    int rc = ::libssh2_sftp_symlink_ex(sftp, path, path_len, target, target_len,
                                       resolve_action);
    switch (resolve_action)
    {
    case LIBSSH2_SFTP_READLINK:
    case LIBSSH2_SFTP_REALPATH:
        if (rc < 0)
        {
            ec = ::ssh::filesystem::detail::last_sftp_error_code(session, sftp,
                                                                 e_msg);
        }
        break;

    case LIBSSH2_SFTP_SYMLINK:
    default:
        if (rc != 0)
        {
            assert(rc < 0);
            ec = ::ssh::filesystem::detail::last_sftp_error_code(session, sftp,
                                                                 e_msg);
        }

        break;
    }

    return rc;
}

/**
 * Exception wrapper around libssh2_sftp_symlink_ex.
 *
 * The return value has no meaning if `resolve_action` is
 * `LIBSSH2_SFTP_SYMLINK`.  If `resolve_action` is`LIBSSH2_SFTP_READLINK` and
 * `LIBSSH2_SFTP_REALPATH` the return code is the number of bytes written to
 * the buffer.
 */
inline int symlink_ex(LIBSSH2_SESSION* session, LIBSSH2_SFTP* sftp,
                      const char* path, unsigned int path_len, char* target,
                      unsigned int target_len, int resolve_action)
{
    boost::system::error_code ec;
    std::string message;

    int rc = symlink_ex(session, sftp, path, path_len, target, target_len,
                        resolve_action, ec, message);

    if (ec)
    {
        SSH_DETAIL_THROW_API_ERROR_CODE_WITH_PATH(
            ec, message, "libssh2_sftp_symlink_ex", path, path_len);
    }

    return rc;
}

/**
 * Error-fetching wrapper around libssh2_sftp_symlink.
 */
inline void
symlink(LIBSSH2_SESSION* session, LIBSSH2_SFTP* sftp, const char* path,
        unsigned int path_len, const char* target, unsigned int target_len,
        boost::system::error_code& ec,
        boost::optional<std::string&> e_msg = boost::optional<std::string&>())
{
    // Uses the raw libssh2_sftp_symlink_ex function so we aren't forced
    // to use strlen.
    symlink_ex(session, sftp, path, path_len, const_cast<char*>(target),
               target_len, LIBSSH2_SFTP_SYMLINK, ec, e_msg);
}

/**
 * Exception wrapper around libssh2_sftp_symlink.
 */
inline void symlink(LIBSSH2_SESSION* session, LIBSSH2_SFTP* sftp,
                    const char* path, unsigned int path_len, const char* target,
                    unsigned int target_len)
{
    // Uses the raw libssh2_sftp_symlink_ex function so we aren't forced
    // to use strlen.
    symlink_ex(session, sftp, path, path_len, const_cast<char*>(target),
               target_len, LIBSSH2_SFTP_SYMLINK);
}

/**
 * Error-fetching wrapper around libssh2_sftp_stat_ex.
 */
inline void
stat(LIBSSH2_SESSION* session, LIBSSH2_SFTP* sftp, const char* path,
     unsigned int path_len, int stat_type, LIBSSH2_SFTP_ATTRIBUTES* attributes,
     boost::system::error_code& ec,
     boost::optional<std::string&> e_msg = boost::optional<std::string&>())
{
    int rc =
        ::libssh2_sftp_stat_ex(sftp, path, path_len, stat_type, attributes);
    if (rc < 0)
    {
        ec =
            ssh::filesystem::detail::last_sftp_error_code(session, sftp, e_msg);
    }
}

/**
 * Exception wrapper around libssh2_sftp_stat_ex.
 */
inline void stat(LIBSSH2_SESSION* session, LIBSSH2_SFTP* sftp, const char* path,
                 unsigned int path_len, int stat_type,
                 LIBSSH2_SFTP_ATTRIBUTES* attributes)
{
    boost::system::error_code ec;
    std::string message;

    stat(session, sftp, path, path_len, stat_type, attributes, ec, message);

    if (ec)
    {
        SSH_DETAIL_THROW_API_ERROR_CODE_WITH_PATH(
            ec, message, "libssh2_sftp_stat_ex", path, path_len);
    }
}

/**
 * Error-fetching wrapper around libssh2_sftp_fstat_ex.
 */
inline void
fstat(LIBSSH2_SESSION* session, LIBSSH2_SFTP* sftp, LIBSSH2_SFTP_HANDLE* handle,
      LIBSSH2_SFTP_ATTRIBUTES* attributes, int fstat_type,
      boost::system::error_code& ec,
      boost::optional<std::string&> e_msg = boost::optional<std::string&>())
{
    int rc = ::libssh2_sftp_fstat_ex(handle, attributes, fstat_type);
    if (rc != 0)
    {
        ec =
            ssh::filesystem::detail::last_sftp_error_code(session, sftp, e_msg);
    }
}

/**
 * Exception wrapper around libssh2_sftp_fstat_ex.
 */
inline void fstat(LIBSSH2_SESSION* session, LIBSSH2_SFTP* sftp,
                  LIBSSH2_SFTP_HANDLE* handle,
                  LIBSSH2_SFTP_ATTRIBUTES* attributes, int fstat_type)
{
    boost::system::error_code ec;
    std::string message;

    fstat(session, sftp, handle, attributes, fstat_type, ec, message);

    if (ec)
    {
        SSH_DETAIL_THROW_API_ERROR_CODE(ec, message, "libssh2_sftp_fstat_ex");
    }
}

/**
 * Error-fetching wrapper around libssh2_sftp_unlink_ex.
 */
inline void
unlink_ex(LIBSSH2_SESSION* session, LIBSSH2_SFTP* sftp, const char* path,
          unsigned int path_len, boost::system::error_code& ec,
          boost::optional<std::string&> e_msg = boost::optional<std::string&>())
{
    int rc = ::libssh2_sftp_unlink_ex(sftp, path, path_len);
    if (rc < 0)
    {
        ec =
            ssh::filesystem::detail::last_sftp_error_code(session, sftp, e_msg);
    }
}

/**
 * Exception wrapper around libssh2_sftp_unlink_ex.
 */
inline void unlink_ex(LIBSSH2_SESSION* session, LIBSSH2_SFTP* sftp,
                      const char* path, unsigned int path_len)
{
    boost::system::error_code ec;
    std::string message;

    unlink_ex(session, sftp, path, path_len, ec, message);

    if (ec)
    {
        SSH_DETAIL_THROW_API_ERROR_CODE_WITH_PATH(
            ec, message, "libssh2_sftp_unlink_ex", path, path_len);
    }
}

/**
 * Error-fetching wrapper around libssh2_sftp_mkdir_ex.
 */
inline void
mkdir_ex(LIBSSH2_SESSION* session, LIBSSH2_SFTP* sftp, const char* path,
         unsigned int path_len, long mode, boost::system::error_code& ec,
         boost::optional<std::string&> e_msg = boost::optional<std::string&>())
{
    int rc = ::libssh2_sftp_mkdir_ex(sftp, path, path_len, mode);
    if (rc < 0)
    {
        ec =
            ssh::filesystem::detail::last_sftp_error_code(session, sftp, e_msg);
    }
}

/**
 * Exception wrapper around libssh2_sftp_mkdir_ex.
 */
inline void mkdir_ex(LIBSSH2_SESSION* session, LIBSSH2_SFTP* sftp,
                     const char* path, unsigned int path_len, long mode)
{
    boost::system::error_code ec;
    std::string message;

    mkdir_ex(session, sftp, path, path_len, mode, ec, message);

    if (ec)
    {
        SSH_DETAIL_THROW_API_ERROR_CODE_WITH_PATH(
            ec, message, "libssh2_sftp_mkdir_ex", path, path_len);
    }
}

/**
 * Error-fetching wrapper around libssh2_sftp_rmdir_ex.
 */
inline void
rmdir_ex(LIBSSH2_SESSION* session, LIBSSH2_SFTP* sftp, const char* path,
         unsigned int path_len, boost::system::error_code& ec,
         boost::optional<std::string&> e_msg = boost::optional<std::string&>())
{
    int rc = ::libssh2_sftp_rmdir_ex(sftp, path, path_len);
    if (rc < 0)
    {
        ec =
            ssh::filesystem::detail::last_sftp_error_code(session, sftp, e_msg);
    }
}

/**
 * Exception wrapper around libssh2_sftp_rmdir_ex.
 */
inline void rmdir_ex(LIBSSH2_SESSION* session, LIBSSH2_SFTP* sftp,
                     const char* path, unsigned int path_len)
{
    boost::system::error_code ec;
    std::string message;

    rmdir_ex(session, sftp, path, path_len, ec, message);

    if (ec)
    {
        SSH_DETAIL_THROW_API_ERROR_CODE_WITH_PATH(
            ec, message, "libssh2_sftp_rmdir_ex", path, path_len);
    }
}

/**
 * Error-fetching wrapper around libssh2_sftp_rename_ex
 */
inline void
rename(LIBSSH2_SESSION* session, LIBSSH2_SFTP* sftp, const char* source,
       unsigned int source_len, const char* destination,
       unsigned int destination_len, long flags, boost::system::error_code& ec,
       boost::optional<std::string&> e_msg = boost::optional<std::string&>())
{
    int rc = ::libssh2_sftp_rename_ex(sftp, source, source_len, destination,
                                      destination_len, flags);
    if (rc)
    {
        ec =
            ssh::filesystem::detail::last_sftp_error_code(session, sftp, e_msg);
    }
}

/**
 * Exception wrapper around libssh2_sftp_rename_ex
 */
inline void rename(LIBSSH2_SESSION* session, LIBSSH2_SFTP* sftp,
                   const char* source, unsigned int source_len,
                   const char* destination, unsigned int destination_len,
                   long flags)
{
    boost::system::error_code ec;
    std::string message;

    rename(session, sftp, source, source_len, destination, destination_len,
           flags, ec, message);

    if (ec)
    {
        SSH_DETAIL_THROW_API_ERROR_CODE_WITH_PATH(
            ec, message, "libssh2_sftp_rename_ex", source, source_len);
    }
}

/**
 * Error-fetching wrapper around libssh2_sftp_read.
 */
inline ssize_t
read(LIBSSH2_SESSION* session, LIBSSH2_SFTP* sftp,
     LIBSSH2_SFTP_HANDLE* file_handle, char* buffer, size_t buffer_len,
     boost::system::error_code& ec,
     boost::optional<std::string&> e_msg = boost::optional<std::string&>())
{
    ssize_t count = ::libssh2_sftp_read(file_handle, buffer, buffer_len);
    if (count < 0)
    {
        ec = ::ssh::filesystem::detail::last_sftp_error_code(session, sftp,
                                                             e_msg);
    }

    return count;
}

/**
 * Exception wrapper around libssh2_sftp_read.
 */
inline ssize_t read(LIBSSH2_SESSION* session, LIBSSH2_SFTP* sftp,
                    LIBSSH2_SFTP_HANDLE* file_handle, char* buffer,
                    size_t buffer_len)
{
    boost::system::error_code ec;
    std::string message;

    ssize_t count =
        read(session, sftp, file_handle, buffer, buffer_len, ec, message);

    if (ec)
    {
        SSH_DETAIL_THROW_API_ERROR_CODE(ec, message, "libssh2_sftp_read");
    }

    return count;
}

/**
 * Error-fetching wrapper around libssh2_sftp_write.
 */
inline ssize_t
write(LIBSSH2_SESSION* session, LIBSSH2_SFTP* sftp,
      LIBSSH2_SFTP_HANDLE* file_handle, const char* data, size_t data_len,
      boost::system::error_code& ec,
      boost::optional<std::string&> e_msg = boost::optional<std::string&>())
{
    ssize_t count = ::libssh2_sftp_write(file_handle, data, data_len);
    if (count < 0)
    {
        ec = ::ssh::filesystem::detail::last_sftp_error_code(session, sftp,
                                                             e_msg);
    }

    return count;
}

/**
 * Exception wrapper around libssh2_sftp_write.
 */
inline ssize_t write(LIBSSH2_SESSION* session, LIBSSH2_SFTP* sftp,
                     LIBSSH2_SFTP_HANDLE* file_handle, const char* data,
                     size_t data_len)
{
    boost::system::error_code ec;
    std::string message;

    ssize_t count =
        write(session, sftp, file_handle, data, data_len, ec, message);

    if (ec)
    {
        SSH_DETAIL_THROW_API_ERROR_CODE(ec, message, "libssh2_sftp_write");
    }

    return count;
}

/**
 * Error-fetching wrapper around libssh2_sftp_readdir_ex.
 */
inline int readdir_ex(
    LIBSSH2_SESSION* session, LIBSSH2_SFTP* sftp, LIBSSH2_SFTP_HANDLE* handle,
    char* buffer, size_t buffer_len, char* longentry, size_t longentry_len,
    LIBSSH2_SFTP_ATTRIBUTES* attrs, boost::system::error_code& ec,
    boost::optional<std::string&> e_msg = boost::optional<std::string&>())
{
    int rc = ::libssh2_sftp_readdir_ex(handle, buffer, buffer_len, longentry,
                                       longentry_len, attrs);

    if (rc < 0)
    {
        ec = ::ssh::filesystem::detail::last_sftp_error_code(session, sftp,
                                                             e_msg);
    }

    return rc;
}

/**
 * Exception wrapper around libssh2_sftp_readdir_ex.
 */
inline int readdir_ex(LIBSSH2_SESSION* session, LIBSSH2_SFTP* sftp,
                      LIBSSH2_SFTP_HANDLE* handle, char* buffer,
                      size_t buffer_len, char* longentry, size_t longentry_len,
                      LIBSSH2_SFTP_ATTRIBUTES* attrs)
{
    boost::system::error_code ec;
    std::string message;

    int rc = readdir_ex(session, sftp, handle, buffer, buffer_len, longentry,
                        longentry_len, attrs, ec, message);

    if (ec)
    {
        SSH_DETAIL_THROW_API_ERROR_CODE(ec, message, "libssh2_sftp_readdir_ex");
    }

    return rc;
}
}
}
}
} // namespace ssh::detail::libssh2::filesystem

#endif
