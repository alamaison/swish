/**
    @file

    Explorer folder that handles remote files and folders.

    @if licence

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

    @endif
*/

#include "RemoteFolder.h"

#include "SftpDirectory.h"
#include "SftpDataObject.h"
#include "IconExtractor.h"
#include "ExplorerCallback.h"      // Interaction with Explorer window
#include "UserInteraction.h"       // Implementation of ISftpConsumer
#include "data_object/ShellDataObject.hpp"  // PidlFormat
#include "DropTarget.hpp"          // CDropTarget
#include "Registry.h"
#include "properties/properties.h" // File properties handler
#include "properties/column.h"     // Column details
#include "swish/debug.hpp"
#include "swish/exception.hpp"     // com_exception

#include <string>

using swish::shell_folder::CDropTarget;
using swish::shell_folder::data_object::PidlFormat;
using swish::exception::com_exception;

using ATL::CComObject;
using ATL::CComPtr;
using ATL::CComVariant;
using ATL::CComBSTR;
using ATL::CString;

using std::wstring;

using namespace swish;

/*--------------------------------------------------------------------------*/
/*                     Remaining IShellFolder functions.                    */
/* Eventually these should be replaced by the internal interfaces of        */
/* CFolder and CSwishFolder.                                                */
/*--------------------------------------------------------------------------*/

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

	try
	{
		// Create SFTP connection for this folder using hwndOwner for UI
		CComPtr<ISftpProvider> spProvider = 
			_CreateConnectionForFolder(hwndOwner);

		// Create directory handler and get listing as PIDL enumeration
		CSftpDirectory directory(root_pidl(), spProvider);
		*ppEnumIDList = directory.GetEnum(grfFlags).Detach();
	}
	catchCom()
	
	return S_OK;
}

/**
 * Convert path string relative to this folder into a PIDL to the item.
 *
 * @implementing IShellFolder
 *
 * @todo  Handle the attributes parameter.  Will need to contact server
 * as the PIDL we create is fake and will not have correct folderness, etc.
 */
STDMETHODIMP CRemoteFolder::ParseDisplayName(
	HWND hwnd, IBindCtx *pbc, PWSTR pwszDisplayName, ULONG *pchEaten,
	PIDLIST_RELATIVE *ppidl, __inout_opt ULONG *pdwAttributes)
{
	ATLTRACE(__FUNCTION__" called (pwszDisplayName=%ws)\n", pwszDisplayName);
	ATLENSURE_RETURN_HR(pwszDisplayName, E_POINTER);
	ATLENSURE_RETURN_HR(*pwszDisplayName != L'\0', E_INVALIDARG);
	ATLENSURE_RETURN_HR(ppidl, E_POINTER);

	// The string we are trying to parse should be of the form:
	//    directory/directory/filename
	// or 
    //    filename
	wstring strDisplayName(pwszDisplayName);

	// May have / to separate path segments
	wstring::size_type nSlash = strDisplayName.find_first_of(L'/');
	wstring strSegment;
	if (nSlash == 0) // Unix machine - starts with folder called /
	{
		strSegment = strDisplayName.substr(0, 1);
	}
	else
	{
		strSegment = strDisplayName.substr(0, nSlash);
	}

	// Create child PIDL for this path segment
	HRESULT hr = S_OK;
	try
	{
		CRemoteItem pidl(strSegment.c_str());

		// Bind to subfolder and recurse if there were other path segments
		if (nSlash != wstring::npos)
		{
			wstring strRest = strDisplayName.substr(nSlash+1);

			CComPtr<IShellFolder> spSubfolder;
			hr = BindToObject(pidl, pbc, IID_PPV_ARGS(&spSubfolder));
			ATLENSURE_SUCCEEDED(hr);

			wchar_t wszRest[MAX_PATH];
			::wcscpy_s(wszRest, ARRAYSIZE(wszRest), strRest.c_str());

			CRelativePidl pidlRest;
			hr = spSubfolder->ParseDisplayName(
				hwnd, pbc, wszRest, pchEaten, &pidlRest, pdwAttributes);
			ATLENSURE_SUCCEEDED(hr);

			*ppidl = CRelativePidl(pidl, pidlRest).Detach();
		}
		else
		{
			*ppidl = pidl.Detach();
		}
	}
	catchCom()

	return hr;
}

/**
 * Retrieve the display name for the specified file object or subfolder.
 *
 * @implementing IShellFolder
 */
