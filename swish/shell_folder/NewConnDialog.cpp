/**
    @file

	WTL dialog box where user enters host connection information.

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

#include "NewConnDialog.h"
#include "host_management.hpp"
#include "swish/remotelimits.h"  // Text field limits
#include "swish/debug.hpp"

#include "Registry.h"
#include "HostPidl.h"

#include <vector>

using ATL::CString;
using WTL::CStatic;

using std::vector;

using swish::host_management::ConnectionExists;

namespace { //private

	const int DEFAULT_PORT = 22;

	const PWSTR FORBIDDEN_CHARS = L"@: \t\n\r\b\"'\\";
	const PWSTR FORBIDDEN_PATH_CHARS = L"\"\t\n\r\b\\";

	const PWSTR ICON_MODULE = L"user32.dll";
	const int ICON_ERROR = 103;
	const int ICON_INFO = 104;
	const int ICON_SIZE = 16;

	/**
	 * Load a small (16x16) icon from user32.dll by ordinal.
	 */
	HICON LoadSmallSystemIcon(int icon)
	{
		HMODULE module = ::GetModuleHandle(ICON_MODULE);

		HANDLE iconHandle = ::LoadImage(
			module, MAKEINTRESOURCE(icon), IMAGE_ICON, ICON_SIZE, 
			ICON_SIZE, 0);
		ATLASSERT_REPORT(iconHandle, ::GetLastError());

		return static_cast<HICON>(iconHandle);
	}
}

/**
 * Construct dialogue instance and load system icons for status messages.
 */
CNewConnDialog::CNewConnDialog() : m_uPort(DEFAULT_PORT), 
                                   m_infoIcon(LoadSmallSystemIcon(ICON_INFO)), 
                                   m_errorIcon(LoadSmallSystemIcon(ICON_ERROR)),
								   m_fLoadedInitial(false)
{}

/**
 * Handle dialog initialisation by copying member data into Win32 fields.
 *
 * The member data may have been set using the @ref Accessors "accessor 
 * methods".  Once copied, these fields are validated and the 
 * dialog modified accordingly
 *
 * @pre the dialog must have been initialised by calling DoModal() or
 *      Create() before this function is called.  The fields must exist in
 *      order to copy data into them.
 *
 * @see SetUser() SetHost() SetPath() SetPort()
 * @see _UpdateValidity()
 */
LRESULT CNewConnDialog::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&)
{
	// Save handles to status controls which will be continually updated
	m_status = GetDlgItem(IDC_HOSTDLG_STATUS);
	m_icon = GetDlgItem(IDC_HOSTDLG_STATUS_ICON);

	// Copy any intial data into Win32 controls
	DoDataExchange(DDX_LOAD);
	m_fLoadedInitial = true; // Initial load phase complete; start reacting
	                         // to changes

	// Redraw the window to match the state of the fields
	_UpdateValidity();

	return 1;  // Let the system set the focus
}

/**
 * Handle a change event from one of the dialog fields.
 *
 * The data is the fields is revalidated and the dialog modified accordingly.
 *
 * @pre the dialog must have been initialised by calling DoModal() or
 *      Create() before this function is called.
 *
 * @see _UpdateValidity()
 */
LRESULT CNewConnDialog::OnChange(WORD, WORD, HWND, BOOL&)
{
	if (m_fLoadedInitial) // Skip update during initial load phase
		_UpdateValidity();
	return 0;
}

/**
 * Handle the OK button click event by ending the dialog.
 *
 * The data in the Win32 dialog fields is copied to the member variables
 * thereby making it available to the accessor methods.
 *
 * @pre The _IsValid() function must have passed allowing _UpdateValidity() to
 *      enable the OK button before this handler could be invoked.  This means 
 *      we don't need to check the fields.
 *
 * @pre The dialog must have been initialised by calling DoModal() or
 *      Create() before this function is called.  The fields must exist in
 *      order to copy data from them.
 *
 * @returns IDOK to the caller of DoModal()
 *
 * @see GetUser() GetHost() GetPath() GetPort()
 * @see OnCancel()
 */
LRESULT CNewConnDialog::OnOK(WORD, WORD wID, HWND, BOOL&)
{
	// Save data from Win32 objects into member fields
	DoDataExchange(DDX_SAVE);
	EndDialog(wID);
	return 0;
}

/**
 * Handle the Cancel button click event by ending the dialog.
 *
 * @pre the dialog must have been initialised by calling DoModal() or
 *      Create() before this function is called.  The dialog must exist before
 *      it can be terminated.
 *
 * @returns IDCANCEL to the caller of DoModal()
 *
 * @see OnOK()
 */
LRESULT CNewConnDialog::OnCancel(WORD, WORD wID, HWND, BOOL&)
{
	EndDialog(wID);
	return 0;
}

/**
 * Get value of the connection name field.
 *
 * @returns the friendly connection name (label).
 */
CString CNewConnDialog::GetName() const
{
	return m_strName;
}

/**
 * Get value of the user name field.
 */
