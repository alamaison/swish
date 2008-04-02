/*  Implements a wrapper round the command-line psftp.exe program

    Copyright (C) 2007  Alexander Lamaison <awl03@doc.ic.ac.uk>

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
#include "PuttyWrapper.h"

/**
 * Constructor for wrapper around PuTTY SFTP executable (psftp.exe).
 *
 * Launches psftp.exe and monitors its execution via a background thread.
 * The standard input, output and error handles are redirected internally
 * so that this class's member functions can be used to communicate with
 * pstfp.exe.
 *
 * @param pszPsftpPath The absolute path to the psftp.exe executable.
 */
CPuttyWrapper::CPuttyWrapper( PCTSTR pszPsftpPath ) :
	m_strPsftpPath(pszPsftpPath),
	m_hToChildRead(NULL),
	m_hToChildWrite(NULL),
	m_hFromChildRead(NULL),
	m_hFromChildWrite(NULL),
	m_hChildProcess(NULL),
	m_hChildMonitorThread(NULL),
	m_hChildExitEvent(NULL),
	m_fRunThread(FALSE)
{
	SECURITY_ATTRIBUTES sa;
	::ZeroMemory( &sa, sizeof(sa) );

	sa.nLength = sizeof(sa); 
	sa.bInheritHandle = TRUE; 
	sa.lpSecurityDescriptor = NULL; 

	// Create STDIN/STDOUT pipes for child process

	// Create a pipe to send information to child's STDIN between
	// m_hToChildWrite (this end) and m_hToChildRead (child's end).
	// Ensure that this end the pipe is not inherited.
	REPORT(::CreatePipe(&m_hToChildRead, &m_hToChildWrite, &sa, 0));
	REPORT(::SetHandleInformation(m_hToChildWrite, HANDLE_FLAG_INHERIT, 0));

	// Likewise for STDOUT
	REPORT(::CreatePipe(&m_hFromChildRead, &m_hFromChildWrite, &sa, 0));
	REPORT(::SetHandleInformation(m_hFromChildRead, HANDLE_FLAG_INHERIT, 0));

	// Start up the child process with redirected handles
	m_hChildProcess = LaunchChildProcess(
		GetChildPath(),      // Application name
		NULL,                // Command line
		m_hFromChildWrite,   // stdOut
		m_hToChildRead,      // stdIn
		m_hFromChildWrite,   // stdErr
		FALSE                // Show window?
	);
	if (!m_hChildProcess) throw ChildLaunchException( ::GetLastError() );

	// Create exit event
	// Triggers when child forcibly killed by call to TerminateChildProcess()
	m_hChildExitEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
	if (!m_hChildExitEvent) throw ChildLaunchException( ::GetLastError() );

	// Launch the thread that monitors the child process.
	DWORD dwThreadId;
	m_fRunThread = TRUE;
	m_hChildMonitorThread = ::CreateThread(
		NULL,                     // default security attributes
		0,                        // use default stack size  
		staticChildMonitorThread, // thread function 
		this,                     // argument to thread function 
		0,                        // use default creation flags 
		&dwThreadId               // returns the thread identifier 
	);
	if (!m_hChildMonitorThread) throw ChildLaunchException( ::GetLastError() );
}

CPuttyWrapper::~CPuttyWrapper(void)
{
	TerminateChildProcess();
}

/**
 * Thread to monitor the lifecycle of the child process.
 */
