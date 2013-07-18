/**
    @file

    SSH session authentication.

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

#include "authenticated_session.hpp"

#include "swish/provider/KeyboardInteractive.hpp"
#include "swish/utils.hpp" // WideStringToUtf8String

#include <ssh/knownhost.hpp> // openssh_knownhost_collection
#include <ssh/session.hpp>
#include <ssh/sftp.hpp> // sftp_channel

#include <winapi/com/catch.hpp> // WINAPI_COM_CATCH_AUTO_INTERFACE

#include <comet/bstr.h> // bstr_t
#include <comet/error.h> // com_error

#include <boost/filesystem.hpp> // wpath
#include <boost/filesystem/fstream.hpp> // ofstream
#include <boost/move/move.hpp>

#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <cassert>
#include <exception>
#include <string>

#include <libssh2.h>
#include <libssh2_sftp.h>

using swish::connection::authenticated_session;
using swish::connection::running_session;
using swish::utils::WideStringToUtf8String;
using swish::utils::home_directory;

using ssh::host_key::hexify;
using ssh::host_key::host_key;
using ssh::knownhost::find_result;
using ssh::knownhost::openssh_knownhost_collection;
using ssh::session;
using ssh::sftp::sftp_channel;

using comet::com_error;
using comet::bstr_t;

using boost::filesystem::wpath;
using boost::filesystem::ofstream;
using boost::move;
using boost::mutex;

using std::exception;
using std::string;
using std::wstring;


namespace swish {
namespace connection {

namespace {

const wpath known_hosts_path =
    home_directory<wpath>() / L".ssh" / L"known_hosts";

void verify_host_key(
    const wstring& host, running_session& session, ISftpConsumer *pConsumer)
{
    assert(pConsumer);

    ssh::session sess(session.get_session());

    string utf8_host = WideStringToUtf8String(host);

    host_key key = sess.hostkey();
    bstr_t hostkey_algorithm = key.algorithm_name();
    bstr_t hostkey_hash = hexify(key.md5_hash());

    assert(!hostkey_hash.empty());
    assert(!hostkey_algorithm.empty());
    
    // YUK YUK YUK: Accessing and modifying host key files should not be
    // here.  It should be done by the callback.

    // make sure known_hosts file exists
    create_directories(known_hosts_path.parent_path());
    ofstream(known_hosts_path, std::ios::app);

    openssh_knownhost_collection hosts(
        session.get_session().get(), known_hosts_path);

    find_result result = hosts.find(utf8_host, key);
    if (result.mismatch())
    {
        HRESULT hr = pConsumer->OnHostkeyMismatch(
            bstr_t(host).in(), hostkey_hash.in(), hostkey_algorithm.in());
        if (hr == S_OK)
        {
            update(hosts, utf8_host, key, result); // update known_hosts
            hosts.save(known_hosts_path);
        }
        else if (hr == S_FALSE)
            return; // continue but don't add
        else
            BOOST_THROW_EXCEPTION(
                com_error("User aborted on host key mismatch", E_ABORT));
    }
    else if (result.not_found())
    {
        HRESULT hr = pConsumer->OnHostkeyUnknown(
            bstr_t(host).in(), hostkey_hash.in(), hostkey_algorithm.in());
        if (hr == S_OK)
        {
            add(hosts, utf8_host, key); // add to known_hosts
            hosts.save(known_hosts_path);
        }
        else if (hr == S_FALSE)
            return; // continue but don't add
        else
            BOOST_THROW_EXCEPTION(
                com_error("User aborted on unknown host key", E_ABORT));
    }
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
HRESULT password_authentication(
    const string& utf8_username, running_session& session, ISftpConsumer *pConsumer)
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
            session.get_raw_session(), utf8_username.data(), utf8_username.size(),
            utf8_password.data(), utf8_password.size(),
            NULL);
        // TODO: handle password change callback here
    } while (ret != 0);

    assert(SUCCEEDED(hr));
    assert(ret == 0);
    assert(libssh2_userauth_authenticated(session.get_raw_session())); // Double-check
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
HRESULT keyboard_interactive_authentication(
    const string& utf8_username, running_session& session, ISftpConsumer *pConsumer)
{
    // Create instance of keyboard-interactive authentication handler
    CKeyboardInteractive handler(pConsumer);
    
    // Pass pointer to handler in session abstract and begin authentication.
    // The static callback method (last parameter) will extract the 'this'
    // pointer from the session and use it to invoke the handler instance.
    // If the user cancels the operation, our callback should throw an
    // E_ABORT exception which we catch here.
    *libssh2_session_abstract(session.get_raw_session()) = &handler;
    int rc = libssh2_userauth_keyboard_interactive_ex(
        session.get_raw_session(), utf8_username.data(), utf8_username.size(),
        &(CKeyboardInteractive::OnKeyboardInteractive));
    
    // Check for two possible types of failure
    if (FAILED(handler.GetErrorState()))
        return handler.GetErrorState();

    assert(rc || libssh2_userauth_authenticated(session.get_raw_session())); // Double-check
    return (rc == 0) ? S_OK : E_FAIL;
}


HRESULT pubkey_auth_the_nasty_old_way(
    const string& utf8_username, running_session& session,
    ISftpConsumer *pConsumer)
{
    assert(pConsumer);
    if (!pConsumer)
        return E_POINTER;

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
            session.get_raw_session(), utf8_username.data(), utf8_username.size(),
            publicKey.c_str(), privateKey.c_str(), "");
        if (rc)
            return E_ABORT;

        assert(libssh2_userauth_authenticated(session.get_raw_session())); // Double-check
    }
    WINAPI_COM_CATCH();

    return S_OK;
}

HRESULT public_key_authentication(
    const string& utf8_username, running_session& yukky_session,
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

    ssh::session session(yukky_session.get_session());

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
void authenticate_user(
    const wstring& user, running_session& session, ISftpConsumer *pConsumer) throw(...)
{
    assert(!user.empty());
    assert(user[0] != '\0');
    string utf8_username = WideStringToUtf8String(user);

    // Check which authentication methods are available
    char *szAuthList = libssh2_userauth_list(
        session.get_raw_session(), utf8_username.data(), utf8_username.size());
    if (!szAuthList || *szAuthList == '\0')
    {
        // If empty, server refused to let user connect
        BOOST_THROW_EXCEPTION(
            std::exception("No supported authentication methods found"));
    }

    // Try each supported authentication method in turn until one succeeds
    HRESULT hr = E_FAIL;
    if (::strstr(szAuthList, "publickey"))
    {
        hr = public_key_authentication(utf8_username, session, pConsumer);
    }
    if (FAILED(hr) && ::strstr(szAuthList, "keyboard-interactive"))
    {
        hr = keyboard_interactive_authentication(
            utf8_username, session, pConsumer);
        if (hr == E_ABORT)
            BOOST_THROW_EXCEPTION(
                com_error(
                    "User aborted during keyboard-interactive authentication",
                    E_ABORT));
    }
    if (FAILED(hr) && ::strstr(szAuthList, "password"))
    {
        hr = password_authentication(utf8_username, session, pConsumer);
    }

    if (FAILED(hr))
        BOOST_THROW_EXCEPTION(com_error(hr));
}

running_session create_and_authenticate(
    const wstring& host, unsigned int port, const wstring& user,
    ISftpConsumer* consumer)
{
    running_session session(host, port);

    verify_host_key(host, session, consumer);
    // Legal to fail here, e.g. user refused to accept host key

    authenticate_user(user, session, consumer);
    // Legal to fail here, e.g. wrong password/key

    assert(session.get_session().authenticated());

    return move(session);
}

}

authenticated_session::authenticated_session(
    const wstring& host, unsigned int port, const wstring& user,
    ISftpConsumer* consumer)
    :
m_session(create_and_authenticate(host, port, user, consumer)),
m_sftp_channel(m_session.get_session()) {}

authenticated_session::authenticated_session(
    BOOST_RV_REF(authenticated_session) other)
:
m_session(move(other.m_session)), m_sftp_channel(move(other.m_sftp_channel)) {}

authenticated_session& authenticated_session::operator=(
    BOOST_RV_REF(authenticated_session) other)
{
    swap(authenticated_session(move(other)), *this);
    return *this;
}

void swap(authenticated_session& lhs, authenticated_session& rhs)
{
    boost::swap(lhs.m_session, rhs.m_session);
    std::swap(lhs.m_sftp_channel, rhs.m_sftp_channel);
}

session authenticated_session::get_session() const
{
    return m_session.get_session();
}

LIBSSH2_SESSION* authenticated_session::get_raw_session()
{
    return m_session.get_raw_session();
}

sftp_channel authenticated_session::get_sftp_channel() const
{
    return m_sftp_channel;
}

mutex::scoped_lock authenticated_session::aquire_lock()
{
    return m_session.aquire_lock();
}

bool authenticated_session::is_dead()
{
   return m_session.is_dead();
}

}} // namespace swish::connection