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

#include "stdafx.h"
#include "SftpDirectory.h"
#include "RemotePidl.h"

#define S_IFMT     0170000 /* type of file */
#define S_IFDIR    0040000 /* directory 'd' */
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)

/**
 * Fetches directory listing from the server as an enumeration.
 *
 * This listing is cached as a collection of PIDLs in the m_vecPidls member.
 *
 * @param grfFlags  Flags specifying the types of files to include in the
 *                  enumeration.
 */
HRESULT CSftpDirectory::_Fetch( SHCONTF grfFlags )
{
	HRESULT hr;

	// Interpret supported SHCONTF flags
	bool fIncludeFolders = (grfFlags & SHCONTF_FOLDERS) != 0;
	bool fIncludeNonFolders = (grfFlags & SHCONTF_NONFOLDERS) != 0;
	bool fIncludeHidden = (grfFlags & SHCONTF_INCLUDEHIDDEN) != 0;

	// Get listing enumerator
	CComPtr<IEnumListing> spEnum;
	hr = m_connection.spProvider->GetListing(CComBSTR(m_strDirectory), &spEnum);
	if (SUCCEEDED(hr))
	{
		m_vecPidls.clear();

		do {
			Listing lt;
			ULONG cElementsFetched = 0;
			hr = spEnum->Next(1, &lt, &cElementsFetched);
			if (hr == S_OK)
			{
				if (!fIncludeFolders && S_ISDIR(lt.uPermissions))
					continue;
				if (!fIncludeNonFolders && !S_ISDIR(lt.uPermissions))
					continue;
				if (!fIncludeHidden && (lt.bstrFilename[0] == OLECHAR('.')))
					continue;

				PITEMID_CHILD pidl;
				hr = m_PidlManager.Create(
					CString(lt.bstrFilename),
					CString(lt.bstrOwner),
					CString(lt.bstrGroup),
					lt.uPermissions,
					lt.uSize,
					lt.dateModified,
					S_ISDIR(lt.uPermissions),
					&pidl
				);
				ATLASSERT(SUCCEEDED(hr));
				if (SUCCEEDED(hr))
				{
					m_vecPidls.push_back(pidl);
				}

				/*::SysFreeString(lt.bstrFilename);
				::SysFreeString(lt.bstrGroup);
				::SysFreeString(lt.bstrOwner);*/
			}
		} while (hr == S_OK);
	}

	return hr;
}

/**
 * Retrieve an IEnumIDList to enumerate directory contents.
 *
 * This function returns an enumerator which can be used to iterate through
 * the contents of the directory as a series of PIDLs.  This contents
 * must have been previously obtaqined from the server by a call to Fetch().
 *
 * @param [out]  The location in which to return the IEnumIDList.
 */
HRESULT CSftpDirectory::GetEnum(
	IEnumIDList **ppEnumIDList, __in SHCONTF grfFlags )
{
	typedef CComEnumOnSTL<IEnumIDList, &__uuidof(IEnumIDList), PITEMID_CHILD,
	                      _CopyChildPidl, vector<CChildPidl> >
	        CComEnumIDList;

	HRESULT hr;

	// Fetch listing and cache in m_vecPidls
	hr = _Fetch(grfFlags);
	ATLENSURE_RETURN_HR(SUCCEEDED(hr), hr);

	// Create copy of our vector of PIDL and put into an AddReffed 'holder'
	CComPidlHolder *pHolder = NULL;
	hr = pHolder->CreateInstance(&pHolder);
	ATLASSERT(SUCCEEDED(hr));
	if (SUCCEEDED(hr))
	{
		pHolder->AddRef();

		hr = pHolder->Copy(m_vecPidls);
		ATLENSURE_RETURN_HR(SUCCEEDED(hr), hr);

		// Create enumerator
		CComObject<CComEnumIDList> *pEnumIDList;
		hr = pEnumIDList->CreateInstance(&pEnumIDList);
		ATLASSERT(SUCCEEDED(hr));
		if (SUCCEEDED(hr))
		{
			pEnumIDList->AddRef();

			// Give enumerator back-reference to holder of our copied collection
			hr = pEnumIDList->Init( pHolder->GetUnknown(), pHolder->m_coll );
			ATLASSERT(SUCCEEDED(hr));
			if (SUCCEEDED(hr))
			{
				hr = pEnumIDList->QueryInterface(ppEnumIDList);
				ATLASSERT(SUCCEEDED(hr));
			}

			pEnumIDList->Release();
		}
		pHolder->Release();
	}

	return hr;
}

bool CSftpDirectory::Rename(
	__in PCUITEMID_CHILD pidlOldFile, PCTSTR pszNewFilename )
{
	CString strOldFilename = m_PidlManager.GetFilename(pidlOldFile);

	VARIANT_BOOL fWasTargetOverwritten = VARIANT_FALSE;
	HRESULT hr = m_connection.spProvider->Rename(
		CComBSTR(m_strDirectory+strOldFilename),
		CComBSTR(m_strDirectory+pszNewFilename),
		&fWasTargetOverwritten
	);
	if (hr != S_OK)
		AtlThrow(hr);

	return (fWasTargetOverwritten == VARIANT_TRUE);
}

void CSftpDirectory::Delete( __in PCUITEMID_CHILD pidlFile )
{
	CRemoteChildPidl pidl(pidlFile);
	CComBSTR strPath(m_strDirectory + pidl.GetFilename());
	
	HRESULT hr;
	if (pidl.IsFolder())
		hr = m_connection.spProvider->DeleteDirectory(strPath);
	else
		hr = m_connection.spProvider->Delete(strPath);
	if (hr != S_OK)
		AtlThrow(hr);
}

// CSftpDirectory