DWORD CPuttyWrapper::ChildMonitorThread()
{
	// Set up events to wait upon
	HANDLE hWaitHandle[2];
	hWaitHandle[0] = m_hChildExitEvent; // Forcible exit
	hWaitHandle[1] = m_hChildProcess;   // Child terminates naturally

	while (m_fRunThread)
	{
		// Wait for signal on handles but also pump window messages. This
		// allows this object to work correctly when part of a COM component.
		/*
		Adapted from http://msdn2.microsoft.com/en-us/library/ms809971.aspx:
		
		Rather than using thread synchronization objects the marshaling 
		process for STAs translates the call into a WM_USER message and 
		posts the result to a hidden top-level window associated with
		the apartment when it was created. 
		
		If the apartment's thread is suspended without specifying that 
		it wants to be reawakened for windows messages, such as with a 
		call to WaitForObject, the apartment's message queue will not 
		be pumped and access to the apartment will be blocked. 
		
		To ensure that your object does not inadvertently block itself 
		or other objects in the apartment, always use MsgWaitForObject 
		or MsgWaitForMultipleObjects when suspending thread execution.
		*/

		switch (::MsgWaitForMultipleObjects(2, hWaitHandle, 
			FALSE, INFINITE, QS_POSTMESSAGE))
		{
			case WAIT_OBJECT_0 + 0:	// Forcible exit
				ASSERT(!m_fRunThread);
				break;

			case WAIT_OBJECT_0 + 1:	// Child terminated
				ASSERT(m_fRunThread);
				m_fRunThread = FALSE;
				break;
			case WAIT_OBJECT_0 + 2: // Pump window messages
				MSG msg ;
				while(::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
					::DispatchMessage(&msg);
				break;
		}
	}
	// Virtual function to notify derived class that
	// child process is terminated.
	// Application must call TerminateChildProcess()
	// but not direcly from this thread!
	//OnChildTerminate();
	return 0;
}

/**
 * Start the child process with redirected input, output and error handles.
 *
 * The child process to be launched is specified by application name and
 * command line (@see MSDN documentation on CreateProcess for more information).
 * the handles to which to redirect the child's standard I/O are passed as
 * parameters to the function.
 *
 * @param pszApplicationName The path of the executable module, either
 *                           absolute or relative to the current 
 *                           directory/drive.  @b Optional.  Note: this is
 *                           for the executable alone; command-line arguments
 *                           cannot be passed in this parameter.
 * @param pszCommandLine     The command-line arguments to the executable or,
 *                           in the absence of an executable specified in
 *                           @a pszApplicationName, the executable's path
 *                           followed by the command-line arguments.
 * @param hStdOut            The handle to redirect the child's @c stdout to.
 * @param hStdIn             The handle to redirect the child's @c stdin to.
 * @param hStdErr            The handle to redirect the child's @c stderr to.
 * @param bShowChildWindow   Whether the console window of the child process
 *                           should be displayed.
 * @returns The handle of the child process if successful, otherwise NULL.
 */
HANDLE CPuttyWrapper::LaunchChildProcess( 
	PCTSTR pszApplicationName, PTSTR pszCommandLine,
    HANDLE hStdOut, HANDLE hStdIn, HANDLE hStdErr, BOOL bShowChildWindow)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	::ZeroMemory( &si, sizeof(si) );
	::ZeroMemory( &pi, sizeof(pi) );

	si.cb = sizeof(si);
	si.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	si.wShowWindow = bShowChildWindow ? SW_SHOW: SW_HIDE;
	// dwFlags must include STARTF_USESHOWWINDOW for wShowWindow 
	// to have an effect

	// Set up read end of STDIN and write end of STOUT to be used by child
	si.hStdInput =  hStdIn;
	si.hStdOutput = hStdOut;
	si.hStdError =  hStdErr;

	// Start the child process
	BOOL fResult = ::CreateProcess(
		pszApplicationName, // Application name
		pszCommandLine,     // Command line
		NULL,               // Process handle not inheritable
		NULL,               // Thread handle not inheritable
		TRUE,               // Set handle inheritance to TRUE
		CREATE_NEW_CONSOLE, // Creation flags
		NULL,               // Use parent's environment block
		NULL,               // Use parent's starting directory 
		&si,                // Pointer to STARTUPINFO structure
		&pi                 // Pointer to PROCESS_INFORMATION structure
	);

	if (!fResult) return NULL;

	REPORT(::CloseHandle(pi.hThread)); // Close unnecessary handles

	return pi.hProcess;
}

/**
 * Forcibly kill the child process.
 *
 * The child can be killed either by calling this directly or by destroying
 * the CPuttyWrapper instance whose destructor will call this function.
 *
 * @see ~CPuttyWrapper()
 */
