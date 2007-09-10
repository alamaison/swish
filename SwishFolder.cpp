// SwishFolder.cpp : Implementation of CSwishFolder

#include "stdafx.h"
#include "remotelimits.h"
#include "SwishFolder.h"
#include "ConnCopyPolicy.h"

/*------------------------------------------------------------------------------
 * CSwishFolder::GetClassID() : IPersist
 * Retrieves the class identifier (CLSID) of the object
 *----------------------------------------------------------------------------*/
STDMETHODIMP CSwishFolder::GetClassID( CLSID* pClsid )
{
	ATLTRACE("CSwishFolder::GetClassID called\n");

    if (pClsid == NULL)
        return E_POINTER;

	*pClsid = __uuidof(CSwishFolder);
    return S_OK;
}

/*------------------------------------------------------------------------------
 * CSwishFolder::Initialize() : IPersistFolder
 * Assigns a fully qualified PIDL to the new object which we store for later
 *----------------------------------------------------------------------------*/
STDMETHODIMP CSwishFolder::Initialize( LPCITEMIDLIST pidl )
{
	ATLTRACE("CSwishFolder::Initialize called\n");

	ATLASSERT( pidl != NULL );
    m_pidlRoot = m_PidlManager.Copy( pidl );
	return S_OK;
}

/*------------------------------------------------------------------------------
 * CSwishFolder::BindToObject : IShellFolder
 * Subfolder of root folder opened: Create and initialize new CSwishFolder 
 * COM object to represent subfolder and return its IShellFolder interface
 *----------------------------------------------------------------------------*/
STDMETHODIMP CSwishFolder::BindToObject( LPCITEMIDLIST pidl, LPBC pbcReserved, 
										 REFIID riid, void** ppvOut )
{
	ATLTRACE("CSwishFolder::BindToObject called\n");

	HRESULT hr;
	CComObject<CSwishFolder> *pSwishFolder;

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

	hr = CComObject<CSwishFolder>::CreateInstance( &pSwishFolder );

	if (FAILED(hr))
        return hr;

    pSwishFolder->AddRef();

	// Retrieve requested interface
	hr = pSwishFolder->QueryInterface( riid, ppvOut );
	if (FAILED(hr))
	{
		pSwishFolder->Release();
		return E_NOINTERFACE;
	}

	// Object initialization - pass the object its parent folder (this)
    // and the pidl it will be browsing to.
    hr = pSwishFolder->_init( this, pidl );
	if (FAILED(hr))
		pSwishFolder->Release();

	return hr;
}

// EnumObjects() creates a COM object that implements IEnumIDList.
STDMETHODIMP CSwishFolder::EnumObjects( HWND hwndOwner, DWORD dwFlags, 
										LPENUMIDLIST* ppEnumIDList )
{
	ATLTRACE("CSwishFolder::EnumObjects called\n");

	HRESULT hr;
	HOSTPIDL pDataTemp;

    if ( NULL == ppEnumIDList )
        return E_POINTER;

    *ppEnumIDList = NULL;

    m_vecConnData.clear();

	wcscpy_s(pDataTemp.wszLabel, MAX_LABEL_LENZ, L"Example Host 1");
	wcscpy_s(pDataTemp.wszUser, MAX_USERNAME_LEN, L"user1");
	wcscpy_s(pDataTemp.wszHost, MAX_HOSTNAME_LEN, L"host1.example.com");
	wcscpy_s(pDataTemp.wszPath, MAX_PATH_LEN, L"/home/user1");
	pDataTemp.uPort = 22;
	m_vecConnData.push_back(pDataTemp);

	wcscpy_s(pDataTemp.wszLabel, MAX_LABEL_LENZ, L"Café, prix 7€");
	wcscpy_s(pDataTemp.wszUser, MAX_USERNAME_LEN, L"user2");
	wcscpy_s(pDataTemp.wszHost, MAX_HOSTNAME_LEN, L"host2.example.com");
	wcscpy_s(pDataTemp.wszPath, MAX_PATH_LEN, L"/home/user2");
	pDataTemp.uPort = 22;
	m_vecConnData.push_back(pDataTemp);

//	wcscpy_s(pDataTemp.wszLabel, MAX_LABEL_LENZ, L"ﻛﻤﺒﻴﻮﺗﺮ");
	wcscpy_s(pDataTemp.wszLabel, MAX_LABEL_LENZ, L"العربية");
	wcscpy_s(pDataTemp.wszUser, MAX_USERNAME_LEN, L"شيدا");
	wcscpy_s(pDataTemp.wszHost, MAX_HOSTNAME_LEN, L"host3.example.com");
	wcscpy_s(pDataTemp.wszPath, MAX_PATH_LEN, L"/home/شيدا");
	pDataTemp.uPort = 2222;

	m_vecConnData.push_back(pDataTemp);

    // Create an enumerator with CComEnumOnSTL<> and our copy policy class.
	CComObject<CEnumIDListImpl>* pEnum;

    hr = CComObject<CEnumIDListImpl>::CreateInstance ( &pEnum );

    if ( FAILED(hr) )
        return hr;

    // AddRef() the object while we're using it.

    pEnum->AddRef();

    // Init the enumerator.  Init() will AddRef() our IUnknown (obtained with
    // GetUnknown()) so this object will stay alive as long as the enumerator 
    // needs access to the vector m_vecDriveLtrs.

    hr = pEnum->Init ( GetUnknown(), m_vecConnData );

    // Return an IEnumIDList interface to the caller.

    if ( SUCCEEDED(hr) )
        hr = pEnum->QueryInterface ( IID_IEnumIDList, (void**) ppEnumIDList );

    pEnum->Release();

    return hr;
}

