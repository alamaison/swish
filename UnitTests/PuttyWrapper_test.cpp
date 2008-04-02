#include "stdafx.h"
#include "PuttyWrapper_test.h"

#include <list>
using std::list;

#define READ_STARTUP_MESSAGE _T("psftp: no hostname specified; use \"open host.name\" to connect\r\npsftp> ")
#define READ_STARTUP_MESSAGE_LEN 70
#define WRITE_OPEN_COMMAND _T("open %s@%s\r\n")
#define READ_OPEN_REPLY_HEAD _T("Remote working directory is /")
#define READ_OPEN_REPLY_TAIL _T("\r\npsftp> ")

#define PROMPT _T("psftp> ")
#define LS_PATH _T("/tmp")
#define SPACE_DELIMITER _T(" ")

CPPUNIT_TEST_SUITE_REGISTRATION( CPuttyWrapper_test );

CPuttyWrapper_test::CPuttyWrapper_test() : m_pPutty(NULL) {}

void CPuttyWrapper_test::setUp()
{
	CPPUNIT_ASSERT_NO_THROW(
		m_pPutty = new CPuttyWrapper( GetExePath() )
	);
}

void CPuttyWrapper_test::testRead()
{
	CString strActual;
	CString strExpected = READ_STARTUP_MESSAGE;
	
	CPPUNIT_ASSERT_NO_THROW( strActual = m_pPutty->Read() );
	CPPUNIT_ASSERT_EQUAL( // Ensure CString is accurately reporting its length
		(int)::_tcslen(strActual),
		strActual.GetLength()
	);
	CPPUNIT_ASSERT_EQUAL( strExpected, strActual );
}

/* In reality, this not only tests remote writing, it also tests remote reading
   unlike testRead above which only tests local reading (i.e. will pass even
   if remote host is unreachable */
void CPuttyWrapper_test::testWrite()
{
	ULONG cBytesWritten;
	CString strPrompt = PROMPT;
	CString strWrite;
	strWrite.Format( WRITE_OPEN_COMMAND, GetUserName(), GetHostName() );
	CString strActual;
	CString strExpected;

	CPPUNIT_ASSERT_NO_THROW( strActual = m_pPutty->Read() );
	CPPUNIT_ASSERT_EQUAL( // Ensure CString is accurately reporting its length
		(int)::_tcslen(strActual),
		strActual.GetLength() );

	// Test empty string write
	CPPUNIT_ASSERT_NO_THROW( cBytesWritten = m_pPutty->Write(_T("")) );

	// Test zero-size buffer write
	CPPUNIT_ASSERT_NO_THROW(
		cBytesWritten = m_pPutty->Write(_T("abracadabra"), 0) );
	CPPUNIT_ASSERT_EQUAL( (ULONG)0, cBytesWritten );

	// Test write single char non-null-terminated buffer and readback
	TCHAR c[] = {_T('\n')};
	CPPUNIT_ASSERT_NO_THROW( cBytesWritten = m_pPutty->Write(c, 1) );
	CPPUNIT_ASSERT_EQUAL( (ULONG)1, cBytesWritten );
	CPPUNIT_ASSERT_NO_THROW( strActual = m_pPutty->Read() );
	CPPUNIT_ASSERT_EQUAL( // Ensure CString is accurately reporting its length
		(int)::_tcslen(strActual),
		strActual.GetLength() );
	CPPUNIT_ASSERT_EQUAL( strPrompt, strActual );

	// Test write command and readback
	CPPUNIT_ASSERT_NO_THROW( cBytesWritten = m_pPutty->Write(strWrite) );
	CPPUNIT_ASSERT_NO_THROW( strActual = m_pPutty->Read() );
	CPPUNIT_ASSERT_EQUAL( (int)::_tcslen(strActual), strActual.GetLength() );

	// Must be of format: Remote working directory is /....\r\npsftp>
	strExpected = READ_OPEN_REPLY_HEAD;
	CPPUNIT_ASSERT_EQUAL(
		strExpected, strActual.Left( strExpected.GetLength() )
	);
	strExpected = READ_OPEN_REPLY_TAIL;
	CPPUNIT_ASSERT_EQUAL(
		strExpected, strActual.Right( strExpected.GetLength() )
	);
}