void CPuttyWrapper::TerminateChildProcess()
{
	// Tell the threads to exit and wait for process thread to die.
	m_fRunThread = FALSE;
	REPORT(::SetEvent(m_hChildExitEvent));
	Sleep(500);

	// Check the process thread.
	if (m_hChildMonitorThread)
	{
		VERIFY(
			::WaitForSingleObject(m_hChildMonitorThread, 1000) != WAIT_TIMEOUT
		);
		m_hChildMonitorThread = NULL;
	}

	// Close all child handles first.
	if (m_hFromChildWrite)
		VERIFY(::CloseHandle(m_hFromChildWrite));
	m_hFromChildWrite = NULL;
	if (m_hToChildRead)
		VERIFY(::CloseHandle(m_hToChildRead));
	m_hToChildRead = NULL;
    Sleep(100);

	// Close all parent handles.
	if (m_hFromChildRead)
		VERIFY(::CloseHandle(m_hFromChildRead));
	m_hFromChildRead = NULL;
	if (m_hToChildWrite)
		VERIFY(::CloseHandle(m_hToChildWrite));
	m_hToChildWrite = NULL;
    Sleep(100);

	// Stop the child process if not already stopped.
	if (IsChildRunning())
    {
		VERIFY(::TerminateProcess(m_hChildProcess, 1));
		VERIFY(::WaitForSingleObject(m_hChildProcess, 1000) != WAIT_TIMEOUT);
    }
	m_hChildProcess = NULL;

	// Clean up the exit event
	if (m_hChildExitEvent != NULL)
		VERIFY(::CloseHandle(m_hChildExitEvent));
	m_hChildExitEvent = NULL;
}

/**
 * Check if the child process is running. 
 *
 * On NT/2000 the handle must have PROCESS_QUERY_INFORMATION access.
 */
BOOL CPuttyWrapper::IsChildRunning() const
{
	DWORD dwExitCode;
    if (m_hChildProcess == NULL) return FALSE;
	::GetExitCodeProcess(m_hChildProcess, &dwExitCode);
	return (dwExitCode == STILL_ACTIVE) ? TRUE: FALSE;
}

/**
 * Retrieves a listing for the given path on the remote system using 'ls'
 *
 * @param szPath [in] the path to be listed
 *
 * @returns the list of files as a list of strings, once per file:
 *          e.g. drwxr-xr-x   13 root     root         4096 Nov 22  2006 usr
 *
 * @pre szPath points to a string and the string is not empty
 *
 * @throws TODO
 */
list<CString> CPuttyWrapper::RunLS( __in PCTSTR szPath )
{
	ATLASSERT(szPath); ATLASSERT(szPath[0]);

	CString strCommand = _T("ls ");
	strCommand += szPath;
	strCommand += _T("\r\n");
	Write( strCommand );
	
	CString strRawListing = Read();

	list<CString> lstResults;
	int iPos = 0;
	CString strToken;
	do
	{
		strToken = strRawListing.Tokenize(_T("\r\n"), iPos);
		lstResults.push_back( strToken );
	} while (strToken != _T("psftp> "));
	// TODO: This might break if a files is named "psftp> "

	// First line is not a file listing; it specifies the directory. Remove it.
	ATLASSERT( lstResults.front().Left(18) == _T("Listing directory ") );
	lstResults.erase( lstResults.begin() );

	// Last line is prompt again. Remove it.
	ATLASSERT( lstResults.back() == _T("psftp> ") );
	lstResults.pop_back();

	return lstResults;
}

/**
 * Reads from the child process's stdout until it is empty.
 *
 * @returns a string containing all the text read
 *
 * @throws CharacterConversionException if an error occurs when converting
 *         the characters from the child process's character set to a CString
 * @throws ChildTerminatedException if the read fails due to a broken pipe
 * @throws ChildCommunicationException if other error
 */
CString CPuttyWrapper::Read()
{
	CString strBuffer;
	while (!GetSizeOfDataInPipe()) {}; // TODO: replace this. busy wait?
	while (GetSizeOfDataInPipe())
	{
		strBuffer += ReadOneBufferWorth();
	}

	return strBuffer;
}

