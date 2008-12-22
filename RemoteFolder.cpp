/*  Implementation of Explorer folder handling remote files and folders.

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
#include "remotelimits.h"
#include "RemoteFolder.h"
#include "SftpDirectory.h"
#include "NewConnDialog.h"
#include "IconExtractor.h"
#include "ExplorerCallback.h" // For interaction with Explorer window
#include "UserInteraction.h"  // For implementation of ISftpConsumer
#include "HostPidl.h"
#include "ShellDataObject.h"

#include <ATLComTime.h>
#include <atlrx.h> // For regular expressions

#include <vector>
using std::vector;
using std::iterator;

void CRemoteFolder::ValidatePidl(PCUIDLIST_RELATIVE pidl)
const throw(...)
{
	if (pidl == NULL)
		AtlThrow(E_POINTER);

	if (!CRemoteItemList::IsValid(pidl))
		AtlThrow(E_INVALIDARG);
}

CLSID CRemoteFolder::GetCLSID()
const
{
	return __uuidof(this);
}

/**
 * Create and initialise new folder object for subfolder.
 */
CComPtr<IShellFolder> CRemoteFolder::CreateSubfolder(PCIDLIST_ABSOLUTE pidlRoot)
const throw(...)
{
	// Create CRemoteFolder initialised with its root PIDL
	CComPtr<IShellFolder> spFolder = CRemoteFolder::Create(pidlRoot);
	ATLENSURE_THROW(spFolder, E_NOINTERFACE);

	return spFolder;
}

/**
 * Create an instance of our Shell Folder View callback handler.
 */
CComPtr<IShellFolderViewCB> CRemoteFolder::GetFolderViewCallback()
const throw(...)
{
	return CExplorerCallback::Create(GetRootPIDL());
}

/**
 * Determine the relative order of two file objects or folders.
 *
 * @implementing CFolder
 *
 * Given their PIDLs, compare the two items and return a value
 * indicating the result of the comparison:
 * - Negative: pidl1 < pidl2
 * - Positive: pidl1 > pidl2
 * - Zero:     pidl1 == pidl2
 *
 * @todo  Take account of fCompareAllFields and fCanonical flags.
 */
int CRemoteFolder::ComparePIDLs(
	PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2, USHORT uColumn,
	bool fCompareAllFields, bool fCanonical)
const throw(...)
{
	CRemoteItemListHandle item1(pidl1);
	CRemoteItemListHandle item2(pidl2);

	try
	{
		switch (uColumn)
		{
		case 0: // Filename
				// - also default for fCompareAllFields and fCanonical
			return wcscmp(item1.GetFilename(), item2.GetFilename());
		case 1: // Owner
			return wcscmp(item1.GetOwner(), item2.GetOwner());
		case 2: // Group
			return wcscmp(item1.GetGroup(), item2.GetGroup());
		case 3: // File Permissions: drwxr-xr-x form
			return item1.GetPermissions() - item2.GetPermissions();
		case 4: // File size in bytes
			// We have to do this with a series of if-statements as the 
			// file sizes are ULONGLONGs and a subtraction may overflow
			if (item1.GetFileSize() == item2.GetFileSize())
				return 0;
			else if (item1.GetFileSize() < item2.GetFileSize())
				return -1;
			else
				return 1;
		case 5: // Last modified date
			// We have to do this with a series of if-statements as the 
			// COleDateTime object wraps a floating-point number (double)
			if (item1.GetDateModified() == item2.GetDateModified())
				return 0;
			else if (item1.GetDateModified() < item2.GetDateModified())
				return -1;
			else
				return 1;
		default:
			UNREACHABLE;
			AtlThrow(E_UNEXPECTED);
		}
	}
	catch (CRemoteItemListHandle::InvalidPidlException)
	{
		UNREACHABLE;
		AtlThrow(E_INVALIDARG);
	}
}

/**
 * Create an IEnumIDList which enumerates the items in this folder.
 *
 * @implementing IShellFolder
 *
 * @param[in]  hwndOwner     An optional window handle to be used if 
 *                           enumeration requires user input.
 * @param[in]  grfFlags      Flags specifying which types of items to include 
 *                           in the enumeration. Possible flags are from the 
 *                           @c SHCONT enum.
 * @param[out] ppEnumIDList  Location in which to return the IEnumIDList.
 *
 * @retval S_FALSE if the are no matching items to enumerate.
 */
STDMETHODIMP CRemoteFolder::EnumObjects(
	HWND hwndOwner, SHCONTF grfFlags, IEnumIDList **ppEnumIDList)
{
	METHOD_TRACE;
	ATLENSURE_RETURN_HR(ppEnumIDList, E_POINTER);

    *ppEnumIDList = NULL;

	// Create SFTP connection object for this folder using hwndOwner for UI
	CConnection conn;
	try
	{
		conn = _CreateConnectionForFolder( hwndOwner );
	}
	catchCom()

	// Get path by extracting it from chain of PIDLs starting with HOSTPIDL
	CString strPath = _ExtractPathFromPIDL(GetRootPIDL());
	ATLASSERT(!strPath.IsEmpty());

    // Create directory handler and get listing as PIDL enumeration
	try
	{
 		CSftpDirectory directory( conn, strPath );
		*ppEnumIDList = directory.GetEnum(grfFlags);
	}
	catchCom()
	
	return S_OK;
}

