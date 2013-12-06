/**
    @file

    RAII lifetime management of libssh2 file handles.

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

#ifndef SSH_DETAIL_FILE_HANDLE_STATE_HPP
#define SSH_DETAIL_FILE_HANDLE_STATE_HPP

#include <ssh/detail/libssh2/sftp.hpp> // open
#include <ssh/detail/sftp_channel_state.hpp>

#include <boost/move/move.hpp> // BOOST_RV_REF
#include <boost/noncopyable.hpp>

#include <string>

#include <libssh2_sftp.h> // LIBSSH2_SFTP_HANDLE

namespace ssh {
namespace detail {

inline LIBSSH2_SFTP_HANDLE* do_open(
    boost::shared_ptr<sftp_channel_state> sftp,
    const char* filename, unsigned int filename_len, unsigned long flags,
    long mode, int open_type)
{
    session_state::scoped_lock lock = sftp->aquire_lock();

    return libssh2::sftp::open(
        sftp->session_ptr(), sftp->sftp_ptr(), filename, filename_len, flags,
        mode, open_type);
}

/**
 * RAII object managing SFTP file handle state that must be maintained together.
 *
 * Manages the graceful opening/closing of file handles.
 */
class file_handle_state : private boost::noncopyable
{
    BOOST_MOVABLE_BUT_NOT_COPYABLE(file_handle_state)

public:

    typedef sftp_channel_state::scoped_lock scoped_lock;

    /**
     * Creates a new file handle that closes itself in a thread-safe manner
     * when it goes out of scope.
     */
    file_handle_state(
        boost::shared_ptr<sftp_channel_state> sftp,
        const char* filename, unsigned int filename_len, unsigned long flags,
        long mode, int open_type)
        :
    m_sftp(sftp),
    m_handle(
        do_open(m_sftp, filename, filename_len, flags, mode, open_type)) {}

    file_handle_state(BOOST_RV_REF(file_handle_state) other)
        : m_sftp(boost::move(other.m_sftp)),
          m_handle(boost::move(other.m_handle))
    {}

    ~file_handle_state() throw()
    {
        sftp_channel_state::scoped_lock lock = m_sftp->aquire_lock();

        ::libssh2_sftp_close_handle(m_handle);
    }

    scoped_lock aquire_lock()
    {
        return m_sftp->aquire_lock();
    }

    LIBSSH2_SESSION* session_ptr()
    {
        return m_sftp->session_ptr();
    }

    LIBSSH2_SFTP* sftp_ptr()
    {
        return m_sftp->sftp_ptr();
    }

    LIBSSH2_SFTP_HANDLE* file_handle()
    {
        return m_handle;
    }

private:

    boost::shared_ptr<sftp_channel_state> m_sftp;
    LIBSSH2_SFTP_HANDLE* m_handle;
};

}} // namespace ssh::detail

#endif