/**
 * Writes the null-terminated string in the buffer to the child process's stdin.
 *
 * @pre the string in the buffer must be NULL-terminated or the function
 *      may overrun the buffer or never return
 *
 * @param ptszIn the NULL-terminated buffer of characters to be written
 *
 * @returns the number of *bytes* written to the child process
 *
 * @throws ChildTerminatedException if the write fails due to a broken pipe
 * @throws CharacterConversionException if an error occurs when converting
 *         the characters to the character set expected by the child process
 * @throws ChildCommunicationException if other error
 */
ULONG CPuttyWrapper::Write( PCTSTR ptszIn )
{
	ATLASSERT( ptszIn );

	// Assumes null-terminated input string (ptszIn)
	return Write( ptszIn, (ULONG)::_tcslen(ptszIn) );
}

/**
 * Writes the characters in the buffer to the child process's stdin.
 *
 * As the size of the buffer is explicitly given, the characters do not
 * need to be NULL-terminated.
 *
 * @param ptszIn the buffer of characters to be written
 * @param cchIn  the length of the buffer in *characters*
 *
 * @returns the number of *bytes* written to the child process
 *
 * @throws ChildTerminatedException if the write fails due to a broken pipe
 * @throws CharacterConversionException if an error occurs when converting
 *         the characters to the character set expected by the child process
 * @throws ChildCommunicationException if other error
 */
ULONG CPuttyWrapper::Write( PCTSTR ptszIn, ULONG cchIn )
{
	ATLASSERT(ptszIn); ATLASSERT(GetStdout());

	const ULONG cOemBufSize = 2*cchIn; // Double to be sure we have enough space

	// Child process expects text in the OEM codepage character set.
	// We need to convert the input string from Unicode or ANSI depending on the
	// build settings.
	char *pBuffer = new char[ cOemBufSize ]; 
		ULONG cbConvertedLength = 
			ConvertToOemChars( ptszIn, cchIn, pBuffer, cOemBufSize );
		ULONG cBytesWritten = 
			WriteOemCharsToConsole( pBuffer, cbConvertedLength );
	delete [] pBuffer;

	ATLASSERT( cbConvertedLength == cBytesWritten );

	return cBytesWritten;
}

/**
 * Returns the size of the data in bytes currently available in the stdin pipe.
 *
 * @returns Size of available data in BYTES
 * @throws ChildTerminatedException if the pipe from the child process is broken
 */
DWORD CPuttyWrapper::GetSizeOfDataInPipe()
{
	ATLASSERT(GetStdin());
	DWORD nTotalBytesAvailable = 0;

	if(!::PeekNamedPipe(
		GetStdin(),            // Handle to pipe
		NULL,                  // No buffer, not fetching bytes
		NULL,                  // Not needed, not fetching bytes
		NULL,                  // Not needed, not fetching bytes
		&nTotalBytesAvailable, // This is what we are after
		NULL                   // Not needed, not fetching bytes
	))
	{
		throw ChildTerminatedException();
	}

	return nTotalBytesAvailable;
}

/**
 * Read a chunk of text from child process, enough to fill an internal buffer.
 *
 * The buffer is an internal CHAR array.  As this is not an appropriate 
 * format for further use, it is converted into a CString.
 *
 * @returns a chunk of text of arbitrary size read from the child process
 * 
 * @throws CharacterConversionException if an error occurs when converting
 *         to a CString
 * @throws ChildTerminatedException if the read fails due to a broken pipe
 * @throws ChildCommunicationException if other error
 */
CString CPuttyWrapper::ReadOneBufferWorth()
{
#ifdef _DEBUG
	const ULONG SOURCE_BUFFER_SIZE = 5; // Small buffer catches more bugs
#else
	const ULONG SOURCE_BUFFER_SIZE = 1024;
#endif
	char aBuffer[ SOURCE_BUFFER_SIZE ]; // Holds raw input from console

	ULONG cBytesRead = ReadOemCharsFromConsole(aBuffer, ARRAYSIZE(aBuffer));
	if (cBytesRead == 0) return _T(""); // Nothing to read
	ATLASSERT( cBytesRead <= SOURCE_BUFFER_SIZE );

	// Child process returns text in OEM codepage character set.
	// We need to convert this to Unicode or ANSI depending on the
	// build settings.
	return ConvertFromOemChars( aBuffer, ARRAYSIZE(aBuffer) );
}

