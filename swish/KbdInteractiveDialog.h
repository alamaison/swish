/*  WTL dialog box for keyboard-interactive requests.

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

#pragma once

#include "resource.h"       // main symbols

#include <vector>
using std::vector;

typedef vector<CString> PromptList;
typedef vector<bool>    EchoList;
typedef vector<CString> ResponseList;

class CKbdInteractiveDialog : 
	public CDialogImpl<CKbdInteractiveDialog>
{
public:

	/** Dialog box resource identifier */
	enum { IDD = IDD_KBDINTERACTIVEDIALOG };

	CKbdInteractiveDialog(
		PCWSTR pszName, PCWSTR pszInstruction,
		__in PromptList vecPrompts, __in EchoList vecEcho);
	~CKbdInteractiveDialog();

	ResponseList GetResponses();

    BEGIN_MSG_MAP(CKbdInteractiveDialog)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_HANDLER(IDOK, BN_CLICKED, OnOK)
        COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnCancel)
    END_MSG_MAP()

private:
	/** @name Message handlers */
	// @{
	LRESULT __callback OnInitDialog(MESSAGE_HANDLER_PARAMS);
	// @}

	/** @name Command handlers */
	// @{
	LRESULT __callback OnOK(COMMAND_HANDLER_PARAMS);
	LRESULT __callback OnCancel(COMMAND_HANDLER_PARAMS);
	// @}

	/** @name GUI drawing */
	// @{
	CRect _DrawInstruction(PCWSTR pszInstruction, __in CRect rectDialog);
	CRect _DrawPrompt(
		PCWSTR pszPrompt, __in CPoint point, __in CRect rectDialog);
	CRect _DrawResponseBox(
		bool fHideResponse, __in CPoint point, __in CRect rectDialog);
	CRect _DrawOKCancel(__in CPoint point, __in CRect rectDialog);
	// @}

	void _ExchangeData();

	// Input
	CString      m_strName;
	CString      m_strInstruction;
	PromptList   m_vecPrompts;
	EchoList     m_vecEcho;

	// Output
	vector<HWND> m_vecResponseWindows;
	ResponseList m_vecResponses;
};
