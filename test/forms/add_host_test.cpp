/**
    @file

    Exercise new host dialogue box.

    @if licence

    Copyright (C) 2010  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

#pragma comment(linker, \
    "\"/manifestdependency:type='Win32' "\
    "name='Microsoft.Windows.Common-Controls' "\
    "version='6.0.0.0' "\
    "processorArchitecture='*' "\
    "publicKeyToken='6595b64144ccf1df' "\
    "language='*'\"")

#include "swish/forms/add_host.hpp" // test subject

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(add_host_tests)

namespace {

/**
 * Sends a button click to the Cancel button of the dialog programmatically.
 *
 * @todo  This relies on internal knowledge that the cancel is the 17th control
 *        and the dialog template applies an offset of 100.  Not cool!
 */
DWORD WINAPI click_cancel_thread(LPVOID /*thread_param*/)
{
	::Sleep(1700);
	HWND hwnd = GetForegroundWindow();

	// Send Cancel button click message to dialog box
	//WPARAM wParam = MAKEWPARAM(IDCANCEL, BN_CLICKED);
	//pThis->m_dlg.SendMessage(WM_COMMAND, wParam);

	// Alternatively post left mouse button up/down directly to Cancel button
	::SendMessage(
		::GetDlgItem(hwnd, 116), WM_LBUTTONDOWN, MK_LBUTTON, NULL);
	::SendMessage(::GetDlgItem(hwnd, 116), WM_LBUTTONUP, NULL, NULL);
	return 0;
}

}

BOOST_AUTO_TEST_CASE( show )
{
	DWORD thread_id;
	HANDLE thread = ::CreateThread(
		NULL, 0, click_cancel_thread, NULL, 0, &thread_id);

	try
	{
		swish::forms::add_host(NULL);
	}
	catch (const std::exception& e)
	{
		BOOST_REQUIRE_EQUAL(e.what(), "user cancelled form");
	}

	::CloseHandle(thread);
}

BOOST_AUTO_TEST_SUITE_END()
