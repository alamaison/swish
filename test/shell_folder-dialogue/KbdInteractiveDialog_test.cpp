/**
    @file

    Basic testing of the 'Keyboard-interactive Authentication' dialogue box.

    @if license

    Copyright (C) 2009, 2013  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include <winapi/error.hpp>

#include <boost/test/unit_test.hpp>

#include <string>
#include <utility> // pair, make_pair
#include <vector>

#include "swish/shell_folder/KbdInteractiveDialog.h"

using winapi::last_error;

using std::make_pair;
using std::pair;
using std::string;
using std::vector;

BOOST_AUTO_TEST_SUITE( KbdInteractiveDialog_tests )

namespace {

    const DWORD CLICK_DELAY = 700;

    /**
     * Sends a button click to the Cancel button of the dialog programmatically.
     */
    DWORD WINAPI ClickCancelThread(LPVOID lpThreadParam)
    {
        CKbdInteractiveDialog *pDlg = ((CKbdInteractiveDialog *)lpThreadParam);
        ::Sleep(CLICK_DELAY);

        // Post left mouse button up/down directly to Cancel button
        ::PostMessage(
            pDlg->GetDlgItem(IDCANCEL), WM_LBUTTONDOWN, MK_LBUTTON, NULL);
        ::PostMessage(pDlg->GetDlgItem(IDCANCEL), WM_LBUTTONUP, NULL, NULL);
        return 0;
    }

    /**
     * Sends a button click to the OK button of the dialog programmatically.
     */
    DWORD WINAPI ClickOKThread(LPVOID lpThreadParam)
    {
        CKbdInteractiveDialog *pDlg = ((CKbdInteractiveDialog *)lpThreadParam);
        ::Sleep(CLICK_DELAY);

        // Post left mouse button up/down directly to OK button
        ::PostMessage(
            pDlg->GetDlgItem(IDOK), WM_LBUTTONDOWN, MK_LBUTTON, NULL);
        ::PostMessage(pDlg->GetDlgItem(IDOK), WM_LBUTTONUP, NULL, NULL);
        return 0;
    }

    void _testModalDisplay(CKbdInteractiveDialog& dlg, bool fClickCancel = true)
    {
        // Launch thread which will send button click to dialog
        DWORD dwThreadId;
        HANDLE hClickThread = ::CreateThread(
            NULL, 0,
            (fClickCancel) ? ClickCancelThread : ClickOKThread,
            &dlg, 0, &dwThreadId
            );
        BOOST_REQUIRE( hClickThread );

        // Launch dialog (blocks until dialog ends) and check button ID
        INT_PTR rc = dlg.DoModal();
		if (rc == -1)
			BOOST_THROW_EXCEPTION(last_error());
			
		
		BOOST_CHECK_EQUAL(rc, (INT_PTR) ((fClickCancel) ? IDCANCEL : IDOK));

        // Check that thread terminated
        ::Sleep(CLICK_DELAY);
        DWORD dwThreadStatus;
        ::GetExitCodeThread(hClickThread, &dwThreadStatus);
        BOOST_CHECK( STILL_ACTIVE != dwThreadStatus );

        // Cleanup
        BOOST_CHECK( ::CloseHandle(hClickThread) );
        hClickThread = NULL;
    }
}

BOOST_AUTO_TEST_CASE( single_prompt )
{
    vector<pair<string, bool>> prompts;
    prompts.push_back(make_pair(string("Test prompt:"), true));

    CKbdInteractiveDialog dlg(
        "server-sent name", "server-sent instruction", prompts);

    _testModalDisplay(dlg);
}

BOOST_AUTO_TEST_CASE( single_prompt_no_instruction )
{
    vector<pair<string, bool>> prompts;
    prompts.push_back(make_pair(string("Test prompt:"), true));

    CKbdInteractiveDialog dlg("server-sent name", "", prompts);

    _testModalDisplay(dlg);
}

BOOST_AUTO_TEST_CASE( single_prompt_no_instruction_nor_name )
{
    vector<pair<string, bool>> prompts;
    prompts.push_back(make_pair(string("Test prompt:"), true));

    CKbdInteractiveDialog dlg("", "", prompts);

    _testModalDisplay(dlg);
}

BOOST_AUTO_TEST_CASE( long_instruction )
{
    vector<pair<string, bool>> prompts;
    prompts.push_back(make_pair(string("Test prompt:"), true));

    CKbdInteractiveDialog dlg(
        "server-sent name", 
        "A very very very very long instruction which, as permitted "
        "by the [IETF RFC 4256] SFTP specification, can contain "
        "linebreaks in\r\n"
        "Windows style\r\nUnix style\nlegacy MacOS style\rall of which "
        "should behave correctly.",
        prompts);

    _testModalDisplay(dlg);
}

BOOST_AUTO_TEST_CASE( multiple_prompts )
{
    vector<pair<string, bool>> prompts;
    prompts.push_back(make_pair(string("Test prompt 1:"), true));
    prompts.push_back(make_pair(string("Test prompt 2:"), false));
    prompts.push_back(make_pair(string("Test prompt 3:"), true));

    CKbdInteractiveDialog dlg("", "", prompts);

    _testModalDisplay(dlg);
}

BOOST_AUTO_TEST_CASE( long_prompt )
{
    vector<pair<string, bool>> prompts;
    prompts.push_back(make_pair(string("Test prompt 1:"), true));
    prompts.push_back(
        make_pair(string("Test prompt 2 which is much longer than all the "
        "other prompts:"), false));
    prompts.push_back(make_pair(string("Test prompt 3:"), true));

    CKbdInteractiveDialog dlg("", "", prompts);

    _testModalDisplay(dlg);
}

BOOST_AUTO_TEST_CASE( empty_responses_ok_clicked )
{
    vector<pair<string, bool>> prompts;
    prompts.push_back(make_pair(string("Test prompt 1:"), true));
    prompts.push_back(make_pair(string("Test prompt 2:"), false));
    prompts.push_back(make_pair(string("Test prompt 3:"), true));

    CKbdInteractiveDialog dlg("", "", prompts);

    _testModalDisplay(dlg, false);

    vector<string> responses = dlg.GetResponses();

    BOOST_CHECK_EQUAL(responses.size(), 3U);
    BOOST_CHECK_EQUAL(responses[0], "");
    BOOST_CHECK_EQUAL(responses[1], "");
    BOOST_CHECK_EQUAL(responses[2], "");
}

BOOST_AUTO_TEST_CASE( empty_responses_cancel_clicked )
{
    vector<pair<string, bool>> prompts;
    prompts.push_back(make_pair(string("Test prompt 1:"), true));
    prompts.push_back(make_pair(string("Test prompt 2:"), false));
    prompts.push_back(make_pair(string("Test prompt 3:"), true));

    CKbdInteractiveDialog dlg("", "", prompts);

    _testModalDisplay(dlg, true);

    vector<string> responses = dlg.GetResponses();

    BOOST_CHECK_EQUAL(responses.size(), 0U);
}

BOOST_AUTO_TEST_SUITE_END()
