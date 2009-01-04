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

#include "stdafx.h"
#include "SftpDirectory.h"

#include "SftpDataObject.h"

#define S_IFMT     0170000 /* type of file */
#define S_IFDIR    0040000 /* directory 'd' */
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)

/**
 * Creates and initialises directory instance. 
 *
 * @param pidlDirectory  PIDL to the directory this object represents.  Must
 *                       start at or before a HostItemId.
 * @param conn           SFTP connection container.
 */
CSftpDirectory::CSftpDirectory(
	CHostItemAbsoluteHandle pidlDirectory, CConnection& conn) 
throw(...) : 
	m_connection(conn), 
	m_pidlDirectory(pidlDirectory), 
	// Trim trailing slashes and append single slash
	m_strDirectory(pidlDirectory.GetFullPath().TrimRight(L'/')+L'/')
{
	ATLASSERT(pidlDirectory.FindHostPidl().IsValid()); // Must be a Host PIDL
}

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

				try
				{
					CRemoteItem pidl(
						CString(lt.bstrFilename),
						S_ISDIR(lt.uPermissions),
						CString(lt.bstrOwner),
						CString(lt.bstrGroup),
						false,
						lt.uPermissions,
						lt.uSize,
						lt.dateModified);

					m_vecPidls.push_back(pidl);
				}
				catchCom()

				/*::SysFreeString(lt.bstrFilename);
				::SysFreeString(lt.bstrGroup);
				::SysFreeString(lt.bstrOwner);*/
			}
		} while (hr == S_OK);
	}

	return hr;
}

/**
 * Retrieve an IEnumIDList to enumerate this directory's contents.
 *
 * This function returns an enumerator which can be used to iterate through
 * the contents of this directory as a series of PIDLs.  This listing is a
 * @b copy of the one obtained from the server and will not update to reflect
 * changes.  In order to obtain an up-to-date listing, this function must be 
 * called again to get a new enumerator.
 *
 * @param grfFlags  Flags specifying nature of files to fetch.
 *
 * @returns  Smart pointer to the IEnumIDList.
 * @throws  CAtlException if an error occurs.
 */
CComPtr<IEnumIDList> CSftpDirectory::GetEnum(SHCONTF grfFlags)
{
	typedef CComEnumOnSTL<IEnumIDList, &__uuidof(IEnumIDList), PITEMID_CHILD,
	                      _CopyChildPidl, vector<CChildPidl> >
	        CComEnumIDList;

	HRESULT hr;

	// Fetch listing and cache in m_vecPidls
	hr = _Fetch(grfFlags);
	if (FAILED(hr))
		AtlThrow(hr);

	// Create holder for the collection of PIDLs the enumerator will enumerate
	CComPidlHolder *pHolder;
	hr = pHolder->CreateInstance(&pHolder);
	ATLENSURE_SUCCEEDED(hr);

	// Copy out vector of PIDLs into the holder
	CComPtr<CComPidlHolder> spHolder = pHolder;
	hr = spHolder->Copy(m_vecPidls);
	ATLENSURE_SUCCEEDED(hr);

	// Create enumerator
	CComObject<CComEnumIDList> *pEnumIDList;
	hr = pEnumIDList->CreateInstance(&pEnumIDList);
	ATLENSURE_SUCCEEDED(hr);

	// Give enumerator back-reference to holder of our copied collection
	hr = pEnumIDList->Init( spHolder->GetUnknown(), spHolder->m_coll );
	ATLENSURE_SUCCEEDED(hr);

	return pEnumIDList;
}

/**
 * Get instance of CSftpDirectory for a subdirectory of this directory.
 *
 * @param pidl  Child PIDL of directory within this directory.
 *
 * @returns  Instance of the subdirectory.
 * @throws  CAtlException if error.
 */
CSftpDirectory CSftpDirectory::GetSubdirectory(CRemoteItemHandle pidl)
throw(...)
{
	if (!pidl.IsFolder())
		AtlThrow(E_INVALIDARG);

	return CSftpDirectory(CAbsolutePidl(m_pidlDirectory, pidl), m_connection);
}

/**
 * Get IStream interface to the remote file specified by the given PIDL.
 *
 * This 'file' may also be a directory but the IStream does not give access
 * to its subitems.
 *
 * @param pidl  Child PIDL of item within this directory.
 *
 * @returns  Smart pointer of an IStream interface to the file.
 * @throws  CAtlException if error.
 */
CComPtr<IStream> CSftpDirectory::GetFile(CRemoteItemHandle pidl)
throw(...)
{
	CComPtr<IStream> spStream;
	m_connection.spProvider->GetFile(
		CComBSTR(m_strDirectory + pidl.GetFilename()), &spStream);
	return spStream;
}

/**
 * Create IDataObject instance for the files represented by the array of PIDLs.
 *
 * The DataObject represents the files as a CFSTR_SHELLIDLIST format (PIDLs)
 * and as the CFSTR_FILEDESCRIPTOR/CFSTR_FILECONTENTS pair (IStream).
 *
 * @param cPidl  Number of items in the array.
 * @param aPidl  Array of child PIDLs.
 *
 * @returns  Smart pointer to the IDataObject.
 * @throws  CAtlException if error.
 */
CComPtr<IDataObject> CSftpDirectory::CreateDataObjectFor(
	UINT cPidl, PCUITEMID_CHILD_ARRAY aPidl)
throw(...)
{
	CComPtr<IDataObject> spDo = 
		CDataObject::Create(m_connection, m_pidlDirectory, cPidl, aPidl);
	ATLASSERT(spDo);

	return spDo;
}

bool CSftpDirectory::Rename(
	CRemoteItemHandle pidlOldFile, PCWSTR pwszNewFilename)
throw(...)
{
	VARIANT_BOOL fWasTargetOverwritten = VARIANT_FALSE;

	HRESULT hr = m_connection.spProvider->Rename(
		CComBSTR(m_strDirectory+pidlOldFile.GetFilename()),
		CComBSTR(m_strDirectory+pwszNewFilename),
		&fWasTargetOverwritten
	);
	if (hr != S_OK)
		AtlThrow(hr);

	return (fWasTargetOverwritten == VARIANT_TRUE);
}

void CSftpDirectory::Delete(CRemoteItemHandle pidl)
throw(...)
{
	CComBSTR strPath(m_strDirectory + pidl.GetFilename());
	
	HRESULT hr;
	if (pidl.IsFolder())
		hr = m_connection.spProvider->DeleteDirectory(strPath);
	else
		hr = m_connection.spProvider->Delete(strPath);
	if (hr != S_OK)
		AtlThrow(hr);
}