// CreateViewObject() creates a new COM object that implements IShellView.
STDMETHODIMP CSwishFolder::CreateViewObject( __in_opt HWND hwndOwner, 
                                             __in REFIID riid, 
											 __deref_out void **ppvOut )
{
	ATLTRACE("CSwishFolder::CreateViewObject called\n");
	
    *ppvOut = NULL;

	HRESULT hr = E_NOINTERFACE;

	if (riid == IID_IShellView)
	{            
		SFV_CREATE sfvData = { sizeof(sfvData), 0 };

		hr = QueryInterface( IID_PPV_ARGS(&sfvData.pshf) );
		if (SUCCEEDED(hr))
		{
			sfvData.psvOuter = NULL;
			sfvData.psfvcb = NULL; 
			hr = SHCreateShellFolderView( &sfvData, (IShellView**)ppvOut );
			sfvData.pshf->Release();
		}
	}
	else if (riid == IID_IShellDetails)
    {
		return QueryInterface(riid, ppvOut);
    }

    return hr;
}

/*------------------------------------------------------------------------------
 * CSwishFolder::GetUIObjectOf : IShellFolder
 * Retrieve an optional interface supported by one of the object in the folder.
 * This method is called when the shell is requesting extra inforation
 * about an object such as its icon, context menu, thumnail image etc.
 *----------------------------------------------------------------------------*/
STDMETHODIMP CSwishFolder::GetUIObjectOf( HWND hwndOwner, UINT cPidl,
	__in_ecount_opt(cPidl) PCUITEMID_CHILD_ARRAY aPidl, REFIID riid,
	__reserved LPUINT puReserved, __out void** ppvReturn )
{
	ATLTRACE("CSwishFolder::GetUIObjectOf called\n");

	*ppvReturn = NULL;
	
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
		ATLASSERT( cPidl == 1 ); // Only one file 'selected'
		return QueryInterface(riid, ppvReturn);
    }

    return E_NOINTERFACE;
}

/*------------------------------------------------------------------------------
 * CSwishFolder::GetDisplayNameOf : IShellFolder
 * Retrieves the display name for the specified file object or subfolder.
 *----------------------------------------------------------------------------*/
STDMETHODIMP CSwishFolder::GetDisplayNameOf( __in PCUITEMID_CHILD pidl, 
											 __in SHGDNF uFlags, 
											 __out STRRET *pName )
{
	ATLTRACE("CSwishFolder::GetUIObjectOf called\n");

	CString strName;

	if (uFlags & SHGDN_FORPARSING)
	{
		// We do not care if the name is relative to the folder or the
		// desktop for the parsing name - always return canonical string:
		//     sftp://username@hostname:port/path

		// TODO:  if !SHGDN_INFOLDER the pidl may not be single-level
		// so we should always seek to the last pidl before extracting info

		strName = GetLongNameFromPIDL(pidl, true);
	}
	else if(uFlags & SHGDN_FORADDRESSBAR)
	{
		// We do not care if the name is relative to the folder or the
		// desktop for the parsing name - always return canonical string:
		//     sftp://username@hostname:port/path
		// unless the port is the default port in which case it is ommitted:
		//     sftp://username@hostname/path

		strName = GetLongNameFromPIDL(pidl, false);
	}
	else
	{
		// We do not care if the name is relative to the folder or the
		// desktop for the parsing name - always return the label:
		ATLASSERT(uFlags == SHGDN_NORMAL || uFlags == SHGDN_INFOLDER ||
			(uFlags & SHGDN_FOREDITING));

		strName = GetLabelFromPIDL(pidl);
	}

	// Store in a STRRET and return
	pName->uType = STRRET_WSTR;

	return SHStrDupW( strName, &pName->pOleStr );
}




