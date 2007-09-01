// SwishFolder.cpp : Implementation of CSwishFolder

#include "stdafx.h"
#include "SwishFolder.h"
#include "ConnCopyPolicy.h"
#include "SwishView.h"

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
	PIDLCONNDATA pDataTemp;

    if ( NULL == ppEnumIDList )
        return E_POINTER;

    *ppEnumIDList = NULL;

    m_vecConnData.clear();

	wcscpy_s(pDataTemp.wszUser, MAX_USERNAME_LEN, L"user1");
	wcscpy_s(pDataTemp.wszHost, MAX_HOSTNAME_LEN, L"host1.example.com");
	wcscpy_s(pDataTemp.wszPath, MAX_PATH_LEN, L"/home/user1");
	pDataTemp.uPort = 22;
	m_vecConnData.push_back(pDataTemp);

	wcscpy_s(pDataTemp.wszUser, MAX_USERNAME_LEN, L"user2");
	wcscpy_s(pDataTemp.wszHost, MAX_HOSTNAME_LEN, L"host2.example.com");
	wcscpy_s(pDataTemp.wszPath, MAX_PATH_LEN, L"/home/user2");
	pDataTemp.uPort = 22;
	m_vecConnData.push_back(pDataTemp);

	wcscpy_s(pDataTemp.wszUser, MAX_USERNAME_LEN, L"user3");
	wcscpy_s(pDataTemp.wszHost, MAX_HOSTNAME_LEN, L"host3.example.com");
	wcscpy_s(pDataTemp.wszPath, MAX_PATH_LEN, L"/home/user3");
	pDataTemp.uPort = 22;
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
STDMETHODIMP CSwishFolder::CreateViewObject( HWND hwndOwner, 
                                             REFIID riid, void** ppvOut )
{
	ATLTRACE("CSwishFolder::CreateViewObject called\n");

	HRESULT hr;
	CComObject<CSwishView>* pSwishView;

    if ( NULL == ppvOut )
        return E_POINTER;

    *ppvOut = NULL;

    // Create a new CSwishView COM object.
    hr = CComObject<CSwishView>::CreateInstance ( &pSwishView );

    if ( FAILED(hr) )
        return hr;

    // AddRef() the object while we're using it.
    pSwishView->AddRef();
    
	ATLTRACE("Created new CSwishView COM object instance\n");

    // Object initialization - pass the object its containing folder (this).
    hr = pSwishView->_init( this );
    if ( FAILED(hr) )
    {
		pSwishView->Release();
		return hr;
    }

    // Return the requested interface back to the shell.
    hr = pSwishView->QueryInterface( riid, ppvOut );

    pSwishView->Release();

    return hr;
}

// GetUIObjectOf() returns an interface used to handle events in Explorer's
// tree control.
STDMETHODIMP CSwishFolder::GetUIObjectOf( HWND hwndOwner, UINT uCount,
                                          LPCITEMIDLIST* pPidl, REFIID riid,
                                          LPUINT puReserved, void** ppvReturn )
{
	ATLTRACE("CSwishFolder::GetUIObjectOf called\n");

	*ppvReturn = NULL;

    return E_NOINTERFACE;
}

// GetAttributesOf() returns the attributes for the items whose PIDLs are
// passed in.
STDMETHODIMP CSwishFolder::GetAttributesOf( UINT uCount, LPCITEMIDLIST aPidls[],
                                            LPDWORD pdwAttribs )
{
    // Our items are all just plain ol' items, so the attributes are always
    // 0.  Normally, the attributes returned come from the SFGAO_* constants.

	ATLTRACE("CSwishFolder::GetAttributesOf called\n");

    *pdwAttribs = 0;
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

	ATLASSERT(lParam != NULL); 
	ATLASSERT(pidl1 != NULL); ATLASSERT(pidl2 != NULL);

	LPPIDLDATA pData1 = (LPPIDLDATA) pidl1;
	LPPIDLDATA pData2 = (LPPIDLDATA) pidl2;

	// Rough guestimate: country-code + .
	ATLASSERT(wcslen(pData1->wszHost) > 3 );
	ATLASSERT(wcslen(pData2->wszHost) > 3 );

	return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 
		(unsigned short)wcscmp(pData1->wszHost, pData2->wszHost));
}

// CSwishFolder

