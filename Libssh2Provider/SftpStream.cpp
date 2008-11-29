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

#include "stdafx.h"
#include "SftpStream.h"

/**
 * Construct a new CSftpStream instance with NULL filehandle.
 */
CSftpStream::CSftpStream()
	: m_pHandle(NULL), m_pSftp(NULL), m_pSession(NULL)
{
}

/**
 * Close handle to file and destroy CStfpStream instance.
 */
CSftpStream::~CSftpStream()
{
	if (m_pHandle)
	{
		ATLVERIFY(libssh2_sftp_close(m_pHandle) == 0);
	}
}

/**
 * Initialise the CSftpStream instance with a file path and an SFTP session.
 *
 * The file is opened using SFTP and the CSftpStream provides access to it
 * via the IStream interface.
 */
HRESULT CSftpStream::Initialize(CSession& session, PCSTR pszFilePath)
{
	m_pSftp = session;
	m_pSession = session;

#pragma warning (push)
#pragma warning (disable: 4267) // size_t to unsigned int
	m_pHandle = libssh2_sftp_open(session, pszFilePath, LIBSSH2_FXF_READ, NULL);
#pragma warning (pop)

	if (m_pHandle == NULL)
	{
		UNREACHABLE;
		TRACE("libssh2_sftp_open(%s) failed: %ws",
			pszFilePath, _GetLastErrorMessage());
		return E_UNEXPECTED;
	}

	string strFilePath(pszFilePath);
	m_strFilename = strFilePath.substr(strFilePath.find_last_of('/')+1);
	m_strDirectory = strFilePath.substr(0, strFilePath.find_last_of('/'));

	return S_OK;
}


/*----------------------------------------------------------------------------*
 * COM Interface Functions
 *----------------------------------------------------------------------------*/

/**
 * Read a given number of bytes from the file into the provided buffer.
 *
 * The bytes are read starting at current seek position of the file this stream
 * was initialised for.
 *
 * If the number of bytes read is less than the number requested, this
 * indicates that the end-of-file has been reached.
 *
 * @implements ISequentialStream
 *
 * @param[out] pv       Buffer to read the bytes into.
 * @param[in]  cb       Number of bytes to read.
 * @param[out] pcbRead  Location in which to return the number of bytes that were
 *                      actually read.  This should be correct even if the call 
 *                      results in a failure.  Optional.
 *
 * @return S_OK if successful or an STG_E_* error code if an error occurs.
 * @retval STG_E_INVALIDPOINTER if pv is NULL.
 */
STDMETHODIMP CSftpStream::Read(void *pv, ULONG cb, ULONG *pcbRead)
{
	ATLENSURE_RETURN_HR(pv, STG_E_INVALIDPOINTER);

	try
	{
		ULONG cbRead = 0;
		if (!pcbRead) // Either return count directly or into local temp
			pcbRead = &cbRead;
		_Read(static_cast<char *>(pv), cb, pcbRead);
	}
	catchCom()

	return S_OK;
}

/**
 * Write a given number of bytes from the provided buffer to the file.
 *
 * @implements ISequentialStream
 *
 * @param[in]  pv          Buffer from which to copy the bytes.
 * @param[in]  cb          Number of bytes to write.
 * @param[out] pcbWritten  Location in which to return the number of bytes
 *                         actually written to the file.  This should be correct 
 *                         even if the call results in a failure.  Optional.
 *
 * @return S_OK if successful or an STG_E_* error code if an error occurs.
 *
 * @warning Not yet implemented.
 */
STDMETHODIMP CSftpStream::Write(
	const void * /*pv*/, ULONG /*cb*/, ULONG * /*pcbWritten*/)
{
	return E_NOTIMPL;
}

