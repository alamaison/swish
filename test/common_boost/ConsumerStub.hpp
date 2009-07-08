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

#include <string>

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

	// ISftpConsumer methods
	IFACEMETHODIMP OnPasswordRequest(
		__in BSTR /*bstrRequest*/, __out BSTR *pbstrPassword)
	{
		*pbstrPassword = ATL::CComBSTR(L"").Detach();
		return S_OK;
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
};

}} // namespace test::common_boost
