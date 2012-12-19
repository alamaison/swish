/**
    @file

    Factory producing connected, authenticated CSession objects.

    @if license

    Copyright (C) 2008, 2009, 2010, 2012
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

#include "SessionFactory.hpp"

#include "KeyboardInteractive.hpp"

#include "swish/interfaces/SftpProvider.h" // ISftpConsumer
#include "swish/debug.hpp"
#include "swish/trace.hpp"
#include "swish/utils.hpp"

#include <ssh/knownhost.hpp> // openssh_knownhost_collection
#include <ssh/session.hpp> // session

#include <winapi/com/catch.hpp> // WINAPI_COM_CATCH_AUTO_INTERFACE

#include <comet/bstr.h> // bstr_t;

#include <boost/filesystem.hpp> // wpath
#include <boost/filesystem/fstream.hpp> // ofstream

#include <libssh2.h>
#include <libssh2_sftp.h>

#include "swish/atl.hpp"  // Common ATL setup

#include <exception>
#include <string>

using swish::tracing::trace;
using swish::utils::WideStringToUtf8String;
using swish::utils::home_directory;

using ssh::host_key::hexify;
using ssh::host_key::host_key;
using ssh::knownhost::find_result;
using ssh::knownhost::openssh_knownhost_collection;

using comet::bstr_t;

using boost::filesystem::wpath;
using boost::filesystem::ofstream;

using std::auto_ptr;
using std::exception;
using std::string;

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
 * @throws com_error if any part of this process fails:
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
    _VerifyHostKey(pwszHost, *spSession, pConsumer);
    // Legal to fail here, e.g. user refused to accept host key

    // Authenticate the user with the remote server
    _AuthenticateUser(pwszUser, *spSession, pConsumer);
    // Legal to fail here, e.g. wrong password/key

    spSession->StartSftp();

    return spSession;
}

const wpath known_hosts_path =
    home_directory<wpath>() / L".ssh" / L"known_hosts";

void CSessionFactory::_VerifyHostKey(
    PCWSTR pwszHost, CSession& session, ISftpConsumer *pConsumer)
{
    ATLASSUME(pConsumer);

    ssh::session sess(session.get());

    bstr_t host = pwszHost;
    host_key key = sess.hostkey();
    bstr_t hostkey_algorithm = key.algorithm_name();
    bstr_t hostkey_hash = hexify(key.md5_hash());

    assert(!hostkey_hash.empty());
    assert(!hostkey_algorithm.empty());
    
    trace("host-key fingerprint: %1%\t(%2%)")
        % hostkey_algorithm % hostkey_hash;

    // make sure known_hosts file exists
    create_directories(known_hosts_path.parent_path());
    ofstream(known_hosts_path, std::ios::app);

    openssh_knownhost_collection hosts(session.get(), known_hosts_path);

    find_result result = hosts.find(host.s_str(), key);
    if (result.mismatch())
    {
        HRESULT hr = pConsumer->OnHostkeyMismatch(
            host.in(), hostkey_hash.in(), hostkey_algorithm.in());
        if (hr == S_OK)
        {
            update(hosts, host, key, result); // update known_hosts
            hosts.save(known_hosts_path);
        }
        else if (hr == S_FALSE)
            return; // continue but don't add
        else
            AtlThrow(E_ABORT); // screech to a halt
    }
    else if (result.not_found())
    {
        HRESULT hr = pConsumer->OnHostkeyUnknown(
            host.in(), hostkey_hash.in(), hostkey_algorithm.in());
        if (hr == S_OK)
        {
            add(hosts, host, key); // add to known_hosts
            hosts.save(known_hosts_path);
        }
        else if (hr == S_FALSE)
            return; // continue but don't add
        else
            AtlThrow(E_ABORT); // screech to a halt
    }
}

/**
 * Tries to authenticate the user with the remote server.
 *
 * The remote server is queried for which authentication methods it supports
 * and these are tried one at time until one succeeds in the order:
 * public-key, keyboard-interactive, plain password.
 *
 * @throws com_error if authentication fails:
 * - E_ABORT if user cancelled the operation (via ISftpConsumer)
 * - E_FAIL otherwise
 */
