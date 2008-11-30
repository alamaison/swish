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
#include "RemotePidl.h"
#include "ShellDataObject.h"
#include "DataObjectFactory.h"

#include <ATLComTime.h>
#include <atlrx.h> // For regular expressions

#include <vector>
using std::vector;
using std::iterator;

/**
 * Retrieves the class identifier (CLSID) of the object.
 * 
 * @implementing IPersist
 *
 * @param [in] pClsid  The location to return the CLSID in.
 */
STDMETHODIMP CRemoteFolder::GetClassID( CLSID* pClsid )
{
	ATLTRACE("CRemoteFolder::GetClassID called\n");
	ATLENSURE_RETURN_HR( pClsid, E_POINTER );

	*pClsid = __uuidof(CRemoteFolder);
    return S_OK;
}

/**
 * Assigns an @b absolute PIDL to this folder which we store for later.
 *
 * @implementing IPersistFolder
 *
 * This function is used to tell a folder its place in the system namespace. 
 * if the folder implementation needs to construct a fully qualified PIDL
 * to elements that it contains, the PIDL passed to this method should be 
 * used to construct these.
 *
 * @param pidl  The PIDL that specifies the absolute location of this folder.
 */
STDMETHODIMP CRemoteFolder::Initialize( PCIDLIST_ABSOLUTE pidl )
{
	ATLTRACE("CRemoteFolder::Initialize called\n");

	ATLASSERT( pidl != NULL );
	ATLASSERT( // Must be either identify a host or a remote file/folder
		SUCCEEDED(m_HostPidlManager.IsValid(pidl, PM_LAST_PIDL)) ||
		SUCCEEDED(m_RemotePidlManager.IsValid(pidl, PM_LAST_PIDL))
	);
    m_pidl = static_cast<PIDLIST_ABSOLUTE>( m_RemotePidlManager.Copy(pidl) );
	ATLASSUME( m_pidl );
	return S_OK;
}

/**
 * Retrieves the absolute PIDL of this folder relative to the desktop.
 *
 * @implementing IPersistFolder2
 *
 * @param [out] ppidl  The location to return the copied PIDL in (optional).
 * @returns  @c S_OK if successful, @c S_FALSE if @ref Initialize() has not 
 *           yet been called, an error otherwise.
 */
STDMETHODIMP CRemoteFolder::GetCurFolder( PIDLIST_ABSOLUTE *ppidl )
{
	ATLTRACE("CRemoteFolder::GetCurFolder called\n");
	ATLENSURE_RETURN_HR(ppidl, E_POINTER);

	*ppidl = NULL;
	if (m_pidl == NULL) // Legal to call this before Initialize()
		return S_FALSE;

	// Make a copy of the PIDL that was passed to us in Initialize()
	*ppidl = static_cast<PIDLIST_ABSOLUTE>( m_RemotePidlManager.Copy(m_pidl) );
	ATLENSURE_RETURN_HR(*ppidl, E_OUTOFMEMORY);
	
	return S_OK;
}

/**
 * Subobject of this folder has been requested (typically IShellFolder).
 *
 * @implementing IShellFolder
 *
 * Creates and initialises a new CRemoteFolder to represent subfolder and 
 * returns its IShellFolder interface.
 *
 * @param [in]  pidl    PIDL to the requested object @b relative to this folder.
 * @param [in]  pbc     The binding context (ignored).
 * @param [in]  riid    The interface being requested.
 * @param [out] ppvOut  Location to return requested interface.
 */