/**
 * Copy a given number of bytes from the current IStream another IStream.
 *
 * The bytes are read starting from the current seek position of this stream
 * and are copied into the target stream (pstm) starting at its current
 * seek position.
 *
 * @implements IStream
 *
 * @param[in] pstm         IStream to copy the data into.
 * @param[in] cb           Number of bytes of data to copy.
 * @param[out] pcbRead     Number of bytes that were actually read from this
 *                         stream.  This may differ from cb if the end-of-file
 *                         was reached.  This should be correct even if the 
 *                         call results in a failure.  Optional.
 * @param[out] pcbWritten  Number of byted that were actually written to the
 *                         target stream.  This should be correct even if the 
 *                         call results in a failure.  Optional.
 *
 * @return S_OK if successful or an STG_E_* error code if an error occurs.
 * @retval STG_E_INVALIDPOINTER if pstm is NULL.
 */
STDMETHODIMP CSftpStream::CopyTo(
	IStream *pstm, ULARGE_INTEGER cb, 
	ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
{
	ATLENSURE_RETURN_HR(pstm, STG_E_INVALIDPOINTER);

	try
	{
		_CopyTo(pstm, cb.QuadPart,
			(pcbRead) ? &(pcbRead->QuadPart) : NULL,
			(pcbWritten) ? &(pcbWritten->QuadPart) : NULL);
	}
	catchCom()

	return S_OK;
}

/**
 * Change the location of this stream's seek pointer.
 *
 * The location can be relative to the beginning of the file, to the current
 * position of the seek pointer or to the end of the file depending on the
 * value of dwOrigin.
 *
 * @implements IStream
 *
 * @param[in]  dlibMove         Offset by which to move the seek pointer.
 * @param[in]  dwOrigin         Origin of the seek.  Can be the beginning of 
 *                              the file (STREAM_SEEK_SET), the current seek 
 *                              pointer (STREAM_SEEK_CUR), or the end of the 
 *                              file (STREAM_SEEK_END).
 * @param[out] plibNewPosition  Location in which to return the new position of
 *                              the seek pointer.  Optional.
 *
 * @return S_OK if successful or an STG_E_* error code if an error occurs.
 * @retval STG_E_INVALIDFUNCTION if the operation would move the seek pointer
 *         before the beginning of the file.
 */
STDMETHODIMP CSftpStream::Seek(
	LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
	try
	{
		ULONGLONG uNewPosition = _Seek(dlibMove.QuadPart, dwOrigin);
		if (plibNewPosition)
			plibNewPosition->QuadPart = uNewPosition;
	}
	catchCom()

	return S_OK;
}

/**
 * Retrieve metadata about the stream.
 *
 * The information is returned in a STATSTG structure.  Some of its fields
 * include:
 * - pwcsName:   Name of the file
 * - type:       Type of the object (STGTY_STREAM)
 * - cbSize:     Size of the file
 * - mtime:      File's last modification time
 * - ctime:      File's creation time
 * - atime:      Time the file was last accessed
 * - grfMode:    Access mode specified when the object was opened.
 * This information will NOT include the name of the file if the flag 
 * STATFLAG_NONAME is passed in grfStatFlag.
 *
 * @implements IStream
 *
 * @param[out] pstatstg     A preallocated STATSTG structure to return the
 *                          information in.
 * @param[in]  grfStatFlag  Flag indicating whether or not to include the
 *                          filename in STATSTG information.
 *
 * @return S_OK if successful or an STG_E_* error code if an error occurs.
 * @retval STG_E_INVALIDPOINTER if pstatstg is NULL.
 */
STDMETHODIMP CSftpStream::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
	ATLENSURE_RETURN_HR(pstatstg, STG_E_INVALIDPOINTER);

	try
	{
		*pstatstg = _Stat(!(grfStatFlag & STATFLAG_NONAME));
	}
	catchCom()

	return S_OK;
}

/**
 * Set the size of the file to a given value.
 *
 * If the size is smaller than the current size, the file is truncated.  
 * Otherwise the the intervening space is filled with an undefined value
 *
 * @implements IStream
 *
 * @warning Not yet implemented.
 */
