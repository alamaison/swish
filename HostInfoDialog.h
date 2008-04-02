/*  Declaration of ATL dialog box class for host connection information.

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

#ifndef HOSTINFODIALOG_H
#define HOSTINFODIALOG_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#pragma once
#include "stdafx.h"
#include "resource.h"       // main symbols

/**
 * ATL-based wrapper class for the host information entry dialog box.
 *
 * The dialog is used to obtain SSH connection information from the user
 * in order to make a connection to a remote host.  The dialog has four fields
 * as well as OK and Cancel buttons.
 *
 * Text fields:
 * - "User:" SSH acount user name (IDC_USER)
 * - "Host:" Remote host address/name (IDC_HOST)
 * - "Path:" Path for initial listing (IDC_PATH)
 * Numeric field:
 * - "Port:" TCP/IP port to connect over (IDC_PORT)
 *
 * @image html "HostInfoDialog.png" "Dialog box appearance"
 */
class CHostInfoDialog : 
	public CDialogImpl<CHostInfoDialog>
{
public:
	CHostInfoDialog() : m_uPort(22) {}

	/** @name Accessors
	 * Dialog field setters and getters
	 */
	// @{
	CString GetUser();
	CString GetHost();
	CString GetPath();
	USHORT GetPort();
	void SetUser( LPCTSTR pszUser );
	void SetHost( LPCTSTR pszHost );
	void SetPath( LPCTSTR pszPath );
	void SetPort( USHORT uPort );
	// @}

	/** Dialog box resource identifier */
	enum { IDD = IDD_HOSTINFO_DIALOG };

BEGIN_MSG_MAP(CHostInfoDialog)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	COMMAND_HANDLER(IDOK, BN_CLICKED, OnOK)
	COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnCancel)
	// Handle change in any text field using same handler
	COMMAND_CODE_HANDLER(EN_CHANGE, OnChange)
END_MSG_MAP()

private:
	/** @name Message handlers */
	// @{
	LRESULT __callback OnInitDialog(MESSAGE_HANDLER_PARAMS);
	// @}

	/** @name Command handlers */
	// @{
	LRESULT __callback OnChange(COMMAND_HANDLER_PARAMS);
	LRESULT __callback OnOK(COMMAND_HANDLER_PARAMS);
	LRESULT __callback OnCancel(COMMAND_HANDLER_PARAMS);
	// @}

	/** @name Field validity */
	// @{
	BOOL _IsValid();
	void _HandleValidity();
	// @}

	CString m_strUser, m_strHost, m_strPath;
	USHORT m_uPort;

};

#endif // HOSTINFODIALOG_H
