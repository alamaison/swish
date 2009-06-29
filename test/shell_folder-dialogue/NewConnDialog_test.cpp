/**
    @file

    Basic testing of the 'New Connection' dialogue box.

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

#include "test/common/CppUnitExtensions.h"

// Redefine the 'private' keyword to inject a friend declaration for this 
// test class directly into the target class's header
class CNewConnDialog_test;
#undef private
#define private \
	friend class CNewConnDialog_test; private
#include "swish/shell_folder/NewConnDialog.h"
#undef private

class CNewConnDialog_test : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE( CNewConnDialog_test );
		CPPUNIT_TEST( testGetUser );
		CPPUNIT_TEST( testGetHost );
		CPPUNIT_TEST( testGetPath );
		CPPUNIT_TEST( testGetPort );
		CPPUNIT_TEST( testDoModal );
	CPPUNIT_TEST_SUITE_END();

protected:
	void testGetUser();
	void testGetHost();
	void testGetPath();
	void testGetPort();
	void testDoModal();

private:
	CNewConnDialog m_dlg;

	/* Thread related functions and variables */
	static DWORD WINAPI ClickCancelThread( __in LPVOID lpThreadParam );
};


CPPUNIT_TEST_SUITE_REGISTRATION( CNewConnDialog_test );


void CNewConnDialog_test::testGetUser()
{
	CPPUNIT_ASSERT( m_dlg.GetUser().IsEmpty() );
}

void CNewConnDialog_test::testGetHost()
{
	CPPUNIT_ASSERT( m_dlg.GetHost().IsEmpty() );
}

void CNewConnDialog_test::testGetPath()
{
	CPPUNIT_ASSERT( m_dlg.GetPath().IsEmpty() );
}

void CNewConnDialog_test::testGetPort()
{
	CPPUNIT_ASSERT_EQUAL( (UINT)22, m_dlg.GetPort() ); // Default should be 22

	m_dlg.SetPort( 0 );
	CPPUNIT_ASSERT_EQUAL( (UINT)0, m_dlg.GetPort() );
	m_dlg.SetPort( 65535 );
	CPPUNIT_ASSERT_EQUAL( (UINT)65535, m_dlg.GetPort() );
	m_dlg.SetPort( 65536 );
	CPPUNIT_ASSERT_EQUAL( (UINT)65535, m_dlg.GetPort() );
	m_dlg.SetPort( 22 );
	CPPUNIT_ASSERT_EQUAL( (UINT)22, m_dlg.GetPort() );
}

void CNewConnDialog_test::testDoModal()
{
	DWORD dwThreadId;

	// Launch thread which will send button click to dialog
	HANDLE hClickCancelThread = ::CreateThread(
		NULL, 0, ClickCancelThread, this, 0, &dwThreadId
	);
	CPPUNIT_ASSERT( hClickCancelThread );

	// Launch dialog (blocks until dialog ends) and check button ID
	CPPUNIT_ASSERT_EQUAL( (INT_PTR)IDCANCEL, m_dlg.DoModal() );

	// Check that thread terminated
	::Sleep(700);
	DWORD dwThreadStatus;
	::GetExitCodeThread(hClickCancelThread, &dwThreadStatus);
	CPPUNIT_ASSERT_ASSERTION_FAIL(
		CPPUNIT_ASSERT_EQUAL( STILL_ACTIVE, dwThreadStatus )
	);

	// Cleanup
	CPPUNIT_ASSERT( ::CloseHandle(hClickCancelThread) );
	hClickCancelThread = NULL;
}


/*----------------------------------------------------------------------------*
 * Private functions
 *----------------------------------------------------------------------------*/

/**
 * Sends a button click to the Cancel button of the dialog programmatically.
 */
DWORD WINAPI CNewConnDialog_test::ClickCancelThread(LPVOID lpThreadParam)
// static
{
	CNewConnDialog_test *pThis = ((CNewConnDialog_test *)lpThreadParam);
	::Sleep(700);

	// Send Cancel button click message to dialog box
	//WPARAM wParam = MAKEWPARAM(IDCANCEL, BN_CLICKED);
	//pThis->m_dlg.SendMessage(WM_COMMAND, wParam);

	// Alternatively post left mouse button up/down directly to Cancel button
	::PostMessage(
		pThis->m_dlg.GetDlgItem(IDCANCEL), WM_LBUTTONDOWN, MK_LBUTTON, NULL);
	::PostMessage(pThis->m_dlg.GetDlgItem(IDCANCEL), WM_LBUTTONUP, NULL, NULL);
	return 0;
}