STDMETHODIMP CRemoteFolder::BindToObject( __in PCUIDLIST_RELATIVE pidl,
										  __in_opt IBindCtx *pbc,
										  __in REFIID riid,
										  __out void** ppvOut )
{
	ATLTRACE("CRemoteFolder::BindToObject called\n");
	UNREFERENCED_PARAMETER( pbc );
	*ppvOut = NULL;

	HRESULT hr;

	// We can assume that the PIDL we have been passed and asked to bind to
	// contains one or more REMOTEPIDLs representing the file-system 
	// hierarchy of the target file or folder and may be a child of
	// either a HOSTPIDL or another REMOTEPIDL:
	// <Relative (HOST|REMOTE)PIDL>/REMOTEPIDL[/REMOTEPIDL]*

	// Check that first and last are all REMOTEPIDLs (may be same PIDL)
	hr = m_RemotePidlManager.IsValid( pidl, PM_THIS_PIDL );
	ATLENSURE_RETURN_HR(SUCCEEDED(hr), hr);
	hr = m_RemotePidlManager.IsValid( pidl, PM_LAST_PIDL );
	ATLENSURE_RETURN_HR(SUCCEEDED(hr), hr);

	// Create absolute PIDL by combining with parent HOST or REMOTEPIDL (m_pidl)
	PIDLIST_ABSOLUTE pidlBind = ::ILCombine( m_pidl, pidl );
	ATLENSURE_RETURN_HR(pidlBind, E_OUTOFMEMORY);

	// Create RemoteFolder to bind to PIDL
	CComObject<CRemoteFolder> *pRemoteFolder;
	hr = CComObject<CRemoteFolder>::CreateInstance( &pRemoteFolder );
	ATLENSURE_RETURN_HR(SUCCEEDED(hr), hr);
    pRemoteFolder->AddRef();

	// Initialise RemoteFolder with the absolute PIDL
    hr = pRemoteFolder->Initialize( pidlBind );
    if (SUCCEEDED(hr))
        hr = pRemoteFolder->QueryInterface( riid, ppvOut );

	::ILFree( pidlBind );
    pRemoteFolder->Release();

	/* TODO: Not sure if I have done this properly with QueryInterface
	 * From MS: Implementations of BindToObject can optimize any call to 
	 * it by  quickly failing for IID values that it does not support. For 
	 * example, if the Shell folder object of the subitem does not support 
	 * IRemoteComputer, the implementation should return E_NOINTERFACE 
	 * immediately instead of needlessly creating the Shell folder object 
	 * for the subitem and then finding that IRemoteComputer was not 
	 * supported after all. 
	 * http://msdn2.microsoft.com/en-us/library/ms632954.aspx
	 */

	return hr;
}

/**
 * Create an @c IEnumIDList to allow objects in this folder to be enumerated.
 *
 * @implementing IShellFolder
 *
 * @param [in]  hwndOwner     An optional window handle to be used if 
 *                            enumeration requires user input.
 * @param [in]  dwFlags       Flags specifying which types of items to include 
 *                            in the enumeration. Possible flags are from the 
 *                            @c SHCONT enum.
 * @param [out] ppEnumIDList  Location to return the @c IEnumIDList in.
 */
