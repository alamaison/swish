/*  ATL password dialog box implementation.

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
#include "PasswordDialog.h"

/**
 * Handle dialog initialisation by copying member data into Win32 fields.
 *
 * The member data may have been set using the @ref Accessors 
 * "accessor methods".  Once copied, these fields are validated and the 
 * dialog modified accordingly
 *
 * @pre the dialog must have been initialised by calling DoModal() or
 *      Create() before this function is called.  The fields must exist in
 *      order to copy data into them.
 *
 * @see SetPrompt()
 */
LRESULT CPasswordDialog::OnInitDialog(MESSAGE_HANDLER_PARAMS)
{
	DoDataExchange(DDX_LOAD);
	return 1;  // Let the system set the focus
}

/**
 * Handle the OK button click event by ending the dialog.
 *
 * The data in the Win32 dialog fields is copied to the member variables
 * thereby making it available to the accessor methods.
 *
 * @pre The dialog must have been initialised by calling DoModal() or
 *      Create() before this function is called.  The fields must exist in
 *      order to copy data from them.
 *
 * @returns IDOK to the caller of DoModal()
 *
 * @see GetPassword()
 * @see OnCancel()
 */
LRESULT CPasswordDialog::OnOK(WORD, WORD wID, HWND, BOOL&)
{
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
LRESULT CPasswordDialog::OnCancel(WORD, WORD wID, HWND, BOOL&)
{
	EndDialog(wID);
	return 0;
}

/**
 * Get value of the password field.
 *
 * @pre the OK button must be clicked first in order to copy the data out
 *      of the Win32 field.
 * @returns the password entered by the user.
 */
CString CPasswordDialog::GetPassword() const
{
	return m_strPassword;
}

/**
 * Set the value to be loaded into the password prompt when dialog is displayed.
 *
 * The value set using this function is copied into the Win32 label 
 * when the dialog is initialised.  This is done by the OnInitDialog() 
 * message handler which handle dialog initialisation.
 *
 * @see OnInitDialog()
 */
void CPasswordDialog::SetPrompt( PCTSTR pszPrompt )
{
	m_strPasswordPrompt = pszPrompt;
}
