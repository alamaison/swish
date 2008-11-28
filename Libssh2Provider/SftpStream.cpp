// SftpStream.cpp : Implementation of CSftpStream

#include "stdafx.h"
#include "SftpStream.h"


CSftpStream::CSftpStream()
	: m_pHandle(NULL), m_pSftp(NULL), m_pSession(NULL)
{
}

CSftpStream::~CSftpStream()
{
	if (m_pHandle)
	{
		ATLVERIFY(libssh2_sftp_close(m_pHandle) == 0);
	}
}

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

STDMETHODIMP CSftpStream::Read(void *pv, ULONG cb, ULONG *pcbRead)
{
	ATLENSURE_RETURN_HR(pv, STG_E_INVALIDPOINTER);
	ATLENSURE_RETURN_HR(pcbRead, STG_E_INVALIDPOINTER);

	return _Read(static_cast<char *>(pv), cb, pcbRead);
}

HRESULT CSftpStream::_Read(char *pbuf, ULONG cb, ULONG *pcbRead)
{
	ssize_t cbRead = libssh2_sftp_read(m_pHandle, pbuf, cb);

	if (cbRead < 0) // Error
	{
		UNREACHABLE;
		TRACE("libssh2_sftp_read() failed: %ws", _GetLastErrorMessage());
		return STG_E_INVALIDFUNCTION;
	}

	*pcbRead = cbRead;

	return (cbRead < cb) ? S_FALSE : S_OK;
}

STDMETHODIMP CSftpStream::Write( 
	__in_bcount(cb) const void *pv,
	__in ULONG cb,
	__out_opt ULONG *pcbWritten
)
{
	return E_NOTIMPL;
}

STDMETHODIMP CSftpStream::Seek(
	LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
	try
	{
		size_t uNewPosition = _CalculateNewFilePosition(dlibMove, dwOrigin);

		libssh2_sftp_seek(m_pHandle, uNewPosition);
		if (plibNewPosition)
			plibNewPosition->QuadPart = uNewPosition;
	}
	catchCom()

	return S_OK;
}

size_t CSftpStream::_CalculateNewFilePosition(
	LARGE_INTEGER dlibMove, DWORD dwOrigin) throw(...)
{
	LONGLONG nNewPosition = 0;

	switch(dwOrigin)
	{
	case STREAM_SEEK_SET: // Relative to beginning of file
		nNewPosition = dlibMove.QuadPart;
		break;
	case STREAM_SEEK_CUR: // Relative to current position
	{
		size_t nCurrentPos = libssh2_sftp_tell(m_pHandle);
		nNewPosition = nCurrentPos + dlibMove.QuadPart;
		break;
	}
	case STREAM_SEEK_END: // Relative to end (MUST ACCESS SERVER)
	{
		LONGLONG nOffset = dlibMove.QuadPart;

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

	return static_cast<size_t>(nNewPosition);
}

STDMETHODIMP CSftpStream::SetSize( 
	__in ULARGE_INTEGER libNewSize
)
{
	return E_NOTIMPL;
}

STDMETHODIMP CSftpStream::CopyTo(
	IStream *pstm, ULARGE_INTEGER cb, 
	ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten
)
{
	ATLENSURE_RETURN_HR(pstm, STG_E_INVALIDPOINTER);

	// TODO: This buffer size must be limited, otherwise copying, say, a 4GB
	// file would require 4GB of memory
	void *pv = new byte[cb.QuadPart]; // Intermediate buffer

	// Read data
	ULONG cbRead = 0;
	HRESULT hr = _Read(static_cast<char *>(pv), cb.QuadPart, &cbRead);
	if (FAILED(hr))
	{
		UNREACHABLE;
		delete [] pv;
		return hr;
	}
	if (pcbRead)
		pcbRead->QuadPart = cbRead;

	// Write data
	ULONG cbWritten = 0;
	hr = pstm->Write(pv, cbRead, &cbWritten);
	delete [] pv;
	ATLENSURE_RETURN_HR(SUCCEEDED(hr), hr);
	if (pcbRead)
		pcbRead->QuadPart = cbWritten;

	return hr;
}

STDMETHODIMP CSftpStream::Commit( 
	__in DWORD grfCommitFlags
)
{
	return E_NOTIMPL;
}

STDMETHODIMP CSftpStream::Revert()
{
	return E_NOTIMPL;
}

STDMETHODIMP CSftpStream::LockRegion( 
	__in ULARGE_INTEGER libOffset,
	__in ULARGE_INTEGER cb,
	__in DWORD dwLockType
)
{
	return E_NOTIMPL;
}

STDMETHODIMP CSftpStream::UnlockRegion( 
	__in ULARGE_INTEGER libOffset,
	__in ULARGE_INTEGER cb,
	__in DWORD dwLockType
)
{
	return E_NOTIMPL;
}

STDMETHODIMP CSftpStream::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
	ATLENSURE_RETURN_HR(pstatstg, STG_E_INVALIDPOINTER);

	// Prepare STATSTG
	::ZeroMemory(pstatstg, sizeof STATSTG);
	pstatstg->type = STGTY_STREAM;
	
	// Get file size
	LIBSSH2_SFTP_ATTRIBUTES attrs;
	::ZeroMemory(&attrs, sizeof attrs);
	attrs.flags = LIBSSH2_SFTP_ATTR_SIZE | LIBSSH2_SFTP_ATTR_ACMODTIME;

	if (libssh2_sftp_fstat(m_pHandle, &attrs) != 0)
	{
		UNREACHABLE;
		TRACE("libssh2_sftp_fstat() failed: %ws", _GetLastErrorMessage());
		return STG_E_INVALIDFUNCTION;
	}

	pstatstg->cbSize.QuadPart = attrs.filesize;

	// Get file dates
	LONGLONG ll;

	ll = Int32x32To64(attrs.mtime, 10000000) + 116444736000000000;
	pstatstg->mtime.dwLowDateTime = static_cast<DWORD>(ll);
	pstatstg->mtime.dwHighDateTime = static_cast<DWORD>(ll >> 32);

	ll = Int32x32To64(attrs.atime, 10000000) + 116444736000000000;
	pstatstg->atime.dwLowDateTime = static_cast<DWORD>(ll);
	pstatstg->atime.dwHighDateTime = static_cast<DWORD>(ll >> 32);

	// Provide filename if requested
	if (!(grfStatFlag & STATFLAG_NONAME))
	{
		// Convert filename to OLECHARs
		CA2W szFilename(m_strFilename.c_str(), CP_UTF8);

		// Allocate sufficient memory for OLECHAR version of filename
		size_t cchData = ::wcslen(szFilename)+1;
		size_t cbData = cchData * sizeof OLECHAR;
		pstatstg->pwcsName = static_cast<LPOLESTR>(::CoTaskMemAlloc(cbData));

		// Copy converted filename to STATSTG
		errno_t rc = ::wcscpy_s(pstatstg->pwcsName, cchData, szFilename);
		ATLENSURE_RETURN_HR(rc == 0, STG_E_INSUFFICIENTMEMORY);
	}

	return S_OK;
}

STDMETHODIMP CSftpStream::Clone( 
	__deref_out_opt IStream **ppstm
)
{
	return E_NOTIMPL;
}
