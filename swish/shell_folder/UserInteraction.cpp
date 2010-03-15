/**
    @file

	Component to handle user-interaction between the user and an SftpProvider.

    @if licence

    Copyright (C) 2008, 2009, 2010  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "UserInteraction.h"

#include "PasswordDialog.h"       // Password dialog box
#include "KbdInteractiveDialog.h" // Keyboard-interactive auth dialog box
#include "swish/debug.hpp"

#include <winapi/gui/task_dialog.hpp> // task_dialog
#include <winapi/gui/message_box.hpp> // message_box

#include <comet/bstr.h> // bstr_t

#include <boost/bind.hpp> // bind

#include <atlsafe.h>              // CComSafeArray

#include <string>

using ATL::CComBSTR;
using ATL::CString;
using ATL::CComSafeArray;

using namespace winapi::gui;

using comet::bstr_t;

using std::string;
using std::wstring;

CUserInteraction::CUserInteraction() : m_hwnd(NULL) {}

void CUserInteraction::SetHWND(HWND hwnd)
{
	m_hwnd = hwnd;
}

void CUserInteraction::ClearHWND()
{
	SetHWND(NULL);
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
	if (m_hwnd == NULL)
		return E_FAIL;

	CString strPrompt = bstrRequest;
	ATLASSERT(strPrompt.GetLength() > 0);

	CPasswordDialog dlgPassword;
	dlgPassword.SetPrompt( strPrompt ); // Pass text through from backend
	assert(m_hwnd);
	if (dlgPassword.DoModal(m_hwnd) == IDOK)
	{
		CString strPassword;
		strPassword = dlgPassword.GetPassword();
		*pbstrPassword = strPassword.AllocSysString();
		return S_OK;
	}
	else
		return E_ABORT;
}

STDMETHODIMP CUserInteraction::OnKeyboardInteractiveRequest(
	BSTR bstrName, BSTR bstrInstruction, SAFEARRAY *psaPrompts, 
	SAFEARRAY *psaShowResponses, SAFEARRAY **ppsaResponses
)
{
	if (m_hwnd == NULL)
		return E_FAIL;

	CComSafeArray<BSTR> saPrompts(psaPrompts);
	CComSafeArray<VARIANT_BOOL> saShowPrompts(psaShowResponses);

	// Prompt array and echo mask array should correspond
	ATLASSERT(saPrompts.GetLowerBound() == saShowPrompts.GetLowerBound());
	ATLASSERT(saPrompts.GetUpperBound() == saShowPrompts.GetUpperBound());

	PromptList vecPrompts;
	EchoList vecEcho;
	for (int i = saPrompts.GetLowerBound(); i <= saPrompts.GetUpperBound(); i++)
	{
		vecPrompts.push_back(CString(saPrompts[i]));
		vecEcho.push_back((saShowPrompts[i] == VARIANT_TRUE) ? true : false);
	}

	// Show dialogue and fetch responses when user clicks OK
	CKbdInteractiveDialog dlg(bstrName, bstrInstruction, vecPrompts, vecEcho);
	if (dlg.DoModal(m_hwnd) == IDCANCEL)
		return E_ABORT;
	ResponseList vecResponses = dlg.GetResponses();

	// Create response array. Indices must correspond to prompts array!
	CComSafeArray<BSTR> saResponses(
		saPrompts.GetCount(), saPrompts.GetLowerBound());
	ATLASSERT(vecResponses.size() == saPrompts.GetCount());
	ATLASSERT(saPrompts.GetLowerBound() == saResponses.GetLowerBound());
	ATLASSERT(saPrompts.GetUpperBound() == saResponses.GetUpperBound());

	// SAFEARRAY may start at any index but vector will always start at 0.
	// We need to keep an offset value to map between them
	int nIndexOffset = saPrompts.GetLowerBound();

	// Fill responses SAFEARRAY
	for (int i = saPrompts.GetLowerBound(); i <= saPrompts.GetUpperBound(); i++)
	{
		ATLVERIFY(SUCCEEDED( saResponses.SetAt(
			i, CComBSTR(vecResponses[i-nIndexOffset]).Detach(), false) ));
	}

	*ppsaResponses = saResponses.Detach();

	return S_OK;
}

/**
 * Return the path of the file containing the private key.
 */
STDMETHODIMP CUserInteraction::OnPrivateKeyFileRequest(
	BSTR *pbstrPrivateKeyFile)
{
	ATLENSURE_RETURN_HR(pbstrPrivateKeyFile, E_POINTER);
	*pbstrPrivateKeyFile = NULL;

	return E_NOTIMPL;
}

/**
 * Return the path of the file containing the public key.
 */
STDMETHODIMP CUserInteraction::OnPublicKeyFileRequest(
	BSTR *pbstrPublicKeyFile)
{
	ATLENSURE_RETURN_HR(pbstrPublicKeyFile, E_POINTER);
	*pbstrPublicKeyFile = NULL;

	return E_NOTIMPL;
}

