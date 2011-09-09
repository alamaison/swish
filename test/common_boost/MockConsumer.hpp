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

#include "swish/interfaces/SftpProvider.h"

#include <comet/bstr.h> // bstr_t
#include <comet/server.h> // simple_object

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
	typedef enum tagConfirmOverwriteBehaviour {
		AllowOverwrite,         ///< Return S_OK
		PreventOverwrite,       ///< Return E_ABORT
	} ConfirmOverwriteBehaviour;

	MockConsumer()
		: m_confirmed_overwrite(false),
		  m_confirm_overwrite_behaviour(PreventOverwrite) {}

	void set_password(const std::wstring& password)
	{
		m_password = password;
	}

	void set_confirm_overwrite_behaviour(ConfirmOverwriteBehaviour behaviour)
	{
		m_confirm_overwrite_behaviour = behaviour;
	}

	bool confirmed_overwrite() const { return m_confirmed_overwrite; }

	// ISftpConsumer methods
	IFACEMETHODIMP OnPasswordRequest(BSTR /*bstrRequest*/, BSTR *pbstrPassword)
	{
		*pbstrPassword = NULL;
		if (m_password.empty())
			return E_NOTIMPL;

		*pbstrPassword = comet::bstr_t(m_password).detach();
		return S_OK;
	}

	IFACEMETHODIMP OnKeyboardInteractiveRequest(
		BSTR /*bstrName*/, BSTR /*bstrInstruction*/,
		SAFEARRAY * /*psaPrompts*/, SAFEARRAY * /*psaShowResponses*/,
		SAFEARRAY ** /*ppsaResponses*/)
	{
		return E_NOTIMPL;
	}

	IFACEMETHODIMP OnPrivateKeyFileRequest(BSTR * /*pbstrPrivateKeyFile*/)
	{
		return E_NOTIMPL;
	}

	IFACEMETHODIMP OnPublicKeyFileRequest(BSTR * /*pbstrPublicKeyFile*/)
	{
		return E_NOTIMPL;
	}

	IFACEMETHODIMP OnConfirmOverwrite(
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
	ConfirmOverwriteBehaviour m_confirm_overwrite_behaviour;
	bool m_confirmed_overwrite;
	std::wstring m_password;
};

} // namespace test

#endif