/*------------------------------------------------------------------------------
 * CRemoteFolder::GetUIObjectOf : IShellFolder
 * Retrieve an optional interface supported by objects in the folder.
 * This method is called when the shell is requesting extra information
 * about an object such as its icon, context menu, thumbnail image etc.
 *----------------------------------------------------------------------------*/
STDMETHODIMP CRemoteFolder::GetUIObjectOf( HWND hwndOwner, UINT cPidl,
	__in_ecount_opt(cPidl) PCUITEMID_CHILD_ARRAY aPidl, REFIID riid,
	__reserved LPUINT puReserved, __out void** ppvReturn )
{
	ATLTRACE("CRemoteFolder::GetUIObjectOf called\n");
	(void)hwndOwner; // No user input required
	(void)puReserved;

	*ppvReturn = NULL;
	HRESULT hr = E_NOINTERFACE;
	
	/*
	IContextMenu    The cidl parameter can be greater than or equal to one.
	IContextMenu2   The cidl parameter can be greater than or equal to one.
	IDataObject     The cidl parameter can be greater than or equal to one.
	IDropTarget     The cidl parameter can only be one.
	IExtractIcon    The cidl parameter can only be one.
	IQueryInfo      The cidl parameter can only be one.
	*/

	if (riid == __uuidof(IExtractIcon))
    {
		ATLTRACE("\t\tRequest: IExtractIcon\n");
		ATLASSERT(cPidl == 1);

		CComObject<CIconExtractor> *pExtractor;
		hr = CComObject<CIconExtractor>::CreateInstance(&pExtractor);
		if(SUCCEEDED(hr))
		{
			pExtractor->AddRef();

			pExtractor->Initialize(
				m_RemotePidlManager.GetFilename(aPidl[0]),
				m_RemotePidlManager.IsFolder(aPidl[0])
			);
			hr = pExtractor->QueryInterface(riid, ppvReturn);
			ATLASSERT(SUCCEEDED(hr));

			pExtractor->Release();
		}
    }
	else if (riid == __uuidof(IQueryAssociations))
	{
		ATLTRACE("\t\tRequest: IQueryAssociations\n");
		ATLASSERT(cPidl == 1);

		CComPtr<IQueryAssociations> spAssoc;
		hr = ::AssocCreate(CLSID_QueryAssociations, IID_PPV_ARGS(&spAssoc));
		ATLENSURE_RETURN_HR(SUCCEEDED(hr), hr);
		
		if (m_RemotePidlManager.IsFolder(aPidl[0]))
		{
			// Initialise default assoc provider for Folders
			hr = spAssoc->Init(
				ASSOCF_INIT_DEFAULTTOFOLDER, L"Folder", NULL, NULL);
			ATLENSURE_RETURN_HR(SUCCEEDED(hr), hr);
		}
		else
		{
			// Initialise default assoc provider for given file extension
			CString strExt = L"." + _GetFileExtensionFromPIDL(aPidl[0]);
			hr = spAssoc->Init(
				ASSOCF_INIT_DEFAULTTOSTAR, strExt, NULL, NULL);
			ATLENSURE_RETURN_HR(SUCCEEDED(hr), hr);
		}

		*ppvReturn = spAssoc.Detach();
	}
	else if (riid == __uuidof(IContextMenu))
	{
		ATLTRACE("\t\tRequest: IContextMenu\n");

		CComPtr<IShellFolder> spThisFolder = this;
		ATLENSURE_RETURN_HR(spThisFolder, E_OUTOFMEMORY);

		hr = ::CDefFolderMenu_Create2(GetRootPIDL(), hwndOwner, cPidl, aPidl,
			spThisFolder, MenuCallback, 0, NULL, (IContextMenu **)ppvReturn);
		ATLASSERT(SUCCEEDED(hr));
	}
	else if (riid == __uuidof(IDataObject))
	{
		ATLTRACE("\t\tRequest: IDataObject\n");
		hr = ::CIDLData_CreateFromIDArray(
			GetRootPIDL(), cPidl, 
			reinterpret_cast<PCUIDLIST_RELATIVE_ARRAY>(aPidl),
			(IDataObject **)ppvReturn);
		ATLASSERT(SUCCEEDED(hr));
	}
	else	
		ATLTRACE("\t\tRequest: <unknown>\n");

    return hr;
}

STDMETHODIMP CRemoteFolder::ParseDisplayName( 
	__in_opt HWND hwnd, __in_opt IBindCtx *pbc, __in PWSTR pwszDisplayName,
	__reserved ULONG *pchEaten, __deref_out_opt PIDLIST_RELATIVE *ppidl, 
	__inout_opt ULONG *pdwAttributes)
{
	ATLTRACE(__FUNCTION__" called (pwszDisplayName=%ws)\n", pwszDisplayName);
	ATLENSURE_RETURN_HR(ppidl, E_POINTER);

	ATLTRACENOTIMPL(__FUNCTION__);
}

/*------------------------------------------------------------------------------
 * CRemoteFolder::GetDisplayNameOf : IShellFolder
 * Retrieves the display name for the specified file object or subfolder.
 *----------------------------------------------------------------------------*/