/**
 * Display Yes/No/Cancel dialog to the user with given message.
 *
 * @param [in]  bstrMessage    The prompt to display to the user.
 * @param [in]  bstrYesInfo    The explanation of the Yes option.
 * @param [in]  bstrNoInfo     The explanation of the No option.
 * @param [in]  bstrCancelInfo The explanation of the Cancel option.
 * @param [in]  bstrTitle      The title of the dialog.
 * @param [out] pnResult       The user's choice.
 *
 * @return E_ABORT if the user chooses Cancel, E_FAIL if user interaction is
 *         forbidden and S_OK otherwise.
*/
STDMETHODIMP CUserInteraction::OnYesNoCancel(
	BSTR bstrMessage, BSTR bstrYesInfo, BSTR bstrNoInfo, BSTR bstrCancelInfo,
	BSTR bstrTitle, int *pnResult
)
{
	if (m_hwnd == NULL)
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
	if (m_hwnd == NULL)
		return E_FAIL;

	CString strMessage = _T("The folder already contains a file named '");
	strMessage += bstrExistingFile;
	strMessage += _T("'\r\n\r\nWould you like to replace the existing ");
	strMessage += _T("file\r\n\r\n\t");
	strMessage += bstrExistingFile;
	strMessage += _T("\r\n\r\nwith this one?\r\n\r\n\t");
	strMessage += bstrOldFile;

	int ret = ::IsolationAwareMessageBox(m_hwnd, strMessage, NULL, 
		MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);

	if (ret == IDYES)
		return S_OK;
	else
		return E_ABORT;
}

STDMETHODIMP CUserInteraction::OnConfirmOverwriteEx(
	Listing /*ltOldFile*/, Listing /*ltExistingFile*/ )
{
	// Add your function implementation here.
	return E_NOTIMPL;
}

STDMETHODIMP CUserInteraction::OnReportError( BSTR bstrMessage )
{
	if (m_hwnd == NULL)
		return E_FAIL;

	::IsolationAwareMessageBox(m_hwnd, CComBSTR(bstrMessage), NULL,
		MB_OK | MB_ICONERROR);
	return S_OK;
}

namespace {

	/** Click handler callback. */
	HRESULT return_hr(HRESULT hr) { return hr; }

}

STDMETHODIMP CUserInteraction::OnHostkeyMismatch(
	BSTR bstrHostName, BSTR bstrHostKey, BSTR bstrHostKeyType)
{
	wstring title = L"Mismatched host-key";
	wstring instruction = L"WARNING: the SSH host-key has changed!";

	wstring message = 
		L"The SSH host-key sent by '" + bstr_t(bstrHostName) + "' to identify "
		"itself doesn't match the known key for this server.  This "
		"could mean a third-party is pretending to be the computer you're "
		"trying to connect to or the system administrator may have just "
		"changed the key.\n\n"
		"It is important to check this is the right key fingerprint:\n\n"
		"        " + bstrHostKeyType + "    " + bstrHostKey;
	
	wstring choices = L"\n\n"
		L"To update the known key for this host click Yes.\n"
		L"To connect to the server without updating the key click No.\n"
		L"Click Cancel unless you are sure the key is correct.";

	try
	{
		task_dialog::task_dialog<HRESULT> td(
			m_hwnd, instruction, message, title,
			winapi::gui::task_dialog::icon_type::warning, true,
			boost::bind(return_hr, E_ABORT));
		td.add_button(
			L"I trust this key: &update and connect\n"
			L"You won't have to verify this key again unless it changes",
			boost::bind(return_hr, S_OK));
		td.add_button(
			L"I trust this key: &just connect\n"
			L"You will be warned about this key again next time you connect",
			boost::bind(return_hr, S_FALSE));
		td.add_button(
			L"&Cancel\n"
			L"Choose this option unless you are sure the key is correct",
			boost::bind(return_hr, E_ABORT), true);
		return td.show();
	}
	catch (std::exception)
	{
		message_box::button_type::type button = message_box::message_box(
			m_hwnd, instruction + L"\n\n" + message + choices, title,
			message_box::box_type::yes_no_cancel,
			message_box::icon_type::warning, 3);
		switch (button)
		{
		case message_box::button_type::yes:
			return S_OK;
		case message_box::button_type::no:
			return S_FALSE;
		case message_box::button_type::cancel:
		default:
			return E_ABORT;
		}
	}
}


STDMETHODIMP CUserInteraction::OnHostkeyUnknown(
	BSTR bstrHostName, BSTR bstrHostKey, BSTR bstrHostKeyType)
{
	wstring title = L"Unknown host-key";
	wstring message = L"The server '" + bstr_t(bstrHostName) + "' has "
		"identified itself with an SSH host-key whose fingerprint is:\n\n"
		"        " + bstrHostKeyType + "    " + bstrHostKey + "\n\n"
		"If you are not expecting this key, a third-party may be pretending "
		"to be the computer you're trying to connect to.";

	try
	{
		wstring instruction = L"Verify unknown SSH host-key";
		task_dialog::task_dialog<HRESULT> td(
			m_hwnd, instruction, message, title,
			winapi::gui::task_dialog::icon_type::information, true,
			boost::bind(return_hr, E_ABORT));
		td.add_button(
			L"I trust this key: &store and connect\n"
			L"You won't have to verify this key again unless it changes",
			boost::bind(return_hr, S_OK));
		td.add_button(
			L"I trust this key: &just connect\n"
			L"You will be asked to verify the key again next time you connect",
			boost::bind(return_hr, S_FALSE));
		td.add_button(
			L"&Cancel\n"
			L"Choose this option unless you are sure the key is correct",
			boost::bind(return_hr, E_ABORT), true);
		return td.show();
	}
	catch (std::exception)
	{
		wstring choices = L"\n\n"
			L"To store this as the known key for this server click Yes.\n"
			L"To connect to the server without storing the key click No.\n"
			L"Click Cancel unless you are sure the key is correct.";
		message_box::button_type::type button = message_box::message_box(
			m_hwnd, message + choices, title,
			message_box::box_type::yes_no_cancel,
			message_box::icon_type::information, 3);
		switch (button)
		{
		case message_box::button_type::yes:
			return S_OK;
		case message_box::button_type::no:
			return S_FALSE;
		case message_box::button_type::cancel:
		default:
			return E_ABORT;
		}
	}
}
