/**
    @file

    Mock implementation of an ISftpConsumer.

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

    @endif
*/

#ifndef TEST_COMMON_BOOST_MOCK_CONSUMER_HPP
#define TEST_COMMON_BOOST_MOCK_CONSUMER_HPP
#pragma once

#include "swish/provider/sftp_provider.hpp"

#include <comet/bstr.h> // bstr_t
#include <comet/error.h> // com_error
#include <comet/safearray.h>
#include <comet/server.h> // simple_object

#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION
#include <boost/foreach.hpp> // BOOST_FOREACH
#include <boost/test/test_tools.hpp> // BOOST_ERROR

#include <cassert> // assert
#include <string>

namespace test {

class MockConsumer : public comet::simple_object<ISftpConsumer>
{
public:

    typedef ISftpConsumer interface_is;

    /**
     * Possible behaviours of file overwrite confirmation handlers
     * OnConfirmOverwrite.
     */
    enum ConfirmOverwriteBehaviour {
        AllowOverwrite,         ///< Return S_OK
        PreventOverwrite,       ///< Return E_ABORT
    };

    /**
     * Possible behaviours of mock password request handler OnPasswordRequest.
     */
    enum PasswordBehaviour {
        EmptyPassword,    ///< Return an empty string
        CustomPassword,   ///< Return the string set with SetPassword
        WrongPassword,    ///< Return a very unlikely sequence of characters
        FailPassword,     ///< Throw exception if password requested
        AbortPassword,    ///< Return E_ABORT (simulate user cancelled)
    };

    /**
     * Possible behaviours of mock keyboard-interactive request handler 
     * OnKeyboardInteractiveRequest.
     */
    enum KeyboardInteractiveBehaviour {
        EmptyResponse,    ///< Return an empty BSTR (not NULL, "")
        CustomResponse,   ///< Return the string set with SetPassword
        WrongResponse,    ///< Return a very unlikely sequence of characters
        NullResponse,     ///< Return NULL and S_OK (catastrophic failure)
        FailResponse,     ///< Return E_FAIL
        AbortResponse,    ///< Return E_ABORT (simulate user cancelled)
        ThrowResponse     ///< Throw exception if kb-interaction requested
    };

    /**
     * Possible behaviours of mock public-key file requests.
     */
    enum PublicKeyBehaviour {
        EmptyKeys,    ///< Return an empty BSTR (not NULL, "")
        CustomKeys,   ///< Return the strings set with SetKeys
        WrongKeys,    ///< Return the wrong, but existing, key files
        InvalidKeys,  ///< Return key files that don't exist
        NullKeys,     ///< Return NULL and S_OK (catastrophic failure)
        FailKeys,     ///< Return E_FAIL
        AbortKeys,    ///< Return E_ABORT (simulate user cancelled)
        ThrowKeys     ///< Throw exception if keys requested
    };

    MockConsumer()
        :
          m_password_behaviour(FailPassword),
          m_password_attempt_count(0),
          m_password_attempt_count_max(1),
          m_keyboard_interative_behaviour(ThrowResponse),
          m_pubkey_behaviour(ThrowKeys),
          m_ki_attempt_count(0),
          m_ki_attempt_count_max(1),
          m_confirm_overwrite_behaviour(PreventOverwrite),
          m_confirmed_overwrite(false) {}

    void set_password(const std::wstring& password)
    {
        m_password = password;
    }

    void set_password_behaviour(PasswordBehaviour behaviour)
    {
        m_password_behaviour = behaviour;
    }

    void set_password_max_attempts(int max)
    {
        m_password_attempt_count_max = max;
    }

    void set_keyboard_interactive_behaviour(
        KeyboardInteractiveBehaviour behaviour)
    {
        m_keyboard_interative_behaviour = behaviour;
    }

    void set_keyboard_interactive_max_attempts(int max)
    {
        m_ki_attempt_count_max = max;
    }