STDMETHODIMP CRemoteFolder::GetDisplayNameOf( __in PCUITEMID_CHILD pidl, 
											 __in SHGDNF uFlags, 
											 __out STRRET *pName )
{
	ATLTRACE("CRemoteFolder::GetDisplayNameOf called\n");

	CString strName;

	if (uFlags & SHGDN_FORPARSING)
	{
		// We do not care if the name is relative to the folder or the
		// desktop for the parsing name - always return canonical string:
		//     sftp://username@hostname:port/path

		PIDLIST_ABSOLUTE pidlAbsolute = ::ILCombine( GetRootPIDL(), pidl );
		strName = _GetLongNameFromPIDL(pidlAbsolute, true);
		::ILFree(pidlAbsolute);
	}
	else if(uFlags & SHGDN_FORADDRESSBAR)
	{
		// We do not care if the name is relative to the folder or the
		// desktop for the parsing name - always return canonical string:
		//     sftp://username@hostname:port/path
		// unless the port is the default port in which case it is ommitted:
		//     sftp://username@hostname/path


		PIDLIST_ABSOLUTE pidlAbsolute = ::ILCombine( GetRootPIDL(), pidl );
		strName = _GetLongNameFromPIDL(pidlAbsolute, false);
		::ILFree(pidlAbsolute);
	}
	else
	{
		// We do not care if the name is relative to the folder or the
		// desktop for the parsing name - always return the filename:
		ATLASSERT(uFlags == SHGDN_NORMAL || uFlags == SHGDN_INFOLDER ||
			(uFlags & SHGDN_FOREDITING));

		strName = _GetFilenameFromPIDL(pidl);
	}

	// Store in a STRRET and return
	pName->uType = STRRET_WSTR;

	return SHStrDupW( strName, &pName->pOleStr );
}

STDMETHODIMP CRemoteFolder::SetNameOf(
	__in_opt HWND hwnd, __in PCUITEMID_CHILD pidl, __in LPCWSTR pszName,
	SHGDNF /*uFlags*/, __deref_out_opt PITEMID_CHILD *ppidlOut)
{
	if (ppidlOut)
		*ppidlOut = NULL;

	try
	{
		// Create SFTP connection object for this folder using hwndOwner for UI
		CConnection conn = _CreateConnectionForFolder( hwnd );

		// Get path by extracting it from chain of PIDLs starting with HOSTPIDL
		CString strPath = _ExtractPathFromPIDL(GetRootPIDL());
		ATLASSERT(!strPath.IsEmpty());

		// Create instance of our directory handler class
		CSftpDirectory directory( conn, strPath );

		// Rename file
		boolean fOverwritten = directory.Rename( pidl, pszName );

		// Create new PIDL from old one
		PITEMID_CHILD pidlNewFile = ::ILCloneChild(pidl);
		HRESULT hr = ::StringCchCopy(
			reinterpret_cast<PREMOTEPIDL>(pidlNewFile)->wszFilename,
			MAX_FILENAME_LENZ,
			pszName
		);
		ATLENSURE_SUCCEEDED(hr);

		// Make PIDLs absolute
		PIDLIST_ABSOLUTE pidlOld =  ::ILCombine(GetRootPIDL(), pidl);
		PIDLIST_ABSOLUTE pidlNew =  ::ILCombine(GetRootPIDL(), pidlNewFile);

		// Return new child pidl if requested else dispose of it
		if (ppidlOut)
			*ppidlOut = pidlNewFile;
		else
			::ILFree(pidlNewFile);

		// Update the shell by passing both PIDLs
		bool fIsFolder = m_RemotePidlManager.IsFolder(pidl);
		if (fOverwritten)
		{
			::SHChangeNotify(
				SHCNE_DELETE, SHCNF_IDLIST | SHCNF_FLUSH, pidlNew, NULL
			);
		}
		::SHChangeNotify(
			(fIsFolder) ? SHCNE_RENAMEFOLDER : SHCNE_RENAMEITEM,
			SHCNF_IDLIST | SHCNF_FLUSH, pidlOld, pidlNew
		);

		::ILFree(pidlOld);
		::ILFree(pidlNew);

		return S_OK;
	}
	catchCom()
}

/*------------------------------------------------------------------------------
 * CRemoteFolder::GetAttributesOf : IShellFolder
 * Returns the attributes for the items whose PIDLs are passed in.
 *----------------------------------------------------------------------------*/
STDMETHODIMP CRemoteFolder::GetAttributesOf(
	UINT cIdl,
	__in_ecount_opt( cIdl ) PCUITEMID_CHILD_ARRAY aPidl,
	__inout SFGAOF *pdwAttribs )
{
	ATLTRACE("CRemoteFolder::GetAttributesOf called\n");

	// Search through all PIDLs and check if they are all folders
	bool fAllAreFolders = true;
	for (UINT i = 0; i < cIdl; i++)
	{
		ATLASSERT(SUCCEEDED(m_RemotePidlManager.IsValid(aPidl[i])));
		if (!m_RemotePidlManager.IsFolder(aPidl[i]))
		{
			fAllAreFolders = false;
			break;
		}
	}

	// Search through all PIDLs and check if they are all 'dot' files
	bool fAllAreDotFiles = true;
	for (UINT i = 0; i < cIdl; i++)
	{
		if (m_RemotePidlManager.GetFilename(aPidl[i])[0] != _T('.'))
		{
			fAllAreDotFiles = false;
			break;
		}
	}

	DWORD dwAttribs = 0;
	if (fAllAreFolders)
	{
		dwAttribs |= SFGAO_FOLDER;
		dwAttribs |= SFGAO_HASSUBFOLDER;
	}
	if (fAllAreDotFiles)
		dwAttribs |= SFGAO_GHOSTED;

	dwAttribs |= SFGAO_CANRENAME;
	dwAttribs |= SFGAO_CANDELETE;

    *pdwAttribs &= dwAttribs;

    return S_OK;
}