STDMETHODIMP CRemoteFolder::EnumObjects(
	__in_opt HWND hwndOwner, 
	__in SHCONTF grfFlags,
	__deref_out_opt IEnumIDList **ppEnumIDList )
{
	ATLTRACE("CRemoteFolder::EnumObjects called\n");
	ATLASSERT(m_pidl);

    if (ppEnumIDList == NULL)
        return E_POINTER;
    *ppEnumIDList = NULL;

	// Create SFTP connection object for this folder using hwndOwner for UI
	CConnection conn;
	try
	{
		conn = _CreateConnectionForFolder( hwndOwner );
	}
	catchCom()

	// Get path by extracting it from chain of PIDLs starting with HOSTPIDL
	CString strPath = _ExtractPathFromPIDL( m_pidl );
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
 * CRemoteFolder::CreateViewObject : IShellFolder
 * Returns the requested COM interface for aspects of the folder's 
 * functionality, primarily the ShellView object but also context menus etc.
 * Contrasted with GetUIObjectOf which performs a similar function but
 * for the objects containted *within* the folder.
 *----------------------------------------------------------------------------*/
STDMETHODIMP CRemoteFolder::CreateViewObject( __in_opt HWND hwndOwner, 
                                             __in REFIID riid, 
											 __deref_out void **ppvOut )
{
	ATLTRACE("CRemoteFolder::CreateViewObject called\n");
	ATLASSERT(m_pidl); // Check this object has been initialised
	(void)hwndOwner; // Not a custom folder view.  No parent window needed
	
    *ppvOut = NULL;

	HRESULT hr = E_NOINTERFACE;

	/* Commonly requested interfaces:
	    IShellView
		IContextMenu
		IExtractIcon
		IQueryInfo
		IShellDetails
		IDropTarget
	*/

	if (riid == IID_IShellView)
	{
		ATLTRACE("\t\tRequest: IID_IShellView\n");
		SFV_CREATE sfvData = { sizeof(sfvData), 0 };

		// Create an instance of our Shell Folder View callback handler
		CComPtr<IShellFolderViewCB> spExplorerCB;
		CExplorerCallback::MakeInstance(m_pidl, &spExplorerCB);

		// Create a pointer to this IShellFolder
		CComPtr<IShellFolder> spFolder = this;
		ATLENSURE_RETURN_HR(spFolder, E_NOINTERFACE);

		// Pack pointers into SFV_CREATE
		sfvData.psfvcb = spExplorerCB;
		sfvData.pshf = spFolder;
		sfvData.psvOuter = NULL;

		// Create Default Shell Folder View object (aka DEFVIEW)
		hr = SHCreateShellFolderView( &sfvData, (IShellView**)ppvOut );
	}
	else if (riid == IID_IShellDetails)
    {
		ATLTRACE("\t\tRequest: IID_IShellDetails\n");
		return QueryInterface(riid, ppvOut);
    }
	
	ATLTRACE("\t\tRequest: <unknown>\n");

    return hr;
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

		hr = ::AssocCreate(CLSID_QueryAssociations, riid, ppvReturn);
		ATLENSURE_RETURN_HR(SUCCEEDED(hr), hr);
		
		if (m_RemotePidlManager.IsFolder(aPidl[0]))
		{
			// Initialise default assoc provider for Folders
			hr = reinterpret_cast<IQueryAssociations*>(*ppvReturn)->Init(
				ASSOCF_INIT_DEFAULTTOFOLDER, _T("Folder"), NULL, NULL);
			ATLASSERT(SUCCEEDED(hr));
		}
		else
		{
			// Initialise default assoc provider for given file extension
			CString strExt = _T(".") + _GetFileExtensionFromPIDL(aPidl[0]);
			hr = reinterpret_cast<IQueryAssociations*>(*ppvReturn)->Init(
				ASSOCF_INIT_DEFAULTTOSTAR, strExt, NULL, NULL);
			ATLASSERT(SUCCEEDED(hr));
		}
	}
	else if (riid == __uuidof(IContextMenu))
	{
		ATLTRACE("\t\tRequest: IContextMenu\n");

		// Get keys associated with filetype from registry
		// We only take into account the item that was right-clicked on 
		// (the first array element) even if this was a multi-selection
		//
		// This article says that we don't need to specify the key but we do:
		// http://groups.google.com/group/microsoft.public.platformsdk.shell/
		// browse_thread/thread/6f07525eaddea29d/
		HKEY *aKeys; UINT cKeys;
		ATLENSURE_RETURN_HR(SUCCEEDED(
			_GetRegistryKeysForPidl(aPidl[0], &cKeys, &aKeys)),
			E_UNEXPECTED  // Might fail if registry is corrupted
		);

		CComPtr<IShellFolder> spThisFolder = this;
		ATLENSURE_RETURN_HR(spThisFolder, E_OUTOFMEMORY);

		hr = ::CDefFolderMenu_Create2(m_pidl, hwndOwner, cPidl, aPidl,
			spThisFolder, MenuCallback, cKeys, aKeys,
			(IContextMenu **)ppvReturn);
	}
	else if (riid == __uuidof(IDataObject))
	{
		ATLTRACE("\t\tRequest: IDataObject\n");

		try
		{
			// Create connection object for this folder with hwndOwner for UI
			CConnection conn = _CreateConnectionForFolder(hwndOwner);

			CComPtr<IDataObject> spDo(
				CDataObjectFactory::CreateDataObjectFromPIDLs(
					conn, m_pidl, cPidl, aPidl));
			ATLASSERT(spDo);
			*(IDataObject **)ppvReturn = spDo.Detach();
		}
		catchCom()

		hr = S_OK;
	}
	else	
		ATLTRACE("\t\tRequest: <unknown>\n");

    return hr;
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

		PIDLIST_ABSOLUTE pidlAbsolute = ::ILCombine( m_pidl, pidl );
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


		PIDLIST_ABSOLUTE pidlAbsolute = ::ILCombine( m_pidl, pidl );
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
		CString strPath = _ExtractPathFromPIDL( m_pidl );
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
		PIDLIST_ABSOLUTE pidlOld =  ::ILCombine(m_pidl, pidl);
		PIDLIST_ABSOLUTE pidlNew =  ::ILCombine(m_pidl, pidlNewFile);

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
	dwAttribs |= SFGAO_CANCOPY;

    *pdwAttribs &= dwAttribs;

    return S_OK;
}