STDMETHODIMP CSftpStream::SetSize(ULARGE_INTEGER /*libNewSize*/)
{
	return E_NOTIMPL;
}

/**
 * Create a new stream with separate seek pointer that references the same file.
 *
 * @implements IStream
 *
 * @warning Not yet implemented.
 */
STDMETHODIMP CSftpStream::Clone(IStream ** /*ppstm*/)
{
	return E_NOTIMPL;
}

/**
 * Flushes changes to the the stream to storage.
 * @implements IStream
 * @warning Not implemented.  Transactions not supported.
 */
STDMETHODIMP CSftpStream::Commit(DWORD /*grfCommitFlags*/)
{
	return E_NOTIMPL;
}

/**
 * Discards all changes that have been made to stream since Commit was called.
 * @implements IStream
 * @warning Not implemented.  Transactions not supported.
 */
STDMETHODIMP CSftpStream::Revert()
{
	return E_NOTIMPL;
}

/**
 * Lock a range of bytes.  Locking is not supported by this stream.
 * @implements IStream
 * @returns STG_E_INVALIDFUNCTION.
 */
STDMETHODIMP CSftpStream::LockRegion(
	ULARGE_INTEGER /*libOffset*/, ULARGE_INTEGER /*cb*/, DWORD /*dwLockType*/)
{
	return STG_E_INVALIDFUNCTION;
}

/**
 * Remove the lock placed on a range of bytes by LockRegion.  Locking is not
 * supported by this stream.
 * @implements IStream
 * @returns STG_E_INVALIDFUNCTION.
 */
STDMETHODIMP CSftpStream::UnlockRegion(
	ULARGE_INTEGER /*libOffset*/, ULARGE_INTEGER /*cb*/, DWORD /*dwLockType*/)
{
	return STG_E_INVALIDFUNCTION;
}

/*----------------------------------------------------------------------------*
 * Private functions
 *----------------------------------------------------------------------------*/

#define THRESHOLD 39990 ///< Buffer size threshold after which calls to
                        ///< libssh2_sftp_read() fail

/**
 * Read cb bytes into buffer pbuf.  Perform the read operation in chunks
 * less than THRESHOLD to avoid libssh2_sftp_read() failure with buffers
 * greater than 39992 bytes.
 *
 * @returns  Number of bytes actually read in out-parameter pcbRead.  This
 *           should contain the correct value even if the call fails (throws 
 *           an exception).
 * @throws   CAtlException with STG_E_* code if an error occurs.
 */
void CSftpStream::_Read(char *pbuf, ULONG cb, ULONG *pcbRead) throw(...)
{
	ATLASSERT(pcbRead);
	*pcbRead = 0;
	char *p = pbuf;

	try 
	{
		ULONG cbChunk = 0;
		ULONG cbRead = 0;
		do {
			cbChunk = min(static_cast<ULONG>(cb + pbuf - p), THRESHOLD);
			cbRead = _ReadOne(p, cbChunk);
			p += cbRead;
		} while (cbRead == cbChunk /* not EOF */ &&
			     p - pbuf < cb /* wants more */);
	}
	catch(...)
	{
		*pcbRead = static_cast<ULONG>(p - pbuf);
		throw;
	}

	*pcbRead = static_cast<ULONG>(p - pbuf);
}

/**
 * Read cb bytes into buffer pbuf.
 *
 * @returns  Number of bytes actually read.
 * @throws   CAtlException with STG_E_* code if an error occurs.
 */
ULONG CSftpStream::_ReadOne(char *pbuf, ULONG cb) throw(...)
{
	ssize_t cbRead = libssh2_sftp_read(m_pHandle, pbuf, cb);

	if (cbRead < 0)
	{
		UNREACHABLE;
		TRACE("libssh2_sftp_read() failed: %ws", _GetLastErrorMessage());
		AtlThrow(STG_E_INVALIDFUNCTION);
	}

	return cbRead;
}