/*------------------------------------------------------------------------------
 * CRemoteFolder::GetDefaultColumn : IShellFolder2
 * Gets the default sorting and display columns.
 *----------------------------------------------------------------------------*/
STDMETHODIMP CRemoteFolder::GetDefaultColumn( DWORD dwReserved, 
											 __out ULONG *pSort, 
											 __out ULONG *pDisplay )
{
	ATLTRACE("CRemoteFolder::GetDefaultColumn called\n");
	(void)dwReserved;

	// Sort and display by the filename
	*pSort = 0;
	*pDisplay = 0;

	return S_OK;
}

/*------------------------------------------------------------------------------
 * CRemoteFolder::GetDefaultColumnState : IShellFolder2
 * Returns the default state for the column specified by index.
 *----------------------------------------------------------------------------*/
STDMETHODIMP CRemoteFolder::GetDefaultColumnState( __in UINT iColumn, 
												  __out SHCOLSTATEF *pcsFlags )
{
	ATLTRACE("CRemoteFolder::GetDefaultColumnState called\n");

	switch (iColumn)
	{
	case 0: // Display name (Label)
	case 1: // Hostname
	case 2: // Username
	case 4: // Remote filesystem path
		*pcsFlags = SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT; break;
	case 3: // SFTP port
		*pcsFlags = SHCOLSTATE_TYPE_INT | SHCOLSTATE_ONBYDEFAULT; break;
	case 5: // Type
		*pcsFlags = SHCOLSTATE_TYPE_STR | SHCOLSTATE_SECONDARYUI; break;
	default:
		return E_FAIL;
	}

	return S_OK;
}

STDMETHODIMP CRemoteFolder::GetDetailsEx( __in PCUITEMID_CHILD pidl, 
										 __in const SHCOLUMNID *pscid,
										 __out VARIANT *pv )
{
	ATLTRACE("CRemoteFolder::GetDetailsEx called\n");

	// If pidl: Request is for an item detail.  Retrieve from pidl and
	//          return string
	// Else:    Request is for a column heading.  Return heading as BSTR

	// Display name (Label)
	if (IsEqualPropertyKey(*pscid, PKEY_ItemNameDisplay))
	{
		ATLTRACE("\t\tRequest: PKEY_ItemNameDisplay\n");
		
		return pidl ?
			_FillDetailsVariant( m_RemotePidlManager.GetFilename(pidl), pv ) :
			_FillDetailsVariant( _T("Name"), pv );
	}
	// Owner
	else if (IsEqualPropertyKey(*pscid, PKEY_FileOwner))
	{
		ATLTRACE("\t\tRequest: PKEY_FileOwner\n");
		
		return pidl ?
			_FillDetailsVariant( m_RemotePidlManager.GetOwner(pidl), pv ) :
			_FillDetailsVariant( _T("Owner"), pv );
	}
	// Group
	else if (IsEqualPropertyKey(*pscid, PKEY_SwishRemoteGroup))
	{
		ATLTRACE("\t\tRequest: PKEY_SwishRemoteGroup\n");
		
		return pidl ?
			_FillDetailsVariant( m_RemotePidlManager.GetGroup(pidl), pv ) :
			_FillDetailsVariant( _T("Group"), pv );
	}
	// File permissions: drwxr-xr-x form
	else if (IsEqualPropertyKey(*pscid, PKEY_SwishRemotePermissions))
	{
		ATLTRACE("\t\tRequest: PKEY_SwishRemotePermissions\n");
		
		return pidl ?
			_FillDetailsVariant( m_RemotePidlManager.GetPermissionsStr(pidl), pv ) :
			_FillDetailsVariant( _T("Permissions"), pv );
	}
	// File size in bytes
	else if (IsEqualPropertyKey(*pscid, PKEY_Size))
	{
		ATLTRACE("\t\tRequest: PKEY_Size\n");
		
		return pidl ?
			_FillUI8Variant( m_RemotePidlManager.GetFileSize(pidl), pv ) :
			_FillDetailsVariant( _T("Size"), pv );
	}
	// Last modified date
	else if (IsEqualPropertyKey(*pscid, PKEY_DateModified))
	{
		ATLTRACE("\t\tRequest: PKEY_DateModified\n");

		return pidl ?
			_FillDateVariant( m_RemotePidlManager.GetLastModified(pidl), pv ) :
			_FillDetailsVariant( _T("Last Modified"), pv );
	}
	ATLTRACE("\t\tRequest: <unknown>\n");

	// Assert unless request is one of the supported properties
	// TODO: System.FindData tiggers this
	// UNREACHABLE;

	return E_FAIL;
}

/*------------------------------------------------------------------------------
 * CRemoteFolder::GetDefaultColumnState : IShellFolder2
 * Convert column to appropriate property set ID (FMTID) and property ID (PID).
 * IMPORTANT:  This function defines which details are supported as
 * GetDetailsOf() just forwards the columnID here.  The first column that we
 * return E_FAIL for marks the end of the supported details.
 *----------------------------------------------------------------------------*/
