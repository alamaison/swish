/**
    @file

    SSH session object.

    @if license

    Copyright (C) 2010, 2012, 2013  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#ifndef SSH_SESSION_HPP
#define SSH_SESSION_HPP

#include <ssh/agent.hpp>
#include <ssh/detail/libssh2/session.hpp> // ssh::detail::libssh2::session
#include <ssh/detail/libssh2/userauth.hpp> // ssh::detail::libssh2::userauth
#include <ssh/host_key.hpp>
#include <ssh/filesystem.hpp> // sftp_filesystem

#include <boost/algorithm/string/classification.hpp> // is_any_of
#include <boost/algorithm/string/split.hpp>
#include <boost/exception_ptr.hpp>
#include <boost/filesystem/path.hpp> // path, used for key paths
#include <boost/make_shared.hpp>
#include <boost/optional/optional.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/algorithm_ext/for_each.hpp>
#include <boost/range/algorithm_ext/push_back.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/system/error_code.hpp> // error_code, errc
#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <string>
#include <utility> // pair, make_pair
#include <vector>

#include <libssh2.h>

namespace ssh {

    namespace filesystem {

        class sftp_filesystem;

    }

namespace detail {

    inline std::pair<std::string, bool> convert_prompt(
        const LIBSSH2_USERAUTH_KBDINT_PROMPT& prompt)
    {
        return std::make_pair(
            std::string(prompt.text, prompt.length), prompt.echo);
    }

    inline void convert_response(
        LIBSSH2_USERAUTH_KBDINT_RESPONSE& raw_response,
        const std::string& response)
    {
        // XXX: Should use session MALLOC here
        raw_response.text =
            static_cast<char*>(
                malloc(response.length() * sizeof(std::string::value_type)));
        // XXX: what happens if we encounter an exception after this point?
        raw_response.length = response.length();

        response.copy(raw_response.text, raw_response.length);
    }


    /**
     * Glue between libssh2's ideas of a responder and this c++ wrapper's
     * responder.
     *
     * It's not safe to throw exceptions through libssh2 C code, so we have to
     * catch them in the static callback (dethunker), and somehow communicate
     * them back to the C++ code which can safely rethrow them.
     *
     * The only available channel of communication is the challenge-responder
     * in the abstract but the user provides that so we don't want them to have
     * to provide anything special.  This class adds the 'something special'
     * by wrapping the challenge-responder and stashing anything needed to
     * interpret the result.
     */
    template<typename ChallengeResponder>
    class challenge_response_translator
    {
    public:
        challenge_response_translator(const ChallengeResponder& resp)
            : m_responder(resp), m_called(false) {}

        /**
         * Perform the challenge-response authentication translating between the
         * interfaces as we go.
         */
        bool do_challenge_response(
            LIBSSH2_SESSION* session,
            const std::string username)
        {
            boost::system::error_code ec;
            std::string message;

            // IMPORTANT: No need to lock here.  The session is locked by the
            // caller to encompass setting the abstract so that the abstract
            // isn't inadvertently overwritten.

            libssh2::userauth::keyboard_interactive_ex(
                session, username.data(), username.size(),
                &dethunker<ChallengeResponder>, ec, message);

            return translate_status(ec, message);
        }

        void operator()(
            const char* name, int name_len, const char* instruction,
            int instruction_len, int num_prompts, 
            const LIBSSH2_USERAUTH_KBDINT_PROMPT* raw_prompts,
            LIBSSH2_USERAUTH_KBDINT_RESPONSE* raw_responses) throw()
        {
            m_called = true;

            try
            {
                call_inner_responder(
                    name, name_len, instruction, instruction_len, num_prompts,
                    raw_prompts, raw_responses);
            }
            catch (...)
            {
                m_exception = boost::current_exception();
            }
        }

    private:

        /**
         * Merge any errors reported by libssh2 with any exception throw in
         * the responder.
         *
         * Merging the two is non-trivial.  There are at least 9 scenarios
         * that can arise:
         * (1) Authentication was successful:
         *    a) and the responder executed completely
         *    b) despite the responder throwing an exception. Possible because
         *       the exception just causes outstanding responses to be sent to
         *       the server blank, and the server may be satisfied with these
         *       blank responses.  There is no way to abort authentication via
         *       the callback.
         *       TODO: maybe modify libssh2 to allow aborting from the callback.
         *    c) without needing to call the responder.  Scary.
         * (2) Authentication positively rejected:
         *    a) even though the responder executed completely. E.g. the user
         *       gave the wrong response.
         *    b) because the responder threw an exception and the server
         *       rejected the (possibly partially-complete) responses.
         *    c) without needing to call the responder, e.g. kb-interactive not
         *       set up properly on the server (yes, this does actually happen,
         *       we tested it - e.g the cygwin server).
         * (3) Some other failure occurred:
         *    a) even though the responder executed completely.
         *    b) the responder threw an exception but the failure is unrelated
         *       (because it's not possible to abort, it must be unrelated).
         *    c) before needing to call the responder.
         */
        bool translate_status(
            const boost::system::error_code& ec, const std::string& message)
        {
            if (!ec)
            {
                // Situation (1) above.  Merge all three situations and just
                // report the successful authentication.  Any exception thrown in the
                // responder is ignored.
                //
                // XXX: There is a tricky use-case here. If a user cancels a
                //      challenge-response prompt and that causes an exception,
                //      the caller has no way to tell that the user cancelled if the
                //      authentication nevertheless succeeded.  Arguably, that is
                //      the correct behaviour as it is more important to know the
                //      authentication state of the session than the user's
                //      response.  An even better solution would be if we could
                //      abort authentication from the callback but that may not be
                //      possible.  RFC 4256 Section 3.4 says that sending the wrong
                //      number of responses back must always result in failure, so
                //      responding with zero replies might work ... unless the server
                //      sent zero prompts.
                return true;
            }
            else if (ec == boost::system::errc::permission_denied)
            {
                // Situation (2) above. 
                // a) is a non-error failure.  In other words, the kind of
                // failure that wouldn't be reported to the user with an error
                // dialog. The most likely response to this result is to attempt
                // authentication again.  It wouldn't be appropriate to report 
                // these failures as exceptions so, instead, we return false.
                //
                // b) and c) are both errors so we need to throw exceptions.
                // We can only tell c) and a) apart by whether the responder
                // was called so we have to wrap the given responder to record
                // that information. For b) the most relevant exception is the
                // one thrown by the wrapped responder which is also by this
                // class.

                if (!m_called)
                {
                    // c)
                    assert(!m_exception);

                    BOOST_THROW_EXCEPTION(
                        boost::system::system_error(ec, message));
                }
                else if (m_exception)
                {
                    // b)
                    boost::rethrow_exception(*m_exception);
                }
                else
                {
                    // a)
                    assert(m_called);
                    return false;
                }
            }
            else
            {
                // Situation (3) above
                BOOST_THROW_EXCEPTION(boost::system::system_error(ec, message));
            }

            // If the user cancels the operation, our callback should throw an
            // E_ABORT exception which we catch here.
        }

        /**
         * Unpacks the stashed responder.
         */
        template<typename ChallengeResponder>
        static void dethunker(
            const char* name, int name_len, const char* instruction,
            int instruction_len, int num_prompts, 
            const LIBSSH2_USERAUTH_KBDINT_PROMPT* raw_prompts,
            LIBSSH2_USERAUTH_KBDINT_RESPONSE* raw_responses, void **abstract)
        throw()
        {
            challenge_response_translator<ChallengeResponder>& responder =
                *static_cast<challenge_response_translator<ChallengeResponder>*>(*abstract);

            responder(
                name, name_len, instruction, instruction_len, num_prompts,
                raw_prompts, raw_responses);
        }

        /**
         * Do the two-way interface translation.
         */
        void call_inner_responder(
            const char* name, int name_len, const char* instruction,
            int instruction_len, int num_prompts, 
            const LIBSSH2_USERAUTH_KBDINT_PROMPT* raw_prompts,
            LIBSSH2_USERAUTH_KBDINT_RESPONSE* raw_responses)
        {
            std::vector<std::pair<std::string, bool>> prompts;
            boost::push_back(prompts,
                boost::iterator_range<const LIBSSH2_USERAUTH_KBDINT_PROMPT*>(
                raw_prompts, raw_prompts + num_prompts)
                |
                boost::adaptors::transformed(convert_prompt));

            // Either the name or the instruction may be a NULL pointer as they
            // are optional fields
            std::string name_string = 
                name ?
                    std::string(name, name_len) :
                    std::string();
            std::string instruction_string = 
                instruction ?
                    std::string(instruction, instruction_len) :
                    std::string();

            boost::range::for_each(
                boost::iterator_range<LIBSSH2_USERAUTH_KBDINT_RESPONSE*>(
                raw_responses, raw_responses + num_prompts),
                m_responder(name_string, instruction_string, prompts),
                convert_response);
        }

        ChallengeResponder m_responder;
        bool m_called;
        boost::optional<boost::exception_ptr> m_exception;
    };
}