CString CNewConnDialog::GetUser() const
{
	return m_strUser;
}

/**
 * Get value of the host name field.
 */
CString CNewConnDialog::GetHost() const
{
	return m_strHost;
}

/**
 * Get value of the path field.
 */
CString CNewConnDialog::GetPath() const
{
	return m_strPath;
}

/**
 * Get value of the port field.
 */
UINT CNewConnDialog::GetPort() const
{
	return m_uPort;
}

/**
 * Set the value to be loaded into the name field when dialog is displayed.
 *
 * The value set using this function is copied into the Win32 dialog field
 * when the dialog is initialised.  This is done by the OnInitDialog() 
 * message handler which handle dialog initialisation.
 *
 * @see OnInitDialog()
 */
void CNewConnDialog::SetName(PCWSTR pwszName)
{
	m_strName = pwszName;
}

/**
 * Set the value to be loaded into the user name field when dialog is displayed.
 *
 * The value set using this function is copied into the Win32 dialog field
 * when the dialog is initialised.  This is done by the OnInitDialog() 
 * message handler which handle dialog initialisation.
 *
 * @see OnInitDialog()
 */
void CNewConnDialog::SetUser(PCWSTR pwszUser)
{
	m_strUser = pwszUser;
}

/**
 * Set the value to be loaded into the host name field when dialog is displayed.
 *
 * The value set using this function is copied into the Win32 dialog field
 * when the dialog is initialised.  This is done by the OnInitDialog() 
 * message handler which handle dialog initialisation.
 *
 * @see OnInitDialog()
 */
void CNewConnDialog::SetHost(PCWSTR pwszHost)
{
	m_strHost = pwszHost;
}

/**
 * Set the value to be loaded into the path field when dialog is displayed.
 *
 * The value set using this function is copied into the Win32 dialog field
 * when the dialog is initialised.  This is done by the OnInitDialog() 
 * message handler which handle dialog initialisation.
 *
 * @see OnInitDialog()
 */
void CNewConnDialog::SetPath(PCWSTR pwszPath)
{
	m_strPath = pwszPath;
}

/**
 * Set the value to be loaded into the port field when dialog is displayed.
 *
 * The value set using this function is copied into the Win32 dialog field
 * when the dialog is initialised.  This is done by the OnInitDialog() 
 * message handler which handle dialog initialisation.  If the value set is
 * greater than the maximum allowed port value, @c MAX_PORT is used instead.
 *
 * @see OnInitDialog()
 */
void CNewConnDialog::SetPort(UINT uPort)
{
	m_uPort = min(uPort, MAX_PORT);
}

/**
 * Set the status message to the given text and make the control visible.
 */
void CNewConnDialog::_ShowStatus(PCWSTR pwszMessage) 
{
	m_status.SetWindowText(pwszMessage);
	m_status.ShowWindow(SW_SHOW);
}

/**
 * Set the status message to a string resource and make the control visible.
 */
void CNewConnDialog::_ShowStatus(int id) 
{
	CString strMessage;
	ATLVERIFY(strMessage.LoadString(id));
	_ShowStatus(strMessage);
}

/**
 * Hide the status message.
 */
void CNewConnDialog::_HideStatus() 
{
	m_status.ShowWindow(SW_HIDE);
}

/**
 * Display an information icon (blue 'i') next to the status message.
 */
void CNewConnDialog::_ShowStatusInfoIcon()
{
	m_icon.SetIcon(m_infoIcon);
	m_icon.ShowWindow(SW_SHOW);
}

/**
 * Display an error icon (red 'X') next to the status message.
 */
void CNewConnDialog::_ShowStatusErrorIcon()
{
	m_icon.SetIcon(m_errorIcon);
	m_icon.ShowWindow(SW_SHOW);
}

/**
 * Hide the status icon.
 */
void CNewConnDialog::_HideStatusIcon()
{
	m_icon.ShowWindow(SW_HIDE);
}

/**
 * Checks if the value in the dialog box Name field is valid.
 *
 * Criteria:
 * - The field must not contain more than @ref MAX_LABEL_LEN characters.
 *
 * @pre the dialog must have been initialised by calling DoModal() or
 *      Create() before this function is called.  The fields must exist in
 *      order to check them.
 *
 * @see _UpdateValidity()
 */
bool CNewConnDialog::_IsValidName() const
{
	ATLASSERT(m_hWnd); // Must call DoModal() or Create() first

	CString strUser = GetName();
	return strUser.GetLength() <= MAX_LABEL_LEN;
}

/**
 * Checks if the value in the dialog box User field is valid.
 *
 * Criteria:
 * - The field must not contain more than @ref MAX_USERNAME_LEN characters 
 *   and must not contain any characters from @ref FORBIDDEN_CHARS.
 *
 * @pre the dialog must have been initialised by calling DoModal() or
 *      Create() before this function is called.  The fields must exist in
 *      order to check them.
 *
 * @todo The validity criteria are woefully inadequate:
 * - There are many characters that are not allowed in usernames.
 *   The test should really check that characters are all from an allowed list
 *   rather than not from a forbidden list.
 * - Windows usernames can contain spaces.  These must be escaped.
 *
 * @see _UpdateValidity()
 */
