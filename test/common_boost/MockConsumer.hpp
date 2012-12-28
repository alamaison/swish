/**
    @file

    Mock implementation of an ISftpConsumer.

    @if license

    Copyright (C) 2010  Alexander Lamaison <awl03@doc.ic.ac.uk>

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
#include <comet/bstr.h> // bstr_t
#include <comet/safearray.h>
#include <comet/server.h> // simple_object

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
        EmptyPassword,    ///< Return an empty BSTR (not NULL, "")
        CustomPassword,   ///< Return the string set with SetPassword
        WrongPassword,    ///< Return a very unlikely sequence of characters
        NullPassword,     ///< Return NULL and S_OK (catastrophic failure)
        FailPassword,     ///< Return E_FAIL
        AbortPassword,    ///< Return E_ABORT (simulate user cancelled)
        ThrowPassword     ///< Throw exception if password requested
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

    MockConsumer()
        :
          m_password_behaviour(ThrowPassword),
          m_password_attempt_count(0),
          m_password_attempt_count_max(1),
          m_keyboard_interative_behaviour(ThrowResponse),
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

    void set_confirm_overwrite_behaviour(ConfirmOverwriteBehaviour behaviour)
    {
        m_confirm_overwrite_behaviour = behaviour;
    }

    bool confirmed_overwrite() const { return m_confirmed_overwrite; }

    // ISftpConsumer methods
    HRESULT OnPasswordRequest(BSTR request, BSTR *password_out)
    {
        ++m_password_attempt_count;
        *password_out = NULL;
        if (m_password.empty())
            return E_NOTIMPL;

        BOOST_CHECK_GT(::SysStringLen(request), 0U);
        
        // Perform chosen test behaviour
        // The three password cases which should never succeed will try to send
        // their 'reply' up to m_nMaxPassword time to simulate a user repeatedly
        // trying the wrong password and then giving up. 
        if (m_password_attempt_count > m_password_attempt_count_max)
            return E_FAIL;

        comet::bstr_t password;
        switch (m_password_behaviour)
        {
        case CustomPassword:
            password = m_password;
            break;
        case WrongPassword:
            password = L"WrongPasswordXyayshdkhjhdk";
            break;
        case EmptyPassword:
            // leave password blank
            break;
        case NullPassword:
            *password_out = NULL;
            return S_OK;
        case FailPassword:
            return E_FAIL;
        case AbortPassword:
            return E_ABORT;
        case ThrowPassword:
            BOOST_FAIL("Unexpected call to " __FUNCTION__);
            return E_FAIL;
        default:
            BOOST_FAIL(
                "Unreachable: Unrecognised OnPasswordRequest() behaviour");
            return E_UNEXPECTED;
        }

        *password_out = password.detach();
        return S_OK;
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

    HRESULT OnPrivateKeyFileRequest(BSTR * /*pbstrPrivateKeyFile*/)
    {
        return E_NOTIMPL;
    }

    HRESULT OnPublicKeyFileRequest(BSTR * /*pbstrPublicKeyFile*/)
    {
        return E_NOTIMPL;
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

    KeyboardInteractiveBehaviour m_keyboard_interative_behaviour;
    int m_ki_attempt_count;
    int m_ki_attempt_count_max;

    ConfirmOverwriteBehaviour m_confirm_overwrite_behaviour;
    bool m_confirmed_overwrite;

    std::wstring m_password;
};

} // namespace test

#endif
