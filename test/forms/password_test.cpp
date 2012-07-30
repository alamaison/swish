/**
    @file

    Exercise password dialogue box.

    @if license

    Copyright (C) 2010, 2011  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#include "swish/forms/password.hpp" // test subject

#include <boost/test/unit_test.hpp>

#include <string>

BOOST_AUTO_TEST_SUITE(password_tests)

/**
 * Sends a button click to the Cancel button of the dialog programmatically.
 *
 * @todo  This relies on internal knowledge that the cancel is the 4th control
 *        and the dialog template applies an offset of 100.  Not cool!
 */
DWORD WINAPI click_cancel_thread(LPVOID /*thread_param*/)
{
    ::Sleep(1700);
    HWND hwnd = GetForegroundWindow();

    // Post left mouse button up/down directly to Cancel button
    ::SendMessage(
        ::GetDlgItem(hwnd, 103), WM_LBUTTONDOWN, MK_LBUTTON, NULL);
    ::SendMessage(::GetDlgItem(hwnd, 103), WM_LBUTTONUP, NULL, NULL);
    return 0;
}

BOOST_AUTO_TEST_CASE( show )
{
    DWORD thread_id;
    HANDLE thread = ::CreateThread(
        NULL, 0, click_cancel_thread, NULL, 0, &thread_id);

    std::wstring password;
    BOOST_CHECK(
        !swish::forms::password_prompt(
            NULL, L"Oi! Gimme a password", password));
    BOOST_CHECK(password.empty());

    ::CloseHandle(thread);
}

BOOST_AUTO_TEST_SUITE_END()
