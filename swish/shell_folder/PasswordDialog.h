/**
    @file

	Declaration of ATL dialog box class for user password entry.

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

#include "resource.h"       // main symbols

#include "swish/atl.hpp"    // Common ATL setup
#include <atlwin.h>         // ATL windowing classes
#include <atlstr.h>         // CString

#include "wtl.hpp"          // WTL
#include <atlddx.h>         // WTL DDX/DDV

/**
 * ATL-based wrapper class for the password entry dialog box.
 *
 * The dialog is used to obtain a password from the user
 * in order to make a connection to a remote host.  The dialog has one field
 * as well as OK and Cancel buttons.
 */
class CPasswordDialog : 
	public ATL::CDialogImpl<CPasswordDialog>,
	public WTL::CWinDataExchange<CPasswordDialog>
{
public:

	/** Dialog box resource identifier */
	enum { IDD = IDD_PASSWORD_DIALOG };

	BEGIN_MSG_MAP(CPasswordDialog)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_HANDLER(IDOK, BN_CLICKED, OnOK)
		COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnCancel)
	END_MSG_MAP()

	BEGIN_DDX_MAP(CPasswordDialog)
		DDX_TEXT(IDC_PASSWORD, m_strPassword)
		DDX_TEXT_LEN(IDC_PASSWORD_LABEL, m_strPasswordPrompt, 64)
	END_DDX_MAP()

	/** @name Accessors
	 * Dialog field setters and getters
	 */
	// @{
	ATL::CString GetPassword() const;
	void SetPrompt( __in PCTSTR pszPrompt );
	// @}

private:

	ATL::CString m_strPassword;
	ATL::CString m_strPasswordPrompt;

	/** @name Message handlers */
	// @{
	LRESULT __callback OnInitDialog(UINT, WPARAM, LPARAM, BOOL&);
	// @}

	/** @name Command handlers */
	// @{
	LRESULT __callback OnOK(WORD, WORD, HWND, BOOL&);
	LRESULT __callback OnCancel(WORD, WORD, HWND, BOOL&);
	// @}
};