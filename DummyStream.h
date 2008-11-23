// DummyStream.h : Declaration of the CDummyStream

#pragma once
#include "resource.h"       // main symbols

// CDummyStream
[
	coclass,
	default(IStream),
	threading(apartment),
	vi_progid("Swish.DummyStream"),
	progid("Swish.DummyStream.1"),
	version(1.0),
	uuid("96EE89A7-88FF-4FD3-8134-67E5E3205797"),
	helpstring("DummyStream Class")
]
class ATL_NO_VTABLE CDummyStream :
	public IStream
{
public:
	CDummyStream() : 
		m_szData("Dummy file contents."), 
		m_pSeek(static_cast<const void *>(m_szData))
	{
	}

	DECLARE_PROTECT_FINAL_CONSTRUCT()
	HRESULT FinalConstruct()
	{
		return S_OK;
	}

	void FinalRelease()
	{
	}

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
	const void *m_pSeek;
	const char *m_szData;
};

