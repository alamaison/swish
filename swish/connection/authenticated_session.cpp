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
#include <boost/optional/optional.hpp>

#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <cassert>
#include <exception>
#include <string>
#include <vector>

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

using comet::bstr_t;
using comet::com_error;
using comet::com_ptr;

using boost::filesystem::path;
using boost::filesystem::wpath;
using boost::filesystem::ofstream;
using boost::move;
using boost::mutex;
using boost::optional;

using std::exception;
using std::pair;
using std::string;
using std::vector;
using std::wstring;


namespace swish {
namespace connection {

namespace {

const wpath known_hosts_path =
    home_directory<wpath>() / L".ssh" / L"known_hosts";

void verify_host_key(
    const wstring& host, running_session& session,
    com_ptr<ISftpConsumer> consumer)
{
    assert(consumer);

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
        HRESULT hr = consumer->OnHostkeyMismatch(
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
        HRESULT hr = consumer->OnHostkeyUnknown(
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
 * @throws `std::exception`
 *     if authentication fails for an unexpected reason, in other words,
 *     a reason other than the user cancelling the authentication.
 *
 * @returns
 *     `true` if authentication was successful,
 *     `false` if the user aborted early.
 * @note that unsuccessful is not a return value as the function keeps
 *       re-prompting until successful or cancelled.
 */
bool password_authentication(
    const string& utf8_username, running_session& session,
    com_ptr<ISftpConsumer> consumer)
{
    // Loop until successfully authenticated or user cancels
    for(;;)
    {
        optional<wstring> password = consumer->prompt_for_password();
        if (!password)
        {
            return false;
        }
        
        string utf8_password = WideStringToUtf8String(*password);
        if (session.get_session().authenticate_by_password(
            utf8_username, utf8_password))
        {
            return true;
        }

        // TODO: handle password change callback here
    }
}

namespace {

    class user_aborted_authentication : 
        public virtual boost::exception, public std::runtime_error
    {
    public:
        user_aborted_authentication() :
          boost::exception(), std::runtime_error("User aborted authentication")
          {}
    };

    /**
     * Delegates challenge-response to a consumer.
     */
    class consumer_responder
    {
    public:

        consumer_responder(com_ptr<ISftpConsumer> consumer)
            : m_consumer(consumer) {}

        template<typename PromptRange>
        vector<string> operator()(
            const string& title, const string& instructions,
            const PromptRange& prompts)
        {
            optional<vector<string>> responses =
                m_consumer->challenge_response(title, instructions, prompts);
            if (responses)
            {
                return *responses;
            }
            else
            {
                BOOST_THROW_EXCEPTION(user_aborted_authentication());
            }
        }

    private:
        com_ptr<ISftpConsumer> m_consumer;
    };
}

/**
 * Authenticates with remote host by challenge-response interaction.
 *
 * This uses the ISftpConsumer callback to challenge the user for various
 * pieces of information (usually just their password).
 *
 * @returns
 *     `true` if authentication successful, `false` if the `consumer` reports
 *     that the user aborted authentication.
 *
 * @throws `ssh_error`
 *     if unexpected SSH-related failure while trying to authenticate or if the
 *     server positively rejects authentication without even calling the
 *     `responder`.
 * @throws `std::exception`
 *     if authentication fails for an unexpected reason, in other words,
 *     a reason other than the user cancelling the authentication.
 *     If authentication fails because the `consumer` threw an exception,
 *     that exception will be the one throw out of this method.
 * @note that unsuccessful authentication is not a return value as the function
 *     keeps re-prompting until successful or cancelled.
 */
bool keyboard_interactive_authentication(
    const string& utf8_username, running_session& session, com_ptr<ISftpConsumer> consumer)
{
    // Loop until successfully authenticated or user cancels.
    // Unlike simple password authentication, the user cancelling an interactive
    // authentication isn't signalled by the return code because interactive
    // authentications can't actually be aborted.  Instead we find out about
    // an abortion when authentication fails and the responder threw an
    // exception.  Therefore we catch our custom "user aborted" exception
    // below and translate that into a boolean result here.
    try
    {
        while (!session.get_session().authenticate_interactively(
            utf8_username, consumer_responder(consumer))) {}
    }
    catch (const user_aborted_authentication& e)
    {
        return false;
    }

    assert(session.get_session().authenticated()); // Double-check
    return true;
}


bool public_key_file_based_authentication(
    const string& utf8_username, running_session& session,
    com_ptr<ISftpConsumer> consumer)
{
    assert(consumer);

    optional<pair<path, path>> key_files = consumer->key_files();
    if (key_files)
    {
        // TODO: unlock public key using passphrase
        session.get_session().authenticate_by_key_files(
            utf8_username, key_files->second, key_files->first, "");

        assert(session.get_session().authenticated());

        return true;
    }
    else
    {
        return false;
    }

    return S_OK;
}

bool public_key_agent_authentication(
    const string& utf8_username, running_session& session,
    com_ptr<ISftpConsumer> consumer)
{
    try
    {
        BOOST_FOREACH(
            ssh::agent::identity key, session.get_session().agent_identities())
        {
            try
            {
                key.authenticate(utf8_username);
                return true;
            }
            catch (const exception&)
            { /* Ignore and try the next */ }
        }
    }
    catch(const exception&)
    { /* No agent running probably.  Either way, give up. */ }

    // None of the agent identities worked.  Sob. Back to passwords then.
    return false;
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
    const wstring& user, running_session& session, com_ptr<ISftpConsumer> consumer) throw(...)
{
    assert(!user.empty());
    assert(user[0] != '\0');
    string utf8_username = WideStringToUtf8String(user);

    vector<string> methods =
        session.get_session().authentication_methods(utf8_username);

    if (methods.empty())
    {
        BOOST_THROW_EXCEPTION(
            std::exception("No supported authentication methods found"));
    }

    // Try each supported authentication method in turn until one succeeds
    if (find(methods.begin(), methods.end(), "publickey") != methods.end())
    {
        // This old way is only kept around to support the tests.  Its almost
        // useless for anything else as we don't pass the 'consumer' enough
        // information to identify which key to use.
        if (public_key_file_based_authentication(
            utf8_username, session, consumer))
        {
            return;
        }

        // OK, now lets try it the nice new way using agents.
        if (public_key_agent_authentication(utf8_username, session, consumer))
        {
            return;
        }
    }

    if (find(methods.begin(), methods.end(), "keyboard-interactive")
        != methods.end())
    {
        if (keyboard_interactive_authentication(
            utf8_username, session, consumer))
        {
            return;
        }
        else
        {
            BOOST_THROW_EXCEPTION(
                com_error(
                    "User aborted challenge-response authentication", E_ABORT));
        }
    }

    if (find(methods.begin(), methods.end(), "password") != methods.end())
    {
        if (password_authentication(utf8_username, session, consumer))
        {
            return;
        }
        else
        {
            BOOST_THROW_EXCEPTION(
                com_error("User aborted password authentication", E_ABORT));
        }
    }

    BOOST_THROW_EXCEPTION(
        com_error("No authentication method succeeded", E_FAIL));
}

running_session create_and_authenticate(
    const wstring& host, unsigned int port, const wstring& user,
    com_ptr<ISftpConsumer> consumer)
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
    com_ptr<ISftpConsumer> consumer)
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