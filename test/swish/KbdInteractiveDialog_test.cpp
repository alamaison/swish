#pragma once

#include "pch.h"
#include "test/common/CppUnitExtensions.h"

// Redefine the 'private' keyword to inject a friend declaration for this 
// test class directly into the target class's header
class CKbdInteractiveDialog_test;
#undef private
#define private \
	friend class CKbdInteractiveDialog_test; private
#include "swish/shell_folder/KbdInteractiveDialog.h"
#undef private

class CKbdInteractiveDialog_test : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE( CKbdInteractiveDialog_test );
		CPPUNIT_TEST( testSinglePrompt );
		CPPUNIT_TEST( testSinglePromptNoInstruction );
		CPPUNIT_TEST( testSinglePromptNoInstructionNorName );
		CPPUNIT_TEST( testLongInstruction );
		CPPUNIT_TEST( testMultiplePrompts );
		CPPUNIT_TEST( testLongPrompt );
		CPPUNIT_TEST( testEmptyResponsesOKClicked );
		CPPUNIT_TEST( testEmptyResponsesCancelClicked );
	CPPUNIT_TEST_SUITE_END();

public:
	CKbdInteractiveDialog_test() {}
	~CKbdInteractiveDialog_test() {}
	void setUp() {}
	void tearDown() {}

protected:

	void testSinglePrompt()
	{
		PromptList vecPrompts;
		vecPrompts.push_back(L"Test prompt:");

		EchoList vecEcho;
		vecEcho.push_back(true);

		CKbdInteractiveDialog dlg(
			L"server-sent name", L"server-sent instruction",
			vecPrompts, vecEcho
		);

		_testModalDisplay(dlg);
	}

	void testSinglePromptNoInstruction()
	{
		PromptList vecPrompts;
		vecPrompts.push_back(L"Test prompt:");

		EchoList vecEcho;
		vecEcho.push_back(true);

		CKbdInteractiveDialog dlg(
			L"server-sent name", L"", vecPrompts, vecEcho
		);

		_testModalDisplay(dlg);
	}

	void testSinglePromptNoInstructionNorName()
	{
		PromptList vecPrompts;
		vecPrompts.push_back(L"Test prompt:");

		EchoList vecEcho;
		vecEcho.push_back(true);

		CKbdInteractiveDialog dlg(L"", L"", vecPrompts, vecEcho);

		_testModalDisplay(dlg);
	}

	void testLongInstruction()
	{
		PromptList vecPrompts;
		vecPrompts.push_back(L"Test prompt:");

		EchoList vecEcho;
		vecEcho.push_back(true);

		CKbdInteractiveDialog dlg(
			L"server-sent name", 
			L"A very very very very long instruction which, as permitted "
			L"by the [IETF RFC 4256] SFTP specification, can contain "
			L"linebreaks in\r\n"
			L"Windows style\r\nUnix style\nlegacy MacOS style\rall of which "
			L"should behave correctly.",
			vecPrompts, vecEcho
		);

		_testModalDisplay(dlg);
	}

	void testMultiplePrompts()
	{
		PromptList vecPrompts;
		vecPrompts.push_back(L"Test prompt 1:");
		vecPrompts.push_back(L"Test prompt 2:");
		vecPrompts.push_back(L"Test prompt 3:");

		EchoList vecEcho;
		vecEcho.push_back(true);
		vecEcho.push_back(false);
		vecEcho.push_back(true);

		CKbdInteractiveDialog dlg(L"", L"", vecPrompts, vecEcho);

		_testModalDisplay(dlg);
	}

	void testLongPrompt()
	{
		PromptList vecPrompts;
		vecPrompts.push_back(L"Test prompt 1:");
		vecPrompts.push_back(
			L"Test prompt 2 which is much longer than all the other prompts:");
		vecPrompts.push_back(L"Test prompt 3:");

		EchoList vecEcho;
		vecEcho.push_back(true);
		vecEcho.push_back(false);
		vecEcho.push_back(true);

		CKbdInteractiveDialog dlg(L"", L"", vecPrompts, vecEcho);

		_testModalDisplay(dlg);
	}

	void testEmptyResponsesOKClicked()
	{
		PromptList vecPrompts;
		vecPrompts.push_back(L"Test prompt 1:");
		vecPrompts.push_back(L"Test prompt 2:");
		vecPrompts.push_back(L"Test prompt 3:");

		EchoList vecEcho;
		vecEcho.push_back(true);
		vecEcho.push_back(false);
		vecEcho.push_back(true);

		CKbdInteractiveDialog dlg(L"", L"", vecPrompts, vecEcho);

		_testModalDisplay(dlg, false);

		ResponseList vecResponses = dlg.GetResponses();

		CPPUNIT_ASSERT_EQUAL((size_t)3, vecResponses.size());
		CPPUNIT_ASSERT(vecResponses[0] == L"");
		CPPUNIT_ASSERT(vecResponses[1] == L"");
		CPPUNIT_ASSERT(vecResponses[2] == L"");
	}

	void testEmptyResponsesCancelClicked()
	{
		PromptList vecPrompts;
		vecPrompts.push_back(L"Test prompt 1:");
		vecPrompts.push_back(L"Test prompt 2:");
		vecPrompts.push_back(L"Test prompt 3:");

		EchoList vecEcho;
		vecEcho.push_back(true);
		vecEcho.push_back(false);
		vecEcho.push_back(true);

		CKbdInteractiveDialog dlg(L"", L"", vecPrompts, vecEcho);

		_testModalDisplay(dlg, true);

		ResponseList vecResponses = dlg.GetResponses();

		CPPUNIT_ASSERT_EQUAL((size_t)0, vecResponses.size());
	}
private:
#define CLICK_DELAY 700

	void _testModalDisplay(CKbdInteractiveDialog& dlg, bool fClickCancel = true)
	{
		// Launch thread which will send button click to dialog
		DWORD dwThreadId;
		HANDLE hClickThread = ::CreateThread(
			NULL, 0,
			(fClickCancel) ? ClickCancelThread : ClickOKThread,
			&dlg, 0, &dwThreadId
		);
		CPPUNIT_ASSERT( hClickThread );

		// Launch dialog (blocks until dialog ends) and check button ID
		CPPUNIT_ASSERT_EQUAL(
			(INT_PTR) ((fClickCancel) ? IDCANCEL : IDOK),
			dlg.DoModal()
		);

		// Check that thread terminated
		::Sleep(CLICK_DELAY);
		DWORD dwThreadStatus;
		::GetExitCodeThread(hClickThread, &dwThreadStatus);
		CPPUNIT_ASSERT( STILL_ACTIVE != dwThreadStatus );

		// Cleanup
		CPPUNIT_ASSERT( ::CloseHandle(hClickThread) );
		hClickThread = NULL;
	}

	/**
	 * Sends a button click to the Cancel button of the dialog programmatically.
	 */
	static DWORD WINAPI ClickCancelThread( __in LPVOID lpThreadParam)
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
	static DWORD WINAPI ClickOKThread( __in LPVOID lpThreadParam)
	{
		CKbdInteractiveDialog *pDlg = ((CKbdInteractiveDialog *)lpThreadParam);
		::Sleep(CLICK_DELAY);

		// Post left mouse button up/down directly to OK button
		::PostMessage(
			pDlg->GetDlgItem(IDOK), WM_LBUTTONDOWN, MK_LBUTTON, NULL);
		::PostMessage(pDlg->GetDlgItem(IDOK), WM_LBUTTONUP, NULL, NULL);
		return 0;
	}

};

CPPUNIT_TEST_SUITE_REGISTRATION( CKbdInteractiveDialog_test );