/**
 * Writes the OEM format characters in the buffer to the child process's input.
 *
 * @param pBuffer  the buffer holding the characters to be written
 * @param cbSize   the size of the buffer in bytes
 *
 * @return the number of characters written to the child process (should
 *         be the same as the size of the buffer)
 *
 * @pre the characters in the buffer are in the OEM codepage character set
 *
 * @throws ChildTerminatedException if the write fails due to a broken pipe
 * @throws ChildCommunicationException if other error
 */
ULONG CPuttyWrapper::WriteOemCharsToConsole( const char* pBuffer, ULONG cbSize )
{
	ATLASSERT(pBuffer);
	ULONG cBytesWritten;
	if (!::WriteFile(
		GetStdout(),            // Handle to file
		pBuffer,                // LPVOID pointer to source buffer
		cbSize,                 // Number of bytes to write
		&cBytesWritten,         // [out] Actual number of bytes written
		NULL))
	{
		if (GetLastError() == ERROR_BROKEN_PIPE)
			throw ChildTerminatedException();
		else
			throw ChildCommunicationException();
	}

	ATLASSERT(cBytesWritten == cbSize);

	return cBytesWritten;
}

/**
 * Reads as many characters as possible from the child's output into the buffer.
 *
 * The child is a console process, therefore the characters are in the
 * OEM codepage format.  This function may read few characters than the 
 * size of the buffer if:
 *   - A write operation completes on the write end of the pipe or
 *   - An error occurs
 * Any space left at the end of the buffer is zeroed out.
 *
 * @param pBuffer       [out] pointer to the target buffer
 * @param cbBufferSize  [in]  size of the target buffer in bytes/characters
 *
 * @return The actual number of bytes/character read into the buffer
 *
 * @throws ChildTerminatedException if the read fails due to a broken pipe
 * @throws ChildCommunicationException if other error
 */
ULONG CPuttyWrapper::ReadOemCharsFromConsole(char *pBuffer, ULONG cbBufferSize)
{
	ATLASSERT(pBuffer); ATLASSERT(GetStdin());
	if (cbBufferSize == 0) return 0;

	::ZeroMemory( pBuffer, cbBufferSize );

	ULONG cBytesRead;
	if (!::ReadFile(
		GetStdin(),             // Handle to pipe
		pBuffer,                // LPVOID pointer to target buffer
		cbBufferSize,           // Max number of bytes to be read
		&cBytesRead,            // [out] Actual number of bytes read
		NULL))
	{
		if (GetLastError() == ERROR_BROKEN_PIPE)
			throw ChildTerminatedException();
		else
			throw ChildCommunicationException();
	}

	ATLASSERT( cBytesRead <= cbBufferSize );
	return cBytesRead;
}

/**
 * Converts a buffer of OEM code page characters into a CString.
 *
 * Console processes use text stored as OEM codepage characters.  To be used
 * correctly in the rest of the program this should be converted to another
 * form as soon as possible.  We have chose to use CStrings.
 *
 * The buffer does not need to be null-terminated as all characters up 
 * to the given size are converted.  See preconditions for case where is
 * is null-terminated.
 *
 * @param pBuffer       [in] pointer to the buffer holding the OEM characters
 * @param cbBufferSize  [in] number of characters/bytes in the buffer
 *
 * @return The result of converting the buffer into a CString
 *
 * @pre the characters in the buffer are in the OEM codepage character set
 * @pre if the input buffer is null-terminated, the NULL character must
 *      be the last character of the buffer, i.e. cbBufferSize
 *
 * @throws CharacterConversionException if an error occurs in the conversion
 *         process
 */
