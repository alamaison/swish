/**
    @file

    Stub implementation of an ISftpConsumer.

    @if license

    Copyright (C) 2009, 2010, 2013  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "swish/provider/sftp_provider.hpp"

#include "test/common_boost/helpers.hpp"

#include <winapi/com/catch.hpp> // WINAPI_COM_CATCH_AUTO_INTERFACE

#include <comet/server.h> // simple_object

#include <boost/throw_exception.hpp> // BOOST_THROW_EXCEPTION

#include <string>

#include <boost/filesystem.hpp>

namespace test {

/**
 * Very simple consumer that just handles authentication via pub-key.
*/
class CConsumerStub : public comet::simple_object<ISftpConsumer>
{
public:

    typedef ISftpConsumer interface_is;

    CConsumerStub(
        boost::filesystem::path privatekey, boost::filesystem::path publickey)
        : m_privateKey(privatekey), m_publicKey(publickey) {}

    // ISftpConsumer methods

    virtual boost::optional<std::wstring> prompt_for_password()
    {
        return boost::optional<std::wstring>();
    }

    virtual boost::optional<
        std::pair<boost::filesystem::path, boost::filesystem::path>>
        key_files()
    {
        return std::make_pair(m_privateKey, m_publicKey);
    }

    virtual boost::optional<std::vector<std::string>> challenge_response(
        const std::string& /*title*/, const std::string& /*instructions*/,
        const std::vector<std::pair<std::string, bool>>& /*prompts*/)
    {
        BOOST_ERROR("Unexpected call to "__FUNCTION__);
        BOOST_THROW_EXCEPTION(std::exception("Not implemented"));
    }

    HRESULT OnConfirmOverwrite(
        BSTR /*bstrOldFile*/, BSTR /*bstrNewFile*/)
    {
        BOOST_ERROR("Unexpected call to "__FUNCTION__);
        return E_NOTIMPL;
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
    boost::filesystem::path m_privateKey;
    boost::filesystem::path m_publicKey;
};

} // namespace test
