/**
    @file

    IStream interface around the libssh2 SFTP file access functions.

    @if license

    Copyright (C) 2008, 2009, 2012, 2013
    Alexander Lamaison <awl03@doc.ic.ac.uk>

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

    @endif
*/

#include "SftpStream.hpp"

#include "swish/debug.hpp"                   // Debug macros

#include <winapi/com/catch.hpp> // WINAPI_COM_CATCH_AUTO_INTERFACE

#include <comet/error.h> // com_error

#include <boost/throw_exception.hpp>         // BOOST_THROW_EXCEPTION

#include <atlstr.h>                          // CString

#include <libssh2.h>
#include <libssh2_sftp.h>

using swish::connection::running_session;

using ATL::CA2W;
using ATL::CString;

using comet::com_error;

using boost::mutex;
using boost::shared_ptr;

using std::string;


namespace { // private

/**
 * Maximum size of any single copy.
 *
 * Must be smaller than maximum ULONG.  @see CSftpStream::_CopyOne for more
 * explanation.
 */
static const ULONG COPY_CHUNK = 32500;

/**
 * Maps between libssh2 SFTP error codes and an appropriate error string.
 *
 * @param uError  SFTP error code as returned by libssh2_sftp_last_error().
 */
CString GetSftpErrorMessage(ULONG uError)
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

/**
 * Retrieves a string description of the last error reported by libssh2.
 *
 * In the case that the last SSH error is an SFTP error it returns the SFTP
 * error message in preference.
 */
CString GetLastErrorMessage(const running_session& session)
{
    int nErr; PSTR pszErr; int cchErr;

    nErr = libssh2_session_last_error(session, &pszErr, &cchErr, false);
    if (nErr == LIBSSH2_ERROR_SFTP_PROTOCOL)
    {
        ULONG uErr = libssh2_sftp_last_error(session);
        return GetSftpErrorMessage(uErr);
    }
    else // A non-SFTP error occurred
        return CString(pszErr);
}


/**
 * Translate an SFTP error code returned by libssh2 into a COM 
 * FACILITY_STORAGE error as expected to be returned by IStreams.
 *
 * Explorer turns these error codes into messages which it displays to
 * the user.
 */
HRESULT sftp_error_to_storage_error(unsigned long sftp_error)
{
    switch (sftp_error)
    {
    case LIBSSH2_FX_OK:
        return S_OK;

    case LIBSSH2_FX_EOF:
    case LIBSSH2_FX_FAILURE:
    case LIBSSH2_FX_BAD_MESSAGE:
        return STG_E_CANTSAVE;
        
    case LIBSSH2_FX_NO_CONNECTION:
    case LIBSSH2_FX_CONNECTION_LOST:
        return STG_E_INCOMPLETE;

    case LIBSSH2_FX_NO_SUCH_FILE:
    case LIBSSH2_FX_NO_SUCH_PATH:
        return STG_E_FILENOTFOUND;

    case LIBSSH2_FX_PERMISSION_DENIED:
        return STG_E_ACCESSDENIED;

    case LIBSSH2_FX_OP_UNSUPPORTED:
        return STG_E_UNIMPLEMENTEDFUNCTION;

    case LIBSSH2_FX_INVALID_HANDLE:
        return STG_E_INVALIDHANDLE;

    case LIBSSH2_FX_FILE_ALREADY_EXISTS:
        return STG_E_FILEALREADYEXISTS;

    case LIBSSH2_FX_WRITE_PROTECT:
        return STG_E_DISKISWRITEPROTECTED;

    case LIBSSH2_FX_NO_MEDIA:
    case LIBSSH2_FX_NO_SPACE_ON_FILESYSTEM:
    case LIBSSH2_FX_QUOTA_EXCEEDED:
        return STG_E_MEDIUMFULL;

    case LIBSSH2_FX_LOCK_CONFLICT:
        return STG_E_LOCKVIOLATION;

    case LIBSSH2_FX_INVALID_FILENAME:
        return STG_E_INVALIDPARAMETER;

    case LIBSSH2_FX_UNKNOWN_PRINCIPAL:
    case LIBSSH2_FX_DIR_NOT_EMPTY:
    case LIBSSH2_FX_NOT_A_DIRECTORY:
    case LIBSSH2_FX_LINK_LOOP:
    default:
        return STG_E_INVALIDFUNCTION;
    }
}

/**
 * Return last session SFTP error as a COM FACILITY_STORAGE error code.
 */
HRESULT last_storage_error(running_session& session)
{
    switch (libssh2_session_last_error(session, NULL, NULL, false))
    {
    case LIBSSH2_ERROR_NONE:
        return S_OK;
    case LIBSSH2_ERROR_SFTP_PROTOCOL:
        return sftp_error_to_storage_error(libssh2_sftp_last_error(session));
    case LIBSSH2_ERROR_ALLOC:
        return STG_E_INSUFFICIENTMEMORY;
    default:
        return STG_E_INVALIDFUNCTION;
    }
}

int safe_libssh2_sftp_close_handle(LIBSSH2_SFTP_HANDLE* handle)
{
    return (handle) ? libssh2_sftp_close_handle(handle) : 0;
}

} // namespace