void CPuttyWrapper_test::testGetSizeOfDataInPipe()
{
	// We test this function once here immediately just to make sure that
	// it doesn't blow up.  It may well return 0 here (see below).
	CPPUNIT_ASSERT_NO_THROW( m_pPutty->GetSizeOfDataInPipe() );

	// Now we test the function again, taking notice of its output.
	// It is valid for the function to return 0 immediately as the child
	// process may not have had time to print anything to StdOut.
	// We force a sleep here to test for the output we eventually expect.
	::Sleep(300);
	DWORD expected = READ_STARTUP_MESSAGE_LEN;
	DWORD actual = m_pPutty->GetSizeOfDataInPipe();
	CPPUNIT_ASSERT_EQUAL( expected, actual );
}

void CPuttyWrapper_test::testRunLS()
{
	CString strWrite;
	strWrite.Format( WRITE_OPEN_COMMAND, GetUserName(), GetHostName() );
	CString strActual;
	CString strExpected;

	CPPUNIT_ASSERT_NO_THROW( strActual = m_pPutty->Read() );

	// Connect
	CPPUNIT_ASSERT_NO_THROW( m_pPutty->Write(strWrite) );
	CPPUNIT_ASSERT_NO_THROW( strActual = m_pPutty->Read() );
	// Must be of format: Remote working directory is /....\r\npsftp>
	strExpected = READ_OPEN_REPLY_HEAD;
	CPPUNIT_ASSERT_EQUAL(
		strExpected, strActual.Left(strExpected.GetLength())
	);
	strExpected = READ_OPEN_REPLY_TAIL;
	CPPUNIT_ASSERT_EQUAL(
		strExpected, strActual.Right(strExpected.GetLength())
	);

	// Get listing
	list<CString> lstLs;
	list<CString>::iterator itLs;
	CPPUNIT_ASSERT_NO_THROW( lstLs = m_pPutty->RunLS( LS_PATH ) );
	CPPUNIT_ASSERT( lstLs.size() > 0 );

	// Check format of listing is sensible
	for (itLs=lstLs.begin(); itLs!=lstLs.end(); itLs++)
	{
		CString strRow = *itLs;
		CString strPermissions, strHardLinks, strOwner, strGroup;
		CString strSize, strMonth, strDate, strTimeYear, strFilename;
		int iPos = 0;
		strPermissions = strRow.Tokenize( SPACE_DELIMITER, iPos );
		CPPUNIT_ASSERT( !strPermissions.IsEmpty() );
		strHardLinks = strRow.Tokenize( SPACE_DELIMITER, iPos );
		CPPUNIT_ASSERT( !strHardLinks.IsEmpty() );
		strOwner = strRow.Tokenize( SPACE_DELIMITER, iPos );
		CPPUNIT_ASSERT( !strOwner.IsEmpty() );
		strGroup = strRow.Tokenize( SPACE_DELIMITER, iPos );
		CPPUNIT_ASSERT( !strGroup.IsEmpty() );
		strSize = strRow.Tokenize( SPACE_DELIMITER, iPos );
		CPPUNIT_ASSERT( !strSize.IsEmpty() );
		strMonth = strRow.Tokenize( SPACE_DELIMITER, iPos );
		CPPUNIT_ASSERT( !strMonth.IsEmpty() );
		strDate = strRow.Tokenize( SPACE_DELIMITER, iPos );
		CPPUNIT_ASSERT( !strDate.IsEmpty() );
		strTimeYear = strRow.Tokenize( SPACE_DELIMITER, iPos );
		CPPUNIT_ASSERT( !strTimeYear.IsEmpty() );
		strFilename = strRow.Right( iPos );
		CPPUNIT_ASSERT( !strFilename.IsEmpty() );

		CPPUNIT_ASSERT(
			strPermissions[0] == _T('d') ||
			strPermissions[0] == _T('b') ||
			strPermissions[0] == _T('c') ||
			strPermissions[0] == _T('l') ||
			strPermissions[0] == _T('p') ||
			strPermissions[0] == _T('s') ||
			strPermissions[0] == _T('-'));
	}
}