STDMETHODIMP CRemoteFolder::GetDisplayNameOf( 
	PCUITEMID_CHILD pidl, SHGDNF uFlags, STRRET *pName)
{
	METHOD_TRACE;
	ATLENSURE_RETURN_HR(!::ILIsEmpty(pidl), E_INVALIDARG);
	ATLENSURE_RETURN_HR(pName, E_POINTER);

	::ZeroMemory(pName, sizeof STRRET);

	try
		{
		CString strName;
		CRemoteItem rpidl(pidl);

		bool fForParsing = (uFlags & SHGDN_FORPARSING) != 0;

		if (fForParsing || (uFlags & SHGDN_FORADDRESSBAR))
		{
			if (!(uFlags & SHGDN_INFOLDER))
			{
				// Bind to parent
				CComPtr<IShellFolder> spParent;
				PCUITEMID_CHILD pidlThisFolder = NULL;
				HRESULT hr = ::SHBindToParent(
					root_pidl(), IID_PPV_ARGS(&spParent), &pidlThisFolder);
				ATLASSERT(SUCCEEDED(hr));

				STRRET strret;
				::ZeroMemory(&strret, sizeof strret);
				hr = spParent->GetDisplayNameOf(
					pidlThisFolder, uFlags, &strret);
				ATLASSERT(SUCCEEDED(hr));
				ATLASSERT(strret.uType == STRRET_WSTR);

				strName += strret.pOleStr;
				strName += L'/';
			}

			// Add child path - include extension if FORPARSING
			strName += rpidl.GetFilename(fForParsing);
		}
		else if (uFlags & SHGDN_FOREDITING)
		{
			strName = rpidl.GetFilename();
		}
		else
		{
			ATLASSERT(uFlags == SHGDN_NORMAL || uFlags == SHGDN_INFOLDER);

			strName = rpidl.GetFilename(false);
		}

		// Store in a STRRET and return
		pName->uType = STRRET_WSTR;

		return SHStrDupW( strName, &pName->pOleStr );
	}
	catchCom()
}

/**
 * Rename item.
 *
 * @implementing IShellFolder
 */
STDMETHODIMP CRemoteFolder::SetNameOf(
	HWND hwnd, PCUITEMID_CHILD pidl, LPCWSTR pwszName,
	SHGDNF /*uFlags*/, PITEMID_CHILD *ppidlOut)
{
	if (ppidlOut)
		*ppidlOut = NULL;

	try
	{
		// Create SFTP connection object for this folder using hwnd for UI
		CComPtr<ISftpProvider> spProvider = _CreateConnectionForFolder(hwnd);

		// Rename file
		CSftpDirectory directory(root_pidl(), spProvider);
		bool fOverwritten = directory.Rename(pidl, pwszName);

		// Create new PIDL from old one
		CRemoteItem pidlNewFile;
		pidlNewFile.Attach(::ILCloneChild(pidl));
		pidlNewFile.SetFilename(pwszName);

		// Make PIDLs absolute
		CAbsolutePidl pidlOld(root_pidl(), pidl);
		CAbsolutePidl pidlNew(root_pidl(), pidlNewFile);

		// Return new child pidl if requested else dispose of it
		if (ppidlOut)
			*ppidlOut = pidlNewFile.Detach();

		// Update the shell by passing both PIDLs
		if (fOverwritten)
		{
			::SHChangeNotify(
				SHCNE_DELETE, SHCNF_IDLIST | SHCNF_FLUSH, pidlNew, NULL
			);
		}
		CRemoteItemHandle rpidl(pidl);
		::SHChangeNotify(
			(rpidl.IsFolder()) ? SHCNE_RENAMEFOLDER : SHCNE_RENAMEITEM,
			SHCNF_IDLIST | SHCNF_FLUSH, pidlOld, pidlNew
		);

		return S_OK;
	}
	catchCom()
}

/**
 * Returns the attributes for the items whose PIDLs are passed in.
 *
 * @implementing IShellFolder
 */
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
		CRemoteItemHandle rpidl(aPidl[i]);
		ATLASSERT(rpidl.IsValid());
		if (!rpidl.IsFolder())
		{
			fAllAreFolders = false;
			break;
		}
	}

	// Search through all PIDLs and check if they are all 'dot' files
	bool fAllAreDotFiles = true;
	for (UINT i = 0; i < cIdl; i++)
	{
		CRemoteItemHandle rpidl(aPidl[i]);
		if (rpidl.GetFilename()[0] != L'.')
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
		dwAttribs |= SFGAO_DROPTARGET;
	}
	if (fAllAreDotFiles)
	{
		dwAttribs |= SFGAO_GHOSTED;
		dwAttribs |= SFGAO_HIDDEN;
	}
	dwAttribs |= SFGAO_CANRENAME;
	dwAttribs |= SFGAO_CANDELETE;
	dwAttribs |= SFGAO_CANCOPY;

    *pdwAttribs &= dwAttribs;

    return S_OK;
}

