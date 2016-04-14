/**
    @file

    SFTP error reporting.

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

#include <ssh/ssh_error.hpp> // last_error_code

#include <boost/exception/errinfo_file_name.hpp> // errinfo_file_name
#include <boost/exception/info.hpp>              // errinfo_api_function
#include <boost/throw_exception.hpp>             // throw_exception

#include <string>

#include <libssh2_sftp.h> // LIBSSH2_FX_*, LIBSSH2_ERROR_SFTP_PROTOCOL,
                          // libssh2_sftp_last_error

namespace ssh
{
namespace filesystem
{

inline boost::system::error_category& sftp_error_category();

namespace detail
{

// Cutting LIBSSH2_ prefix off because the FX codes correspond to codes in
// the spec, not just in the library

#define SSH_CASE_SFTP_RETURN_STRINGISED(x)                                     \
    case LIBSSH2_##x:                                                          \
        return #x;

inline std::string sftp_error_code_to_string(unsigned long code)
{
    switch (code)
    {
        SSH_CASE_SFTP_RETURN_STRINGISED(FX_OK);
        SSH_CASE_SFTP_RETURN_STRINGISED(FX_EOF);
        SSH_CASE_SFTP_RETURN_STRINGISED(FX_NO_SUCH_FILE);
        SSH_CASE_SFTP_RETURN_STRINGISED(FX_PERMISSION_DENIED);
        SSH_CASE_SFTP_RETURN_STRINGISED(FX_FAILURE);
        SSH_CASE_SFTP_RETURN_STRINGISED(FX_BAD_MESSAGE);
        SSH_CASE_SFTP_RETURN_STRINGISED(FX_NO_CONNECTION);
        SSH_CASE_SFTP_RETURN_STRINGISED(FX_CONNECTION_LOST);
        SSH_CASE_SFTP_RETURN_STRINGISED(FX_OP_UNSUPPORTED);
        SSH_CASE_SFTP_RETURN_STRINGISED(FX_INVALID_HANDLE);
        SSH_CASE_SFTP_RETURN_STRINGISED(FX_NO_SUCH_PATH);
        SSH_CASE_SFTP_RETURN_STRINGISED(FX_FILE_ALREADY_EXISTS);
        SSH_CASE_SFTP_RETURN_STRINGISED(FX_WRITE_PROTECT);
        SSH_CASE_SFTP_RETURN_STRINGISED(FX_NO_MEDIA);
        SSH_CASE_SFTP_RETURN_STRINGISED(FX_NO_SPACE_ON_FILESYSTEM);
        SSH_CASE_SFTP_RETURN_STRINGISED(FX_QUOTA_EXCEEDED);
        SSH_CASE_SFTP_RETURN_STRINGISED(FX_UNKNOWN_PRINCIPAL);
        SSH_CASE_SFTP_RETURN_STRINGISED(FX_LOCK_CONFLICT);
        SSH_CASE_SFTP_RETURN_STRINGISED(FX_DIR_NOT_EMPTY);
        SSH_CASE_SFTP_RETURN_STRINGISED(FX_NOT_A_DIRECTORY);
        SSH_CASE_SFTP_RETURN_STRINGISED(FX_INVALID_FILENAME);
        SSH_CASE_SFTP_RETURN_STRINGISED(FX_LINK_LOOP);
    default:
        assert(!"Unknown code");
        return boost::lexical_cast<std::string>(code);
    }
}

#undef SSH_CASE_SFTP_RETURN_STRINGISED

class _sftp_error_category : public boost::system::error_category
{
    typedef boost::system::error_category super;

public:
    virtual const char* name() const BOOST_NOEXCEPT
    {
        return "sftp";
    }

    virtual std::string message(int code) const
    {
        return sftp_error_code_to_string(code);
    }

    virtual boost::system::error_condition
    default_error_condition(int code) const BOOST_NOEXCEPT
    {
        switch (code)
        {
        case LIBSSH2_FX_NO_SUCH_FILE:
            return boost::system::errc::no_such_file_or_directory;

        case LIBSSH2_FX_FILE_ALREADY_EXISTS:
            return boost::system::errc::file_exists;

        case LIBSSH2_FX_OP_UNSUPPORTED:
            return boost::system::errc::operation_not_supported;
        default:
            return this->super::default_error_condition(code);
        }
    }

    virtual bool equivalent(
        int code,
        const boost::system::error_condition& condition) const BOOST_NOEXCEPT
    {
        // Any match with the code's default condition is equivalent. The
        // switch below only needs to match _extra_ conditions that are
        // also equivalent

        if (condition == default_error_condition(code))
        {
            return true;
        }

        switch (code)
        {
        case LIBSSH2_FX_OP_UNSUPPORTED:
            return condition == boost::system::errc::not_supported;
        default:
            return condition == default_error_condition(code);
        }
    }

private:
    _sftp_error_category()
    {
    }

    friend boost::system::error_category&
    ssh::filesystem::sftp_error_category();
};
}

inline boost::system::error_category& sftp_error_category()
{
    // C++ standard says this instance is shared across all translation units
    // http://stackoverflow.com/a/1389403/67013
    static detail::_sftp_error_category instance;
    return instance;
}

namespace detail
{

/**
 * Last error encountered by the SFTP channel as an `error_code` and
 * optional error description message.
 */
inline boost::system::error_code last_sftp_error_code(
    LIBSSH2_SESSION* session, LIBSSH2_SFTP* sftp,
    boost::optional<std::string&> e_msg = boost::optional<std::string&>())
{
    // Failing libssh2_sftp_* functions can set an SSH error defined
    // by the library or an SFTP error defined in the SFTP standard,
    // in which case the SSH error will be LIBSSH2_ERROR_SFTP_PROTOCOL.
    // This function checks which case it is and packages the error
    // with the corresponding category.

    boost::system::error_code error =
        ::ssh::detail::last_error_code(session, e_msg);

    if (error.value() == LIBSSH2_ERROR_SFTP_PROTOCOL)
    {
        error = boost::system::error_code(::libssh2_sftp_last_error(sftp),
                                          sftp_error_category());
    }

    return error;
}
}
}
} // namespace ssh::filesystem

#endif