void CSessionFactory::_AuthenticateUser(
    PCWSTR pwszUser, CSession& session, ISftpConsumer *pConsumer) throw(...)
{
    ATLASSUME(pwszUser[0] != '\0');
    string utf8_username = WideStringToUtf8String(pwszUser);

    // Check which authentication methods are available
    char *szAuthList = libssh2_userauth_list(
        session, utf8_username.data(), utf8_username.size());
    if (!szAuthList || *szAuthList == '\0')
    {
        // If empty, server refused to let user connect
        BOOST_THROW_EXCEPTION(
            std::exception("No supported authentication methods found"));
    }

    TRACE("Authentication methods: %s", szAuthList);

    // Try each supported authentication method in turn until one succeeds
    HRESULT hr = E_FAIL;
    if (::strstr(szAuthList, "publickey"))
    {
        TRACE("Trying public-key authentication");
        hr = _PublicKeyAuthentication(utf8_username, session, pConsumer);
    }
    if (FAILED(hr) && ::strstr(szAuthList, "keyboard-interactive"))
    {
        TRACE("Trying keyboard-interactive authentication");
        hr = _KeyboardInteractiveAuthentication(
            utf8_username, session, pConsumer);
        if (hr == E_ABORT)
            AtlThrow(hr); // User cancelled
    }
    if (FAILED(hr) && ::strstr(szAuthList, "password"))
    {
        TRACE("Trying simple password authentication");
        hr = _PasswordAuthentication(utf8_username, session, pConsumer);
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
 * @throws com_error if authentication fails:
 * - E_ABORT if user cancelled the operation (via ISftpConsumer)
 * - E_FAIL otherwise
 */
HRESULT CSessionFactory::_PasswordAuthentication(
    const string& utf8_username, CSession& session, ISftpConsumer *pConsumer)
{
    HRESULT hr;
    bstr_t prompt = L"Please enter your password:";
    bstr_t password;

    // Loop until successfully authenticated or request returns cancel
    int ret = -1; // default to failure
    do {
        hr = pConsumer->OnPasswordRequest(prompt.in(), password.out());
        if FAILED(hr)
            return hr;
        
        string utf8_password = WideStringToUtf8String(password.w_str());
        ret = libssh2_userauth_password_ex(
            session, utf8_username.data(), utf8_username.size(),
            utf8_password.data(), utf8_password.size(),
            NULL);
        // TODO: handle password change callback here
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
 * @throws com_error if authentication fails:
 * - E_ABORT if user cancelled the operation (via ISftpConsumer)
 * - E_FAIL otherwise
 */
HRESULT CSessionFactory::_KeyboardInteractiveAuthentication(
    const string& utf8_username, CSession& session, ISftpConsumer *pConsumer)
{
    // Create instance of keyboard-interactive authentication handler
    CKeyboardInteractive handler(pConsumer);
    
    // Pass pointer to handler in session abstract and begin authentication.
    // The static callback method (last parameter) will extract the 'this'
    // pointer from the session and use it to invoke the handler instance.
    // If the user cancels the operation, our callback should throw an
    // E_ABORT exception which we catch here.
    *libssh2_session_abstract(session) = &handler;
    int rc = libssh2_userauth_keyboard_interactive_ex(
        session, utf8_username.data(), utf8_username.size(),
        &(CKeyboardInteractive::OnKeyboardInteractive));
    
    // Check for two possible types of failure
    if (FAILED(handler.GetErrorState()))
        return handler.GetErrorState();

    ATLASSERT(rc || libssh2_userauth_authenticated(session)); // Double-check
    return (rc == 0) ? S_OK : E_FAIL;
}

namespace {

    HRESULT pubkey_auth_the_nasty_old_way(
        const string& utf8_username, CSession& session,
        ISftpConsumer *pConsumer)
    {
        ATLENSURE_RETURN_HR(pConsumer, E_POINTER);

        try
        {
            bstr_t private_key_path;
            HRESULT hr =
                pConsumer->OnPrivateKeyFileRequest(private_key_path.out());
            if (FAILED(hr))
                return hr;

            bstr_t public_key_path;
            hr = pConsumer->OnPublicKeyFileRequest(public_key_path.out());
            if (FAILED(hr))
                return hr;

            string privateKey = WideStringToUtf8String(private_key_path.w_str());
            string publicKey = WideStringToUtf8String(public_key_path.w_str());

            // TODO: unlock public key using passphrase
            int rc = libssh2_userauth_publickey_fromfile_ex(
                session, utf8_username.data(), utf8_username.size(),
                publicKey.c_str(), privateKey.c_str(), "");
            if (rc)
                return E_ABORT;

            ATLASSERT(libssh2_userauth_authenticated(session)); // Double-check
        }
        WINAPI_COM_CATCH();

        return S_OK;
    }
}

HRESULT CSessionFactory::_PublicKeyAuthentication(
    const string& utf8_username, CSession& yukky_session,
    ISftpConsumer *pConsumer)
{
    // This old way is only kept around to support the tests.  Its almost
    // useless for anything else as we don't pass the 'consumer' enough
    // information to identify which key to use.
    HRESULT hr = pubkey_auth_the_nasty_old_way(
        utf8_username, yukky_session, pConsumer);
    if (SUCCEEDED(hr))
        return hr;

    // OK, now lets do it the nice new way using agents.

    ssh::session session(yukky_session.get());

    try
    {
        BOOST_FOREACH(ssh::agent::identity key, session.agent_identities())
        {
            try
            {
                key.authenticate(utf8_username);
                return S_OK;
            }
            catch (const exception&)
            { /* Ignore and try the next */ }
        }
    }
    catch(const exception&)
    { /* No agent running probably.  Either way, give up. */ }

    // None of the agent identities worked.  Sob. Back to passwords then.
    return E_ABORT;
}

#pragma warning (pop)