/**
    @file

    Stub implementation of an ISftpConsumer.

    @if licence

    Copyright (C) 2009  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "swish/atl.hpp"
#include "swish/CoFactory.hpp"
#include "swish/catch_com.hpp"
#include "swish/interfaces/SftpProvider.h"

#include "test/common_boost/helpers.hpp"

#include <string>

#include <boost/filesystem.hpp>

namespace test {
namespace common_boost {

class ATL_NO_VTABLE CConsumerStub :
	public ATL::CComObjectRootEx<ATL::CComObjectThreadModel>,
	public ISftpConsumer,
	public swish::CCoFactory<CConsumerStub>
{
public:

	BEGIN_COM_MAP(CConsumerStub)
		COM_INTERFACE_ENTRY(ISftpConsumer)
	END_COM_MAP()

	void SetKeyPaths(
		boost::filesystem::path privatekey, boost::filesystem::path publickey)
	{
		m_privateKey = privatekey;
		m_publicKey = publickey;
	}

	// ISftpConsumer methods
	IFACEMETHODIMP OnPasswordRequest(
		__in BSTR /*bstrRequest*/, __out BSTR *pbstrPassword)
	{
		*pbstrPassword = NULL;
		return E_NOTIMPL;
	}

	IFACEMETHODIMP OnKeyboardInteractiveRequest(
		__in BSTR /*bstrName*/, __in BSTR /*bstrInstruction*/,
		__in SAFEARRAY * /*psaPrompts*/,
		__in SAFEARRAY * /*psaShowResponses*/,
		__deref_out SAFEARRAY ** /*ppsaResponses*/)
	{
		BOOST_ERROR("Unexpected call to "__FUNCTION__);
		return E_NOTIMPL;
	}

	/**
	 * Return the path of the file containing the private key.
	 *
	 * The path is set via SetKeyPaths().
	 */
	IFACEMETHODIMP OnPrivateKeyFileRequest(__out BSTR *pbstrPrivateKeyFile)
	{
		ATLENSURE_RETURN_HR(pbstrPrivateKeyFile, E_POINTER);
		*pbstrPrivateKeyFile = NULL;

		try
		{
			ATL::CComBSTR bstrPrivate = m_privateKey.file_string().c_str();
			*pbstrPrivateKeyFile = bstrPrivate.Detach();
		}
		catchCom()

		return S_OK;
	}

	/**
	 * Return the path of the file containing the public key.
	 *
	 * The path is set via SetKeyPaths().
	 */
	IFACEMETHODIMP OnPublicKeyFileRequest(__out BSTR *pbstrPublicKeyFile)
	{
		ATLENSURE_RETURN_HR(pbstrPublicKeyFile, E_POINTER);
		*pbstrPublicKeyFile = NULL;

		try
		{
			ATL::CComBSTR bstrPublic = m_publicKey.file_string().c_str();
			*pbstrPublicKeyFile = bstrPublic.Detach();
		}
		catchCom()

		return S_OK;
	}

	IFACEMETHODIMP OnYesNoCancel(
		__in BSTR /*bstrMessage*/, __in_opt BSTR /*bstrYesInfo*/, 
		__in_opt BSTR /*bstrNoInfo*/, __in_opt BSTR /*bstrCancelInfo*/,
		__in BSTR /*bstrTitle*/, __out int * /*piResult*/)
	{
		BOOST_ERROR("Unexpected call to "__FUNCTION__);
		return E_NOTIMPL;
	}

	IFACEMETHODIMP OnConfirmOverwrite(
		__in BSTR /*bstrOldFile*/,
		__in BSTR /*bstrNewFile*/)
	{
		BOOST_ERROR("Unexpected call to "__FUNCTION__);
		return E_NOTIMPL;
	}

	IFACEMETHODIMP OnConfirmOverwriteEx(
		__in Listing /*ltOldFile*/,
		__in Listing /*ltNewFile*/)
	{
		BOOST_ERROR("Unexpected call to "__FUNCTION__);
		return E_NOTIMPL;
	}

	IFACEMETHODIMP OnReportError(
		__in BSTR bstrMessage)
	{
		std::wstring message(bstrMessage);
		BOOST_ERROR(
			std::wstring(L"Unexpected call to "__FUNCTIONW__) + message);
		return S_OK;
	}

private:
	boost::filesystem::path m_privateKey;
	boost::filesystem::path m_publicKey;
};

}} // namespace test::common_boost