/**
 * Construct a new CSftpStream instance with a file path and an SFTP session.
 *
 * The file is opened using SFTP and the CSftpStream provides access to it
 * via the IStream interface.
 */
CSftpStream::CSftpStream(
    shared_ptr<running_session> session, const string& file, OpenFlags flags)
    : m_session(session)
{
    // Map between CSftpStream flags and libssh2 flags
    unsigned long libssh2_flags = 0;
    if (flags & read)
        libssh2_flags |= LIBSSH2_FXF_READ;
    if (flags & write)
        libssh2_flags |= LIBSSH2_FXF_WRITE | LIBSSH2_FXF_TRUNC;
    if (flags & create)
        libssh2_flags |= LIBSSH2_FXF_CREAT;

    long mode = // rw-r--r--
        LIBSSH2_SFTP_S_IRUSR | LIBSSH2_SFTP_S_IWUSR | LIBSSH2_SFTP_S_IRGRP |
        LIBSSH2_SFTP_S_IROTH;

    mutex::scoped_lock lock = m_session->aquire_lock();

    m_handle = shared_ptr<LIBSSH2_SFTP_HANDLE>(
        libssh2_sftp_open(*m_session, file.c_str(), libssh2_flags, mode),
        safe_libssh2_sftp_close_handle);
    if (!m_handle)
    {
        HRESULT hr = last_storage_error(*m_session);
        BOOST_THROW_EXCEPTION(
            com_error(GetLastErrorMessage(*m_session).GetString(), hr));
    }

    m_strFilename = file.substr(file.find_last_of('/')+1);
    m_strDirectory = file.substr(0, file.find_last_of('/'));
}

/**
 * Close handle to file and destroy CStfpStream instance.
 */