STDMETHODIMP CRemoteFolder::MapColumnToSCID( __in UINT iColumn, 
										    __out PROPERTYKEY *pscid )
{
	ATLTRACE("CRemoteFolder::MapColumnToSCID called\n");

	switch (iColumn)
	{
	case 0: // Display name (Label)
		*pscid = PKEY_ItemNameDisplay; break;
	case 1: // Owner
		*pscid = PKEY_FileOwner; break;
	case 2: // Group
		*pscid = PKEY_SwishRemoteGroup; break;
	case 3: // File Permissions: drwxr-xr-x form
		*pscid= PKEY_SwishRemotePermissions; break;
	case 4: // File size in bytes
		*pscid = PKEY_Size; break;
	case 5: // Last modified date
		*pscid = PKEY_DateModified; break;
	default:
		return E_FAIL;
	}

	return S_OK;
}

/*------------------------------------------------------------------------------
 * CRemoteFolder::GetDetailsOf : IShellDetails
 * Returns detailed information on the items in a folder.
 * This function operates in two distinctly different ways:
 * If pidl is NULL:
 *     Retrieves the information on the view columns, i.e., the names of
 *     the columns themselves.  The index of the desired column is given
 *     in iColumn.  If this column does not exist we return E_FAIL.
 * If pidl is not NULL:
 *     Retrieves the specific item information for the given pidl and the
 *     requested column.
 * The information is returned in the SHELLDETAILS structure.
 *
 * Most of the work is delegated to GetDetailsEx by converting the column
 * index to a PKEY with MapColumnToSCID.  This function also now determines
 * what the index of the last supported detail is.
 *----------------------------------------------------------------------------*/
STDMETHODIMP CRemoteFolder::GetDetailsOf( __in_opt PCUITEMID_CHILD pidl, 
										 __in UINT iColumn, 
										 __out LPSHELLDETAILS pDetails )
{
	ATLTRACE("CRemoteFolder::GetDetailsOf called, iColumn=%u\n", iColumn);

	PROPERTYKEY pkey;

	// Lookup PKEY and use it to call GetDetailsEx
	HRESULT hr = MapColumnToSCID(iColumn, &pkey);
	if (SUCCEEDED(hr))
	{
		VARIANT pv;

		// Get details and convert VARIANT result to SHELLDETAILS for return
		hr = GetDetailsEx(pidl, &pkey, &pv);
		if (SUCCEEDED(hr))
		{
			CString strSrc;

			switch (pv.vt)
			{
			case VT_BSTR:
				strSrc = pv.bstrVal;
				::SysFreeString(pv.bstrVal);

				if(!pidl) // Header requested
					pDetails->fmt = LVCFMT_LEFT;
				break;
			case VT_UI8:
				strSrc.Format(_T("%u"), pv.ullVal);

				if(!pidl) // Header requested
					pDetails->fmt = LVCFMT_RIGHT;
				break;
			case VT_DATE:
				{
				COleDateTime date = pv.date;
				strSrc = date.Format();
				if(!pidl) // Header requested
					pDetails->fmt = LVCFMT_LEFT;
				break;
				}
			default:
				UNREACHABLE;
			}

			pDetails->str.uType = STRRET_WSTR;
			SHStrDup(strSrc, &pDetails->str.pOleStr);

			if(!pidl) // Header requested
				pDetails->cxChar = strSrc.GetLength()^2;
		}

		VariantClear( &pv );
	}

	return hr;
}

/**
 * Cracks open the @c DFM_* callback messages and dispatched them to handlers.
 */
HRESULT CRemoteFolder::OnMenuCallback(
	HWND hwnd, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	ATLTRACE(__FUNCTION__" called (uMsg=%d)\n", uMsg);
	UNREFERENCED_PARAMETER(hwnd);

	switch (uMsg)
	{
	case DFM_MERGECONTEXTMENU:
		return this->OnMergeContextMenu(
			hwnd,
			pdtobj,
			static_cast<UINT>(wParam),
			*reinterpret_cast<QCMINFO *>(lParam)
		);
	case DFM_INVOKECOMMAND:
		return this->OnInvokeCommand(
			hwnd,
			pdtobj,
			static_cast<int>(wParam),
			reinterpret_cast<PCWSTR>(lParam)
		);
	case DFM_INVOKECOMMANDEX:
		return this->OnInvokeCommandEx(
			hwnd,
			pdtobj,
			static_cast<int>(wParam),
			reinterpret_cast<PDFMICS>(lParam)
		);
	default:
		return E_NOTIMPL;
	}
}

/**
 * Handle @c DFM_MERGECONTEXTMENU callback.
 */
HRESULT CRemoteFolder::OnMergeContextMenu(
	HWND hwnd, IDataObject *pDataObj, UINT uFlags, QCMINFO& info )
{
	UNREFERENCED_PARAMETER(hwnd);
	UNREFERENCED_PARAMETER(pDataObj);
	UNREFERENCED_PARAMETER(uFlags);
	UNREFERENCED_PARAMETER(info);

	// It seems we have to return S_OK even if we do nothing else or Explorer
	// won't put Open as the default item and in the right order
	return S_OK;
}

/**
 * Handle @c DFM_INVOKECOMMAND callback.
 */
HRESULT CRemoteFolder::OnInvokeCommand(
	HWND hwnd, IDataObject *pDataObj, int idCmd, PCWSTR pszArgs )
{
	ATLTRACE(__FUNCTION__" called (hwnd=%p, pDataObj=%p, idCmd=%d, "
		"pszArgs=%ls)\n", hwnd, pDataObj, idCmd, pszArgs);

	return S_FALSE;
}

