/*  Manage remote directory as a collection of PIDLs.

    Copyright (C) 2007, 2008  Alexander Lamaison <awl03@doc.ic.ac.uk>

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
#include "stdafx.h"
#include "resource.h"          // main symbols

#include "SftpProvider.h"      // For interface to back-end data providers
#include "UserInteraction.h"   // For implementation of ISftpConsumer
#include "Connection.h"        // For SFTP Connection container
#include "RemotePidlManager.h" // To create REMOTEPIDLs

#include <vector>
using std::vector;

// CSftpDirectory
[
	coclass,
	default(IUnknown),
	threading(apartment),
	vi_progid("Swish.SftpDirectory"),
	progid("Swish.SftpDirectory.1"),
	version(1.0),
	uuid("b816a84b-5022-11dc-9153-0090f5284f85"),
	helpstring("SftpDirectory Class")
]
class ATL_NO_VTABLE CSftpDirectory
{
public:
	CSftpDirectory() : m_fInitialised(false) {}

	/**
	 * Creates and initialises directory instance. 
	 *
	 * @param [in]  conn      SFTP connection container.
	 * @param [in]  grfFlags  Flags specifying nature of enumeration.
	 * @param [out] ppReturn  Location in which to return the IEnumIDList.
	 */
	static HRESULT MakeInstance(
		__in CConnection& conn, __in SHCONTF grfFlags,
		__deref_out CComObject<CSftpDirectory> **ppDirectory )
	{
		HRESULT hr;

		// Create instance of our folder enumerator class
		CComObject<CSftpDirectory>* pSftpDirectory;
		hr = CComObject<CSftpDirectory>::CreateInstance( &pSftpDirectory );
		ATLENSURE_RETURN_HR(SUCCEEDED(hr), hr);

		pSftpDirectory->AddRef();

		hr = pSftpDirectory->Initialize( conn, grfFlags );
		ATLASSERT(SUCCEEDED(hr));

		*ppDirectory = pSftpDirectory;

		return hr;
	}

	HRESULT Initialize( __in CConnection& conn, __in SHCONTF grfFlags );
	HRESULT Fetch( PCTSTR pszPath );
	HRESULT GetEnum( __deref_out IEnumIDList **ppEnumIDList );


private:
	BOOL m_fInitialised;
	CComPtr<ISftpProvider> m_spProvider; ///< Connection to SFTP backend.
	CComPtr<ISftpConsumer> m_spConsumer; ///< User-interaction handler.
	SHCONTF m_grfFlags; ///< Flags specifying type of file to enumerate.
	CRemotePidlManager m_PidlManager;
	vector<PITEMID_CHILD> m_vecPidls; ///< Directory contents as PIDLs.

	time_t _ConvertDate( __in DATE dateValue ) const;
};


/**
 * Copy-policy to manage copying and destruction of PITEMID_CHILD pidls.
 */
struct _CopyChildPidl
{
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