/*------------------------------------------------------------------------------
 * CSwishFolder::GetAttributesOf : IShellFolder
 * Returns the attributes for the items whose PIDLs are passed in.
 *----------------------------------------------------------------------------*/
STDMETHODIMP CSwishFolder::GetAttributesOf(
	UINT cIdl,
	__in_ecount_opt( cIdl ) PCUITEMID_CHILD_ARRAY aPidl,
	__inout SFGAOF *pdwAttribs )
{
	ATLTRACE("CSwishFolder::GetAttributesOf called\n");

	DWORD dwAttribs = 0;
    dwAttribs |= SFGAO_FOLDER;
    dwAttribs |= SFGAO_HASSUBFOLDER;
    *pdwAttribs &= dwAttribs;

    return S_OK;
}

/*------------------------------------------------------------------------------
 * CSwishFolder::CompareIDs : IShellFolder
 * Determines the relative order of two file objects or folders.
 * Given their item identifier lists, the two objects are compared and a
 * result code is returned.
 *   Negative: pidl1 < pidl2
 *   Positive: pidl1 > pidl2
 *   Zero:     pidl1 == pidl2
 *----------------------------------------------------------------------------*/
STDMETHODIMP CSwishFolder::CompareIDs( LPARAM lParam, LPCITEMIDLIST pidl1,
									   LPCITEMIDLIST pidl2 )
{
	ATLTRACE("CSwishFolder::CompareIDs called\n");

	ATLASSERT(pidl1 != NULL); ATLASSERT(pidl2 != NULL);

	// Rough guestimate: country-code + .
	ATLASSERT(m_PidlManager.GetHost(pidl1).GetLength() > 3 );
	ATLASSERT(m_PidlManager.GetHost(pidl2).GetLength() > 3 );

	return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 
		(unsigned short)wcscmp(m_PidlManager.GetHost(pidl1), 
		                       m_PidlManager.GetHost(pidl2)));
}

/*------------------------------------------------------------------------------
 * CSwishFolder::EnumSearches : IShellFolder2
 * Returns pointer to interface allowing client to enumerate search objects.
 * We do not support search objects.
 *----------------------------------------------------------------------------*/
STDMETHODIMP CSwishFolder::EnumSearches( IEnumExtraSearch **ppEnum )
{
	return E_NOINTERFACE;
}

/*------------------------------------------------------------------------------
 * CSwishFolder::GetDefaultColumn : IShellFolder2
 * Gets the default sorting and display columns.
 *----------------------------------------------------------------------------*/
STDMETHODIMP CSwishFolder::GetDefaultColumn( DWORD dwReserved, 
											 __out ULONG *pSort, 
											 __out ULONG *pDisplay )
{
	// Sort and display by the label (friendly display name)
	*pSort = 0;
	*pDisplay = 0;

	return S_OK;
}

/*------------------------------------------------------------------------------
 * CSwishFolder::GetDefaultColumnState : IShellFolder2
 * Returns the default state for the column specified by index.
 *----------------------------------------------------------------------------*/
STDMETHODIMP CSwishFolder::GetDefaultColumnState( __in UINT iColumn, 
												  __out SHCOLSTATEF *pcsFlags )
{
	switch (iColumn)
	{
	case 0: // Display name (Label)
	case 1: // Hostname
	case 2: // Username
	case 4: // Remote filesystem path
		*pcsFlags = SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT; break;
	case 3: // SFTP port
		*pcsFlags = SHCOLSTATE_TYPE_INT | SHCOLSTATE_ONBYDEFAULT; break;
	default:
		return E_FAIL;
	}

	return S_OK;
}