#define COPY_CHUNK ULONG_MAX ///< Maximum size of any single copy operation.

/**
 * Copy cb bytes into IStream pstm.
 *
 * @returns  Number of bytes actaully read and written in out-parameters
 *           pcbRead and pcbWritten.  These should contain the correct values
 *           even if the call fails (throws an exception).
 * @throws   CAtlException with STG_E_* code if an error occurs.
 */
void CSftpStream::_CopyTo(
	IStream *pstm, ULONGLONG cb, 
	ULONGLONG *pcbRead, ULONGLONG *pcbWritten) throw(...)
{
	// Either return counts directly or redirect into local variable
	ULONGLONG cbRead, cbWritten;
	if (!pcbRead)
		pcbRead = &cbRead;
	if (!pcbWritten)
		pcbWritten = &cbWritten;
	*pcbRead = *pcbWritten = 0;

	// Perform copy operation in chunks COPY_CHUNK bytes big
	do {
		ULONG cbReadOne = 0;
		ULONG cbWrittenOne = 0;
		ULONG cbChunk = static_cast<ULONG>(min(cb - *pcbRead, COPY_CHUNK));
		try
		{
			_CopyOne(pstm, cbChunk, &cbReadOne, &cbWrittenOne);
			*pcbRead += cbReadOne;
			*pcbWritten += cbWrittenOne;
		}
		catch(...)
		{
			// The counts must be updated even in the failure case
			*pcbRead += cbReadOne;
			*pcbWritten += cbWrittenOne;
			throw;
		}
	} while (*pcbRead < cb);
}

/**
 * Copy one buffer's-worth of bytes into IStream pstm.
 *
 * The IStream Write() method can only operate on a ULONG quantity
 * of bytes but CopyTo() can specify a ULONGLONG quantity.  _CopyTo() must call
 * this function repeatedly with a buffer smaller than COPY_CHUNK.
 *
 * @returns  Number of bytes actaully read and written in out-parameters
 *           pcbRead and pcbWritten.  These should contain the correct values
 *           even if the call fails (throws an exception).
 * @throws   CAtlException with STG_E_* code if an error occurs.
 *
 * @todo  Performance could be improved by continuing the _Read() operation
 *        in the background while writing the buffer to the target stream.
 */
void CSftpStream::_CopyOne(
	IStream *pstm, ULONG cb, ULONG *pcbRead, ULONG *pcbWritten) throw(...)
{
	// TODO: This buffer size must be limited, otherwise copying, say, a 4GB
	// file could require 4GB of memory
	void *pv = new byte[cb]; // Intermediate buffer

	// Read data
	ULONG cbRead = 0;
	if (!pcbRead) // Either return count directly or into local variable
		pcbRead = &cbRead;
	try
	{
		_Read(static_cast<char *>(pv), cb, pcbRead);
	}
	catch(...)
	{
		UNREACHABLE;
		delete [] pv;
		throw;
	}

	// Write data
	ULONG cbWritten = 0;
	if (!pcbWritten) // Either return count directly or into local variable
		pcbWritten = &cbWritten;
	HRESULT hr = pstm->Write(pv, *pcbRead, pcbWritten);
	delete [] pv;
	ATLENSURE_SUCCEEDED(hr);
}

/**
 * Move the seek pointer by nMove bytes (may be negative).
 *
 * @returns  New location of seek pointer.
 * @throws   CAtlException with STG_E_* code if an error occurs.
 */
ULONGLONG CSftpStream::_Seek(LONGLONG nMove, DWORD dwOrigin) throw(...)
{
	ULONGLONG uNewPosition = _CalculateNewFilePosition(nMove, dwOrigin);

	libssh2_sftp_seek2(m_pHandle, uNewPosition);

	return uNewPosition;
}

/**
 * Create STATSTG structure for the stream.
 *
 * @throws  CAtlException with STG_E_* code if an error occurs.
 */