/**
 * An SSH session connected to a host.
 */
// The underlying session is both allocated and connected to the host by
// allocate_and_connect_session.  This function also makes sure it will be
// first disconnected before it is freed.  The implementation of `class session`
// need to worry about these details, they are already taken care of.
class session
{
public:
    
    /**
     * Start a new SSH session with a host.
     *
     * The host is listening on the other end of the given socket.
     *
     * The constructor will throw an exception if it cannot connect to the host
     * or negotiate an SSH session.  Therefore any instance of this class
     * begins life successfully connected to the host.  Of course, the
     * connection may break subsequently and the server is free to terminate
     * the session at any time.
     *
     * @param socket
     *     The socket through which to communicate with the listening server.
     * @param disconnection_message
     *     An optional message sent to the server when the session is
     *     destroyed.
     */
    session(
        int socket,
        const std::string& disconnection_message=
            "libssh2 C++ bindings session destructor") :
    m_session(
        boost::make_shared<detail::session_state>(
            socket, disconnection_message))
    {}

    /**
     * Hostkey sent by the server to identify itself.
     */
    ssh::host_key hostkey() const
    {
        return ssh::host_key(*m_session);
    }

    /**
     * Names of the methods the server claims are available for
     * authentication.
     *
     * The server is allowed to lie.
     *
     * An empty list does not necessarily mean no methods are available. It
     * might mean that authentication has already succeeded or that no
     * authentication was needed.  Calling this method has the side effect
     * of authenticating the session in the latter case.
     */
    std::vector<std::string> authentication_methods(
        const std::string& username)
    {
        boost::system::error_code ec;
        std::string message;

        // Locking until we copy out the method string owned by the session.
        // We don't want another thread inadvertently causing it to be
        // overwritten While we're reading it.
        detail::session_state::scoped_lock lock = m_session->aquire_lock();

        const char* method_list = detail::libssh2::userauth::list(
            m_session->session_ptr(), username.data(), username.size(),
            ec, message);

        if (!method_list)
        {
            // Because the userauth list is fetched by trying to authenticate
            // with method "none", a NULL return might mean that no
            // authentication was needed. The error code disambiguates this
            // from a true error.

            if (!ec)
            {
                assert(authenticated());
                return std::vector<std::string>();
            }
            else
            {
                BOOST_THROW_EXCEPTION(boost::system::system_error(ec, message));
            }
        }
        else
        {
            std::vector<std::string> methods;
            boost::split(methods, method_list, boost::is_any_of(","));

            return methods;
        }
    }


