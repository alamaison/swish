// DummyStream.cpp : Implementation of CDummyStream

#include "standard.h"

#include "DummyStream.h"
#include <shlwapi.h>

CDummyStream::CDummyStream() : 
    m_szData(NULL), 
    m_pSeek(static_cast<const void *>(m_szData))
{
}

CDummyStream::~CDummyStream()
{
    if (m_szData)
        delete [] m_szData;
}

HRESULT CDummyStream::Initialize(PCSTR pszFilePath)
{
    size_t len = ::strlen(pszFilePath) + 1;
    m_szData = new char[len];
    errno_t err = ::strcpy_s(m_szData, len, pszFilePath);

    return (err == 0) ? S_OK : E_FAIL;
}

STDMETHODIMP CDummyStream::Read(void *pv, ULONG cb, ULONG *pcbRead)
{
    size_t cbData = ::strlen(m_szData);

    if (cbData < cb)
    {
        memcpy(pv, m_szData, cbData);
        *pcbRead = static_cast<ULONG>(cbData);
    }
    else
    {
        memcpy(pv, m_szData, cb);
        *pcbRead = static_cast<ULONG>(cb);
    }

    return S_OK;
}

STDMETHODIMP CDummyStream::Write( 
    const void * /*pv*/, ULONG /*cb*/, ULONG * /*pcbWritten*/
)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDummyStream::Seek(
    __in LARGE_INTEGER dlibMove,
    __in DWORD /*dwOrigin*/,
    __out ULARGE_INTEGER *plibNewPosition
)
{
    ATLENSURE_RETURN_HR(dlibMove.QuadPart >= 0, STG_E_INVALIDFUNCTION);

    if (plibNewPosition)
    {
        plibNewPosition->QuadPart = dlibMove.QuadPart;
    }

    return S_OK;
}

STDMETHODIMP CDummyStream::SetSize( 
    __in ULARGE_INTEGER /*libNewSize*/
)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDummyStream::CopyTo( 
    __in IStream *pstm,
    __in ULARGE_INTEGER cb,
    __out ULARGE_INTEGER *pcbRead,
    __out ULARGE_INTEGER *pcbWritten
)
{
    HRESULT hr = S_OK;

    size_t cbData = ::strlen(m_szData);

    if (cbData < cb.QuadPart)
    {
        ULONG cbWritten = 0;
        hr = pstm->Write(m_szData, static_cast<ULONG>(cbData), &cbWritten);
        ATLENSURE_RETURN_HR(SUCCEEDED(hr), hr);
        if (pcbRead)
            pcbRead->QuadPart = cbData;
        if (pcbWritten)
            pcbWritten->QuadPart = cbWritten;
    }
    else
    {
        ULONG cbWritten = 0;
        hr = pstm->Write(
            m_szData, static_cast<ULONG>(cb.QuadPart), &cbWritten);
        ATLENSURE_RETURN_HR(SUCCEEDED(hr), hr);
        if (pcbRead)
            pcbRead->QuadPart = cb.QuadPart;
        if (pcbWritten)
            pcbWritten->QuadPart = cbWritten;
    }

    return hr;
}

STDMETHODIMP CDummyStream::Commit( 
    __in DWORD /*grfCommitFlags*/
)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDummyStream::Revert()
{
    return E_NOTIMPL;
}

STDMETHODIMP CDummyStream::LockRegion( 
    __in ULARGE_INTEGER /*libOffset*/,
    __in ULARGE_INTEGER /*cb*/,
    __in DWORD /*dwLockType*/
)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDummyStream::UnlockRegion( 
    __in ULARGE_INTEGER /*libOffset*/,
    __in ULARGE_INTEGER /*cb*/,
    __in DWORD /*dwLockType*/
)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDummyStream::Stat( 
    __out STATSTG *pstatstg,
    __in DWORD grfStatFlag
)
{
    ATLENSURE_RETURN_HR(pstatstg, STG_E_INVALIDPOINTER);

    HRESULT hr;

    ::ZeroMemory(pstatstg, sizeof STATSTG);

    if (grfStatFlag & !STATFLAG_NONAME)
    {
        hr = ::SHStrDup(L"bob", &(pstatstg->pwcsName));
        ATLENSURE_RETURN_HR(SUCCEEDED(hr), STG_E_INSUFFICIENTMEMORY);
    }
    
    pstatstg->type = STGTY_STREAM;
    pstatstg->cbSize.QuadPart = 33;
    return S_OK;
}

STDMETHODIMP CDummyStream::Clone( 
    __deref_out_opt IStream ** /*ppstm*/
)
{
    return E_NOTIMPL;
}