CString CPuttyWrapper::ConvertFromOemChars( char *pBuffer, ULONG cbBufferSize )
{
	ATLASSERT(pBuffer);
	if (cbBufferSize == 0) return _T("");

	CString strOut;                     // Holds converted text result

#ifdef _UNICODE
	// We run the conversion function twice: once with the last parameter
	// set to 0 to retrieve the required length of the target buffer in
	// WCHAR units and a second time to perform the actual conversion.
	// This allows us to be sure we have allocated a buffer with enough
	// space.
	ULONG cchTargetLen = 
		::MultiByteToWideChar(CP_OEMCP, 0, pBuffer, cbBufferSize, NULL, 0);
	if (cchTargetLen == 0)
		throw CharacterConversionException();

	// Allocate a buffer in the CString of the required size, convert the
	// OEM characters into it and release the buffer
	WCHAR *pConvertedBuffer = strOut.GetBuffer( cchTargetLen );
	ULONG cchActualTargetLen = ::MultiByteToWideChar(
		CP_OEMCP,         // OEM codepage used by console processes
		0,                // No flags
		pBuffer,          // Source (CHAR) buffer
		cbBufferSize,     // Source not NULL-terminated, need exact string size
		pConvertedBuffer, // Target (WCHAR) buffer
		cchTargetLen      // Target buffer size
	);
	ATLASSERT( cchActualTargetLen == cchTargetLen );

	// Release the CString buffer.  The input characters may have been
	// null terminated.  If they were, we must let the
	// CString calculate its own length. If not, we assign it a length.
	if (pBuffer[cbBufferSize-1] == NULL)
		strOut.ReleaseBuffer();
	else
		strOut.ReleaseBuffer( cchActualTargetLen );
	ATLASSERT( (int)::_tcslen(strOut) == strOut.GetLength() );
#else // ANSI: Not tested!
	CHAR *pConvertedBuffer = strOut.GetBuffer( cbBufferSize );
	fSuccess = OemToCharBuff( pBuffer, pConvertedBuffer, cbBufferSize );
	if (pBuffer[cbBufferSize-1] == NULL)
		strOut.ReleaseBuffer();
	else
		strOut.ReleaseBuffer( cbBufferSize );
#endif

	if (cchActualTargetLen == 0)
		throw CharacterConversionException();

	return strOut;
}

/**
 * Converts a buffer of TCHARS into OEM code page characters.
 *
 * Console processes use text stored as OEM codepage characters.  The 
 * rest of the program should not use this formate.  This helper function
 * converts them appropriately.
 *
 * The buffers do not need to be null-terminated as all characters up 
 * to the given sizes are converted.
 *
 * @param pBufferIn  [in]  pointer to the buffer holding the TCHAR characters
 * @param cchInSize  [in]  number of *characters* (not bytes) in the buffer
 * @param pBufferOut [out] pointer to the buffer to hold the result
 * @param cbOutSize  [in]  size of the result buffer in *bytes*
 *
 * @return the number of *bytes* written to the result buffer (likely smaller
 *         than the size of the buffer itself)
 *
 * @throws CharacterConversionException if an error occurs in the conversion
 *         process
 */
ULONG CPuttyWrapper::ConvertToOemChars( const TCHAR *pBufferIn, ULONG cchInSize,
                                        char *pBufferOut, ULONG cbOutSize )
{
	ATLASSERT(pBufferIn); ATLASSERT(pBufferOut);
	if (cchInSize == 0) return 0;

	ULONG cBytesConverted;

#ifdef _UNICODE
#ifdef _DEBUG
	// Check that result of conversion will be able to fit in buffer
	ULONG cbResultLen = ::WideCharToMultiByte(CP_OEMCP, 0, pBufferIn, cchInSize,
		                                      NULL, 0, NULL, NULL);
	if (cbOutSize < cbResultLen) throw InsufficientBufferException();
#endif
	cBytesConverted = ::WideCharToMultiByte(
		CP_OEMCP,   // OEM codepage for console processes
		0,          // No flags
		pBufferIn,  // Source (WCHAR) buffer
		cchInSize,  // Source not NULL-terminated, need exact string size
		pBufferOut, // Target (CHAR) buffer
		cbOutSize,  // Target buffer size
		NULL,       // Default character
		NULL        // Was default character used?
	);
#else // ANSI: Not tested!
	cBytesConverted = CharToOemBuf(pBufferIn, pBufferOut, cchInSize);
#endif

	if (cBytesConverted == 0) throw CharacterConversionException();
	return cBytesConverted;
}

/**
 * Returns the path to the psftp.exe executable.
 */
PCTSTR CPuttyWrapper::GetChildPath()
{
	return m_strPsftpPath;
}

// CPuttyWrapper