    bool authenticated() const
    {
        detail::session_state::scoped_lock lock = m_session->aquire_lock();

        return ::libssh2_userauth_authenticated(m_session->session_ptr()) != 0;
    }

    /**
     * Simple password authentication.
     * 
     * @param username
     *     UTF-8 string identifying the user to authenticate as.
     * @param password
     *     Password as a UTF-8 string.
     *
     * @returns
     *     `true` if authentication successful, `false` if not.
     *
     * @throws `boost::system::system_error`
     *     if unexpected failure while trying to authenticate.
     *
     * @todo  Handle password change callback.
     */
    bool authenticate_by_password(
        const std::string& username, const std::string& password)
    {
        boost::system::error_code ec;
        std::string message;

        {
            detail::session_state::scoped_lock lock = m_session->aquire_lock();

            detail::libssh2::userauth::password(
                m_session->session_ptr(), username.data(), username.size(),
                password.data(), password.size(), NULL, ec, message);
        }

        if (!ec)
        {
            return true;
        }
        else if (ec == boost::system::errc::permission_denied)
        {
            // The incorrect password failure is not reported as an exception
            // because it is not exceptional.
            return false;
        }
        else
        {
            BOOST_THROW_EXCEPTION(boost::system::system_error(ec, message));
        }
    }