CSftpStream::~CSftpStream()
{
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
 * @return S_OK if successful, S_FALSE if successful but fewer bytes were read 
 *         than requested or an STG_E_* error code if an error occurs.
 * @retval STG_E_INVALIDPOINTER if pv is NULL.
 *
 * Unlike POSIX read(), MSDN makes clear that any short Read() indicates the
 * end-of-file [1].  POSIX read() allows short reads and only treats 0 as the
 * end-of-file.  Therefore, if we get a short read from the server, we keep
 * reading until we fill the buffer on the server return 0.
 *
 * [1] http://msdn.microsoft.com/en-us/library/aa380011%28v=vs.85%29.aspx
 */
STDMETHODIMP CSftpStream::Read(void* pv, ULONG cb, ULONG* pcbRead)
{
    if (pcbRead)
        *pcbRead = 0;
    ATLENSURE_RETURN_HR(pv, STG_E_INVALIDPOINTER);

    try
    {
        ULONG cbTemp = 0; // Either return count directly or discard in cbTemp
        if (!pcbRead)
            pcbRead = &cbTemp;
        _Read(static_cast<char*>(pv), cb, *pcbRead);
    }
    WINAPI_COM_CATCH_AUTO_INTERFACE();

    return (*pcbRead < cb) ? S_FALSE : S_OK;
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
 * @retval STG_E_INVALIDPOINTER if pv is NULL.
 *
 * MSDN seems to imply that, unlike POSIX write(), Write() cannot return
 * short except in the error case [1].  Therefore we err on the side of caution
 * and block until all the data has been written.
 *
 * [1] http://msdn.microsoft.com/en-us/library/aa380014%28v=VS.85%29.aspx
 */
STDMETHODIMP CSftpStream::Write(
    const void* pv, ULONG cb, ULONG* pcbWritten)
{
    if (pcbWritten)
        *pcbWritten = 0;
    ATLENSURE_RETURN_HR(pv, STG_E_INVALIDPOINTER);

    try
    {
        ULONG cbTemp = 0; // Either return count directly or discard in cbTemp
        if (!pcbWritten)
            pcbWritten = &cbTemp;
        _Write(static_cast<const char*>(pv), cb, *pcbWritten);
    }
    WINAPI_COM_CATCH_AUTO_INTERFACE();

    return S_OK;
}

/**
 * Copy a given number of bytes from the current IStream to another IStream.
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
 * @param[out] pcbWritten  Number of bytes that were actually written to the
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

    // If the count-output variables are not provided, redirect them to local
    // temporaries so that the remainder of the algorithm doesn't have to
    // repeatedly check their existence
    ULONGLONG cbRead, cbWritten;

    try
    {
        _CopyTo(pstm, cb.QuadPart,
            (pcbRead) ? pcbRead->QuadPart : cbRead,
            (pcbWritten) ? pcbWritten->QuadPart : cbWritten);
    }
    WINAPI_COM_CATCH_AUTO_INTERFACE();

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
    WINAPI_COM_CATCH_AUTO_INTERFACE();

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
    WINAPI_COM_CATCH_AUTO_INTERFACE();

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
 *----------------------------------------------------------------------------*

/**
 * Read cb bytes into buffer pbuf.  
 *
 * @returns  Number of bytes actually read in out-parameter cbRead.  This
 *           should contain the correct value even if the call fails (throws 
 *           an exception).
 * @throws   com_error with STG_E_* code if an error occurs.
 */
void CSftpStream::_Read(char* pbuf, ULONG cb, ULONG& cbRead)
{
    ULONG rc;

    cbRead = 0;
    do {
        rc = _ReadOne(pbuf + cbRead, cb - cbRead);
        cbRead += rc;
    } while (rc != 0 && cbRead < cb);
}

ULONG CSftpStream::_ReadOne(char* pbuf, ULONG cb)
{
    mutex::scoped_lock lock = m_session->aquire_lock();

    ssize_t rc = libssh2_sftp_read(m_handle.get(), pbuf, cb);
    if (rc < 0)
    {
        TRACE("libssh2_sftp_read() failed: %ws", 
            GetLastErrorMessage(*m_session));
        BOOST_THROW_EXCEPTION(com_error(last_storage_error(*m_session)));
    }
    else
    {
        return rc;
    }
}

/**
 * Write cb bytes from buffer pbuf onto the stream.
 *
 * @returns  Number of bytes actually written in out-parameter cbRead.  This
 *           should contain the correct value even if the call fails (throws 
 *           an exception).
 * @throws   com_error with STG_E_* code if an error occurs.
 */
void CSftpStream::_Write(const char* pbuf, ULONG cb, ULONG& cbWritten)
{
    ULONG rc;
    
    cbWritten = 0;
    do {
        rc = _WriteOne(pbuf + cbWritten, cb - cbWritten);
        cbWritten += rc;
    }
    while (cbWritten < cb);
}


ULONG CSftpStream::_WriteOne(const char* pbuf, ULONG cb)
{
    mutex::scoped_lock lock = m_session->aquire_lock();

    ssize_t rc = libssh2_sftp_write(m_handle.get(), pbuf, cb);
    if (rc < 0)
    {
        TRACE("libssh2_sftp_write() failed: %ws", 
            GetLastErrorMessage(*m_session));

        // XXX: maybe we have to return STG_E_CANTSAVE here (see MSDN)
        HRESULT hr = last_storage_error(*m_session);
        BOOST_THROW_EXCEPTION(com_error(hr));
    }
    return rc;
}

/**
 * Copy cb bytes into IStream pstm.
 *
 * @returns  Number of bytes actaully read and written in out-parameters
 *           cbRead and cbWritten.  These should be set correctly
 *           even if the call fails (throws an exception).
 * @throws   com_error with STG_E_* code if an error occurs.
 */
void CSftpStream::_CopyTo(
    IStream *pstm, ULONGLONG cb, 
    ULONGLONG& cbRead, ULONGLONG& cbWritten) throw(...)
{
    cbRead = cbWritten = 0;

    // Perform copy operation in chunks COPY_CHUNK bytes big
    do {
        ULONG cbReadOne;
        ULONG cbWrittenOne;
        ULONG cbChunk = static_cast<ULONG>(min(cb - cbRead, COPY_CHUNK));
        try
        {
            _CopyOne(pstm, cbChunk, cbReadOne, cbWrittenOne);
            cbRead += cbReadOne;
            cbWritten += cbWrittenOne;
        }
        catch(...)
        {
            // The counts must be updated even in the failure case
            cbRead += cbReadOne;
            cbWritten += cbWrittenOne;
            throw;
        }
    } while (cbRead < cb);
}

/**
 * Copy one buffer's-worth of bytes into IStream pstm.
 *
 * _CopyTo() will call this function repeatedly with a buffer smaller 
 * than COPY_CHUNK.  The reasons for this are twofold:
 * - We must not blindly allocate a single buffer the same size as the file
 *   as a large file would lead to a large allocation of memory.
 * - The IStream Write() method can only operate on a ULONG quantity
 *   of bytes but CopyTo() can specify a ULONGLONG quantity.
 *
 * @returns  Number of bytes actaully read and written in out-parameters
 *           cbRead and cbWritten.  These should be set correctly 
 *           even if the call fails (throws an exception).
 * @throws   com_error with STG_E_* code if an error occurs.
 *
 * @todo  Performance could be improved by continuing the _Read() operation
 *        in the background while writing the buffer to the target stream.
 */
void CSftpStream::_CopyOne(
    IStream *pstm, ULONG cb, ULONG& cbRead, ULONG& cbWritten) throw(...)
{
    void *pv = new byte[cb]; // Intermediate buffer

    // Read data
    try
    {
        _Read(static_cast<char *>(pv), cb, cbRead);
    }
    catch(...)
    {
        UNREACHABLE;
        delete [] pv;
        throw;
    }

    // Write data
    cbWritten = 0; // Could remove this if we trust the stream to set it properly
    HRESULT hr = pstm->Write(pv, cbRead, &cbWritten);
    delete [] pv;
    ATLENSURE_SUCCEEDED(hr);
}

/**
 * Move the seek pointer by nMove bytes (may be negative).
 *
 * @returns  New location of seek pointer.
 * @throws   com_error with STG_E_* code if an error occurs.
 */
ULONGLONG CSftpStream::_Seek(LONGLONG nMove, DWORD dwOrigin) throw(...)
{
    ULONGLONG uNewPosition = _CalculateNewFilePosition(nMove, dwOrigin);

    mutex::scoped_lock lock = m_session->aquire_lock();

    libssh2_sftp_seek64(m_handle.get(), uNewPosition);

    return uNewPosition;
}

/**
 * Create STATSTG structure for the stream.
 *
 * @throws  com_error with STG_E_* code if an error occurs.
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

    {
        mutex::scoped_lock lock = m_session->aquire_lock();

        if (libssh2_sftp_fstat(m_handle.get(), &attrs) != 0)
        {
            UNREACHABLE;
            TRACE("libssh2_sftp_fstat() failed: %ws", 
                GetLastErrorMessage(*m_session));

            HRESULT hr = last_storage_error(*m_session);
            BOOST_THROW_EXCEPTION(com_error(hr));
        }
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
 * @throws  com_error with STG_E_* code if an error occurs.
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
        mutex::scoped_lock lock = m_session->aquire_lock();

        nNewPosition = libssh2_sftp_tell64(m_handle.get());
        nNewPosition += nMove;
        break;
    }
    case STREAM_SEEK_END: // Relative to end (MUST ACCESS SERVER)
    {
        LONGLONG nOffset = nMove;

        // Get size of file from server
        LIBSSH2_SFTP_ATTRIBUTES attrs;
        ::ZeroMemory(&attrs, sizeof attrs);
        attrs.flags = LIBSSH2_SFTP_ATTR_SIZE;

        mutex::scoped_lock lock = m_session->aquire_lock();

        if (libssh2_sftp_fstat(m_handle.get(), &attrs) != 0)
        {        
            HRESULT hr = last_storage_error(*m_session);
            BOOST_THROW_EXCEPTION(com_error(hr));
        }

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