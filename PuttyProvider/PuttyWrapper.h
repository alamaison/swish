/*  Interface of command-line wrapper for Putty SFTP client (psftp.exe) 

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

#ifndef PUTTYWRAPPER_H
#define PUTTYWRAPPER_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <list>
using std::list;

#ifdef _DEBUG
#define WITH_LOGGING
#endif

/**
 * Wrapper around @c psftp.exe.
 */
class CPuttyWrapper
{
public:
	CPuttyWrapper( PCTSTR pszPsftpPath );
	~CPuttyWrapper();

	CString ReadLine();
	CString Read();
	ULONG Write( PCTSTR pszIn );
	ULONG Write( PCTSTR pszIn, ULONG cchIn );
	list<CString> RunLS( __in PCTSTR strPath );

	/** @name Exception types */
	// @{
	struct ChildLaunchException
	{
		ChildLaunchException(DWORD dwLastError) { m_dwLastError = dwLastError; }
		CString GetErrorMessage()
		{
			PTSTR lpMsgBuf;
			::FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(PTSTR)&lpMsgBuf, 0, NULL
			);
			CString strReturn = lpMsgBuf;
			::LocalFree(lpMsgBuf);
			return strReturn;
		}
		DWORD m_dwLastError;
	};
	struct ChildCommunicationException {};
	struct ChildTerminatedException : ChildCommunicationException {};
	struct CharacterConversionException : ChildCommunicationException {};
	struct InsufficientBufferException : CharacterConversionException {};
	// @}

private:

	typedef enum tagLogDirection
	{
		READ,
		WRITE
	} LogDirection;
	void _LogSetup();
	void _LogStart(LogDirection enumDirection);
	void _LogStop(LogDirection enumDirection);
	void _LogEntry( __in_bcount(cbSize) const char* pBuffer, ULONG cbSize );
	void _WriteToLogFile( PCSTR strText );

	PCTSTR GetChildPath();

	HANDLE GetStdin() { return m_hFromChildRead; }
	HANDLE GetStdout() { return m_hToChildWrite; }

	CString ReadOneBufferWorth();
	CString ReadOneBufferWorth( ULONG cbBufferSize );
	DWORD GetSizeOfDataInPipe();
	long FindNewlineInPipe();

	ULONG ReadOemCharsFromConsole(
		__out_bcount(cbSize) char *pBuffer, ULONG cbSize
	);
	ULONG WriteOemCharsToConsole(
		__in_bcount(cbSize) const char *pBuffer, ULONG cbSize
	);

	ULONG ConvertToOemChars(
		__in_bcount(cbInSize) const TCHAR *pBufferIn, ULONG cbInSize,
		__out_bcount(cbOutSize) char *pBufferOut, ULONG cbOutSize
	);
	CString ConvertFromOemChars(
		__in_bcount(cbBufferSize) char *pBuffer, ULONG cbBufferSize
	);

	/** @name ChildLifecycle	Child lifecycle management */
	// @{
	HANDLE LaunchChildProcess(
		__in_opt PCTSTR pszApplicationName,
		__inout_opt PTSTR pszCommandLine,
		__in HANDLE stdOut, 
		__in HANDLE stdIn, 
		__in HANDLE stdErr, 
		__in BOOL bShowChildWindow
	);
	void TerminateChildProcess();
	BOOL IsChildRunning() const;

	/**
	 * Static handler for ChildMonitorThread() instance function.
	 *
	 * Dispatches thread workload to correct instance's child monitor method.
	 * @param lpThreadParam		pointer to CPuttyWrapper instance
	 */
	static DWORD WINAPI staticChildMonitorThread( __in LPVOID lpThreadParam )
		{ return ((CPuttyWrapper *)lpThreadParam)->ChildMonitorThread(); }

	/* Thread(s) handling child in the background */
	DWORD ChildMonitorThread();
	// @}

	const CString m_strPsftpPath; ///< Stores the path of psftp.exe
	BOOL m_fRunThread;            ///< Flag controlling thread lifetimes
	/*< The thread(s) can be started and remain running as long as this 
	    is set.  When this is unset (FALSE) the thread(s) will start 
	    committing suicide */

	/** @name Handles */
	// @{
	HANDLE m_hToChildRead;
	HANDLE m_hToChildWrite;
	HANDLE m_hFromChildRead;
	HANDLE m_hFromChildWrite;

	HANDLE m_hChildProcess;       ///< Handle to the child process
	HANDLE m_hChildMonitorThread; ///< Monitors the child process's lifecycle

	HANDLE m_hChildExitEvent;     ///< Unnatural exit e.g. kill child requested

#ifdef WITH_LOGGING
	HANDLE m_hLog;                ///< Handle to the debug log
#endif
	// @}
};

#endif // PUTTYWRAPPER_H