/**
 * Returns detailed information on the items in a folder.
 *
 * @implementing IShellDetails
 *
 * This function operates in two distinctly different ways:
 * If pidl is NULL:
 *     Retrieves the information on the view columns, i.e., the names of
 *     the columns themselves.  The index of the desired column is given
 *     in iColumn.  If this column does not exist we return E_FAIL.
 * If pidl is not NULL:
 *     Retrieves the specific item information for the given pidl and the
 *     requested column.
 * The information is returned in the SHELLDETAILS structure.
 */
STDMETHODIMP CRemoteFolder::GetDetailsOf(
	PCUITEMID_CHILD pidl, UINT iColumn, SHELLDETAILS* psd)
{
	ATLTRACE("CRemoteFolder::GetDetailsOf called, iColumn=%u\n", iColumn);
	ATLENSURE_RETURN_HR(psd, E_POINTER);

	try
	{
		if (!pidl) // Header requested
			*psd = properties::column::GetHeader(iColumn);
		else
			*psd = properties::column::GetDetailsFor(pidl, iColumn);
	}
	catchCom()
	return S_OK;
}

/**
 * Get property of an item as a VARIANT.
 *
 * @implementing IShellFolder2
 *
 * The work is delegated to the properties functions in the swish::properties
 * namespace
 */
STDMETHODIMP CRemoteFolder::GetDetailsEx(
	PCUITEMID_CHILD pidl, const SHCOLUMNID* pscid, VARIANT* pv)
{
	METHOD_TRACE;
	ATLENSURE_RETURN_HR(pscid, E_POINTER);
	ATLENSURE_RETURN_HR(pv, E_POINTER);
	ATLENSURE_RETURN_HR(!::ILIsEmpty(pidl), E_INVALIDARG);

	try
	{
		::VariantInit(pv);

		CComVariant var = properties::GetProperty(pidl, *pscid);

		HRESULT hr = var.Detach(pv);
		ATLENSURE_SUCCEEDED(hr);
	}
	catchCom()
	return S_OK;
}

/**
 * Returns the default state for the column specified by index.
 * @implementing IShellFolder2
 */
STDMETHODIMP CRemoteFolder::GetDefaultColumnState(
	UINT iColumn, SHCOLSTATEF* pcsFlags)
{
	METHOD_TRACE;
	ATLENSURE_RETURN_HR(pcsFlags, E_POINTER);

	try
	{
		*pcsFlags = properties::column::GetDefaultState(iColumn);
	}
	catchCom()
	return S_OK;
}

/**
 * Convert column to appropriate property set ID (FMTID) and property ID (PID).
 *
 * @implementing IShellFolder2
 */
STDMETHODIMP CRemoteFolder::MapColumnToSCID(UINT iColumn, SHCOLUMNID* pscid)
{
	METHOD_TRACE;
	ATLENSURE_RETURN_HR(pscid, E_POINTER);

	try
	{
		*pscid =  properties::column::MapColumnIndexToSCID(iColumn);
	}
	catchCom()
	return S_OK;
}

/*--------------------------------------------------------------------------*/
/*                     CFolder NVI internal interface.                      */
/* These method implement the internal interface of the CFolder abstract    */
/* class                                                                    */
/*--------------------------------------------------------------------------*/

/**
 * Return the folder's registered CLSID
 *
 * @implementing CFolder
 */
CLSID CRemoteFolder::clsid() const
{
	return __uuidof(this);
}

/**
 * Sniff PIDLs to determine if they are of our type.  Throw if not.
 *
 * @implementing CFolder
 */
void CRemoteFolder::validate_pidl(PCUIDLIST_RELATIVE pidl) const
{
	if (pidl == NULL)
		throw com_exception(E_POINTER);

	if (!CRemoteItemList::IsValid(pidl))
		throw com_exception(E_INVALIDARG);
}

/**
 * Create and initialise new folder object for subfolder.
 *
 * @implementing CFolder
 *
 * Create new CRemoteFolder initialised with its root PIDL.  CRemoteFolder
 * only have instances of themselves as subfolders.
 */