    void set_key_files(
        const std::string& private_key, const std::string& public_key)
    {
        m_private_key_file = private_key;
        m_public_key_file = public_key;
    }

    void set_pubkey_behaviour(PublicKeyBehaviour behaviour)
    {
        m_pubkey_behaviour = behaviour;
    }

    void set_confirm_overwrite_behaviour(ConfirmOverwriteBehaviour behaviour)
    {
        m_confirm_overwrite_behaviour = behaviour;
    }

    bool was_asked_to_confirm_overwrite() const
    {
        return m_confirmed_overwrite;
    }

    // ISftpConsumer methods
    virtual boost::optional<std::wstring> prompt_for_password()
    {
        ++m_password_attempt_count;
        
        // Perform chosen test behaviour
        // The three password cases which should never succeed will try to send
        // their 'reply' up to m_nMaxPassword time to simulate a user repeatedly
        // trying the wrong password and then giving up. 
        if (m_password_attempt_count > m_password_attempt_count_max)
            BOOST_THROW_EXCEPTION(
                comet::com_error("Too many attempts", E_FAIL));

        switch (m_password_behaviour)
        {
        case CustomPassword:
            return m_password;
        case WrongPassword:
            return L"WrongPasswordXyayshdkhjhdk";
        case EmptyPassword:
            // leave password blank
            return std::wstring();
        case FailPassword:
            BOOST_THROW_EXCEPTION(
                comet::com_error("Mock fail behaviour", E_FAIL));
        case AbortPassword:
            return boost::optional<std::wstring>();
        default:
            BOOST_FAIL(
                "Unreachable: Unrecognised OnPasswordRequest() behaviour");
            return boost::optional<std::wstring>();
        }
    }

    HRESULT OnKeyboardInteractiveRequest(
        BSTR /*bstrName*/, BSTR /*bstrInstruction*/,
        SAFEARRAY * prompts, SAFEARRAY * show_responses,
        SAFEARRAY ** responses_out)
    {
        ++m_ki_attempt_count;

        comet::safearray_t<comet::bstr_t> prompts_array
            = comet::safearray_t<comet::bstr_t>::create_const_reference(
                prompts);
        comet::safearray_t<comet::variant_bool_t> show_responses_array
            = comet::safearray_t<comet::bstr_t>::create_const_reference(
                show_responses);

        BOOST_FOREACH(comet::bstr_t prompt, prompts_array)
        {
            BOOST_CHECK_GT(prompt.size(), 0U);
        }

        BOOST_CHECK_EQUAL(
            prompts_array.lower_bound(), show_responses_array.lower_bound());
        BOOST_CHECK_EQUAL(prompts_array.size(), show_responses_array.size());

        // Perform chosen test behaviour
        // The three response cases which should never succeed will try to send
        // their 'reply' up to m_nMaxKbdAttempts time to simulate a user repeatedly
        // trying the wrong password and then giving up.
        if (m_ki_attempt_count > m_ki_attempt_count_max)
            return E_FAIL;

        comet::bstr_t response;
        switch (m_keyboard_interative_behaviour)
        {
        case CustomResponse:
            response = m_password;
            break;
        case WrongResponse:
            response = L"WrongPasswordXyayshdkhjhdk";
            break;
        case EmptyResponse:
            // leave response empty
            break;
        case NullResponse:
            *responses_out = NULL;
            return S_OK;
        case FailResponse:
            return E_FAIL;
        case AbortResponse:
            return E_ABORT;
        case ThrowResponse:
            BOOST_FAIL("Unexpected call to " __FUNCTION__);
            return E_FAIL;
        default:
            BOOST_FAIL("Unreachable: Unrecognised "
                "OnKeyboardInteractiveRequest() behaviour");
            return E_UNEXPECTED;
        }

        // Create responses.  Return password BSTR as first response.  Any other 
        // prompts are responded to with an empty string.
        comet::safearray_t<comet::bstr_t> responses;
        responses.push_back(response);
        while (responses.size() < prompts_array.size())
        {
            responses.push_back(comet::bstr_t());
        }

        *responses_out = responses.detach();
        return S_OK;
    }

