/**
    @file

    Stub implementation of an ISftpConsumer.

    @if license

    Copyright (C) 2009, 2010  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#pragma once

#include "swish/interfaces/SftpProvider.h"

#include "test/common_boost/helpers.hpp"

#include <winapi/com/catch.hpp> // WINAPI_COM_CATCH_AUTO_INTERFACE

#include <comet/bstr.h> // bstr_t
#include <comet/server.h> // simple_object

#include <string>

#include <boost/filesystem.hpp>

namespace test {

class CConsumerStub : public comet::simple_object<ISftpConsumer>
{
public:

    typedef ISftpConsumer interface_is;

    CConsumerStub(
        boost::filesystem::path privatekey, boost::filesystem::path publickey)
        : m_privateKey(privatekey), m_publicKey(publickey) {}

    // ISftpConsumer methods
    IFACEMETHODIMP OnPasswordRequest(BSTR /*bstrRequest*/, BSTR *pbstrPassword)
    {
        *pbstrPassword = NULL;
        return E_NOTIMPL;
    }

    IFACEMETHODIMP OnKeyboardInteractiveRequest(
        BSTR /*bstrName*/, BSTR /*bstrInstruction*/,
        SAFEARRAY * /*psaPrompts*/, SAFEARRAY * /*psaShowResponses*/,
        SAFEARRAY ** /*ppsaResponses*/)
    {
        BOOST_ERROR("Unexpected call to "__FUNCTION__);
        return E_NOTIMPL;
    }

    /**
     * Return the path of the file containing the private key.
     *
     * The path is set via SetKeyPaths().
     */
    IFACEMETHODIMP OnPrivateKeyFileRequest(BSTR *pbstrPrivateKeyFile)
    {
        ATLENSURE_RETURN_HR(pbstrPrivateKeyFile, E_POINTER);
        *pbstrPrivateKeyFile = NULL;

        try
        {
            *pbstrPrivateKeyFile = comet::bstr_t::detach(
                m_privateKey.file_string());
        }
        WINAPI_COM_CATCH_AUTO_INTERFACE();

        return S_OK;
    }

    /**
     * Return the path of the file containing the public key.
     *
     * The path is set via SetKeyPaths().
     */
    IFACEMETHODIMP OnPublicKeyFileRequest(BSTR *pbstrPublicKeyFile)
    {
        ATLENSURE_RETURN_HR(pbstrPublicKeyFile, E_POINTER);
        *pbstrPublicKeyFile = NULL;

        try
        {
            *pbstrPublicKeyFile = comet::bstr_t::detach(
                m_publicKey.file_string());
        }
        WINAPI_COM_CATCH_AUTO_INTERFACE();

        return S_OK;
    }

    IFACEMETHODIMP OnConfirmOverwrite(
        BSTR /*bstrOldFile*/, BSTR /*bstrNewFile*/)
    {
        BOOST_ERROR("Unexpected call to "__FUNCTION__);
        return E_NOTIMPL;
    }

    IFACEMETHODIMP OnHostkeyMismatch(
        BSTR /*bstrHostName*/, BSTR /*bstrHostKey*/, BSTR /*bstrHostKeyType*/)
    {
        return S_FALSE;
    }

    IFACEMETHODIMP OnHostkeyUnknown(
        BSTR /*bstrHostName*/, BSTR /*bstrHostKey*/, BSTR /*bstrHostKeyType*/)
    {
        return S_FALSE;
    }

private:
    boost::filesystem::path m_privateKey;
    boost::filesystem::path m_publicKey;
};

} // namespace test
