/**
    @file

    Factory producing connected, authenticated CSession objects.

    @if licence

    Copyright (C) 2008, 2009  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "pch.h"
#include "SessionFactory.hpp"

#include "KeyboardInteractive.hpp"
#include "common/debug.hpp"

#include <libssh2.h>
#include <libssh2_sftp.h>

#include "common/atl.hpp"         // Common ATL setup

#include <string>

using ATL::CT2A;
using ATL::CComBSTR;

using std::string;
using std::auto_ptr;

#pragma warning (push)
#pragma warning (disable: 4267) // ssize_t to unsigned int

/**
 * Creates and authenticates a CSession object with the given parameters.
 *
 * @param pwszHost   Name of the remote host to connect the CSession object to.
 * @param pwszUser   User to connect to remote host as.
 * @param uPort      Port on the remote host to connect to.
 * @param pConsumer  Pointer to an ISftpConsumer which will be used for any 
 *                   user-interaction such as requesting a password for
 *                   authentication.
 * @returns  An auto_ptr to a CSession which is connected to the given 
 *           host (subject to verification of the host's key), authenticated and 
 *           over which an SFTP channel has been started.
 *
 * @throws CAtlException if any part of this process fails:
 * - E_ABORT if user cancelled the operation (via ISftpConsumer)
 * - E_FAIL otherwise
 */
/* static */ auto_ptr<CSession> CSessionFactory::CreateSftpSession(
	PCWSTR pwszHost, unsigned int uPort, PCWSTR pwszUser,
	ISftpConsumer *pConsumer) throw(...)
{
	auto_ptr<CSession> spSession( new CSession() );
	spSession->Connect(pwszHost, uPort);

    // Check the hostkey against our known hosts
	_VerifyHostKey(*spSession, pConsumer);
	// Legal to fail here, e.g. user refused to accept host key

    // Authenticate the user with the remote server
	_AuthenticateUser(pwszUser, *spSession, pConsumer);
	// Legal to fail here, e.g. wrong password/key

	spSession->StartSftp();

	return spSession;
}

void CSessionFactory::_VerifyHostKey(
	CSession& session, ISftpConsumer *pConsumer) throw(...)
{
	ATLASSUME(pConsumer); UNREFERENCED_PARAMETER(pConsumer);

	TRACE("Verifying host key:");
	const char *fingerprint = 
		libssh2_hostkey_hash(session, LIBSSH2_HOSTKEY_HASH_MD5);
	(void)fingerprint;
	const char *key_type = 
		libssh2_session_methods(session, LIBSSH2_METHOD_HOSTKEY);
	(void)key_type;
	
	TRACE("Fingerprint (%s)", key_type);

	// TODO: check the key is known here
	ATLTRACE("\t");
	for (int i = 0; i < 16; i++) {
		ATLTRACE("%02X ", (unsigned char)fingerprint[i]);
	}
	ATLTRACE("\n");
}

/**
 * Tries to authenticate the user with the remote server.
 *
 * The remote server is queried for which authentication methods it supports
 * and these are tried one at time until one succeeds in the order:
 * public-key, keyboard-interactive, plain password.
 *
 * @throws CAtlException if authentication fails:
 * - E_ABORT if user cancelled the operation (via ISftpConsumer)
 * - E_FAIL otherwise
 */
void CSessionFactory::_AuthenticateUser(
	PCWSTR pwszUser, CSession& session, ISftpConsumer *pConsumer) throw(...)
{
	ATLASSUME(pwszUser[0] != '\0');
	CT2A szUsername(pwszUser);

	// Check which authentication methods are available
	char *szAuthList = libssh2_userauth_list(
		session, szUsername, ::strlen(szUsername));
	if (!szAuthList || *szAuthList == '\0')
	{
		TRACE("No supported authentication methods found!");
		AtlThrow(E_FAIL); // If empty, server refused to let user connect
	}

	TRACE("Authentication methods: %s", szAuthList);

	// Try each supported authentication method in turn until one succeeds
	HRESULT hr = E_FAIL;
	if (::strstr(szAuthList, "publickey"))
	{
		TRACE("Trying public-key authentication");
		hr = _PublicKeyAuthentication(szUsername, session, pConsumer);
	}
	if (FAILED(hr) && ::strstr(szAuthList, "keyboard-interactive"))
	{
		TRACE("Trying keyboard-interactive authentication");
		hr = _KeyboardInteractiveAuthentication(szUsername, session, pConsumer);
		if (hr == E_ABORT)
			AtlThrow(hr); // User cancelled
	}
	if (FAILED(hr) && ::strstr(szAuthList, "password"))
	{
		TRACE("Trying simple password authentication");
		hr = _PasswordAuthentication(szUsername, session, pConsumer);
	}

	if (FAILED(hr))
		AtlThrow(hr);
}