bool CNewConnDialog::_IsValidUser() const
{
	ATLASSERT(m_hWnd); // Must call DoModal() or Create() first

	CString strUser = GetUser();
	return strUser.GetLength() <= MAX_USERNAME_LEN
		&& strUser.FindOneOf(FORBIDDEN_CHARS) < 0;
}

/**
 * Checks if the value in the dialog box Host field is valid.
 *
 * Criteria:
 * - The field must not contain more than @ref MAX_HOSTNAME_LEN characters 
 *   and must not contain any characters from @ref FORBIDDEN_CHARS
 *
 * @pre the dialog must have been initialised by calling DoModal() or
 *      Create() before this function is called.  The fields must exist in
 *      order to check them.
 *
 * @todo The validity criteria are woefully inadequate:
 * - There are many characters that are not allowed in hostnames.
 *   The test should really check that characters are all from an allowed list
 *   rather than not from a forbidden list.
 *
 * @see _UpdateValidity()
 */
bool CNewConnDialog::_IsValidHost() const
{
	ATLASSERT(m_hWnd); // Must call DoModal() or Create() first

	CString strHost = GetHost();
	return strHost.GetLength() <= MAX_HOSTNAME_LEN
		&& strHost.FindOneOf(FORBIDDEN_CHARS) < 0;
}

/**
 * Checks if the value in the dialog box Path field is valid.
 *
 * Criteria:
 * - The path field must not contain more than @ref MAX_PATH_LEN characters 
 *   and must not contain any characters from @ref FORBIDDEN_PATH_CHARS.
 *
 * @pre the dialog must have been initialised by calling DoModal() or
 *      Create() before this function is called.  The fields must exist in
 *      order to check them.
 *
 * @todo The validity criteria are woefully inadequate:
 * - Paths can contain almost any character.  Some will have to be escaped.
 *
 * @see _UpdateValidity()
 */
bool CNewConnDialog::_IsValidPath() const
{
	ATLASSERT(m_hWnd); // Must call DoModal() or Create() first

	CString strPath = GetPath();
	return strPath.GetLength() <= MAX_PATH_LEN 
		&& strPath.FindOneOf(FORBIDDEN_PATH_CHARS) < 0;
}

/**
 * Checks if the value in the dialog box Port field is valid.
 *
 * Criteria:
 * - The field must contain a number between 0 and 65535 (@ref MAX_PORT).
 *
 * @pre the dialog must have been initialised by calling DoModal() or
 *      Create() before this function is called.  The fields must exist in
 *      order to check them.
 *
 * @see _UpdateValidity()
 */
bool CNewConnDialog::_IsValidPort() const
{
	UINT port = GetPort();
	return port >= MIN_PORT && port <= MAX_PORT;
}

/**
 * Disables the OK button if a field in the dialog is invalid.
 *
 * @pre the dialog must have been initialised by calling DoModal() or
 *      Create() before this function is called.  The fields must exist in
 *      order to check them.
 *
 * @see OnOK()
 */
void CNewConnDialog::_UpdateValidity()
{
	ATLASSERT(m_hWnd); // Must call DoModal() or Create() first

	// Copy member data from Win32 object fields
	DoDataExchange(DDX_SAVE);

	bool enableOK = false;

	if (!_IsValidName())
	{
		_ShowStatus(IDS_HOSTDLG_INVALID_NAME);
		_ShowStatusErrorIcon();
	}
	else if (!_IsValidHost())
	{
		_ShowStatus(IDS_HOSTDLG_INVALID_HOST);
		_ShowStatusErrorIcon();
	}
	else if (!_IsValidPort())
	{
		_ShowStatus(IDS_HOSTDLG_INVALID_PORT);
		_ShowStatusErrorIcon();
	}
	else if (!_IsValidUser())
	{
		_ShowStatus(IDS_HOSTDLG_INVALID_USER);
		_ShowStatusErrorIcon();
	}
	else if (!_IsValidPath())
	{
		_ShowStatus(IDS_HOSTDLG_INVALID_PATH);
		_ShowStatusErrorIcon();
	}
	else if (ConnectionExists(GetName().GetString()))
	{
		_ShowStatus(IDS_HOSTDLG_CONNECTION_EXISTS);
		_ShowStatusErrorIcon();
	}
	else if (GetName().GetLength() < 1 || GetHost().GetLength() < 1
		|| GetUser().GetLength() < 1 || GetPath().GetLength() < 1)
	{
		_ShowStatus(IDS_HOSTDLG_COMPLETE_ALL);
		_ShowStatusInfoIcon();
	}
	else
	{
		_HideStatus();
		_HideStatusIcon();
		enableOK = true;
	}

	::EnableWindow(GetDlgItem(IDOK), enableOK);
}
