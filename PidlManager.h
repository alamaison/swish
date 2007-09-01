// PidlMgr.h: interface for the CPidlManager class.
//
//////////////////////////////////////////////////////////////////////

#ifndef PIDLMANAGER_H
#define PIDLMANAGER_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <string.h>
#include "remotelimits.h"

// Data structure to be stored in our PIDLs.
typedef struct PIDLDATA_s
{
	USHORT cb;
	WCHAR wszUser[MAX_USERNAME_LEN];
	WCHAR wszHost[MAX_HOSTNAME_LEN];
	WCHAR wszPath[MAX_PATH_LEN];
	USHORT uPort;
} PIDLDATA, *LPPIDLDATA;

typedef struct PIDLCONNDATA_s
{
	WCHAR wszUser[MAX_USERNAME_LEN];
	WCHAR wszHost[MAX_HOSTNAME_LEN];
	WCHAR wszPath[MAX_PATH_LEN];
	USHORT uPort;
} PIDLCONNDATA, *LPPIDLCONNDATA;
typedef const LPPIDLCONNDATA LPCPIDLCONNDATA;

// Class that creates/destroyes PIDLs and gets data from PIDLs.
class CPidlManager  
{
public:
   CPidlManager();
   ~CPidlManager();

   LPITEMIDLIST Create (LPCWSTR pwszUser, LPCWSTR pwszHost, 
						LPCWSTR pwszPath, USHORT uPort);
   LPITEMIDLIST Copy ( LPCITEMIDLIST );
   void Delete ( LPITEMIDLIST );
   UINT GetSize ( LPCITEMIDLIST );

   LPITEMIDLIST GetNextItem ( LPCITEMIDLIST );
   LPITEMIDLIST GetLastItem ( LPCITEMIDLIST );

   void GetUser( LPCITEMIDLIST pidl, LPWSTR pwszUser, USHORT cBufLen );
   void GetHost( LPCITEMIDLIST pidl, LPWSTR pwszHost, USHORT cBufLen );
   void GetPath( LPCITEMIDLIST pidl, LPWSTR pwszPath, USHORT cBufLen );
   USHORT GetPort( LPCITEMIDLIST pidl );

private:
   LPPIDLDATA GetData ( LPCITEMIDLIST pidl );
   void CopyWSZField( LPWSTR pwszField, LPWSTR pwszBuffer, USHORT cBufLen );
   CComPtr<IMalloc> m_spMalloc;
};

#endif // PIDLMANAGER_H