/*------------------------------------------------------------------------------
 * CRemoteFolder::CompareIDs : IShellFolder
 * Determines the relative order of two file objects or folders.
 * Given their item identifier lists, the two objects are compared and a
 * result code is returned.
 *   Negative: pidl1 < pidl2
 *   Positive: pidl1 > pidl2
 *   Zero:     pidl1 == pidl2
 *----------------------------------------------------------------------------*/
STDMETHODIMP CRemoteFolder::CompareIDs( LPARAM lParam, PCUIDLIST_RELATIVE pidl1,
                                        PCUIDLIST_RELATIVE pidl2 )
{
	ATLTRACE("CRemoteFolder::CompareIDs called\n");
	(void)lParam; // Use default sorting rule always

	ATLASSERT(pidl1 != NULL); ATLASSERT(pidl2 != NULL);
	ATLASSERT(m_RemotePidlManager.GetFilename(pidl1).GetLength() > 0 );
	ATLASSERT(m_RemotePidlManager.GetFilename(pidl2).GetLength() > 0 );

	// Sort by filename
	return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 
		(unsigned short)wcscmp(m_RemotePidlManager.GetFilename(pidl1), 
		                       m_RemotePidlManager.GetFilename(pidl2)));
}

/*------------------------------------------------------------------------------
 * CRemoteFolder::EnumSearches : IShellFolder2
 * Returns pointer to interface allowing client to enumerate search objects.
 * We do not support search objects.
 *----------------------------------------------------------------------------*/
