/*  IStream interface around the libssh2 SFTP file access functions.

    Copyright (C) 2008  Alexander Lamaison <awl03@doc.ic.ac.uk>

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

    In addition, as a special exception, the the copyright holders give you
    permission to combine this program with free software programs or the 
    OpenSSL project's "OpenSSL" library (or with modified versions of it, 
    with unchanged license). You may copy and distribute such a system 
    following the terms of the GNU GPL for this program and the licenses 
    of the other code concerned. The GNU General Public License gives 
    permission to release a modified version without this exception; this 
    exception also makes it possible to release a modified version which 
    carries forward this exception.
*/

#pragma once

#include "resource.h"       // main symbols
#include "stdafx.h"

#include "Session.h"

#include <string>
using std::string;




/**
 * Maps between libssh2 SFTP error codes and an appropriate error string.
 *
 * @param uError  SFTP error code as returned by libssh2_sftp_last_error().
 */
static CString _GetSftpErrorMessage(ULONG uError)
{
	switch (uError)
	{
	case LIBSSH2_FX_OK:
		return _T("Successful");
	case LIBSSH2_FX_EOF:
		return _T("File ended unexpectedly");
	case LIBSSH2_FX_NO_SUCH_FILE:
		return _T("Required file or folder does not exist");
	case LIBSSH2_FX_PERMISSION_DENIED:
		return _T("Permission denied");
	case LIBSSH2_FX_FAILURE:
		return _T("Unknown failure");
	case LIBSSH2_FX_BAD_MESSAGE:
		return _T("Server returned an invalid message");
	case LIBSSH2_FX_NO_CONNECTION:
		return _T("No connection");
	case LIBSSH2_FX_CONNECTION_LOST:
		return _T("Connection lost");
	case LIBSSH2_FX_OP_UNSUPPORTED:
		return _T("Server does not support this operation");
	case LIBSSH2_FX_INVALID_HANDLE:
		return _T("Invalid handle");
	case LIBSSH2_FX_NO_SUCH_PATH:
		return _T("The path does not exist");
	case LIBSSH2_FX_FILE_ALREADY_EXISTS:
		return _T("A file or folder of that name already exists");
	case LIBSSH2_FX_WRITE_PROTECT:
		return _T("This file or folder has been write-protected");
	case LIBSSH2_FX_NO_MEDIA:
		return _T("No media was found");
	case LIBSSH2_FX_NO_SPACE_ON_FILESYSTEM:
		return _T("There is no space left on the server's filesystem");
	case LIBSSH2_FX_QUOTA_EXCEEDED:
		return _T("You have exceeded your disk quota on the server");
	case LIBSSH2_FX_UNKNOWN_PRINCIPLE:
		return _T("Unknown principle");
	case LIBSSH2_FX_LOCK_CONFlICT:
		return _T("Lock conflict");
	case LIBSSH2_FX_DIR_NOT_EMPTY:
		return _T("The folder is not empty");
	case LIBSSH2_FX_NOT_A_DIRECTORY:
		return _T("This file is not a folder");
	case LIBSSH2_FX_INVALID_FILENAME:
		return _T("The filename is not valid on the server's filesystem");
	case LIBSSH2_FX_LINK_LOOP:
		return _T("Operation would cause a link loop which is not permitted");
	default:
		return _T("Unexpected error code returned by server");
	}
}


