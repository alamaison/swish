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
#include "Registry.h" // For saved connection details

#include <string>
using std::wstring;

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

	try
	{
		// Load connections from HKCU\Software\Swish\Connections
		m_vecConnData = CRegistry::LoadConnectionsFromRegistry();
	}
	catchCom()

    // Create an enumerator with CComEnumOnSTL<> and our copy policy class.
	CComObject<CEnumIDListImpl>* pEnum;
    hr = CComObject<CEnumIDListImpl>::CreateInstance( &pEnum );
	ATLENSURE_RETURN_HR(SUCCEEDED(hr), hr);
    pEnum->AddRef();

    // Init the enumerator.  Init() will AddRef() our IUnknown (obtained with
    // GetUnknown()) so this object will stay alive as long as the enumerator 
    // needs access to the vector m_vecConnData.
    hr = pEnum->Init(GetUnknown(), m_vecConnData);

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

		// Get CLSID in {DWORD-WORD-WORD-WORD-WORD.DWORD} form
		LPOLESTR posz;
		::StringFromCLSID(__uuidof(CHostFolder), &posz);

		// Initialise default assoc provider to use Swish CLSID key for data.
		// This is necessary to pick up properties and TileInfo etc.
		hr = spAssoc->Init(0, posz, NULL, NULL);
		::CoTaskMemFree(posz);
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
			CRegistry::GetHostFolderAssocKeys(&cKeys, &aKeys)),
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

/**
 * Convert path string relative to this folder into a PIDL to the item.
 *
 * @todo  Handle the attributes parameter.  Should just return
 * GetAttributesOf() the PIDL we create but it is a bit hazy where the
 * host PIDL's responsibilities end and the remote PIDL's start because
 * of the path embedded in the host PIDL.
 */
STDMETHODIMP CHostFolder::ParseDisplayName(
	HWND hwnd, IBindCtx *pbc, PWSTR pwszDisplayName, ULONG *pchEaten,
	PIDLIST_RELATIVE *ppidl, __inout_opt ULONG *pdwAttributes)
{
	ATLTRACE(__FUNCTION__" called (pwszDisplayName=%ws)\n", pwszDisplayName);
	ATLENSURE_RETURN_HR(pwszDisplayName, E_POINTER);
	ATLENSURE_RETURN_HR(*pwszDisplayName != L'\0', E_INVALIDARG);
	ATLENSURE_RETURN_HR(ppidl, E_POINTER);

	// The string we are trying to parse should be of the form:
	//    sftp://username@hostname:port/path

	wstring strDisplayName(pwszDisplayName);
	if (strDisplayName.empty())
	{
		*ppidl = CloneRootPIDL().Detach();
		return S_OK;
	}

	// Must start with sftp://
	ATLENSURE_RETURN(strDisplayName.substr(0, 7) == L"sftp://");

	// Must have @ to separate username from hostname
	wstring::size_type nAt = strDisplayName.find_first_of(L'@', 7);
	ATLENSURE_RETURN(nAt != wstring::npos);

	// Must have : to separate hostname from port number
	wstring::size_type nColon = strDisplayName.find_first_of(L':', 7);
	ATLENSURE_RETURN(nColon != wstring::npos);
	ATLENSURE_RETURN(nColon > nAt);

	// Must have / to separate port number from path
	wstring::size_type nSlash = strDisplayName.find_first_of(L'/', 7);
	ATLENSURE_RETURN(nSlash != wstring::npos);
	ATLENSURE_RETURN(nSlash > nColon);

	wstring strUser = strDisplayName.substr(7, nAt - 7);
	wstring strHost = strDisplayName.substr(nAt+1, nColon - (nAt+1));
	wstring strPort = strDisplayName.substr(nColon+1, nAt - (nSlash+1));
	wstring strPath = strDisplayName.substr(nSlash+1);
	ATLENSURE_RETURN(!strUser.empty());
	ATLENSURE_RETURN(!strHost.empty());
	ATLENSURE_RETURN(!strPort.empty());
	ATLENSURE_RETURN(!strPath.empty());

	int nPort = _wtoi(strPort.c_str());
	ATLENSURE_RETURN(nPort >= MIN_PORT && nPort <= MAX_PORT);

	// Create child PIDL for this path segment
	HRESULT hr = S_OK;
	try
	{
		CHostItem pidl(
			strUser.c_str(), strHost.c_str(), static_cast<USHORT>(nPort),
			strPath.c_str());

		CComPtr<IShellFolder> spSubfolder;
		hr = BindToObject(pidl, pbc, IID_PPV_ARGS(&spSubfolder));
		ATLENSURE_SUCCEEDED(hr);

		wchar_t wszPath[MAX_PATH];
		::StringCchCopyW(wszPath, ARRAYSIZE(wszPath), strPath.c_str());

		CRelativePidl pidlPath;
		hr = spSubfolder->ParseDisplayName(
			hwnd, pbc, wszPath, pchEaten, &pidlPath, pdwAttributes);
		ATLENSURE_SUCCEEDED(hr);

		*ppidl = CRelativePidl(GetRootPIDL(), pidlPath).Detach();
	}
	catch(CHostItem::InvalidPidlException)
	{
		UNREACHABLE;
		return E_UNEXPECTED;
	}
	catchCom()

	return hr;
}

