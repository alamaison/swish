#include "stdafx.h"
#include "NewConnDialog_test.h"

//CPPUNIT_TEST_SUITE_REGISTRATION( CNewConnDialog_test );

void CNewConnDialog_test::setUp()
{
}

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

void CNewConnDialog_test::tearDown()
{
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