    /**
     * Challenge-response authentication.
     *
     * This is also known as keyboard-interactive authentication.  The server
     * challenges the user by requesting one or more pieces of information. 
     * Once the user has responded, the server may request more information 
     * any number of time until it is either satisfied and authenticates the
     * user or refuses to do so.
     *
     * @param username
     *     UTF-8 string identifying the user to authenticate as.
     * @param responder
     *     Callback to receive the challenges from the server and provide the
     *     corresponding responses.
     *     The callback must be a model of the `ChallengeResponder` concept.
     *     That means it must be callable with a three arguments:
     *      - a string giving the challenge title (may be empty),
     *      - a string giving the challenge instructions (may be empty), and
     *      - a range of zero or more prompts, each a pair whose first member
     *        is the prompt text and whose second member is a boolean indicating
     *        whether the response should be obscured like a password or made
     *        visible.
     *     The call must return a range of responses as strings, one for every
     *     prompt in the same order as the prompts.
     *
     * @returns
     *     `true` if authentication successful, `false` if the server positively
     *      rejected the responses produced by the `responder` callback.
     *
     * @throws `boost::system::system_error`
     *     if unexpected failure while trying to authenticate or if the server
     *     positively rejects authentication without even calling the
     *     `responder`.
     * @throws user-defined-exception
     *     if authentication fails because the `responder` threw an exception,
     *     the exception is throw out of this method.
     *
     * @warning  The responder __must not__ call any code that uses the same
     *           SSH session currently being authenticated.  Doing so results
     *           in undefined behaviour (likely deadlock).
     */
    //
    // We tried to use Boost.Concept here to verify the ChallengeResponder but
    // gave up.  We struggled to do anything useful without BOOST_TYPEOF (which
    // crashed MSVC) and it's not clear what the benefit of the concept would
    // have been in any case.  It didn't make the requirements of the
    // responder any more clear than reading the implementation of this
    // function and I doubt the error messages were any better.
    // Nevertheless, this might be worth having another go at in the future.
    //
    template<typename ChallengeResponder>
    bool authenticate_interactively(
        const std::string& username, ChallengeResponder responder)
    {
        // The libssh2 C API, of course, takes the callback as a plain-old
        // static function.  The caller, however, may have passed us a callable
        // object and we need to be able to call that instead.
        //
        // As is typical of good C APIs, libssh2 gives us a way to sneak a
        // pointer to the callback object (or whatever it might be) through
        // the static callback function via an 'abstract' parameter.
        //
        // We set the abstract via the session.  The static callback function
        // receives that and converts it back to the callable object, which
        // can then be called in the C++ way.
        //
        // As an extra twist in the story, we don't pass the responder directly
        // in the abstract.  Instead we pass a version wrapped so that it can
        // store exceptions encountered, which we rethrow afterwards.

        detail::challenge_response_translator<ChallengeResponder>
            wrapped_responder(responder);

        // IMPORTANT: Locked from this point onwards until returning to the
        // caller so that abstract is not overwritten by another thread
        // before we pull the responder out of it later
        detail::session_state::scoped_lock lock = m_session->aquire_lock();

        *::libssh2_session_abstract(m_session->session_ptr()) =
            &wrapped_responder;

        return wrapped_responder.do_challenge_response(
            m_session->session_ptr(), username);
    }

    /**
     * Public-key authentication.
     *
     * This method requires a path to both the public and private keys because
     * libssh2 does.  It should be possible to derive one from the other so
     * when libssh2 supports this the method will take one fewer argument.
     */
    void authenticate_by_key_files(
        const std::string& username, const boost::filesystem::path& public_key,
        const boost::filesystem::path& private_key,
        const std::string& passphrase)
    {
        detail::session_state::scoped_lock lock = m_session->aquire_lock();

        detail::libssh2::userauth::public_key_from_file(
            m_session->session_ptr(), username.data(), username.size(),
            public_key.external_file_string().c_str(),
            private_key.external_file_string().c_str(), passphrase.c_str());
    }

    /**
     * Connect to any agent running on the system and return object to
     * authenticate using its identities.
     */
    ::ssh::agent_identities agent_identities()
    {
        return ::ssh::agent_identities(m_session);
    }

    /**
     * Create a new connection to the remote filesystem over this SSH session.
     */
    filesystem::sftp_filesystem connect_to_filesystem()
    {
        return filesystem::sftp_filesystem::factory_attorney()(m_session);
    }

private:

    boost::shared_ptr<detail::session_state> m_session;
};

} // namespace ssh

#endif