STDMETHODIMP CRemoteFolder::EnumSearches( IEnumExtraSearch **ppEnum )
{
	ATLTRACE("CRemoteFolder::EnumSearches called\n");
	(void)ppEnum;
	return E_NOINTERFACE;
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

STDMETHODIMP CRemoteFolder::ColumnClick( UINT iColumn )
{
	ATLTRACE("CRemoteFolder::ColumnClick called\n");
	(void)iColumn;
	return S_FALSE;
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
		return S_FALSE;
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
		ATLASSERT(::ILIsEqual(m_pidl, pidlFolder));

		// Build up a list of PIDLs for all the items to be deleted
		RemotePidls vecDeathRow;
		for (UINT i = 0; i < shdo.GetPidlCount(); i++)
		{
			ATLTRACE("PIDL path: %ls\n", _ExtractPathFromPIDL(shdo.GetFile(i)));

			CRemoteRelativePidl pidlFile = shdo.GetRelativeFile(i);

			// May be overkill (it should always be a child) but check anyway
			// because we don't want to accidentally recursively delete the root
			// of a folder tree
			if (::ILIsChild(pidlFile) && !::ILIsEmpty(pidlFile))
			{
				CRemoteChildPidl pidlChild = 
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
		const CRemoteChildPidl& pidl = vecDeathRow[0];
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
	CString strPath = _ExtractPathFromPIDL( m_pidl );
	ATLASSERT(!strPath.IsEmpty());

	// Create instance of our directory handler class
	CSftpDirectory directory( conn, strPath );

	// Delete each item and notify shell
	RemotePidls::const_iterator it = vecDeathRow.begin();
	while (it != vecDeathRow.end())
	{
		directory.Delete( *it );

		// Make PIDL absolute
		CAbsolutePidl pidlFull(m_pidl);
		pidlFull = pidlFull.Join(*it);

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

CString CRemoteFolder::_GetFilenameFromPIDL( PCUITEMID_CHILD pidl )
{
	ATLTRACE("CRemoteFolder::_GetFilenameFromPIDL called\n");
	ATLASSERT(SUCCEEDED(m_RemotePidlManager.IsValid(pidl)));

	// Extract filename from REMOTEPIDL
	CString strName = m_RemotePidlManager.GetFilename(pidl);

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
 * Get a list names of registry keys for the types of the selected file.
 * These keys hold information such as the context menu items and default
 * actions.  (Ficticious) examples include:
 *   HKCU\.ppt
 *   HKCU\PowerPoint.Show
 *   HKCU\PowerPoint.Show.12
 *   HKCU\SystemFileAssociations\.ppt
 *   HKCU\SystemFileAssociations\presentation
 *   HKCU\*
 *   HKCU\AllFileSystemObjects
 * for a file and:
 *   HKCU\Directory
 *   HKCU\Directory\Background
 *   HKCU\Folder
 *   HKCU\*
 *   HKCU\AllFileSystemObjects
 * for a folder.
 *
 * Although we pass in an array of PIDLs, only the first one (the one that is
 * right-clicked on in the case of a context menu
 */
HRESULT CRemoteFolder::_GetRegistryKeysForPidl(
	PCUITEMID_CHILD pidl, UINT *pcKeys, HKEY **paKeys )
{
	LSTATUS rc = ERROR_SUCCESS;	
	vector<CString> vecKeynames;

	// If this is a directory, add directory-specific items
	if (m_RemotePidlManager.IsFolder( pidl ))
	{
		vecKeynames.push_back(_T("Directory"));
		vecKeynames.push_back(_T("Directory\\Background"));
		vecKeynames.push_back(_T("Folder"));
	}
	else
	{
		// Get extension-specific keys
		// We don't want to add the {.ext} key itself to the list of keys but 
		// rather, we should use it's default value to look up its file class. 
		// e.g:
		//   HKCR\.txt => (Default) txtfile
		// so we look up the following key
		//   HKCR\txtfile
		vector<CString> vecExt = _GetExtensionSpecificKeynames( 
			_GetFileExtensionFromPIDL(pidl) );
		vecKeynames.insert(vecKeynames.end(), vecExt.begin(), vecExt.end());
	}

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
 * Get the list of names of registry keys related to a specific file extension.
 *
 * @param szExtension  The extension whose keys will be returned.
 */
vector<CString> CRemoteFolder::_GetExtensionSpecificKeynames(
	PCTSTR szExtension )
{
	ATLTRACE("CRemoteFolder::_GetExtensionSpecificKeynames called\n");
	vector<CString> vecKeynames;
	CString strExtension = CString(_T(".")) + szExtension;

	// Start digging at HKCR\.{szExtension}
	CRegKey reg;
	if (reg.Open(HKEY_CLASSES_ROOT, strExtension, KEY_READ)
		== ERROR_SUCCESS)
	{
		vecKeynames.push_back(strExtension);

		// Try to get registered file class key (extensions's default val)
		TCHAR szClass[2048]; ULONG cchClass = 2048;
		if (reg.QueryStringValue(_T(""), szClass, &cchClass) == ERROR_SUCCESS
		 && cchClass > 1
		 && reg.Open(HKEY_CLASSES_ROOT, szClass, KEY_READ) == ERROR_SUCCESS)
		{
			vecKeynames.push_back(szClass);

			// Does this class contain a CurVer subkey pointing to another
			// version of this file.
			//   e.g.: PowerPoint.Show\CurVer => PowerPoint.Show.12
			CString strCurVer = szClass;
			strCurVer += _T("\\CurVer");
			if (reg.Open(HKEY_CLASSES_ROOT, strCurVer, KEY_READ)
				== ERROR_SUCCESS)
			{
				// Does this CurVer exist?
				TCHAR szCurVer[2048]; ULONG cchCurVer = 2048;
				if (reg.QueryStringValue(_T(""), szCurVer, &cchCurVer) ==
					ERROR_SUCCESS && cchCurVer > 1
				 && reg.Open(HKEY_CLASSES_ROOT, szCurVer, KEY_READ) == 
					ERROR_SUCCESS)
				{
					vecKeynames.push_back(szCurVer);
				}
			}
		}
	}

	// Dig again at HKCR\SystemFileAssociations\.{sxExtension}
	CString strSysFileAssocExt = _T("SystemFileAssociations\\") + strExtension;
	if (reg.Open(HKEY_CLASSES_ROOT, strSysFileAssocExt, KEY_READ)
		== ERROR_SUCCESS)
	{
		vecKeynames.push_back(strSysFileAssocExt);
	}

	// Dig again at HKCR\.{szExtension}\PerceivedType
	if (reg.Open(HKEY_CLASSES_ROOT, strExtension, KEY_READ)
		== ERROR_SUCCESS)
	{
		TCHAR szPerceivedType[2048]; ULONG cchPerceivedType = 2048;
		if (reg.QueryStringValue(
				_T("PerceivedType"), szPerceivedType, &cchPerceivedType)
			== ERROR_SUCCESS && cchPerceivedType > 1)
		{
			CString strPerceivedType = 
				CString(_T("SystemFileAssociations\\")) + szPerceivedType;

			if (reg.Open(HKEY_CLASSES_ROOT, strPerceivedType, KEY_READ)
				== ERROR_SUCCESS)
				vecKeynames.push_back(strPerceivedType);
		}
	}

	if (!vecKeynames.size())
		vecKeynames.push_back(_T("Unknown"));

	ATLASSERT( vecKeynames.size() <= 5 ); 
	return vecKeynames;
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
	ATLASSERT(SUCCEEDED(m_RemotePidlManager.IsValid(pidl)));

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
	ATLASSERT(m_pidl);

	// Find HOSTPIDL part of this folder's absolute pidl to extract server info
	PCUIDLIST_RELATIVE pidlHost = m_HostPidlManager.FindHostPidl( m_pidl );
	ATLASSERT(pidlHost);

	// Extract connection info from PIDL
	CString strUser, strHost, strPath;
	USHORT uPort;
	ATLASSERT(SUCCEEDED( m_HostPidlManager.IsValid(pidlHost) ));
	strHost = m_HostPidlManager.GetHost( pidlHost );
	uPort = m_HostPidlManager.GetPort( pidlHost );
	strUser = m_HostPidlManager.GetUser( pidlHost );
	ATLASSERT(!strUser.IsEmpty());
	ATLASSERT(!strHost.IsEmpty());

	// Return connection from session pool
	return _GetConnection(hwndUserInteraction, strHost, strUser, uPort);
}

// CRemoteFolder
