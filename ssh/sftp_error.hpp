/**
    @file

    SFTP exceptions.

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

#ifndef SSH_SFTP_ERROR_HPP
#define SSH_SFTP_ERROR_HPP

#include <ssh/ssh_error.hpp> // last_error, ssh_error

#include <boost/exception/errinfo_file_name.hpp> // errinfo_file_name
#include <boost/exception/info.hpp> // errinfo_api_function
#include <boost/throw_exception.hpp> // throw_exception

#include <string>

#include <libssh2_sftp.h>
          // LIBSSH2_FX_*, LIBSSH2_ERROR_SFTP_PROTOCOL, libssh2_sftp_last_error

namespace ssh {
namespace sftp {

namespace detail {

    inline const char* sftp_part_of_error_message(unsigned long error)
    {
        switch (error)
        {
        case LIBSSH2_FX_OK:
            return ": FX_OK";
        case LIBSSH2_FX_EOF:
            return ": FX_EOF";
        case LIBSSH2_FX_NO_SUCH_FILE:
            return ": FX_NO_SUCH_FILE";
        case LIBSSH2_FX_PERMISSION_DENIED:
            return ": FX_PERMISSION_DENIED";
        case LIBSSH2_FX_FAILURE:
            return ": FX_FAILURE";
        case LIBSSH2_FX_BAD_MESSAGE:
            return ": FX_BAD_MESSAGE";
        case LIBSSH2_FX_NO_CONNECTION:
            return ": FX_NO_CONNECTION";
        case LIBSSH2_FX_CONNECTION_LOST:
            return ": FX_CONNECTION_LOST";
        case LIBSSH2_FX_OP_UNSUPPORTED:
            return ": FX_OP_UNSUPPORTED";
        case LIBSSH2_FX_INVALID_HANDLE:
            return ": FX_INVALID_HANDLE";
        case LIBSSH2_FX_NO_SUCH_PATH:
            return ": FX_NO_SUCH_PATH";
        case LIBSSH2_FX_FILE_ALREADY_EXISTS:
            return ": FX_FILE_ALREADY_EXISTS";
        case LIBSSH2_FX_WRITE_PROTECT:
            return ": FX_WRITE_PROTECT";
        case LIBSSH2_FX_NO_MEDIA:
            return ": FX_NO_MEDIA";
        case LIBSSH2_FX_NO_SPACE_ON_FILESYSTEM:
            return ": FX_NO_SPACE_ON_FILESYSTEM";
        case LIBSSH2_FX_QUOTA_EXCEEDED:
            return ": FX_QUOTA_EXCEEDED";
        case LIBSSH2_FX_UNKNOWN_PRINCIPAL:
            return ": FX_UNKNOWN_PRINCIPAL";
        case LIBSSH2_FX_LOCK_CONFLICT:
            return ": FX_LOCK_CONFLICT";
        case LIBSSH2_FX_DIR_NOT_EMPTY:
            return ": FX_DIR_NOT_EMPTY";
        case LIBSSH2_FX_NOT_A_DIRECTORY:
            return ": FX_NOT_A_DIRECTORY";
        case LIBSSH2_FX_INVALID_FILENAME:
            return ": FX_INVALID_FILENAME";
        case LIBSSH2_FX_LINK_LOOP:
            return ": FX_LINK_LOOP";
        default:
            return "Unrecognised SFTP error value";
        }
    }

}

class sftp_error : public ::ssh::ssh_error
{
public:
    sftp_error(
        const ssh::ssh_error& error, unsigned long sftp_error_code)
        : ssh_error(error), m_sftp_error(sftp_error_code)
    {
        message() += detail::sftp_part_of_error_message(m_sftp_error);
    }

    unsigned long sftp_error_code() const
    {
        return m_sftp_error;
    }
    
private:
    unsigned long m_sftp_error;
};

namespace detail {

    template<typename T>
    inline void throw_error(
        T& error, const char* current_function, const char* source_file,
        int source_line, const char* api_function,
        const char* path, size_t path_len)
    {
        error <<
            boost::errinfo_api_function(api_function) <<
            boost::throw_function(current_function) <<
            boost::throw_file(source_file) <<
            boost::throw_line(source_line);
        if (path && path_len > 0)
            error << boost::errinfo_file_name(std::string(path, path_len));

        boost::throw_exception(error);
    }

    /**
     * Throw whatever the most appropriate type of exception is.
     *
     * libssh2_sftp_* functions can return either a standard SSH error or a
     * SFTP error.  This function checks and throws the appropriate object.
     */
    inline void throw_last_error(
        LIBSSH2_SESSION* session, LIBSSH2_SFTP* sftp,
        const char* current_function, const char* source_file,
        int source_line, const char* api_function,
        const char* path=NULL, size_t path_len=0U)
    {
        ::ssh::ssh_error error = ::ssh::detail::last_error(session);

        if (error.error_code() == LIBSSH2_ERROR_SFTP_PROTOCOL)
        {
            sftp_error derived_error = 
                sftp_error(error, ::libssh2_sftp_last_error(sftp));
            throw_error(
                derived_error, current_function, source_file, source_line,
                api_function, path, path_len);
        }
        else
        {
            throw_error(
                error, current_function, source_file, source_line, api_function,
                path, path_len);
        }
    }
}

}} // namespace ssh::sftp

#endif