void CPuttyWrapper_test::tearDown()
{
	if (m_pPutty) // Possible for setUp() to fail before m_pPutty constructed
		delete m_pPutty;
}

/*----------------------------------------------------------------------------*
 * Private functions
 *----------------------------------------------------------------------------*/

/**
 * Construct and return the path of the PuTTY executable: psftp.exe.
 *
 * @note Uses the raw CPuttyProvider CLSID directly.  If this were to change
 * this function would break.
 */
CString CPuttyWrapper_test::GetExePath() const
{
	// Get path of Swish DLL e.g. C:\Program Files\Swish\Swish.dll
	// using the CLSID of CPuttyProvider directly
	TCHAR szPath[MAX_PATH];
	CPPUNIT_ASSERT(
		::SHRegGetPath(HKEY_CLASSES_ROOT, 
			_T("CLSID\\{b816a842-5022-11dc-9153-0090f5284f85}\\InprocServer32"),
			0, szPath, NULL)
		== ERROR_SUCCESS
	);

	// Use to contruct psftp path e.g. C:\Program Files\Swish\psftp.exe
	CPPUNIT_ASSERT( ::PathRemoveFileSpec(szPath) );
	CString strExePath(szPath);
	strExePath += _T("\\psftp.exe");
	CPPUNIT_ASSERT( ::PathFileExists(strExePath) );

	return strExePath;
}

/**
 * Get the host name of the machine to connect to for remote testing.
 *
 * The host name is retrieved from the TEST_HOST_NAME environment variable.
 * If this variable is not set, a CPPUNIT exception is thrown.
 * 
 * In order to be useful, the host name should exist and the machine 
 * should be accessible via SSH.
 * 
 * @pre the host name should have at least 3 characters
 * @pre the host name should have less than 255 characters
 *
 * @return the host name
 */
CString CPuttyWrapper_test::GetHostName() const
{
	static CString strHostName;

	if (strHostName.IsEmpty())
	{
		if(!strHostName.GetEnvironmentVariable(_T("TEST_HOST_NAME")))
			CPPUNIT_FAIL("Please set TEST_HOST_NAME environment variable");
	}

	CPPUNIT_ASSERT(!strHostName.IsEmpty());
	CPPUNIT_ASSERT(strHostName.GetLength() > 2);
	CPPUNIT_ASSERT(strHostName.GetLength() < 255);
	
	return strHostName;
}

/**
 * Get the user name of the SSH account to connect to on the remote machine.
 *
 * The user name is retrieved from the TEST_USER_NAME environment variable.
 * If this variable is not set, a CPPUNIT exception is thrown.
 * 
 * In order to be useful, the user name should correspond to a valid SSH
 * account on the testing machine.
 * 
 * @pre the user name should have at least 3 characters
 * @pre the user name should have less than 64 characters
 *
 * @return the user name
 */
CString CPuttyWrapper_test::GetUserName() const
{
	static CString strUser;

	if (strUser.IsEmpty())
	{
		if(!strUser.GetEnvironmentVariable(_T("TEST_USER_NAME")))
			CPPUNIT_FAIL("Please set TEST_USER_NAME environment variable");
	}

	CPPUNIT_ASSERT(!strUser.IsEmpty());
	CPPUNIT_ASSERT(strUser.GetLength() > 2);
	CPPUNIT_ASSERT(strUser.GetLength() < 64);
	
	return strUser;
}