// CSftpStream
[
	coclass,
	default(IUnknown),
	threading(apartment),
	vi_progid("Libssh2Provider.SftpStream"),
	progid("Libssh2Provider.SftpStream.1"),
	version(1.0),
	uuid("2D61E174-26A0-4B3C-8BD2-2E8194A809EA"),
	helpstring("SftpStream Class")
]
class ATL_NO_VTABLE CSftpStream :
	public IStream
{
public:
	/**
	 * Static factory method.
	 *
	 * @returns A smart pointer to the CSftpStream COM object.
	 * @throws  A CAtlException if creation fails.
	 */
	static CComPtr<CSftpStream> Create() throw(...)
	{
		CComObject<CSftpStream> *pObject = NULL;
		HRESULT hr = CComObject<CSftpStream>::CreateInstance(&pObject);
		ATLENSURE_SUCCEEDED(hr);

		pObject->AddRef();
		return pObject;
	}

	CSftpStream();
	~CSftpStream();

	HRESULT Initialize(CSession& session, PCSTR pszFilePath);

	// ISequentialStream methods
	IFACEMETHODIMP Read( 
		__out_bcount_part(cb, pcbRead) void *pv,
		__in ULONG cb,
		__out ULONG *pcbRead
	);

	IFACEMETHODIMP Write( 
		__in_bcount(cb) const void *pv,
		__in ULONG cb,
		__out_opt ULONG *pcbWritten
	);

	// IStream methods
	IFACEMETHODIMP Seek(
		__in LARGE_INTEGER dlibMove,
		__in DWORD dwOrigin,
		__out ULARGE_INTEGER *plibNewPosition
	);

	IFACEMETHODIMP SetSize( 
		__in ULARGE_INTEGER libNewSize
	);

	IFACEMETHODIMP CopyTo( 
		__in IStream *pstm,
		__in ULARGE_INTEGER cb,
		__out ULARGE_INTEGER *pcbRead,
		__out ULARGE_INTEGER *pcbWritten
	);

	IFACEMETHODIMP Commit( 
		__in DWORD grfCommitFlags
	);

	IFACEMETHODIMP Revert();

	IFACEMETHODIMP LockRegion( 
		__in ULARGE_INTEGER libOffset,
		__in ULARGE_INTEGER cb,
		__in DWORD dwLockType
	);

	IFACEMETHODIMP UnlockRegion( 
		__in ULARGE_INTEGER libOffset,
		__in ULARGE_INTEGER cb,
		__in DWORD dwLockType
	);

	IFACEMETHODIMP Stat( 
		__out STATSTG *pstatstg,
		__in DWORD grfStatFlag
	);

	IFACEMETHODIMP Clone( 
		__deref_out_opt IStream **ppstm
	);

private:
	ULONGLONG _CalculateNewFilePosition(
		__in LONGLONG nMove, __in DWORD dwOrigin) throw(...);
	void _Read(
		__out_bcount_part(cb, pcbRead) char *pbuf, __in ULONG cb, 
		__out ULONG *pcbRead) throw(...);
	ULONG _ReadOne(__out_bcount(cb) char *pbuf, ULONG cb) throw(...);
	void _CopyTo(
		__in IStream *pstm, __in ULONGLONG cb, 
		__out ULONGLONG *pcbRead, __out ULONGLONG *pcbWritten) throw(...);
	void _CopyOne(
		__in IStream *pstm, __in ULONG cb,
		__out ULONG *pcbRead, __out ULONG *pcbWritten) throw(...);
	ULONGLONG _Seek(__in LONGLONG nMove, __in DWORD dwOrigin) throw(...);
	STATSTG _Stat(__in bool bWantName) throw(...);

	/**
	 * Retrieves a string description of the last error reported by libssh2.
	 *
	 * In the case that the last SSH error is an SFTP error it returns the SFTP
	 * error message in preference.
	 */
	CString CSftpStream::_GetLastErrorMessage()
	{
		int nErr; PSTR pszErr; int cchErr;

		nErr = libssh2_session_last_error(m_pSession, &pszErr, &cchErr, false);
		if (nErr == LIBSSH2_ERROR_SFTP_PROTOCOL)
		{
			ULONG uErr = libssh2_sftp_last_error(m_pSftp);
			return _GetSftpErrorMessage(uErr);
		}
		else // A non-SFTP error occurred
			return CString(pszErr);
	}

	LIBSSH2_SFTP_HANDLE *m_pHandle;
	LIBSSH2_SESSION *m_pSession;
	LIBSSH2_SFTP *m_pSftp;
	string m_strFilename;
	string m_strDirectory;
};