STDMETHODIMP CSwishFolder::GetDetailsEx( __in PCUITEMID_CHILD pidl, 
										 __in const SHCOLUMNID *pscid,
										 __out VARIANT *pv )
{
	// Only if request is tagged with Swish host property GUID
	ATLASSERT(pscid->fmtid == FMTID_SWISH_HOST);
	if(pscid->fmtid == FMTID_SWISH_HOST)
	{
		VariantInit( pv );
		pv->vt = VT_BSTR;

		if(pidl)
		{
			// Request is for an item detail.  Retrieve from pidl and
			// return string and string length

			CString strSrc;

			switch (pscid->pid)
			{
			case PID_SWISH_HOST_LABEL: // Display name (Label)
				strSrc = m_PidlManager.GetLabel(pidl); break;
			case PID_SWISH_HOST_HOST: // Hostname
				strSrc = m_PidlManager.GetHost(pidl); break;
			case PID_SWISH_HOST_USER: // Username
				strSrc = m_PidlManager.GetUser(pidl); break;
			case PID_SWISH_HOST_PORT: // SFTP port
				strSrc = m_PidlManager.GetPortStr(pidl); break;
			case PID_SWISH_HOST_PATH: // Remote filesystem path
				strSrc = m_PidlManager.GetPath(pidl); break;
			default:
				return E_FAIL;
			}

			pv->bstrVal = strSrc.AllocSysString();
		}
		else
		{
			// Request is for a column heading.  Return heading as BSTR
			
			BSTR bstrHeading;

			switch (pscid->pid)
			{
			case PID_SWISH_HOST_LABEL: // Display name (Label)
				bstrHeading = ::SysAllocString( L"NameEx" ); break;
			case PID_SWISH_HOST_HOST: // Hostname
				bstrHeading = ::SysAllocString( L"HostEx" ); break;
			case PID_SWISH_HOST_USER: // Username
				bstrHeading = ::SysAllocString( L"UsernameEx" ); break;
			case PID_SWISH_HOST_PORT: // SFTP port
				bstrHeading = ::SysAllocString( L"PortEx" ); break;
			case PID_SWISH_HOST_PATH: // Remote filesystem path
				bstrHeading = ::SysAllocString( L"Remote PathEx" ); break;
			default:
				return E_FAIL;
			}

			if (bstrHeading == NULL)
				return E_OUTOFMEMORY;
			pv->bstrVal = bstrHeading;
		}
		return S_OK;
	}

	return E_FAIL;
}

/*------------------------------------------------------------------------------
 * CSwishFolder::GetDefaultColumnState : IShellFolder2
 * Convert column to appropriate property set ID (FMTID) and property ID (PID).
 *----------------------------------------------------------------------------*/
STDMETHODIMP CSwishFolder::MapColumnToSCID( __in UINT iColumn, 
										    __out PROPERTYKEY *pscid )
{
	switch (iColumn)
	{
	case 0: // Display name (Label)
		pscid->pid = PID_SWISH_HOST_LABEL; break;
	case 1: // Hostname
		pscid->pid = PID_SWISH_HOST_HOST; break;
	case 2: // Username
		pscid->pid = PID_SWISH_HOST_USER; break;
	case 3: // SFTP port
		pscid->pid = PID_SWISH_HOST_PORT; break;
	case 4: // Remote filesystem path
		pscid->pid = PID_SWISH_HOST_PATH; break;
	default:
		return E_FAIL;
	}

	// Tag as correct class of property: Swish Host
	pscid->fmtid = FMTID_SWISH_HOST;

	return S_OK;
}

/*------------------------------------------------------------------------------
 * CSwishFolder::CompareIDs : IExtractIcon
 * Extract an icon bitmap given the information passed.
 * We return S_FALSE to tell the shell to extract the icons itself.
 *----------------------------------------------------------------------------*/
STDMETHODIMP CSwishFolder::Extract(
	LPCTSTR pszFile, UINT nIconIndex, HICON *phiconLarge, 
	HICON *phiconSmall, UINT nIconSize )
{
	ATLTRACE("CSwishFolder::Extract called\n");
	return S_FALSE;
}

/*------------------------------------------------------------------------------
 * CSwishFolder::CompareIDs : IExtractIcon
 * Retrieve the location of the appropriate icon.
 * We set all SFTP hosts to have the icon from shell32.dll.
 *----------------------------------------------------------------------------*/
STDMETHODIMP CSwishFolder::GetIconLocation(
	__in UINT uFlags, __out_ecount(cchMax) LPTSTR szIconFile, 
	__in UINT cchMax, __out int *piIndex, __out UINT *pwFlags )
{
	ATLTRACE("CSwishFolder::GetIconLocation called\n");

	// uFlags (specifying which type of use) are ignored for host folder

	// Set host to have the ICS host icon
	StringCchCopy(szIconFile, cchMax, _T("C:\\WINDOWS\\system32\\shell32.dll"));
	*piIndex = 17;
	*pwFlags = GIL_DONTCACHE;

	return S_OK;
}

