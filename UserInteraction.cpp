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

#include "stdafx.h"
#include "UserInteraction.h"
#include "PasswordDialog.h"  // For password dialog box

HRESULT CUserInteraction::Initialize( HWND hwndOwner )
{
	m_hwndOwner = hwndOwner;
	return S_OK;
}

/**
 * Displays UI dialog to get password from user and returns it.
 *
 * @param [in]  bstrRequest    The prompt to display to the user.
 * @param [out] pbstrPassword  The reply from the user - the password.
 *
 * @return E_ABORT if the user chooses Cancel, E_FAIL if user interaction is
 *         forbidden and S_OK otherwise.
 */
STDMETHODIMP CUserInteraction::OnPasswordRequest(
	BSTR bstrRequest, BSTR *pbstrPassword
)
{
	if (m_hwndOwner == NULL)
		return E_FAIL;

	CString strPrompt = bstrRequest;
	ATLASSERT(strPrompt.GetLength() > 0);

	CPasswordDialog dlgPassword;
	dlgPassword.SetPrompt( strPrompt ); // Pass text through from backend
	if (dlgPassword.DoModal() == IDOK)
	{
		CString strPassword;
		strPassword = dlgPassword.GetPassword();
		*pbstrPassword = strPassword.AllocSysString();
		return S_OK;
	}
	else
		return E_ABORT;
}

/**
 * Display Yes/No/Cancel dialog to the user with given message.
 *
 * @param [in]  bstrMessage    The prompt to display to the user.
 * @param [in]  bstrYesInfo    The explanation of the Yes option.
 * @param [in]  bstrNoInfo     The explanation of the No option.
 * @param [in]  bstrCancelInfo The explanation of the Cancel option.
 * @param [in]  bstrTitle      The title of the dialog.
 * @param [out] piResult       The user's choice.
 *
 * @return E_ABORT if the user chooses Cancel, E_FAIL if user interaction is
 *         forbidden and S_OK otherwise.
*/
STDMETHODIMP CUserInteraction::OnYesNoCancel(
	BSTR bstrMessage, BSTR bstrYesInfo, BSTR bstrNoInfo, BSTR bstrCancelInfo,
	BSTR bstrTitle, int *pnResult
)
{
	if (m_hwndOwner == NULL)
		return E_FAIL;

	// Construct unknown key information message
	CString strMessage = bstrMessage;
	CString strTitle = bstrTitle;
	if (bstrYesInfo && ::SysStringLen(bstrYesInfo) > 0)
	{
		strMessage += _T("\r\n");
		strMessage += bstrYesInfo;
	}
	if (bstrNoInfo && ::SysStringLen(bstrNoInfo) > 0)
	{
		strMessage += _T("\r\n");
		strMessage += bstrNoInfo;
	}
	if (bstrCancelInfo && ::SysStringLen(bstrCancelInfo) > 0)
	{
		strMessage += _T("\r\n");
		strMessage += bstrCancelInfo;
	}

	// Display message box
	int msgboxID = ::MessageBox(
		NULL, strMessage, strTitle, 
		MB_ICONWARNING | MB_YESNOCANCEL | MB_DEFBUTTON3 );

	// Process user choice
	switch (msgboxID)
	{
	case IDYES:
		*pnResult = 1; return S_OK;
	case IDNO:
		*pnResult = 0; return S_OK;
	case IDCANCEL:
		*pnResult = -1; return E_ABORT;
	default:
		*pnResult = -2;
		UNREACHABLE;
	}

	return E_ABORT;
}


STDMETHODIMP CUserInteraction::OnConfirmOverwrite(
	BSTR bstrOldFile, BSTR bstrExistingFile )
{
	if (m_hwndOwner == NULL)
		return E_FAIL;

	CString strMessage = _T("The folder already contains a file named '");
	strMessage += bstrExistingFile;
	strMessage += _T("'\r\n\r\nWould you like to replace the existing ");
	strMessage += _T("file\r\n\r\n\t");
	strMessage += bstrExistingFile;
	strMessage += _T("\r\n\r\nwith this one?\r\n\r\n\t");
	strMessage += bstrOldFile;

	int ret = ::IsolationAwareMessageBox(m_hwndOwner, strMessage, NULL, 
		MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);

	if (ret == IDYES)
		return S_OK;
	else
		return E_ABORT;
}

STDMETHODIMP CUserInteraction::OnConfirmOverwriteEx(
	Listing ltOldFile, Listing ltExistingFile )
{
	// Add your function implementation here.
	return E_NOTIMPL;
}

STDMETHODIMP CUserInteraction::OnReportError( BSTR bstrMessage )
{
	if (m_hwndOwner == NULL)
		return E_FAIL;

	::IsolationAwareMessageBox(m_hwndOwner, CComBSTR(bstrMessage), NULL,
		MB_OK | MB_ICONERROR);
	return S_OK;
}

// CUserInteraction
