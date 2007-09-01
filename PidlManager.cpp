// PidlMgr.cpp: implementation of the CPidlManager class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "PidlManager.h"
#include "strsafe.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction

CPidlManager::CPidlManager()
{
	HRESULT hr = SHGetMalloc ( &m_spMalloc );

    ATLASSERT(SUCCEEDED(hr));
}

CPidlManager::~CPidlManager()
{
}

/*------------------------------------------------------------------------------
 * CPidlManager::Create( LPCWSTR, LPCWSTR, LPCWSTR, USHORT )
 * Create a new terminated PIDL using the passed-in connection information
 *----------------------------------------------------------------------------*/
LPITEMIDLIST CPidlManager::Create(LPCWSTR pwszUser, LPCWSTR pwszHost, 
								  LPCWSTR pwszPath, USHORT uPort)
{
	// Calculate the size of the PIDLDATA structure
    USHORT uSize = sizeof(PIDLDATA);
	ATLASSERT(uSize%sizeof(DWORD) == 0); // Must be DWORD-aligned

	// Allocate enough memory for the PIDL (ITEMIDLIST) to hold 
	// a PIDLDATA structure plus the terminator
    LPITEMIDLIST pidlOut = 
		(LPITEMIDLIST)m_spMalloc->Alloc(uSize + sizeof(ITEMIDLIST));

    if(pidlOut)
    {
		// Use first PIDL member as a PIDLDATA structure
		LPPIDLDATA pData = (LPPIDLDATA) pidlOut;

		// Assign values to the members of the PIDL (first SHITEMID)
		pData->cb = uSize;

		HRESULT hr;
		hr = StringCbCopyW(pData->wszUser, sizeof(pData->wszUser), pwszUser);
		if (hr == STRSAFE_E_INSUFFICIENT_BUFFER) ATLTRACE("Username truncated");
		ATLASSERT(hr == S_OK || hr == STRSAFE_E_INSUFFICIENT_BUFFER);

		hr = StringCbCopyW(pData->wszHost, sizeof(pData->wszHost), pwszHost);
		if (hr == STRSAFE_E_INSUFFICIENT_BUFFER) ATLTRACE("Hostname truncated");
		ATLASSERT(hr == S_OK || hr == STRSAFE_E_INSUFFICIENT_BUFFER);

		hr = StringCbCopyW(pData->wszPath, sizeof(pData->wszPath), pwszPath);
		if (hr == STRSAFE_E_INSUFFICIENT_BUFFER) ATLTRACE("Path truncated");
		ATLASSERT(hr == S_OK || hr == STRSAFE_E_INSUFFICIENT_BUFFER);

		pData->uPort = uPort;

		// Create the terminating null PIDL by setting fields to 0.
		LPITEMIDLIST pidlNext = GetNextItem(pidlOut);
		ATLASSERT(pidlNext != NULL);
		pidlNext->mkid.cb = 0;
		pidlNext->mkid.abID[0] = 0;
    }

    return pidlOut;
}


/*---------------------------------------------------------------*/
// CPidlManager::Delete( LPITEMIDLIST )
// Delete a PIDL
/*---------------------------------------------------------------*/
void CPidlManager::Delete(LPITEMIDLIST pidl)
{
    m_spMalloc->Free(pidl);
}


/*---------------------------------------------------------------*/
// CPidlManager::GetNextItem( LPCITEMIDLIST )
// Retrieves the next element in a ITEMIDLIST
/*---------------------------------------------------------------*/
LPITEMIDLIST CPidlManager::GetNextItem ( LPCITEMIDLIST pidl )
{
    ATLASSERT(pidl != NULL);

    return LPITEMIDLIST(LPBYTE(pidl) + pidl->mkid.cb);
}


/*---------------------------------------------------------------*/
// CPidlManager::GetLastItem( LPCITEMIDLIST )
// Retrieves the last element in a ITEMIDLIST
/*---------------------------------------------------------------*/
LPITEMIDLIST CPidlManager::GetLastItem ( LPCITEMIDLIST pidl )
{
	LPITEMIDLIST pidlLast = NULL;

    ATLASSERT(pidl != NULL);

    while ( 0 != pidl->mkid.cb )
	{
        pidlLast = (LPITEMIDLIST) pidl;
        pidl = GetNextItem ( pidl );
	}

    return pidlLast;
}


/*---------------------------------------------------------------*/
// CPidlManager::GetSize( LPITEMIDLIST )
// Retrieves the size of item list 
/*---------------------------------------------------------------*/
UINT CPidlManager::GetSize ( LPCITEMIDLIST pidl )
{
	UINT uSize = 0;
	LPITEMIDLIST pidlTemp = (LPITEMIDLIST) pidl;

    ATLASSERT(pidl != NULL);

    while ( pidlTemp->mkid.cb != 0 )
	{
        uSize += pidlTemp->mkid.cb;
        pidlTemp = GetNextItem ( pidlTemp );
	}  
    
    // add the size of the NULL terminating ITEMIDLIST
    uSize += sizeof(ITEMIDLIST);

    return uSize;
}