/**
 * Handle @c DFM_INVOKECOMMANDEX callback.
 */
HRESULT CRemoteFolder::OnInvokeCommandEx(
	HWND hwnd, IDataObject *pDataObj, int idCmd, PDFMICS pdfmics )
{
	ATLTRACE(__FUNCTION__" called (pDataObj=%p, idCmd=%d, pdfmics=%p)\n",
		pDataObj, idCmd, pdfmics);

	switch (idCmd)
	{
	case DFM_CMD_DELETE:
		return this->OnCmdDelete(hwnd, pDataObj);
	default:
		return S_FALSE;
	}
}

/**
 * Handle @c DFM_CMD_DELETE verb.
 */
HRESULT CRemoteFolder::OnCmdDelete( HWND hwnd, IDataObject *pDataObj )
{
	ATLTRACE(__FUNCTION__" called (hwnd=%p, pDataObj=%p)\n", hwnd, pDataObj);

	try
	{
		CShellDataObject shdo(pDataObj);
		CAbsolutePidl pidlFolder = shdo.GetParentFolder();
		ATLASSERT(::ILIsEqual(GetRootPIDL(), pidlFolder));

		// Build up a list of PIDLs for all the items to be deleted
		RemotePidls vecDeathRow;
		for (UINT i = 0; i < shdo.GetPidlCount(); i++)
		{
			ATLTRACE("PIDL path: %ls\n", _ExtractPathFromPIDL(shdo.GetFile(i)));

			CRemoteItemList pidlFile = shdo.GetRelativeFile(i);

			// May be overkill (it should always be a child) but check anyway
			// because we don't want to accidentally recursively delete the root
			// of a folder tree
			if (::ILIsChild(pidlFile) && !::ILIsEmpty(pidlFile))
			{
				CRemoteItem pidlChild = 
					static_cast<PCITEMID_CHILD>(
					static_cast<PCIDLIST_RELATIVE>(pidlFile));
				vecDeathRow.push_back(pidlChild);
			}
		}

		// Delete
		_Delete(hwnd, vecDeathRow);
	}
	catchCom()

	return S_OK;
}

/*----------------------------------------------------------------------------*/
/* --- Private functions -----------------------------------------------------*/
/*----------------------------------------------------------------------------*/

/**
 * Deletes one or more files or folders after seeking confirmation from user.
 *
 * The list of items to delete is supplied as a list of PIDLs and may contain
 * a mix of files and folders.
 *
 * If just one item is chosen, a specific confirmation message for that item is
 * shown.  If multiple items are to be deleted, a general confirmation message 
 * is displayed asking if the number of items are to be deleted.
 *
 * @param hwnd         Handle to the window used for UI.
 * @param vecDeathRow  Collection of items to be deleted as PIDLs.
 *
 * @throws AtlException if a failure occurs.
 */
void CRemoteFolder::_Delete( HWND hwnd, const RemotePidls& vecDeathRow )
{
	size_t cItems = vecDeathRow.size();

	BOOL fGoAhead = false;
	if (cItems == 1)
	{
		const CRemoteItem& pidl = vecDeathRow[0];
		fGoAhead = _ConfirmDelete(
			hwnd, CComBSTR(pidl.GetFilename()), pidl.IsFolder());
	}
	else if (cItems > 1)
	{
		fGoAhead = _ConfirmMultiDelete(hwnd, cItems);
	}
	else
	{
		UNREACHABLE;
		AtlThrow(E_UNEXPECTED);
	}

	if (fGoAhead)
		_DoDelete(hwnd, vecDeathRow);
}

/**
 * Deletes files or folders.
 *
 * The list of items to delete is supplied as a list of PIDLs and may contain
 * a mix of files and folder.
 *
 * @param hwnd         Handle to the window used for UI.
 * @param vecDeathRow  Collection of items to be deleted as PIDLs.
 *
 * @throws AtlException if a failure occurs.
 */
void CRemoteFolder::_DoDelete( HWND hwnd, const RemotePidls& vecDeathRow )
{
	if (hwnd == NULL)
		AtlThrow(E_FAIL);

	// Create SFTP connection object for this folder using hwndOwner for UI
	CConnection conn = _CreateConnectionForFolder( hwnd );

	// Get path by extracting it from chain of PIDLs starting with HOSTPIDL
	CString strPath = _ExtractPathFromPIDL(GetRootPIDL());
	ATLASSERT(!strPath.IsEmpty());

	// Create instance of our directory handler class
	CSftpDirectory directory( conn, strPath );

	// Delete each item and notify shell
	RemotePidls::const_iterator it = vecDeathRow.begin();
	while (it != vecDeathRow.end())
	{
		directory.Delete( *it );

		// Make PIDL absolute
		CAbsolutePidl pidlFull(GetRootPIDL(), *it);

		// Notify the shell
		::SHChangeNotify(
			((*it).IsFolder()) ? SHCNE_RMDIR : SHCNE_DELETE,
			SHCNF_IDLIST | SHCNF_FLUSHNOWAIT, pidlFull, NULL
		);

		it++;
	}
}

/**
 * Displays dialog seeking confirmation from user to delete a single item.
 *
 * The dialog differs depending on whether the item is a file or a folder.
 *
 * @param hwnd       Handle to the window used for UI.
 * @param bstrName   Name of the file or folder being deleted.
 * @param fIsFolder  Is the item in question a file or a folder?
 *
 * @returns  Whether confirmation was given or denied.
 */
