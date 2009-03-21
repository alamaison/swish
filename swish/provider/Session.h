/*  C++ wrapper round Libssh2 SSH and SFTP session creation.

    Copyright (C) 2008  Alexander Lamaison <awl03@doc.ic.ac.uk>

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
*/

#pragma once

typedef struct _LIBSSH2_SESSION LIBSSH2_SESSION; // Forwards-decls
typedef struct _LIBSSH2_SFTP LIBSSH2_SFTP;

class CSession
{
public:
	CSession() throw(...);
	~CSession();
	operator LIBSSH2_SESSION*() const;
	operator LIBSSH2_SFTP*() const;

	void Connect(PCWSTR pwszHost, unsigned int uPort) throw(...);
	void StartSftp() throw(...);

private:
	LIBSSH2_SESSION *m_pSession;   ///< SSH session
	SOCKET m_socket;               ///< TCP/IP socket to the remote host
	LIBSSH2_SFTP *m_pSftpSession;  ///< SFTP subsystem session
	bool m_bConnected;             ///< Have we already connected to server?

	CSession(const CSession& session); // Intentionally not implemented
	CSession& operator=(const CSession& pidl); // Intentionally not impl
	
	void _OpenSocketToHost(PCWSTR pszHost, unsigned int uPort) throw(...);
	void _CloseSocketToHost() throw();

	void _CreateSession() throw(...);
	void _DestroySession() throw();
	void _ResetSession() throw(...);

	void _CreateSftpChannel() throw(...);
	void _DestroySftpChannel() throw();
};