/*------------------------------------------------------------------------------
 * CPidlManager::GetUser(LPCITEMIDLIST, LPWSTR, USHORT)
 * Retrieves the user name of a given PIDL (last element)
 * Length in BYTEs of return buffer is given as cBufLen
 *----------------------------------------------------------------------------*/
void CPidlManager::GetUser (LPCITEMIDLIST pidl, LPWSTR pwszUser, USHORT cBufLen)
{
	ATLENSURE(cBufLen > 0);
	if ( pidl == NULL )	{ *pwszUser = '\0'; return;	}

	CopyWSZField( GetData(pidl)->wszUser, pwszUser, cBufLen );
}

/*------------------------------------------------------------------------------
 * CPidlManager::GetHost(LPCITEMIDLIST, LPWSTR, USHORT)
 * Retrieves the host name of a given PIDL (last element)
 * Length in BYTEs of return buffer is given as cBufLen
 *----------------------------------------------------------------------------*/
void CPidlManager::GetHost( LPCITEMIDLIST pidl, LPWSTR pwszHost, USHORT cBufLen )
{
	ATLENSURE(cBufLen > 0);
	if ( pidl == NULL )	{ *pwszHost = '\0'; return;	}

	CopyWSZField( GetData(pidl)->wszHost, pwszHost, cBufLen );
}

/*------------------------------------------------------------------------------
 * CPidlManager::GetPath(LPCITEMIDLIST, LPWSTR, USHORT)
 * Retrieves the path of a given PIDL (last element)
 * Length in BYTEs of return buffer is given as cBufLen
 *----------------------------------------------------------------------------*/
void CPidlManager::GetPath(LPCITEMIDLIST pidl, LPWSTR pwszPath, USHORT cBufLen)
{
	ATLENSURE(cBufLen > 0);
	if ( pidl == NULL )	{ *pwszPath = '\0'; return;	}

	CopyWSZField( GetData(pidl)->wszPath, pwszPath, cBufLen );
}

/*------------------------------------------------------------------------------
 * CPidlManager::GetPort(LPCITEMIDLIST)
 * Retrieves the port number of a given PIDL (last element)
 *----------------------------------------------------------------------------*/
USHORT CPidlManager::GetPort( LPCITEMIDLIST pidl )
{
	if ( pidl == NULL )	{ return 0;	}
	return GetData(pidl)->uPort;
}

/*------------------------------------------------------------------------------
 * CPidlManager::CopyWSZField(LPWSTR, LPWSTR, USHORT)
 * Copies a WString field into provided buffer and performs checking
 * Length in BYTEs of return buffer is given as cBufLen
 *----------------------------------------------------------------------------*/
inline void CPidlManager::CopyWSZField( LPWSTR pwszField, LPWSTR pwszBuffer, 
									USHORT cBufLen )
{
	// Copy field into buffer
	// Neither source nor destination of StringCbCopyW can be NULL
	ATLASSERT(pwszField != NULL && pwszBuffer != NULL);
	HRESULT hr = StringCbCopyW(pwszBuffer, cBufLen, pwszField);
	if (hr == STRSAFE_E_INSUFFICIENT_BUFFER) ATLTRACE("Field truncated");
	ATLASSERT(hr == S_OK || hr == STRSAFE_E_INSUFFICIENT_BUFFER);
}

/*------------------------------------------------------------------------------
 * CPidlManager::GetPath(LPCITEMIDLIST)
 * Returns the data structure of the PIDL required to retrieve stored fields
 *----------------------------------------------------------------------------*/
LPPIDLDATA CPidlManager::GetData ( LPCITEMIDLIST pidl )
{
    // Get the last item of the PIDL to make sure we get the right value
    // in case of multiple nesting levels
    LPITEMIDLIST pidlLast = GetLastItem ( pidl );
    return (LPPIDLDATA)( pidlLast->mkid.abID );
}

/*---------------------------------------------------------------*/
// CPidlManager::Copy( LPITEMIDLIST )
// Duplicates a PIDL
/*---------------------------------------------------------------*/
LPITEMIDLIST CPidlManager::Copy ( LPCITEMIDLIST pidlSrc )
{
	LPITEMIDLIST pidlTarget = NULL;
	UINT cbSrc = 0;

    if ( NULL == pidlSrc )
        return NULL;

	// Allocate memory for the new PIDL.

	cbSrc = GetSize ( pidlSrc );
	pidlTarget = (LPITEMIDLIST) m_spMalloc->Alloc ( cbSrc );

	if ( NULL == pidlTarget )
	   return NULL;

	// Copy the source PIDL to the target PIDL.

	CopyMemory ( pidlTarget, pidlSrc, cbSrc );

	return pidlTarget;
}