STATSTG CSftpStream::_Stat(bool bWantName) throw(...)
{	
	// Prepare STATSTG
	STATSTG statstg;
	::ZeroMemory(&statstg, sizeof STATSTG);
	statstg.type = STGTY_STREAM;
		
	// Get file size
	LIBSSH2_SFTP_ATTRIBUTES attrs;
	::ZeroMemory(&attrs, sizeof attrs);
	attrs.flags = LIBSSH2_SFTP_ATTR_SIZE | LIBSSH2_SFTP_ATTR_ACMODTIME;

	if (libssh2_sftp_fstat(m_pHandle, &attrs) != 0)
	{
		UNREACHABLE;
		TRACE("libssh2_sftp_fstat() failed: %ws", _GetLastErrorMessage());
		AtlThrow(STG_E_INVALIDFUNCTION);
	}

	statstg.cbSize.QuadPart = attrs.filesize;

	// Get file dates
	LONGLONG ll;

	ll = Int32x32To64(attrs.mtime, 10000000) + 116444736000000000;
	statstg.mtime.dwLowDateTime = static_cast<DWORD>(ll);
	statstg.mtime.dwHighDateTime = static_cast<DWORD>(ll >> 32);

	ll = Int32x32To64(attrs.atime, 10000000) + 116444736000000000;
	statstg.atime.dwLowDateTime = static_cast<DWORD>(ll);
	statstg.atime.dwHighDateTime = static_cast<DWORD>(ll >> 32);

	// Provide filename if requested
	if (bWantName)
	{
		// Convert filename to OLECHARs
		CA2W szFilename(m_strFilename.c_str(), CP_UTF8);

		// Allocate sufficient memory for OLECHAR version of filename
		size_t cchData = ::wcslen(szFilename)+1;
		size_t cbData = cchData * sizeof OLECHAR;
		statstg.pwcsName = static_cast<LPOLESTR>(::CoTaskMemAlloc(cbData));
		ATLENSURE_THROW(statstg.pwcsName, STG_E_INSUFFICIENTMEMORY);

		// Copy converted filename to STATSTG
		errno_t rc = ::wcscpy_s(statstg.pwcsName, cchData, szFilename);
		if (rc != 0)
		{
			UNREACHABLE;
			::CoTaskMemFree(statstg.pwcsName);
			AtlThrow(STG_E_INSUFFICIENTMEMORY);
		}
	}

	return statstg;
}

/**
 * Calculate new position of the seek pointer.
 *
 * @throws  CAtlException with STG_E_* code if an error occurs.
 */
ULONGLONG CSftpStream::_CalculateNewFilePosition(
	LONGLONG nMove, DWORD dwOrigin) throw(...)
{
	LONGLONG nNewPosition = 0;

	switch(dwOrigin)
	{
	case STREAM_SEEK_SET: // Relative to beginning of file
		nNewPosition = nMove;
		break;
	case STREAM_SEEK_CUR: // Relative to current position
	{
		size_t nCurrentPos = libssh2_sftp_tell(m_pHandle);
		nNewPosition = nCurrentPos + nMove;
		break;
	}
	case STREAM_SEEK_END: // Relative to end (MUST ACCESS SERVER)
	{
		LONGLONG nOffset = nMove;

		// Get size of file from server
		LIBSSH2_SFTP_ATTRIBUTES attrs;
		::ZeroMemory(&attrs, sizeof attrs);
		attrs.flags = LIBSSH2_SFTP_ATTR_SIZE;
		int rc = libssh2_sftp_fstat(m_pHandle, &attrs);
		ATLENSURE_THROW(rc == 0, STG_E_INVALIDFUNCTION);

		nNewPosition = attrs.filesize - nOffset;
		break;
	}
	default:
		ATLENSURE_THROW(false, STG_E_INVALIDFUNCTION);
	}

	if (nNewPosition < 0)
		AtlThrow(STG_E_INVALIDFUNCTION);

	return static_cast<ULONGLONG>(nNewPosition);
}