CComPtr<IShellFolder> CRemoteFolder::subfolder(PCIDLIST_ABSOLUTE pidl) const
{
	// Create CRemoteFolder initialised with its root PIDL
	CComPtr<IShellFolder> folder = CRemoteFolder::Create(pidl);
	ATLENSURE_THROW(folder, E_NOINTERFACE);

	return folder;
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
 */
int CRemoteFolder::compare_pidls(
	PCUITEMID_CHILD pidl1, PCUITEMID_CHILD pidl2,
	int column, bool compare_all_fields, bool canonical)
const
{
	return swish::properties::column::CompareDetailOf(
		pidl1, pidl2, column, compare_all_fields, canonical);
}


/*--------------------------------------------------------------------------*/
/*                    CSwishFolder internal interface.                      */
/* These method override the (usually no-op) implementations of some        */
/* in the CSwishFolder base class                                           */
/*--------------------------------------------------------------------------*/

/**
 * Create an icon extraction helper object for the selected item.
 *
 * @implementing CSwishFolder
 */
CComPtr<IExtractIconW> CRemoteFolder::extract_icon_w(
	HWND /*hwnd*/, PCUITEMID_CHILD pidl)
{
	TRACE("Request: IExtractIconW");

	CRemoteItemHandle rpidl(pidl);
	return CIconExtractor::Create(rpidl.GetFilename(), rpidl.IsFolder());
}

/**
 * Create a file association handler for the selected items.
 *
 * @implementing CSwishFolder
 */
CComPtr<IQueryAssociations> CRemoteFolder::query_associations(
	HWND /*hwnd*/, UINT cpidl, PCUITEMID_CHILD_ARRAY apidl)
{
	TRACE("Request: IQueryAssociations");
	assert(cpidl > 0);

	CComPtr<IQueryAssociations> spAssoc;
	HRESULT hr = ::AssocCreate(
		CLSID_QueryAssociations, IID_PPV_ARGS(&spAssoc));
	ATLENSURE_SUCCEEDED(hr);

	CRemoteItemHandle pidl(apidl[0]);
	
	if (pidl.IsFolder())
	{
		// Initialise default assoc provider for Folders
		hr = spAssoc->Init(
			ASSOCF_INIT_DEFAULTTOFOLDER, L"Folder", NULL, NULL);
		ATLENSURE_SUCCEEDED(hr);
	}
	else
	{
		// Initialise default assoc provider for given file extension
		CString strExt = L"." + pidl.GetExtension();
		hr = spAssoc->Init(
			ASSOCF_INIT_DEFAULTTOSTAR, strExt, NULL, NULL);
		ATLENSURE_SUCCEEDED(hr);
	}

	return spAssoc;
}

/**
 * Create a context menu for the selected items.
 *
 * @implementing CSwishFolder
 */
CComPtr<IContextMenu> CRemoteFolder::context_menu(
	HWND hwnd, UINT cpidl, PCUITEMID_CHILD_ARRAY apidl)
{
	TRACE("Request: IContextMenu");
	assert(cpidl > 0);

	// Get keys associated with filetype from registry.
	// We only take into account the item that was right-clicked on 
	// (the first array element) even if this was a multi-selection.
	//
	// This article says that we don't need to specify the keys:
	// http://groups.google.com/group/microsoft.public.platformsdk.shell/
	// browse_thread/thread/6f07525eaddea29d/
	// but we do for the context menu to appear in versions of Windows 
	// earlier than Vista.
	HKEY *akeys; UINT ckeys;
	ATLENSURE_THROW(SUCCEEDED(
		CRegistry::GetRemoteFolderAssocKeys(apidl[0], &ckeys, &akeys)),
		E_UNEXPECTED  // Might fail if registry is corrupted
	);

	CComPtr<IShellFolder> spThisFolder = this;
	ATLENSURE_THROW(spThisFolder, E_OUTOFMEMORY);

	// Create default context menu from list of PIDLs
	CComPtr<IContextMenu> spMenu;
	HRESULT hr = ::CDefFolderMenu_Create2(
		root_pidl(), hwnd, cpidl, apidl, spThisFolder, 
		MenuCallback, ckeys, akeys, &spMenu);
	if (FAILED(hr))
		throw com_exception(hr);

	return spMenu;
}

/**
 * Create a data object for the selected items.
 *
 * @implementing CSwishFolder
 */
CComPtr<IDataObject> CRemoteFolder::data_object(
	HWND hwnd, UINT cpidl, PCUITEMID_CHILD_ARRAY apidl)
{
	TRACE("Request: IDataObject");
	assert(cpidl > 0);

	// Create connection for this folder with hwnd for UI
	CComPtr<ISftpProvider> spProvider =
		_CreateConnectionForFolder(hwnd);

	return CSftpDataObject::Create(
		cpidl, apidl, root_pidl(), spProvider);
}

/**
 * Create a drop target handler for the folder.
 *
 * @implementing CSwishFolder
 */
CComPtr<IDropTarget> CRemoteFolder::drop_target(HWND hwnd)
{
	TRACE("Request: IDropTarget");

	// Create connection for this folder with hwnd for UI
	CComPtr<ISftpProvider> spProvider =
		_CreateConnectionForFolder(hwnd);
	CHostItemAbsoluteHandle pidl = root_pidl();
	return CDropTarget::Create(spProvider, pidl.GetFullPath().GetString());
}

/**
 * Create an instance of our Shell Folder View callback handler.
 */
CComPtr<IShellFolderViewCB> CRemoteFolder::folder_view_callback(HWND /*hwnd*/)
{
	return CExplorerCallback::Create(root_pidl());
}


/*--------------------------------------------------------------------------*/
/*                         Context menu handlers.                           */
/*--------------------------------------------------------------------------*/

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
	HWND hwnd, IDataObject *pDataObj, int idCmd, PCWSTR pwszArgs )
{
	ATLTRACE(__FUNCTION__" called (hwnd=%p, pDataObj=%p, idCmd=%d, "
		"pwszArgs=%ls)\n", hwnd, pDataObj, idCmd, pwszArgs);

	hwnd; pDataObj; idCmd; pwszArgs; // Unused in Release build
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

	pdfmics; // Unused in Release build

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
		PidlFormat format(pDataObj);
		CAbsolutePidl pidlFolder = format.parent_folder();
		ATLASSERT(::ILIsEqual(root_pidl(), pidlFolder));

		// Build up a list of PIDLs for all the items to be deleted
		RemotePidls vecDeathRow;
		for (UINT i = 0; i < format.pidl_count(); i++)
		{
			CRemoteItemList pidlFile = format.relative_file(i);

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
/*                           Private functions                                */
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
	CComPtr<ISftpProvider> spProvider = _CreateConnectionForFolder( hwnd );

	// Create instance of our directory handler class
	CSftpDirectory directory(root_pidl(), spProvider);

	// Delete each item and notify shell
	RemotePidls::const_iterator it = vecDeathRow.begin();
	while (it != vecDeathRow.end())
	{
		directory.Delete( *it );

		// Make PIDL absolute
		CAbsolutePidl pidlFull(root_pidl(), *it);

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

/**
 * Gets connection for given SFTP session parameters.
 */
CComPtr<ISftpProvider> CRemoteFolder::_GetConnection(
	HWND hwnd, PCWSTR szHost, PCWSTR szUser, UINT uPort ) throw(...)
{
	// Create SFTP Consumer for SftpProvider (used for password reqs etc)
	CComPtr<CUserInteraction> spConsumer = CUserInteraction::CreateCoObject();
	spConsumer->SetHWND(hwnd);

	// Get SFTP Provider from session pool
	CPool pool;
	CComPtr<ISftpProvider> spProvider = pool.GetSession(
		spConsumer, CComBSTR(szHost), CComBSTR(szUser), uPort);

	return spProvider;
}

/**
 * Creates an SFTP connection.
 *
 * The connection is created from the information stored in this
 * folder's PIDL, @c m_pidl, and the window handle to be used as the owner
 * window for any user interaction. This window handle can be NULL but (in order
 * to enforce good UI etiquette - we shouldn't attempt to interact with the user
 * if Explorer isn't expecting us to) any operation which requires user 
 * interaction should quietly fail.  
 *
 * @param hwndUserInteraction  A handle to the window which should be used
 *                             as the parent window for any user interaction.
 * @throws ATL exceptions on failure.
 */
CComPtr<ISftpProvider> CRemoteFolder::_CreateConnectionForFolder(
	HWND hwndUserInteraction )
{

	// Find HOSTPIDL part of this folder's absolute pidl to extract server info
	CHostItemListHandle pidlHost(CHostItemListHandle(root_pidl()).FindHostPidl());
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
