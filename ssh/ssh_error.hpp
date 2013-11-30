/**
    @file

    SSH exception.

    @if license

    Copyright (C) 2010, 2013  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#ifndef SSH_SSH_ERROR_HPP
#define SSH_SSH_ERROR_HPP

#include <boost/exception/exception.hpp> // boost::exception, enable_error_info
#include <boost/exception/errinfo_file_name.hpp> // errinfo_file_name
#include <boost/exception/info.hpp> // errinfo_api_function
#include <boost/throw_exception.hpp> // throw_exception
#include <boost/lexical_cast.hpp>
#include <boost/optional/optional.hpp>
#include <boost/shared_ptr.hpp> // shared_ptr
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>

#include <exception> // std::exception
#include <cassert> // assert
#include <string>

#include <libssh2.h>

namespace ssh {

inline boost::system::error_category& ssh_error_category();

namespace detail {

#define SSH_CASE_RETURN_STRINGISED(x) case x: return #x;

    inline std::string ssh_error_code_to_string(int code)
    {
        switch (code)
        {
            SSH_CASE_RETURN_STRINGISED(LIBSSH2_ERROR_SOCKET_NONE);
            SSH_CASE_RETURN_STRINGISED(LIBSSH2_ERROR_BANNER_RECV);
            SSH_CASE_RETURN_STRINGISED(LIBSSH2_ERROR_BANNER_SEND);
            SSH_CASE_RETURN_STRINGISED(LIBSSH2_ERROR_INVALID_MAC);
            SSH_CASE_RETURN_STRINGISED(LIBSSH2_ERROR_KEX_FAILURE);
            SSH_CASE_RETURN_STRINGISED(LIBSSH2_ERROR_ALLOC);
            SSH_CASE_RETURN_STRINGISED(LIBSSH2_ERROR_SOCKET_SEND);
            SSH_CASE_RETURN_STRINGISED(LIBSSH2_ERROR_KEY_EXCHANGE_FAILURE);
            SSH_CASE_RETURN_STRINGISED(LIBSSH2_ERROR_TIMEOUT);
            SSH_CASE_RETURN_STRINGISED(LIBSSH2_ERROR_HOSTKEY_INIT);
            SSH_CASE_RETURN_STRINGISED(LIBSSH2_ERROR_HOSTKEY_SIGN);
            SSH_CASE_RETURN_STRINGISED(LIBSSH2_ERROR_DECRYPT);
            SSH_CASE_RETURN_STRINGISED(LIBSSH2_ERROR_SOCKET_DISCONNECT);
            SSH_CASE_RETURN_STRINGISED(LIBSSH2_ERROR_PROTO);
            SSH_CASE_RETURN_STRINGISED(LIBSSH2_ERROR_PASSWORD_EXPIRED);
            SSH_CASE_RETURN_STRINGISED(LIBSSH2_ERROR_FILE);
            SSH_CASE_RETURN_STRINGISED(LIBSSH2_ERROR_METHOD_NONE);
            SSH_CASE_RETURN_STRINGISED(LIBSSH2_ERROR_AUTHENTICATION_FAILED);
            SSH_CASE_RETURN_STRINGISED(LIBSSH2_ERROR_PUBLICKEY_UNVERIFIED);
            SSH_CASE_RETURN_STRINGISED(LIBSSH2_ERROR_CHANNEL_OUTOFORDER);
            SSH_CASE_RETURN_STRINGISED(LIBSSH2_ERROR_CHANNEL_FAILURE);
            SSH_CASE_RETURN_STRINGISED(LIBSSH2_ERROR_CHANNEL_REQUEST_DENIED);
            SSH_CASE_RETURN_STRINGISED(LIBSSH2_ERROR_CHANNEL_UNKNOWN);
            SSH_CASE_RETURN_STRINGISED(LIBSSH2_ERROR_CHANNEL_WINDOW_EXCEEDED);
            SSH_CASE_RETURN_STRINGISED(LIBSSH2_ERROR_CHANNEL_PACKET_EXCEEDED);
            SSH_CASE_RETURN_STRINGISED(LIBSSH2_ERROR_CHANNEL_CLOSED);
            SSH_CASE_RETURN_STRINGISED(LIBSSH2_ERROR_CHANNEL_EOF_SENT);
            SSH_CASE_RETURN_STRINGISED(LIBSSH2_ERROR_SCP_PROTOCOL);
            SSH_CASE_RETURN_STRINGISED(LIBSSH2_ERROR_ZLIB);
            SSH_CASE_RETURN_STRINGISED(LIBSSH2_ERROR_SOCKET_TIMEOUT);
            SSH_CASE_RETURN_STRINGISED(LIBSSH2_ERROR_SFTP_PROTOCOL);
            SSH_CASE_RETURN_STRINGISED(LIBSSH2_ERROR_REQUEST_DENIED);
            SSH_CASE_RETURN_STRINGISED(LIBSSH2_ERROR_METHOD_NOT_SUPPORTED);
            SSH_CASE_RETURN_STRINGISED(LIBSSH2_ERROR_INVAL);
            SSH_CASE_RETURN_STRINGISED(LIBSSH2_ERROR_INVALID_POLL_TYPE);
            SSH_CASE_RETURN_STRINGISED(LIBSSH2_ERROR_PUBLICKEY_PROTOCOL);
            SSH_CASE_RETURN_STRINGISED(LIBSSH2_ERROR_EAGAIN);
            SSH_CASE_RETURN_STRINGISED(LIBSSH2_ERROR_BUFFER_TOO_SMALL);
            SSH_CASE_RETURN_STRINGISED(LIBSSH2_ERROR_BAD_USE);
            SSH_CASE_RETURN_STRINGISED(LIBSSH2_ERROR_COMPRESS);
            SSH_CASE_RETURN_STRINGISED(LIBSSH2_ERROR_OUT_OF_BOUNDARY);
            SSH_CASE_RETURN_STRINGISED(LIBSSH2_ERROR_AGENT_PROTOCOL);
            SSH_CASE_RETURN_STRINGISED(LIBSSH2_ERROR_SOCKET_RECV);
            SSH_CASE_RETURN_STRINGISED(LIBSSH2_ERROR_ENCRYPT);
            SSH_CASE_RETURN_STRINGISED(LIBSSH2_ERROR_BAD_SOCKET);
        default:
            assert(!"Unknown code");
            return boost::lexical_cast<std::string>(code);
        }
    }

#undef SSH_CASE_RETURN_STRINGISED

    class _ssh_error_category : public boost::system::error_category
    {
    public:
        const char* name() const
        {
            return "ssh";
        }

        std::string message(int code) const
        {
            return ssh_error_code_to_string(code);
        }

    private:
        _ssh_error_category() {}

        friend boost::system::error_category& ssh::ssh_error_category();
    };

}

inline boost::system::error_category& ssh_error_category()
{
    // C++ standard says this instance is shared across all translation units
    // http://stackoverflow.com/a/1389403/67013
    static detail::_ssh_error_category instance;
    return instance;
}

/**
 * Exception type thrown when libssh2 returns an error.
 */