bool CRemoteFolder::_ConfirmDelete( HWND hwnd, BSTR bstrName, bool fIsFolder )
{
	if (hwnd == NULL)
		return false;

	CString strMessage;
	if (!fIsFolder)
	{
		strMessage = L"Are you sure you want to permanently delete '";
		strMessage += bstrName;
		strMessage += L"'?";
	}
	else
	{
		strMessage = L"Are you sure you want to permanently delete the "
			L"folder '";
		strMessage += bstrName;
		strMessage += L"' and all of its contents?";
	}

	int ret = ::IsolationAwareMessageBox(hwnd, strMessage,
		(fIsFolder) ?
			L"Confirm Folder Delete" : L"Confirm File Delete", 
		MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON1);

	return (ret == IDYES);
}

/**
 * Displays dialog seeking confirmation from user to delete multiple items.
 *
 * @param hwnd    Handle to the window used for UI.
 * @param cItems  Number of items selected for deletion.
 *
 * @returns  Whether confirmation was given or denied.
 */
bool CRemoteFolder::_ConfirmMultiDelete( HWND hwnd, size_t cItems )
{
	if (hwnd == NULL)
		return false;

	CString strMessage;
	strMessage.Format(
		L"Are you sure you want to permanently delete these %d items?", cItems);

	int ret = ::IsolationAwareMessageBox(hwnd, strMessage,
		L"Confirm Multiple Item Delete",
		MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON1);

	return (ret == IDYES);
}

/*------------------------------------------------------------------------------
 * CRemoteFolder::_GetLongNameFromPIDL
 * Retrieve the long name of the file or folder from the given PIDL.
 * The long name is either the canonical form if fCanonical is set:
 *     sftp://username@hostname:port/path
 * or, if not set and if the port is the default port the reduced form:
 *     sftp://username@hostname/path
 *----------------------------------------------------------------------------*/
CString CRemoteFolder::_GetLongNameFromPIDL( PCIDLIST_ABSOLUTE pidl, 
                                             BOOL fCanonical )
{
	ATLTRACE("CRemoteFolder::_GetLongNameFromPIDL called\n");
	ATLASSERT(SUCCEEDED(m_RemotePidlManager.IsValid(pidl, PM_LAST_PIDL)));

	PCUIDLIST_RELATIVE pidlHost = m_HostPidlManager.FindHostPidl(pidl);
	ATLASSERT(pidlHost);
	ATLASSERT(SUCCEEDED(m_HostPidlManager.IsValid(pidlHost, PM_THIS_PIDL)));

	// Construct string from info in PIDL
	CString strName = _T("sftp://");
	strName += m_HostPidlManager.GetUser(pidlHost);
	strName += _T("@");
	strName += m_HostPidlManager.GetHost(pidlHost);
	if (fCanonical || 
	   (m_HostPidlManager.GetPort(pidlHost) != SFTP_DEFAULT_PORT))
	{
		strName += _T(":");
		strName.AppendFormat( _T("%u"), m_HostPidlManager.GetPort(pidlHost) );
	}
	strName += _T("/");
	strName += _ExtractPathFromPIDL(pidl);

	ATLASSERT( strName.GetLength() <= MAX_CANONICAL_LEN );

	return strName;
}

/**
 * Retrieve the full path of the file on the remote system from the given PIDL.
 */
CString CRemoteFolder::_ExtractPathFromPIDL( PCIDLIST_ABSOLUTE pidl )
{
	CString strPath;

	// Find HOSTPIDL part of pidl and use it to get 'root' path of connection
	// (by root we mean the path specified by the user when they added the
	// connection to Explorer, rather than the root of the server's filesystem)
	PCUIDLIST_RELATIVE pidlHost = m_HostPidlManager.FindHostPidl(pidl);
	ATLASSERT(pidlHost);
	ATLASSERT(SUCCEEDED(m_HostPidlManager.IsValid(pidlHost, PM_THIS_PIDL)));
	strPath = m_HostPidlManager.GetPath(pidlHost);

	// Walk over REMOTEPIDLs and append each filename to form the path
	PCUIDLIST_RELATIVE pidlRemote = m_HostPidlManager.GetNextItem(pidlHost);
	while (pidlRemote != NULL)
	{
		if (SUCCEEDED(m_RemotePidlManager.IsValid(pidlRemote, PM_THIS_PIDL)))
		{
			strPath += _T("/");
			strPath += m_RemotePidlManager.GetFilename(pidlRemote);
		}
		pidlRemote = m_RemotePidlManager.GetNextItem(pidlRemote);
	}

	ATLASSERT( strPath.GetLength() <= MAX_PATH_LEN );

	return strPath;
}

CString CRemoteFolder::_GetFilenameFromPIDL(
	CRemoteItemHandle pidl, bool fIncludeExtension)
{
	METHOD_TRACE;
	ATLASSERT(pidl.IsValid());

	// Extract filename from REMOTEPIDL
	CString strName;
	if (fIncludeExtension || pidl.GetFilename()[0] == L'.')
	{
		strName = pidl.GetFilename();
	}
	else
	{
		CString strFilename(pidl.GetFilename());
		int nLimit = strFilename.ReverseFind(L'.');
		if (nLimit < 0)
			nLimit = strFilename.GetLength();

		strName = CString(strFilename, nLimit);
	}

	ATLASSERT( strName.GetLength() <= MAX_PATH_LEN );

	return strName;
}

