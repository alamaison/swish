/**
    @file

    WTL dialog box for keyboard-interactive requests.

    @if license

    Copyright (C) 2008, 2009, 2013  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "resource.h"   // main symbols

#include "swish/atl.hpp"  // Common ATL setup
#include <atlwin.h>     // ATL windowing classes
#include <atltypes.h>   // For CRect and CPoint

#include <string>
#include <utility> // pair
#include <vector>

class CKbdInteractiveDialog : 
    public ATL::CDialogImpl<CKbdInteractiveDialog>
{
public:

    /** Dialog box resource identifier */
    enum { IDD = IDD_KBDINTERACTIVEDIALOG };

    CKbdInteractiveDialog(
        const std::string& title, const std::string& instructions,
        const std::vector<std::pair<std::string, bool>>& prompts);

    std::vector<std::string> GetResponses();

    BEGIN_MSG_MAP(CKbdInteractiveDialog)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_HANDLER(IDOK, BN_CLICKED, OnOK)
        COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnCancel)
    END_MSG_MAP()

private:
    /** @name Message handlers */
    // @{
    LRESULT __callback OnInitDialog(UINT, WPARAM, LPARAM, BOOL&);
    // @}

    /** @name Command handlers */
    // @{
    LRESULT __callback OnOK(WORD, WORD, HWND, BOOL&);
    LRESULT __callback OnCancel(WORD, WORD, HWND, BOOL&);
    // @}

    /** @name GUI drawing */
    // @{
    CRect _DrawInstruction(__in CRect rectDialog);
    CRect _DrawPrompt(
        const std::string&, __in CPoint point, __in CRect rectDialog);
    CRect _DrawResponseBox(
        bool fHideResponse, __in CPoint point, __in CRect rectDialog);
    CRect _DrawOKCancel(__in CPoint point, __in CRect rectDialog);
    // @}

    void _ExchangeData();

    // Input
    std::string m_title;
    std::string m_instructions;
    std::vector<std::pair<std::string, bool>> m_prompts;

    // Output
    std::vector<HWND> m_vecResponseWindows;
    std::vector<std::string> m_responses;
};