class ssh_error :
    public virtual boost::exception, public virtual std::exception
{
public:

    ssh_error(const char* message, int error_code)
        : boost::exception(), std::exception(),
          m_message(message), m_error_code(error_code)
    {
    }

    ssh_error(const char* message, int len, int error_code)
        : boost::exception(), std::exception(message, len),
          m_message(message, len), m_error_code(error_code)
    {
    }

    virtual const char* what() const
    {
        try
        {
            return m_message.c_str();
        }
        catch (std::exception&)
        {
            return "Unknown SSH error";
        }
    }

    int error_code() const
    {
        return m_error_code;
    }

protected:
    std::string& message()
    {
        return m_message;
    }

private:
    std::string m_message;
    int m_error_code;
};

namespace detail {

    /**
     * Last error encountered by the session as an `error_code` and optional
     * error description message.
     */
    inline boost::system::error_code last_error_code(
        LIBSSH2_SESSION* session, 
        boost::optional<std::string&> e_msg=boost::optional<std::string&>())
    {
        int val = 0;

        if (e_msg)
        {
            char* message_buf = NULL; // read-only reference
            int message_len = 0; // len not including NULL-term
            val = ::libssh2_session_last_error(
                session, &message_buf, &message_len, false);

            *e_msg = std::string(message_buf, message_len);
        }
        else
        {
            val = ::libssh2_session_last_errno(session);
        }

        assert(val && "throwing success!");
        return boost::system::error_code(val, ssh_error_category());
    }

    // Assumes given exception supports error info already.
    // We don't know the type of the error-info-enabled exception.  This
    // function means we don't need to as it takes it as a template arg
    template<typename Exception>
    BOOST_ATTRIBUTE_NORETURN inline void throw_api_exception(
        Exception e, const char* current_function, const char* source_file,
        int source_line, const char* api_function,
        const char* path, size_t path_len)
    {
        e <<
            boost::errinfo_api_function(api_function) <<
            boost::throw_function(current_function) <<
            boost::throw_file(source_file) <<
            boost::throw_line(source_line);

        if (path && path_len > 0)
        {
            e << boost::errinfo_file_name(std::string(path, path_len));
        }

        boost::throw_exception(e);
    }

    BOOST_ATTRIBUTE_NORETURN inline void throw_api_error_code(
        boost::system::error_code ec, const std::string& message,
        const char* current_function, const char* source_file,
        int source_line, const char* api_function,
        const char* path, size_t path_len)
    {
        throw_api_exception(
            boost::enable_error_info(boost::system::system_error(ec, message)),
            current_function, source_file, source_line, api_function, path,
            path_len);
    }

    /**
     * Last error encountered by the session as an exception.
     */
    inline ssh_error last_error(LIBSSH2_SESSION* session)
    {
        char* message_buf = NULL; // read-only reference
        int message_len = 0; // len not including NULL-term
        int err = libssh2_session_last_error(
            session, &message_buf, &message_len, false);

        assert(err && "throwing success!");

        return ssh_error(message_buf, message_len, err);
    }

    inline ssh_error last_error(boost::shared_ptr<LIBSSH2_SESSION> session)
    {
        return last_error(session.get());
    }

} // namespace detail

} // namespace ssh

/// @cond INTERNAL

#define SSH_DETAIL_THROW_API_ERROR_CODE(ec, message, api_function) \
    ::ssh::detail::throw_api_error_code( \
        ec, message, BOOST_CURRENT_FUNCTION,__FILE__,__LINE__, api_function, \
        NULL, 0)

#define SSH_DETAIL_THROW_API_ERROR_CODE_WITH_PATH( \
    ec, message, api_function, path, path_len) \
    ::ssh::detail::throw_api_error_code( \
        ec, message, BOOST_CURRENT_FUNCTION,__FILE__,__LINE__, api_function, \
        path, path_len)

/// @endcond

#endif
