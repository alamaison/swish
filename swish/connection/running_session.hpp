/**
    @file

    C++ wrapper round Libssh2 SSH and SFTP session creation.

    @if license

    Copyright (C) 2008, 2009, 2010, 2013
    Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#pragma once

#include <boost/asio/ip/tcp.hpp> // Boost sockets
#include <boost/shared_ptr.hpp> // shared_ptr
#include <boost/thread/mutex.hpp>

#include <string>

typedef struct _LIBSSH2_SESSION LIBSSH2_SESSION; // Forwards-decls
typedef struct _LIBSSH2_SFTP LIBSSH2_SFTP;


namespace swish {
namespace connection {

class running_session
{
public:

    /**
     * Connect to host server and start new SSH connection on given port.
     */
    running_session(const std::wstring& host, unsigned int port);

    ~running_session();
    boost::mutex::scoped_lock aquire_lock();
    operator LIBSSH2_SESSION*() const;
    operator LIBSSH2_SFTP*() const;

    void StartSftp() throw(...);
    bool IsDead();

    boost::shared_ptr<LIBSSH2_SESSION> get() { return m_session; }
    boost::shared_ptr<LIBSSH2_SFTP> sftp() { return m_sftp_session; }
private:
    boost::mutex m_mutex;
    boost::asio::io_service m_io; ///< Boost IO system
    boost::asio::ip::tcp::socket m_socket; ///< TCP/IP socket to remote host
    boost::shared_ptr<LIBSSH2_SESSION> m_session;   ///< SSH session
    boost::shared_ptr<LIBSSH2_SFTP> m_sftp_session;  ///< SFTP subsystem session

    running_session(const running_session& session); // Intentionally not implemented
    running_session& operator=(const running_session& pidl); // Intentionally not impl
    
    void _OpenSocketToHost(const wchar_t* pszHost, unsigned int uPort);
    void _CloseSocketToHost() throw();

    void _CreateSession() throw(...);

    void _CreateSftpChannel() throw(...);
};

}} // namespace swish::connection