    HRESULT OnPrivateKeyFileRequest(BSTR* pbstrPrivateKeyFile)
    {
        comet::bstr_t key_file;
        switch (m_pubkey_behaviour)
        {
        case CustomKeys:
            key_file = m_private_key_file;
            break;
        case WrongKeys:
            key_file = m_public_key_file;
            break;
        case InvalidKeys:
            key_file = "HumptyDumpty";
            break;
        case EmptyKeys:
            // leave key_file empty
            break;
        case NullKeys:
            *pbstrPrivateKeyFile = NULL;
            return S_OK;
        case FailKeys:
            return E_FAIL;
        case AbortKeys:
            return E_ABORT;
        case ThrowKeys:
            BOOST_FAIL("Unexpected call to " __FUNCTION__);
            return E_FAIL;
        default:
            BOOST_FAIL("Unreachable: Unrecognised "  __FUNCTION__ " behaviour");
            return E_UNEXPECTED;
        }

        *pbstrPrivateKeyFile = key_file.detach();
        return S_OK;
    }

    HRESULT OnPublicKeyFileRequest(BSTR* pbstrPublicKeyFile)
    {
        comet::bstr_t key_file;
        switch (m_pubkey_behaviour)
        {
        case CustomKeys:
            key_file = m_public_key_file;
            break;
        case WrongKeys:
            key_file = m_private_key_file;
            break;
        case InvalidKeys:
            key_file = "HumptyDumpty";
                break;
        case EmptyKeys:
            // leave key_file empty
            break;
        case NullKeys:
            *pbstrPublicKeyFile = NULL;
            return S_OK;
        case FailKeys:
            return E_FAIL;
        case AbortKeys:
            return E_ABORT;
        case ThrowKeys:
            BOOST_FAIL("Unexpected call to " __FUNCTION__);
            return E_FAIL;
        default:
            BOOST_FAIL("Unreachable: Unrecognised "  __FUNCTION__ " behaviour");
            return E_UNEXPECTED;
        }

        *pbstrPublicKeyFile = key_file.detach();
        return S_OK;
    }

    HRESULT OnConfirmOverwrite(
        BSTR /*bstrOldFile*/, BSTR /*bstrNewFile*/)
    {
        m_confirmed_overwrite = true;

        switch (m_confirm_overwrite_behaviour)
        {
        case AllowOverwrite:
            return S_OK;
        case PreventOverwrite:
            return E_ABORT;
        default:
            assert(!"Unreachable: Unrecognised "
                "OnConfirmOverwrite() behaviour");
            return E_UNEXPECTED;
        }
    }

    HRESULT OnHostkeyMismatch(
        BSTR /*bstrHostName*/, BSTR /*bstrHostKey*/, BSTR /*bstrHostKeyType*/)
    {
        return S_FALSE;
    }

    HRESULT OnHostkeyUnknown(
        BSTR /*bstrHostName*/, BSTR /*bstrHostKey*/, BSTR /*bstrHostKeyType*/)
    {
        return S_FALSE;
    }

private:

    PasswordBehaviour m_password_behaviour;
    int m_password_attempt_count;
    int m_password_attempt_count_max;
    std::wstring m_password;

    KeyboardInteractiveBehaviour m_keyboard_interative_behaviour;
    int m_ki_attempt_count;
    int m_ki_attempt_count_max;

    PublicKeyBehaviour m_pubkey_behaviour;
    std::string m_public_key_file;
    std::string m_private_key_file;

    ConfirmOverwriteBehaviour m_confirm_overwrite_behaviour;
    bool m_confirmed_overwrite;
};

} // namespace test

#endif