/*------------------------------------------------------------------------------
 * CRemoteFolder::_FillDetailsVariant
 * Initialise the VARIANT whose pointer is passed and fill with string data.
 * The string data can be passed in as a wchar array or a CString.  We allocate
 * a new BSTR and store it in the VARIANT.
 *----------------------------------------------------------------------------*/
HRESULT CRemoteFolder::_FillDetailsVariant( __in PCWSTR szDetail,
										   __out VARIANT *pv )
{
	ATLTRACE("CRemoteFolder::_FillDetailsVariant called\n");

	::VariantInit( pv );
	pv->vt = VT_BSTR;
	pv->bstrVal = ::SysAllocString( szDetail );

	return pv->bstrVal ? S_OK : E_OUTOFMEMORY;
}

/*------------------------------------------------------------------------------
 * CRemoteFolder::_FillDateVariant
 * Initialise the VARIANT whose pointer is passed and fill with date info.
 *----------------------------------------------------------------------------*/
HRESULT CRemoteFolder::_FillDateVariant( __in DATE date, __out VARIANT *pv )
{
	ATLTRACE("CRemoteFolder::_FillDateVariant called\n");

	::VariantInit( pv );
	pv->vt = VT_DATE;
	pv->date = date;

	return S_OK;
}

/*------------------------------------------------------------------------------
 * CRemoteFolder::_FillDateVariant
 * Initialise the VARIANT whose pointer is passed and fill with 64-bit unsigned.
 *----------------------------------------------------------------------------*/
HRESULT CRemoteFolder::_FillUI8Variant( __in ULONGLONG ull, __out VARIANT *pv )
{
	ATLTRACE("CRemoteFolder::_FillUI8Variant called\n");

	::VariantInit( pv );
	pv->vt = VT_UI8;
	pv->ullVal = ull;

	return S_OK; // TODO: return success of VariantInit
}

/**
 * Extracts the extension part of the filename from the given PIDL.
 * The extension does not include the dot.  If the filename has no extension
 * an empty string is returned.
 *
 * @param  The PIDL to get the file extension of.
 */
CString CRemoteFolder::_GetFileExtensionFromPIDL( PCUITEMID_CHILD pidl )
{
	ATLASSERT(CRemoteItemHandle::IsValid(pidl));

	CString strFilename = _GetFilenameFromPIDL(pidl);

	// Build regex to grap
	CAtlRegExp<> re;
	ATLVERIFY( re.Parse(_T("\\.?{[^\\.]*}$")) == REPARSE_ERROR_OK );

	// Run regex agains filename and extract matched section
	CAtlREMatchContext<> match;
	ATLVERIFY( re.Match(strFilename, &match) );
	ATLASSERT( match.m_uNumGroups == 1 );
	
	const TCHAR *szStart; const TCHAR *szEnd;
	match.GetMatch(0, &szStart, &szEnd);
	ptrdiff_t cchLength = szEnd - szStart;

	CString strExtension(szStart, (UINT)cchLength);
	return strExtension;
}

/**
 * Gets connection for given SFTP session parameters.
 */
CConnection CRemoteFolder::_GetConnection(
	HWND hwnd, PCWSTR szHost, PCWSTR szUser, UINT uPort ) throw(...)
{
	HRESULT hr;

	// Create SFTP Consumer to pass to SftpProvider (used for password reqs etc)
	CComPtr<ISftpConsumer> spConsumer;
	hr = CUserInteraction::MakeInstance( hwnd, &spConsumer );
	ATLENSURE_SUCCEEDED(hr);

	// Get SFTP Provider from session pool
	CPool pool;
	CComPtr<ISftpProvider> spProvider = pool.GetSession(
		spConsumer, CComBSTR(szHost), CComBSTR(szUser), uPort);

	// Pack both ends of connection into object
	CConnection conn;
	conn.spProvider = spProvider;
	conn.spConsumer = spConsumer;

	return conn;
}

/**
 * Creates a CConnection object holding the two parts of an SFTP connection.
 *
 * The two parts are the provider (SFTP backend) and consumer (user interaction
 * callback).  The connection is created from the information stored in this
 * folder's PIDL, @c m_pidl, and the window handle to be used as the owner
 * window for any user interaction. This window handle cannot be NULL (in order
 * to enforce good UI etiquette - we should attempt to interact with the user
 * if Explorer isn't expecting us to).  If it is, this function will throw
 * an exception.
 *
 * @param hwndUserInteraction  A handle to the window which should be used
 *                             as the parent window for any user interaction.
 * @throws ATL exceptions on failure.
 */
CConnection CRemoteFolder::_CreateConnectionForFolder(
	HWND hwndUserInteraction )
{
	if (hwndUserInteraction == NULL)
		AtlThrow(E_FAIL);

	// Find HOSTPIDL part of this folder's absolute pidl to extract server info
	CHostItemListHandle pidlHost(CHostItemListHandle(GetRootPIDL()).FindHostPidl());
	ATLASSERT(pidlHost.IsValid());

	// Extract connection info from PIDL
	CString strUser, strHost, strPath;
	USHORT uPort;
	strHost = pidlHost.GetHost();
	uPort = pidlHost.GetPort();
	strUser = pidlHost.GetUser();
	ATLASSERT(!strUser.IsEmpty());
	ATLASSERT(!strHost.IsEmpty());

	// Return connection from session pool
	return _GetConnection(hwndUserInteraction, strHost, strUser, uPort);
}
