/*  Component to handle user-interaction between the user and an SftpProvider.

    Copyright (C) 2008  Alexander Lamaison <awl03@doc.ic.ac.uk>

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
*/

#ifndef USERINTERACTION_H
#define USERINTERACTION_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#pragma once
#include "stdafx.h"
#include "resource.h"       // main symbols

// CUserInteraction
[
	coclass,
	default(ISftpConsumer),
	threading(apartment),
	vi_progid("Swish.UserInteraction"),
	progid("Swish.UserInteraction.1"),
	version(1.0),
	uuid("b816a84a-5022-11dc-9153-0090f5284f85"),
	helpstring("UserInteraction Class")
]
class ATL_NO_VTABLE CUserInteraction :
	public ISftpConsumer
{
public:

	CUserInteraction() : m_hwndOwner(NULL) {}

	HRESULT Initialize( __in HWND hwndOwner );

	/**
	 * Create and initialise an instance of the CUserInteraction class.
	 *
	 * @param [in]  hwndOwner  A window handle to parent window which this 
	 *                         instance should use as the parent for any 
	 *                         user-interaction.
	 * @param [out] ppReturn   The location in which to return the 
	 *                         ISftpConsumer interace pointer for this instance.
	 */
	static HRESULT MakeInstance(
		__in HWND hwndOwner, __deref_out ISftpConsumer **ppReturn )
	{
		HRESULT hr;

		CComObject<CUserInteraction> *pConsumer;
		hr = CComObject<CUserInteraction>::CreateInstance(&pConsumer);
		ATLENSURE_RETURN_HR(SUCCEEDED(hr), hr );

		pConsumer->AddRef();

		pConsumer->Initialize(hwndOwner);
		hr = pConsumer->QueryInterface(ppReturn);
		ATLASSERT(SUCCEEDED(hr));

		pConsumer->Release();
		pConsumer = NULL;

		return hr;
	}

	/**
	 * User interaction callbacks.
	 * @{
	 */
	// ISftpConsumer Methods
	IFACEMETHODIMP OnPasswordRequest(
		__in BSTR bstrRequest, __out BSTR *pbstrPassword
	);
	IFACEMETHODIMP OnKeyboardInteractiveRequest(
		__in BSTR bstrName, __in BSTR bstrInstruction,
		__in SAFEARRAY *psaPrompts,
		__in SAFEARRAY *psaShowResponses,
		__deref_out SAFEARRAY **ppsaResponses
	);
	IFACEMETHODIMP OnYesNoCancel(
		__in BSTR bstrMessage,
		__in_opt BSTR bstrYesInfo,
		__in_opt BSTR bstrNoInfo,
		__in_opt BSTR bstrCancelInfo,
		__in_opt BSTR bstrTitle,
		__out int *piResult
	);
	IFACEMETHODIMP OnConfirmOverwrite(
		__in BSTR bstrOldFile,
		__in BSTR bstrExistingFile
	);
	IFACEMETHODIMP OnConfirmOverwriteEx(
		__in Listing ltOldFile,
		__in Listing ltExistingFile
	);
	IFACEMETHODIMP OnReportError(
		__in BSTR bstrMessage
	);
	/* @} */

private:
	HWND m_hwndOwner; ///< Window to use as parent for user interaction.
};

#endif // USERINTERACTION_H
