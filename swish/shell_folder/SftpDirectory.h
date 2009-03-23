/*  Manage remote directory as a collection of PIDLs.

    Copyright (C) 2007, 2008, 2009  Alexander Lamaison <awl03@doc.ic.ac.uk>

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
*/

#pragma once

#include "HostPidl.h"            // PIDL wrapper classes
#include "RemotePidl.h"
#include "Connection.h"          // For SFTP Connection container
#include "common/catch_com.hpp"  // COM catch block

#include <vector>

/**
 * A COM holder for an STL collection that can be used in an enumeration.
 * The enumerator (IEnumXXX) will take a pointer to this holder when it is
 * created which ensures that the STL collection lives at least as long as
 * the enumerator.
 */
template <typename CollType, typename ThreadingModel=ATL::CComObjectThreadModel>
class CComSTLCopyContainer :
	public ATL::CComObjectRootEx<ThreadingModel>,
	public IUnknown
{
public:
	HRESULT Copy(const CollType& coll)
	{
		try
		{
			m_coll = coll;
			return S_OK;
		}
		catch (...)
		{
			return E_OUTOFMEMORY;
		}
	}

BEGIN_COM_MAP(CComSTLCopyContainer)
	COM_INTERFACE_ENTRY(IUnknown)
END_COM_MAP()

	CollType m_coll;
};

typedef ATL::CComObject<CComSTLCopyContainer< std::vector<CChildPidl> > >
	CComPidlHolder;

class CSftpDirectory
{
public:
	CSftpDirectory(
		__in CAbsolutePidlHandle pidlDirectory, const CConnection& conn)
		throw(...);

	ATL::CComPtr<IEnumIDList> GetEnum(__in SHCONTF grfFlags) throw(...);
	CSftpDirectory GetSubdirectory(__in CRemoteItemHandle pidl) throw(...);
	ATL::CComPtr<IStream> GetFile(__in CRemoteItemHandle pidl) throw(...);
	ATL::CComPtr<IStream> GetFileByPath(PCWSTR pwszPath) throw(...);

	bool Rename(
		__in CRemoteItemHandle pidlOldFile, __in PCWSTR pwszNewFilename)
		throw(...);
	void Delete(__in CRemoteItemHandle pidl) throw(...);

private:
	CConnection m_connection;
	ATL::CString m_strDirectory;        ///< Absolute path to this directory.
	CAbsolutePidl m_pidlDirectory;      ///< Absolute PIDL to this directory.
	std::vector<CChildPidl> m_vecPidls; ///< Directory contents as PIDLs.

	HRESULT _Fetch( __in SHCONTF grfFlags );
};


/**
 * Copy-policy to manage copying and destruction of PITEMID_CHILD pidls.
 */
struct _CopyChildPidl
{
	static HRESULT copy(PITEMID_CHILD *ppidlCopy, const CChildPidl *ppidl)
	{
		try
		{
			*ppidlCopy = ppidl->CopyTo();
		}
		catchCom();

		return S_OK;
	}

	static HRESULT copy(PITEMID_CHILD *ppidlCopy, const PITEMID_CHILD *ppidl)
	{
		*ppidlCopy = ::ILCloneChild(*ppidl);
		if (*ppidlCopy)
			return S_OK;
		else
			return E_OUTOFMEMORY;
	}

	static void init(PITEMID_CHILD* /* ppidl */) { }
	static void destroy(PITEMID_CHILD *ppidl)
	{
		::ILFree(*ppidl);
	}
};
