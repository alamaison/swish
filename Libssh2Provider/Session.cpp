/*  Libssh2 SSH and SFTP session management

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

#include "StdAfx.h"
#include "Session.h"
#include "../remotelimits.h"

#include <ws2tcpip.h>    // Winsock
#include <wspiapi.h>     // Winsock

#include <libssh2.h>
#include <libssh2_sftp.h>

CSession::CSession() throw(...) : 
	m_pSession(NULL), m_pSftpSession(NULL), m_socket(INVALID_SOCKET),
	m_bConnected(false)
{
	_CreateSession();
	ATLASSUME(m_pSession);
}

CSession::~CSession()
{
	_DestroySession();
}

CSession::operator LIBSSH2_SESSION*() const
{
	ATLASSUME(m_pSession);
	return m_pSession;
}

CSession::operator LIBSSH2_SFTP*() const
{
	ATLASSUME(m_pSftpSession);
	return m_pSftpSession;
}

void CSession::Connect(PCWSTR pwszHost, unsigned int uPort) throw(...)
{
	// Are we already connected?
	if (m_bConnected)
		return;
	
	// Connect to host over TCP/IP
	if (m_socket == INVALID_SOCKET)
		_OpenSocketToHost(pwszHost, uPort);

	// Start up libssh2 and trade welcome banners, exchange keys,
    // setup crypto, compression, and MAC layers
	ATLASSERT(m_socket != INVALID_SOCKET);
	if (libssh2_session_startup(*this, static_cast<int>(m_socket)) != 0)
	{
#ifdef _DEBUG
		char *szError;
		int cchError;
		int rc = libssh2_session_last_error(*this, &szError, &cchError, false);
		ATLTRACE("libssh2_sftp_init failed (%d): %s", rc, szError);
#endif
		_ResetSession();
		_CloseSocketToHost();
	
		AtlThrow(E_FAIL); // Legal to fail here, e.g. server refuses banner/kex
	}
	
	// Tell libssh2 we are blocking
	libssh2_session_set_blocking(*this, 1);

	m_bConnected = true;
}

void CSession::StartSftp() throw(...)
{
	_CreateSftpChannel();
}


/*----------------------------------------------------------------------------*
 * Private methods
 *----------------------------------------------------------------------------*/

/**
 * Allocate a blocking LIBSSH2_SESSION instance.
 */
void CSession::_CreateSession() throw(...)
{
	// Create a session instance
	m_pSession = libssh2_session_init();
	ATLENSURE_THROW( m_pSession, E_FAIL );
}

/**
 * Free a LIBSSH2_SESSION instance.
 */
void CSession::_DestroySession() throw()
{
	ATLASSUME(m_pSession);
	if (m_pSession)	// dual of libssh2_session_init()
	{
		libssh2_session_free(m_pSession);
		m_pSession = NULL;
	}
}

/**
 * Destroy and recreate a LIBSSH2_SESSION instance.
 *
 * A session instance which has been used in a libssh2_session_startup call
 * cannot be reused safely.
 */
void CSession::_ResetSession() throw(...)
{
	_DestroySession();
	_CreateSession();
}

/**
 * Start up an SFTP channel on this SSH session.
 */
void CSession::_CreateSftpChannel() throw(...)
{
	ATLASSUME(m_pSftpSession == NULL);

	if (libssh2_userauth_authenticated(*this) == 0)
		AtlThrow(E_UNEXPECTED); // We must be authenticated first

    m_pSftpSession = libssh2_sftp_init(*this); // Start up SFTP session
	if (!m_pSftpSession)
	{
#ifdef _DEBUG
		char *szError;
		int cchError;
		int rc = libssh2_session_last_error(*this, &szError, &cchError, false);
		ATLTRACE("libssh2_sftp_init failed (%d): %s", rc, szError);
#endif
		AtlThrow(E_FAIL);
	}
}

/**
 * Shut down the SFTP channel.
 */
void CSession::_DestroySftpChannel() throw()
{
	if (m_pSftpSession) // dual of libssh2_sftp_init()
	{
		ATLVERIFY( !libssh2_sftp_shutdown(*this) );
		m_pSftpSession = NULL;
	}
}

/**
 * Creates a socket and connects it to the host.
 *
 * The socket is stored as the member variable @c m_socket. The hostname 
 * and port used are passed as parameters.  If the socket has already been 
 * initialised, the function leaves it unchanged (or asserts in a DEBUG build).
 * If any errors occur, the socket is set to @c INVALID_SOCKET and an ATL
 * exception is thrown.
 *
 * @throws a CAtlException if there is a failure.
 *
 * @remarks The socket should be cleaned up when no longer needed using
 *          @c _CloseSocketToHost()
 */
void CSession::_OpenSocketToHost(PCWSTR pwszHost, unsigned int uPort) throw(...)
{
	ATLASSERT(pwszHost[0] != '\0');
	ATLASSERT(uPort >= MIN_PORT && uPort <= MAX_PORT);
	ATLENSURE_THROW(m_socket == INVALID_SOCKET, E_UNEXPECTED); // Already set up

	// The hints address info struct which is passed to getaddrinfo()
	addrinfo aiHints;
	::ZeroMemory(&aiHints, sizeof(aiHints));
	aiHints.ai_family = AF_INET;
	aiHints.ai_socktype = SOCK_STREAM;
	aiHints.ai_protocol = IPPROTO_TCP;

	// Convert numeric port to a UTF-8 string
	char szPort[6];
	ATLENSURE_THROW(!::_itoa_s(uPort, szPort, 6, 10), E_UNEXPECTED);

	// Convert host address to a UTF-8 string
	CW2A pszAddress(pwszHost, CP_UTF8);

	// Call getaddrinfo(). If the call succeeds, paiList will hold a linked list
	// of addrinfo structures containing response information about the host.
	addrinfo *paiList = NULL;
	int rc = ::getaddrinfo(pszAddress, szPort, &aiHints, &paiList);
	// It is valid to fail here - e.g. unknown host
	if (rc)
		AtlThrow(E_FAIL);
	ATLASSERT(paiList);
	ATLASSERT(paiList->ai_addr);

	// Create socket and establish connection
	HRESULT hr;
	m_socket = ::socket(AF_INET, SOCK_STREAM, 0);
	ATLASSERT_REPORT(m_socket != INVALID_SOCKET, ::WSAGetLastError());
	if (m_socket != INVALID_SOCKET)
	{
		int rc = ::connect(
			m_socket, paiList->ai_addr, static_cast<int>(paiList->ai_addrlen));
		if (rc != 0)
		{
			hr = E_FAIL;
			_CloseSocketToHost(); // Clean up socket
		}
		else
			hr = S_OK;
	}
	else
		hr = E_UNEXPECTED;

	::freeaddrinfo(paiList);

	if (FAILED(hr))
		AtlThrow(hr);
}

/**
 * Closes the socket stored in @c m_socket and sets is to @c INVALID_SOCKET.
 */
void CSession::_CloseSocketToHost() throw()
{
	if (m_socket != INVALID_SOCKET)
	{
		ATLVERIFY_REPORT( !::closesocket(m_socket), ::WSAGetLastError() );
		m_socket = INVALID_SOCKET;
	}
}