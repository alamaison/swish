// DummyStream.h : Declaration of the CDummyStream

#pragma once

#include <objidl.h>    // IStream

#include <atlbase.h>   // ATL base classes
#include <atlcom.h>    // ATL CComObject et. al.

class ATL_NO_VTABLE CDummyStream :
    public ATL::CComObjectRoot,
    public IStream
{
public:
    BEGIN_COM_MAP(CDummyStream)
        COM_INTERFACE_ENTRY(IStream)
        COM_INTERFACE_ENTRY(ISequentialStream)
    END_COM_MAP()

    CDummyStream();
    ~CDummyStream();
        
    HRESULT Initialize(PCSTR pszFilePath);

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
    char *m_szData;
};

