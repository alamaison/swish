/**
    @file

    IStream interface around the libssh2 SFTP file access functions.

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

#pragma once

#include "Session.hpp"

#include "swish/atl.hpp"  // Common ATL setup

#include <string>

typedef struct _LIBSSH2_SFTP_HANDLE LIBSSH2_SFTP_HANDLE; // Forward-decl

class ATL_NO_VTABLE CSftpStream :
	public ATL::CComObjectRoot,
	public IStream
{
public:
	BEGIN_COM_MAP(CSftpStream)
		COM_INTERFACE_ENTRY(IStream)
		COM_INTERFACE_ENTRY(ISequentialStream)
	END_COM_MAP()

	/**
	 * Static factory method.
	 *
	 * @returns A smart pointer to the CSftpStream COM object.
	 * @throws  A CAtlException if creation fails.
	 */
	static ATL::CComPtr<CSftpStream> Create() throw(...)
	{
		ATL::CComObject<CSftpStream> *pObject = NULL;
		HRESULT hr = ATL::CComObject<CSftpStream>::CreateInstance(&pObject);
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
		__out ULONG& cbRead) throw(...);
	void _CopyTo(
		__in IStream *pstm, __in ULONGLONG cb, 
		__out ULONGLONG& cbRead, __out ULONGLONG& cbWritten) throw(...);
	void _CopyOne(
		__in IStream *pstm, __in ULONG cb,
		__out ULONG& cbRead, __out ULONG& cbWritten) throw(...);
	ULONGLONG _Seek(__in LONGLONG nMove, __in DWORD dwOrigin) throw(...);
	STATSTG _Stat(__in bool bWantName) throw(...);

	LIBSSH2_SFTP_HANDLE *m_pHandle;
	LIBSSH2_SESSION *m_pSession;
	LIBSSH2_SFTP *m_pSftp;
	std::string m_strFilename;
	std::string m_strDirectory;
};