/*------------------------------------------------------------------------------
 * CSwishFolder::GetDetailsOf : IShellDetails
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
 *----------------------------------------------------------------------------*/
STDMETHODIMP CSwishFolder::GetDetailsOf( __in_opt PCUITEMID_CHILD pidl, 
										 __in UINT iColumn, 
										 __out LPSHELLDETAILS pDetails )
{
	ATLTRACE("CSwishFolder::GetDetailsOf called, iColumn=%u\n", iColumn);
	ATLASSERT(pDetails);

	CString strSrc;
	PWSTR *pStr = &pDetails->str.pOleStr;
	pDetails->str.uType = STRRET_WSTR;

	if(pidl)
	{
		// Request is for an item detail.  Retrieve from pidl and
		// return string and string length

		switch (iColumn)
		{
		case 0: // Display name (Label)
			strSrc = m_PidlManager.GetLabel(pidl); break;
		case 1: // Hostname
			strSrc = m_PidlManager.GetHost(pidl); break;
		case 2: // Username
			strSrc = m_PidlManager.GetUser(pidl); break;
		case 3: // SFTP port
			strSrc = m_PidlManager.GetPortStr(pidl); break;
		case 4: // Remote filesystem path
			strSrc = m_PidlManager.GetPath(pidl); break;
		default:
			return E_FAIL;
		}
		
		//pDetails->cxChar = strSrc.GetLength();
		return SHStrDupW( strSrc, pStr );
	}
	else
	{
		// Request is for a column heading.  Return heading and its length

		pDetails->fmt = LVCFMT_LEFT;

		switch (iColumn)
		{
		case 0: // Display name (Label)
			pDetails->cxChar = 25;
			return SHStrDupW( L"Name", pStr );
		case 1: // Hostname
			pDetails->cxChar = 24;
			return SHStrDupW( L"Host", pStr );
		case 2: // Username
			pDetails->cxChar = 12;
			return SHStrDupW( L"Username", pStr );
		case 3: // SFTP port
			pDetails->cxChar = 7;
			return SHStrDupW( L"Port", pStr );
		case 4: // Remote filesystem path
			pDetails->cxChar = 32;
			return SHStrDupW( L"Remote Path", pStr );
		default:
			return E_FAIL;
		}
	}

	UNREACHABLE;
}

STDMETHODIMP CSwishFolder::ColumnClick( UINT iColumn )
{
	ATLTRACE("CSwishFolder::ColumnClick called\n");
	return S_FALSE;
}

/*----------------------------------------------------------------------------*/
/* --- Private functions -----------------------------------------------------*/
/*----------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
 * CSwishFolder::GetLongNameFromPIDL
 * Retrieve the long name of the host connection from the given PIDL.
 * The long name is either the canonical form if fCanonical is set:
 *     sftp://username@hostname:port/path
 * or, if not set and if the port is the default port the reduced form:
 *     sftp://username@hostname/path
 *----------------------------------------------------------------------------*/
CString CSwishFolder::GetLongNameFromPIDL( PCUITEMID_CHILD pidl, 
										   BOOL fCanonical )
{
	CString strName;
	ATLASSERT(SUCCEEDED(m_PidlManager.IsValid(pidl)));

	// Construct string from info in PIDL
	strName = _T("sftp://");
	strName += m_PidlManager.GetUser(pidl);
	strName += _T("@");
	strName += m_PidlManager.GetHost(pidl);
	if (m_PidlManager.GetPort(pidl) != SFTP_DEFAULT_PORT)
	{
		strName += _T(":");
		strName.AppendFormat( _T("%u"), m_PidlManager.GetPort(pidl) );
	}
	strName += _T("/");
	strName += m_PidlManager.GetPath(pidl);

	ATLASSERT( strName.GetLength() <= MAX_CANONICAL_LEN );

	return strName;
}

/*------------------------------------------------------------------------------
 * CSwishFolder::GetLabelFromPIDL
 * Retrieve the user-assigned label of the host connection from the given PIDL.
 *----------------------------------------------------------------------------*/
CString CSwishFolder::GetLabelFromPIDL( PCUITEMID_CHILD pidl )
{
	CString strName;
	ATLASSERT(SUCCEEDED(m_PidlManager.IsValid(pidl)));

	// Construct string from info in PIDL
	strName = m_PidlManager.GetLabel(pidl);

	ATLASSERT( strName.GetLength() <= MAX_LABEL_LEN );

	return strName;
}

// CSwishFolder