/**
 * Retrieve the display name for the specified file object or subfolder.
 */
STDMETHODIMP CHostFolder::GetDisplayNameOf(
	PCUITEMID_CHILD pidl, SHGDNF uFlags, STRRET *pName)
{
	METHOD_TRACE;
	ATLENSURE_RETURN_HR(!::ILIsEmpty(pidl), E_INVALIDARG);
	ATLENSURE_RETURN_HR(pName, E_POINTER);

	::ZeroMemory(pName, sizeof STRRET);

	CString strName;
	CHostItem hpidl(pidl);

	if (uFlags & SHGDN_FORPARSING)
	{
		if (!(uFlags & SHGDN_INFOLDER))
		{
			// Bind to parent
			CComPtr<IShellFolder> spParent;
			PCUITEMID_CHILD pidlThisFolder;
			HRESULT hr = ::SHBindToParent(
				GetRootPIDL(), IID_PPV_ARGS(&spParent), &pidlThisFolder);
			ATLASSERT(SUCCEEDED(hr));

			STRRET strret;
			::ZeroMemory(&strret, sizeof strret);
			hr = spParent->GetDisplayNameOf(pidlThisFolder, uFlags, &strret);
			ATLASSERT(SUCCEEDED(hr));
			ATLASSERT(strret.uType == STRRET_WSTR);

			strName += strret.pOleStr;
			strName += L'\\';
		}

		strName += hpidl.GetLongName(true);
	}
	else if (uFlags == SHGDN_NORMAL || uFlags & SHGDN_FORADDRESSBAR)
	{
		strName = hpidl.GetLongName(false);
	}
	else if (uFlags == SHGDN_INFOLDER || uFlags & SHGDN_FOREDITING)
	{
		strName = hpidl.GetLabel();
	}
	else
	{
		UNREACHABLE;
		return E_INVALIDARG;
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

	bool fHeader = (pidl == NULL);

	CHostItemHandle hpidl(pidl);

	// If pidl: Request is for an item detail.  Retrieve from pidl and
	//          return string
	// Else:    Request is for a column heading.  Return heading as BSTR

	// Display name (Label)
	if (IsEqualPropertyKey(*pscid, PKEY_ItemNameDisplay))
	{
		TRACE("\t\tRequest: PKEY_ItemNameDisplay\n");

		return _FillDetailsVariant(
			(fHeader) ? L"Name" : hpidl.GetLabel(), pv);
	}
	// Hostname
	else if (IsEqualPropertyKey(*pscid, PKEY_ComputerName))
	{
		TRACE("\t\tRequest: PKEY_ComputerName\n");

		return _FillDetailsVariant(
			(fHeader) ? L"Host" : hpidl.GetHost(), pv);
	}
	// Username
	else if (IsEqualPropertyKey(*pscid, PKEY_SwishHostUser))
	{
		TRACE("\t\tRequest: PKEY_SwishHostUser\n");

		return _FillDetailsVariant(
			(fHeader) ? L"Username" : hpidl.GetUser(), pv);
	}
	// SFTP port
	else if (IsEqualPropertyKey(*pscid, PKEY_SwishHostPort))
	{
		TRACE("\t\tRequest: PKEY_SwishHostPort\n");

		return _FillDetailsVariant(
			(fHeader) ? L"Port" : hpidl.GetPortStr(), pv);
	}
	// Remote filesystem path
	else if (IsEqualPropertyKey(*pscid, PKEY_ItemPathDisplay))
	{
		TRACE("\t\tRequest: PKEY_ItemPathDisplay\n");

		return _FillDetailsVariant(
			(fHeader) ? L"Remote Path" : hpidl.GetPath(), pv);
	}
	// Type: always 'Network Drive'
	else if (IsEqualPropertyKey(*pscid, PKEY_ItemType))
	{
		TRACE("\t\tRequest: PKEY_ItemType\n");

		return _FillDetailsVariant(
			(fHeader) ? L"Type" : L"Network Drive", pv);
	}

	TRACE("\t\tRequest: <unknown>\n");

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
