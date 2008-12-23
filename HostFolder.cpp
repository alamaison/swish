/*  SFTP connections Explorer folder implementation.

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
#include "HostFolder.h"
#include "RemoteFolder.h"
#include "ConnCopyPolicy.h"
#include "ExplorerCallback.h" // For interaction with Explorer window
#include "HostPidl.h"

#include <atlrx.h> // For regular expressions

#include <vector>
using std::vector;
using std::iterator;

void CHostFolder::ValidatePidl(PCUIDLIST_RELATIVE pidl)
const throw(...)
{
	if (pidl == NULL)
		AtlThrow(E_POINTER);

	if (!CHostItemList::IsValid(pidl))
		AtlThrow(E_INVALIDARG);
}

CLSID CHostFolder::GetCLSID()
const
{
	return __uuidof(this);
}

/**
 * Create and initialise new folder object for subfolder.
 */
CComPtr<IShellFolder> CHostFolder::CreateSubfolder(PCIDLIST_ABSOLUTE pidlRoot)
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
CComPtr<IShellFolderViewCB> CHostFolder::GetFolderViewCallback()
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
int CHostFolder::ComparePIDLs(
	PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2, USHORT uColumn,
	bool fCompareAllFields, bool fCanonical)
const throw(...)
{
	CHostItemListHandle item1(pidl1);
	CHostItemListHandle item2(pidl2);

	try
	{
		switch (uColumn)
		{
		case 0: // Display name (Label)
				// - also default for fCompareAllFields and fCanonical
			return wcscmp(item1.GetLabel(), item2.GetLabel());
		case 1: // Hostname
			return wcscmp(item1.GetHost(), item2.GetHost());
		case 2: // Username
			return wcscmp(item1.GetUser(), item2.GetUser());
		case 4: // Remote filesystem path
			return wcscmp(item1.GetPath(), item2.GetPath());
		case 3: // SFTP port
			return item1.GetPort() - item2.GetPort();
		case 5: // Type
			return 0;
		default:
			UNREACHABLE;
			AtlThrow(E_UNEXPECTED);
		}
	}
	catch (CHostItemListHandle::InvalidPidlException)
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
STDMETHODIMP CHostFolder::EnumObjects(
	HWND hwndOwner, SHCONTF grfFlags, IEnumIDList **ppEnumIDList)
{
	METHOD_TRACE;
	ATLENSURE_RETURN_HR(ppEnumIDList, E_POINTER);
	UNREFERENCED_PARAMETER(hwndOwner); // No UI required to access registry

	HRESULT hr;

    *ppEnumIDList = NULL;

	// This folder only contains folders
	if (!(grfFlags & SHCONTF_FOLDERS) ||
		(grfFlags & (SHCONTF_NETPRINTERSRCH | SHCONTF_SHAREABLE)))
		return S_FALSE;

	// Load connections from HKCU\Software\Swish\Connections
	hr = _LoadConnectionsFromRegistry();
	ATLENSURE_RETURN_HR(SUCCEEDED(hr), hr);

    // Create an enumerator with CComEnumOnSTL<> and our copy policy class.
	CComObject<CEnumIDListImpl>* pEnum;
    hr = CComObject<CEnumIDListImpl>::CreateInstance( &pEnum );
	ATLENSURE_RETURN_HR(SUCCEEDED(hr), hr);
    pEnum->AddRef();

    // Init the enumerator.  Init() will AddRef() our IUnknown (obtained with
    // GetUnknown()) so this object will stay alive as long as the enumerator 
    // needs access to the vector m_vecConnData.
    hr = pEnum->Init( GetUnknown(), m_vecConnData );

    // Return an IEnumIDList interface to the caller.
    if (SUCCEEDED(hr))
        hr = pEnum->QueryInterface(__uuidof(IEnumIDList), (void**)ppEnumIDList);

    pEnum->Release();
    return hr;
}

/*------------------------------------------------------------------------------
 * CHostFolder::GetUIObjectOf : IShellFolder
 * Retrieve an optional interface supported by objects in the folder.
 * This method is called when the shell is requesting extra information
 * about an object such as its icon, context menu, thumbnail image etc.
 *----------------------------------------------------------------------------*/
STDMETHODIMP CHostFolder::GetUIObjectOf( HWND hwndOwner, UINT cPidl,
	__in_ecount_opt(cPidl) PCUITEMID_CHILD_ARRAY aPidl, REFIID riid,
	__reserved LPUINT puReserved, __out void** ppvReturn )
{
	ATLTRACE("CHostFolder::GetUIObjectOf called\n");
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

	if (riid == IID_IExtractIconW)
    {
		ATLTRACE("\t\tRequest: IID_IExtractIconW\n");
		ATLASSERT( cPidl == 1 ); // Only one file 'selected'

		hr = QueryInterface(riid, ppvReturn);
    }
	else if (riid == __uuidof(IQueryAssociations))
	{
		ATLTRACE("\t\tRequest: IQueryAssociations\n");
		ATLASSERT(cPidl == 1);

		CComPtr<IQueryAssociations> spAssoc;
		hr = ::AssocCreate(CLSID_QueryAssociations, IID_PPV_ARGS(&spAssoc));
		ATLENSURE_RETURN_HR(SUCCEEDED(hr), hr);
		
		// Initialise default assoc provider for Folders
		hr = spAssoc->Init(0, L"Folder", NULL, NULL);
		ATLENSURE_RETURN_HR(SUCCEEDED(hr), hr);

		*ppvReturn = spAssoc.Detach();
	}
	else if (riid == __uuidof(IContextMenu))
	{
		ATLTRACE("\t\tRequest: IContextMenu\n");

		// Get keys associated with filetype from registry.
		//
		// This article says that we don't need to specify the keys:
		// http://groups.google.com/group/microsoft.public.platformsdk.shell/
		// browse_thread/thread/6f07525eaddea29d/
		// but we do for the context menu to appear in versions of Windows 
		// earlier than Vista.
		HKEY *aKeys; UINT cKeys;
		ATLENSURE_RETURN_HR(SUCCEEDED(
			_GetAssocRegistryKeys(&cKeys, &aKeys)),
			E_UNEXPECTED  // Might fail if registry is corrupted
		);

		CComPtr<IShellFolder> spThisFolder = this;
		ATLENSURE_RETURN_HR(spThisFolder, E_OUTOFMEMORY);

		// Create default context menu from list of PIDLs
		hr = ::CDefFolderMenu_Create2(
			GetRootPIDL(), hwndOwner, cPidl, aPidl, spThisFolder, 
			MenuCallback, cKeys, aKeys, (IContextMenu **)ppvReturn);

		ATLASSERT(SUCCEEDED(hr));
	}
	else if (riid == __uuidof(IDataObject))
	{
		ATLTRACE("\t\tRequest: IDataObject\n");

		// A DataObject is required in order for the call to 
		// CDefFolderMenu_Create2 (above) to succeed on versions of Windows
		// earlier than Vista

		hr = ::CIDLData_CreateFromIDArray(
			GetRootPIDL(), cPidl, 
			reinterpret_cast<PCUIDLIST_RELATIVE_ARRAY>(aPidl),
			(IDataObject **)ppvReturn);
		ATLASSERT(SUCCEEDED(hr));
	}

    return hr;
}

STDMETHODIMP CHostFolder::ParseDisplayName( 
	__in_opt HWND hwnd, __in_opt IBindCtx *pbc, __in PWSTR pwszDisplayName,
	__reserved ULONG *pchEaten, __deref_out_opt PIDLIST_RELATIVE *ppidl, 
	__inout_opt ULONG *pdwAttributes)
{
	ATLTRACE(__FUNCTION__" called (pwszDisplayName=%ws)\n", pwszDisplayName);
	ATLENSURE_RETURN_HR(ppidl, E_POINTER);

	ATLTRACENOTIMPL(__FUNCTION__);
}

/*------------------------------------------------------------------------------
 * CHostFolder::GetDisplayNameOf : IShellFolder
 * Retrieves the display name for the specified file object or subfolder.
 *----------------------------------------------------------------------------*/
STDMETHODIMP CHostFolder::GetDisplayNameOf( __in PCUITEMID_CHILD pidl, 
											 __in SHGDNF uFlags, 
											 __out STRRET *pName )
{
	ATLTRACE("CHostFolder::GetDisplayNameOf called\n");

	CString strName;

	if (uFlags & SHGDN_FORPARSING)
	{
		// We do not care if the name is relative to the folder or the
		// desktop for the parsing name - always return canonical string:
		//     sftp://username@hostname:port/path

		// TODO:  if !SHGDN_INFOLDER the pidl may not be single-level
		// so we should always seek to the last pidl before extracting info

		strName = _GetLongNameFromPIDL(pidl, true);
	}
	else if(uFlags & SHGDN_FORADDRESSBAR)
	{
		// We do not care if the name is relative to the folder or the
		// desktop for the parsing name - always return canonical string:
		//     sftp://username@hostname:port/path
		// unless the port is the default port in which case it is ommitted:
		//     sftp://username@hostname/path

		strName = _GetLongNameFromPIDL(pidl, false);
	}
	else
	{
		// We do not care if the name is relative to the folder or the
		// desktop for the parsing name - always return the label:
		ATLASSERT(uFlags == SHGDN_NORMAL || uFlags == SHGDN_INFOLDER ||
			(uFlags & SHGDN_FOREDITING));

		strName = _GetLabelFromPIDL(pidl);
	}

	// Store in a STRRET and return
	pName->uType = STRRET_WSTR;

	return SHStrDupW( strName, &pName->pOleStr );
}

/*------------------------------------------------------------------------------
 * CHostFolder::GetAttributesOf : IShellFolder
 * Returns the attributes for the items whose PIDLs are passed in.
 *----------------------------------------------------------------------------*/
STDMETHODIMP CHostFolder::GetAttributesOf(
	UINT cIdl,
	__in_ecount_opt( cIdl ) PCUITEMID_CHILD_ARRAY aPidl,
	__inout SFGAOF *pdwAttribs )
{
	ATLTRACE("CHostFolder::GetAttributesOf called\n");
	(void)aPidl; // All items are folders. No need to check PIDL.
	(void)cIdl;

	DWORD dwAttribs = 0;
    dwAttribs |= SFGAO_FOLDER;
    dwAttribs |= SFGAO_HASSUBFOLDER;
    *pdwAttribs &= dwAttribs;

    return S_OK;
}

/*------------------------------------------------------------------------------
 * CHostFolder::GetDefaultColumn : IShellFolder2
 * Gets the default sorting and display columns.
 *----------------------------------------------------------------------------*/
STDMETHODIMP CHostFolder::GetDefaultColumn( DWORD dwReserved, 
											__out ULONG *pSort, 
											__out ULONG *pDisplay )
{
	ATLTRACE("CHostFolder::GetDefaultColumn called\n");
	(void)dwReserved;

	// Sort and display by the label (friendly display name)
	*pSort = 0;
	*pDisplay = 0;

	return S_OK;
}

/*------------------------------------------------------------------------------
 * CHostFolder::GetDefaultColumnState : IShellFolder2
 * Returns the default state for the column specified by index.
 *----------------------------------------------------------------------------*/
STDMETHODIMP CHostFolder::GetDefaultColumnState( __in UINT iColumn, 
												 __out SHCOLSTATEF *pcsFlags )
{
	ATLTRACE("CHostFolder::GetDefaultColumnState called\n");

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

STDMETHODIMP CHostFolder::GetDetailsEx( __in PCUITEMID_CHILD pidl, 
										 __in const SHCOLUMNID *pscid,
										 __out VARIANT *pv )
{
	ATLTRACE("CHostFolder::GetDetailsEx called\n");

	// If pidl: Request is for an item detail.  Retrieve from pidl and
	//          return string
	// Else:    Request is for a column heading.  Return heading as BSTR

	// Display name (Label)
	if (IsEqualPropertyKey(*pscid, PKEY_ItemNameDisplay))
	{
		ATLTRACE("\t\tRequest: PKEY_ItemNameDisplay\n");
		
		return pidl ?
			_FillDetailsVariant( m_HostPidlManager.GetLabel(pidl), pv ) :
			_FillDetailsVariant( _T("Name"), pv );
	}
	// Hostname
	else if (IsEqualPropertyKey(*pscid, PKEY_ComputerName))
	{
		ATLTRACE("\t\tRequest: PKEY_ComputerName\n");

		return pidl ?
			_FillDetailsVariant( m_HostPidlManager.GetHost(pidl), pv ) :
			_FillDetailsVariant( _T("Host"), pv );
	}
	// Username
	else if (IsEqualPropertyKey(*pscid, PKEY_SwishHostUser))
	{
		ATLTRACE("\t\tRequest: PKEY_SwishHostUser\n");
		
		return pidl ?
			_FillDetailsVariant( m_HostPidlManager.GetUser(pidl), pv ) :
			_FillDetailsVariant( _T("Username"), pv );
	}
	// SFTP port
	else if (IsEqualPropertyKey(*pscid, PKEY_SwishHostPort))
	{
		ATLTRACE("\t\tRequest: PKEY_SwishHostPort\n");
		
		return pidl ?
			_FillDetailsVariant( m_HostPidlManager.GetPortStr(pidl), pv ) :
			_FillDetailsVariant( _T("Port"), pv );
	}
	// Remote filesystem path
	else if (IsEqualPropertyKey(*pscid, PKEY_ItemPathDisplay))
	{
		ATLTRACE("\t\tRequest: PKEY_ItemPathDisplay\n");

		return pidl ?
			_FillDetailsVariant( m_HostPidlManager.GetPath(pidl), pv ) :
			_FillDetailsVariant( _T("Remote Path"), pv );
	}
	// Type: always 'Network Drive'
	else if (IsEqualPropertyKey(*pscid, PKEY_ItemType))
	{
		ATLTRACE("\t\tRequest: PKEY_ItemType\n");

		return pidl ?
			_FillDetailsVariant( _T("Network Drive"), pv ) :
			_FillDetailsVariant( _T("Type"), pv );
	}

	ATLTRACE("\t\tRequest: <unknown>\n");

	// Assert unless request is one of the supported properties
	// TODO: System.FindData tiggers this
	// UNREACHABLE;

	return E_FAIL;
}

/*------------------------------------------------------------------------------
 * CHostFolder::GetDefaultColumnState : IShellFolder2
 * Convert column to appropriate property set ID (FMTID) and property ID (PID).
 * IMPORTANT:  This function defines which details are supported as
 * GetDetailsOf() just forwards the columnID here.  The first column that we
 * return E_FAIL for marks the end of the supported details.
 *----------------------------------------------------------------------------*/
STDMETHODIMP CHostFolder::MapColumnToSCID( __in UINT iColumn, 
										    __out PROPERTYKEY *pscid )
{
	ATLTRACE("CHostFolder::MapColumnToSCID called\n");

	switch (iColumn)
	{
	case 0: // Display name (Label)
		*pscid = PKEY_ItemNameDisplay; break;
	case 1: // Hostname
		*pscid = PKEY_ComputerName; break;
	case 2: // Username
		*pscid = PKEY_SwishHostUser; break;
	case 3: // SFTP port
		*pscid= PKEY_SwishHostPort; break;
	case 4: // Remote filesystem path
		*pscid = PKEY_ItemPathDisplay; break;
	case 5: // Type: always 'Network Drive'
		*pscid = PKEY_ItemType; break;
	default:
		return E_FAIL;
	}

	return S_OK;
}

/*------------------------------------------------------------------------------
 * CHostFolder::Extract : IExtractIcon
 * Extract an icon bitmap given the information passed.
 * We return S_FALSE to tell the shell to extract the icons itself.
 *----------------------------------------------------------------------------*/
STDMETHODIMP CHostFolder::Extract( LPCTSTR, UINT, HICON *, HICON *, UINT )
{
	ATLTRACE("CHostFolder::Extract called\n");
	return S_FALSE;
}

/*------------------------------------------------------------------------------
 * CHostFolder::GetIconLocation : IExtractIcon
 * Retrieve the location of the appropriate icon.
 * We set all SFTP hosts to have the icon from shell32.dll.
 *----------------------------------------------------------------------------*/
STDMETHODIMP CHostFolder::GetIconLocation(
	__in UINT uFlags, __out_ecount(cchMax) LPTSTR szIconFile, 
	__in UINT cchMax, __out int *piIndex, __out UINT *pwFlags )
{
	ATLTRACE("CHostFolder::GetIconLocation called\n");
	(void)uFlags; // type of use is ignored for host folder

	// Set host to have the ICS host icon
	StringCchCopy(szIconFile, cchMax, L"shell32.dll");
	*piIndex = 17;
	*pwFlags = GIL_DONTCACHE;

	return S_OK;
}

/*------------------------------------------------------------------------------
 * CHostFolder::GetDetailsOf : IShellDetails
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
STDMETHODIMP CHostFolder::GetDetailsOf( __in_opt PCUITEMID_CHILD pidl, 
										 __in UINT iColumn, 
										 __out LPSHELLDETAILS pDetails )
{
	ATLTRACE("CHostFolder::GetDetailsOf called, iColumn=%u\n", iColumn);

	PROPERTYKEY pkey;

	// Lookup PKEY and use it to call GetDetailsEx
	HRESULT hr = MapColumnToSCID(iColumn, &pkey);
	if (SUCCEEDED(hr))
	{
		CString strSrc;
		VARIANT pv;

		// Get details and convert VARIANT result to SHELLDETAILS for return
		hr = GetDetailsEx(pidl, &pkey, &pv);
		if (SUCCEEDED(hr))
		{
			ATLASSERT(pv.vt == VT_BSTR);
			strSrc = pv.bstrVal;
			::SysFreeString(pv.bstrVal);

			pDetails->str.uType = STRRET_WSTR;
			SHStrDup(strSrc, &pDetails->str.pOleStr);
		}

		VariantClear( &pv );

		if(!pidl) // Header requested
		{
			pDetails->fmt = LVCFMT_LEFT;
			pDetails->cxChar = strSrc.GetLength();
		}
	}

	return hr;
}

/**
 * Cracks open the @c DFM_* callback messages and dispatched them to handlers.
 */
HRESULT CHostFolder::OnMenuCallback(
	HWND hwnd, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	METHOD_TRACE("(uMsg=%d)\n", uMsg);
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
	default:
		return S_FALSE;
	}
}

/**
 * Handle @c DFM_MERGECONTEXTMENU callback.
 */
HRESULT CHostFolder::OnMergeContextMenu(
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


/*----------------------------------------------------------------------------*/
/* --- Private functions -----------------------------------------------------*/
/*----------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
 * CHostFolder::_GetLongNameFromPIDL
 * Retrieve the long name of the host connection from the given PIDL.
 * The long name is either the canonical form if fCanonical is set:
 *     sftp://username@hostname:port/path
 * or, if not set and if the port is the default port the reduced form:
 *     sftp://username@hostname/path
 *----------------------------------------------------------------------------*/
CString CHostFolder::_GetLongNameFromPIDL( PCUITEMID_CHILD pidl, 
										   BOOL fCanonical )
{
	ATLTRACE("CHostFolder::_GetLongNameFromPIDL called\n");

	CString strName;
	ATLASSERT(SUCCEEDED(m_HostPidlManager.IsValid(pidl)));

	// Construct string from info in PIDL
	strName = _T("sftp://");
	strName += m_HostPidlManager.GetUser(pidl);
	strName += _T("@");
	strName += m_HostPidlManager.GetHost(pidl);
	if (fCanonical || (m_HostPidlManager.GetPort(pidl) != SFTP_DEFAULT_PORT))
	{
		strName += _T(":");
		strName.AppendFormat( _T("%u"), m_HostPidlManager.GetPort(pidl) );
	}
	strName += _T("/");
	strName += m_HostPidlManager.GetPath(pidl);

	ATLASSERT( strName.GetLength() <= MAX_CANONICAL_LEN );

	return strName;
}

/*------------------------------------------------------------------------------
 * CHostFolder::_GetLabelFromPIDL
 * Retrieve the user-assigned label of the host connection from the given PIDL.
 *----------------------------------------------------------------------------*/
CString CHostFolder::_GetLabelFromPIDL( PCUITEMID_CHILD pidl )
{
	ATLTRACE("CHostFolder::_GetLabelFromPIDL called\n");

	CString strName;
	ATLASSERT(SUCCEEDED(m_HostPidlManager.IsValid(pidl)));

	// Construct string from info in PIDL
	strName = m_HostPidlManager.GetLabel(pidl);

	ATLASSERT( strName.GetLength() <= MAX_LABEL_LEN );

	return strName;
}

/*------------------------------------------------------------------------------
 * CHostFolder::_FillDetailsVariant
 * Initialise the VARIANT whose pointer is passed and fill with string data.
 * The string data can be passed in as a wchar array or a CString.  We allocate
 * a new BSTR and store it in the VARIANT.
 *----------------------------------------------------------------------------*/
HRESULT CHostFolder::_FillDetailsVariant( __in PCWSTR szDetail,
										   __out VARIANT *pv )
{
	ATLTRACE("CHostFolder::_FillDetailsVariant called\n");

	VariantInit( pv );
	pv->vt = VT_BSTR;
	pv->bstrVal = ::SysAllocString( szDetail );

	return pv->bstrVal ? S_OK : E_OUTOFMEMORY;
}

/**
 * Get a list names of registry keys for the connection items.  This is not 
 * required for Windows Vista but is necessary on any earlier version in
 * order to display the default context menu.
 * Host connection items are treated as folders so the list of keys is:  
 *   HKCU\Directory
 *   HKCU\Directory\Background
 *   HKCU\Folder
 *   HKCU\*
 *   HKCU\AllFileSystemObjects
 */
HRESULT CHostFolder::_GetAssocRegistryKeys(UINT *pcKeys, HKEY **paKeys)
{
	LSTATUS rc = ERROR_SUCCESS;	
	vector<CString> vecKeynames;

	// Add directory-specific items
	vecKeynames.push_back(_T("Directory"));
	vecKeynames.push_back(_T("Directory\\Background"));
	vecKeynames.push_back(_T("Folder"));

	// Add names of keys that apply to items of all types
	vecKeynames.push_back(_T("AllFilesystemObjects"));
	vecKeynames.push_back(_T("*"));

	// Create list of registry handles from list of keys
	vector<HKEY> vecKeys;
	for (UINT i = 0; i < vecKeynames.size(); i++)
	{
		CRegKey reg;
		CString strKey = vecKeynames[i];
		ATLASSERT(strKey.GetLength() > 0);
		ATLTRACE(_T("Opening HKCR\\%s\n"), strKey);
		rc = reg.Open(HKEY_CLASSES_ROOT, strKey, KEY_READ);
		ATLASSERT(rc == ERROR_SUCCESS);
		if (rc == ERROR_SUCCESS)
			vecKeys.push_back(reg.Detach());
	}
	
	ATLASSERT( vecKeys.size() >= 3 );  // The minimum we must have added here
	ATLASSERT( vecKeys.size() <= 16 ); // CDefFolderMenu_Create2's maximum

	HKEY *aKeys = (HKEY *)::SHAlloc(vecKeys.size() * sizeof HKEY); 
	for (UINT i = 0; i < vecKeys.size(); i++)
		aKeys[i] = vecKeys[i];

	*pcKeys = (UINT)vecKeys.size();
	*paKeys = aKeys;

	return S_OK;
}

/**
 * Load all the connections stored in the registry into the member vector.
 *
 * If the vector already contains items, these are cleared.  It's possible
 * that there aren't any connections in the @c Software\\Swish\\Connections 
 * key of the registry, in which case the vector is left empty.
 *
 * @returns S_OK if there were no connections in the registry or if there were
 *          and they were successfully loaded into @c m_vecConnData.  It
 *          returns an error if something unexpected happens such as corrupt
 *          registry structure.
 */
HRESULT CHostFolder::_LoadConnectionsFromRegistry()
{
	CRegKey regConnections;
	LSTATUS rc = ERROR_SUCCESS;
	HRESULT hr = S_OK;

	rc = regConnections.Open(
		HKEY_CURRENT_USER, _T("Software\\Swish\\Connections")
	);
	if (rc != ERROR_SUCCESS)
		return S_OK; // Legal to fail here - may be first ever connection

	m_vecConnData.clear();

	int iSubKey = 0;
	TCHAR szLabel[2048]; 
	do {
		DWORD cchLabel = 2048;
		rc = regConnections.EnumKey(iSubKey, szLabel, &cchLabel);
		if (rc == ERROR_SUCCESS)
		{
			hr = _LoadConnectionDetailsFromRegistry(szLabel);
			ATLASSERT(SUCCEEDED(hr));
		}
		iSubKey++;
	} while (rc == ERROR_SUCCESS);

	ATLASSERT_REPORT(rc == ERROR_NO_MORE_ITEMS, rc);
	ATLASSERT(regConnections.Close() == ERROR_SUCCESS);

	return hr;
}

/**
 * Loads a single connection from the registry and adds it to the member vector.
 *
 * @pre The @c Software\\Swish\\Connections registry key exists
 * @pre The connection is present as a subkey of the 
 *      @c Software\\Swish\\Connections registry key whose name is given
 *      by @p szLabel.
 * @param szLabel The name of the connection to load.
 * @returns S_OK if successfull, E_FAIL if the registry key does not exist
 *          and E_UNEXPECTED if the registry is corrupted.
 */
HRESULT CHostFolder::_LoadConnectionDetailsFromRegistry( PCTSTR szLabel )
{
	CRegKey regConnection;
	LSTATUS rc = ERROR_SUCCESS;
	HRESULT hr = S_OK;
	PITEMID_CHILD pDataTemp; // Not simply HOSTPIDL, has terminator

	// Target variables to load values into
	TCHAR szHost[MAX_HOSTNAME_LENZ]; ULONG cchHost = MAX_HOSTNAME_LENZ;
	DWORD dwPort;
	TCHAR szUser[MAX_USERNAME_LENZ]; ULONG cchUser = MAX_USERNAME_LENZ;
	TCHAR szPath[MAX_PATH_LENZ];     ULONG cchPath = MAX_PATH_LENZ;

	// Open HKCU\Software\Swish\Connections\<szLabel> registry key
	CString strKey = CString("Software\\Swish\\Connections\\") + szLabel;
	rc = regConnection.Open(HKEY_CURRENT_USER, strKey);
	ATLENSURE_REPORT_HR(rc == ERROR_SUCCESS, rc, E_FAIL);

	// Load values
	rc = regConnection.QueryStringValue(_T("Host"), szHost, &cchHost); // Host

	rc = regConnection.QueryDWORDValue(_T("Port"), dwPort);            // Port
	ATLENSURE_REPORT_HR(rc == ERROR_SUCCESS, rc, E_UNEXPECTED);
	ATLASSERT(dwPort >= MIN_PORT);
	ATLASSERT(dwPort <= MAX_PORT);
	USHORT uPort = static_cast<USHORT>(dwPort);

	rc = regConnection.QueryStringValue(_T("User"), szUser, &cchUser); // User
	ATLENSURE_REPORT_HR(rc == ERROR_SUCCESS, rc, E_UNEXPECTED);

	rc = regConnection.QueryStringValue(_T("Path"), szPath, &cchPath); // Path
	ATLENSURE_REPORT_HR(rc == ERROR_SUCCESS, rc, E_UNEXPECTED);

	// Create new Listing and push onto end of vector
	hr = m_HostPidlManager.Create(
		szLabel, szUser, szHost, szPath, uPort, &pDataTemp );
	ATLASSERT(SUCCEEDED(hr));
	m_vecConnData.push_back(*(m_HostPidlManager.Validate(pDataTemp)));
	m_HostPidlManager.Delete(pDataTemp);

	ATLASSERT(regConnection.Close() == ERROR_SUCCESS);
	return hr;
}