/**
 * Authenticates with remote host by asking the user to supply a password.
 *
 * This uses the callback to the SftpConsumer to obtain the password from
 * the user.  If the password is wrong or other error occurs, the user is
 * asked for the password again.  This repeats until the user supplies a 
 * correct password or cancels the request.
 *
 * @throws CAtlException if authentication fails:
 * - E_ABORT if user cancelled the operation (via ISftpConsumer)
 * - E_FAIL otherwise
 */
HRESULT CSessionFactory::_PasswordAuthentication(
	PCSTR szUsername, CSession& session, ISftpConsumer *pConsumer)
{
	HRESULT hr;
	CComBSTR bstrPrompt = _T("Please enter your password:");
	CComBSTR bstrPassword;

	// Loop until successfully authenticated or request returns cancel
	int ret = -1; // default to failure
	do {
		hr = pConsumer->OnPasswordRequest( bstrPrompt, &bstrPassword );
		if FAILED(hr)
			return hr;
		CT2A szPassword(bstrPassword);
		ret = libssh2_userauth_password(session, szUsername, szPassword);
		// TODO: handle password change callback here
		bstrPassword.Empty(); // Prevent memory leak on repeat
	} while (ret != 0);

	ATLASSERT(SUCCEEDED(hr)); ATLASSERT(ret == 0);
	ATLASSERT(libssh2_userauth_authenticated(session)); // Double-check
	return hr;
}

/**
 * Authenticates with remote host by challenge-response interaction.
 *
 * This uses the ISftpConsumer callback to challenge the user for various
 * pieces of information (usually just their password).
 *
 * @throws CAtlException if authentication fails:
 * - E_ABORT if user cancelled the operation (via ISftpConsumer)
 * - E_FAIL otherwise
 */
HRESULT CSessionFactory::_KeyboardInteractiveAuthentication(
	PCSTR szUsername, CSession& session, ISftpConsumer *pConsumer)
{
	// Create instance of keyboard-interactive authentication handler
	CKeyboardInteractive handler(pConsumer);
	
	// Pass pointer to handler in session abstract and begin authentication.
	// The static callback method (last parameter) will extract the 'this'
	// pointer from the session and use it to invoke the handler instance.
	// If the user cancels the operation, our callback should throw an
	// E_ABORT exception which we catch here.
	*libssh2_session_abstract(session) = &handler;
	int rc = libssh2_userauth_keyboard_interactive(session,
		szUsername, &(CKeyboardInteractive::OnKeyboardInteractive));
	
	// Check for two possible types of failure
	if (FAILED(handler.GetErrorState()))
		return handler.GetErrorState();

	ATLASSERT(rc || libssh2_userauth_authenticated(session)); // Double-check
	return (rc == 0) ? S_OK : E_FAIL;
}

HRESULT CSessionFactory::_PublicKeyAuthentication(
	PCSTR szUsername, CSession& session, ISftpConsumer *pConsumer)
{
	ATLASSUME(pConsumer); UNREFERENCED_PARAMETER(pConsumer);

	// TODO: use proper file paths
	const char *keyfile1="~/.ssh/id_rsa.pub";
	const char *keyfile2="~/.ssh/id_rsa";
	// TODO: unlock public key using passphrase
	if (libssh2_userauth_publickey_fromfile(
			session, szUsername, keyfile1, keyfile2, ""))
		return E_ABORT;

	ATLASSERT(libssh2_userauth_authenticated(session)); // Double-check
	return S_OK;
}

#pragma warning (pop)