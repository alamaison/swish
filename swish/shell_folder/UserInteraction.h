/**
    @file

	Component to handle user-interaction between the user and an SftpProvider.

    @if licence

    Copyright (C) 2008, 2009  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "SftpProvider.h"   // ISftpProvider/ISftpConsumer interfaces

#include "swish/atl.hpp"  // Common ATL setup

#include "swish/CoFactory.hpp"

class ATL_NO_VTABLE CUserInteraction :
	public ISftpConsumer,
	public ATL::CComObjectRoot,
	public swish::CCoFactory<CUserInteraction> // Needs to be public
{
public:

	BEGIN_COM_MAP(CUserInteraction)
		COM_INTERFACE_ENTRY(ISftpConsumer)
	END_COM_MAP()

	CUserInteraction();
	void SetHWND(__in_opt HWND hwnd);
	void ClearHWND();

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
	HWND m_hwnd;      ///< Window to use as parent for user interaction.
};

