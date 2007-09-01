// SwishView.cpp : Implementation of CSwishView

#include "stdafx.h"
#include "SwishView.h"

UINT CSwishView::sm_uListID = 101;

/*------------------------------------------------------------------------------
 * CSwishView::GetWindow : IOleWindow
 * Return window handle.
 *----------------------------------------------------------------------------*/
STDMETHODIMP CSwishView::GetWindow( HWND* phwnd )
{
	ATLTRACE("CSwishView::GetWindow called\n");

    *phwnd = m_hWnd;
    return S_OK;
}

/*------------------------------------------------------------------------------
 * CSwishView::CreateViewWindow : IShellView
 * Create container window for folder view.
 * This window houses the ListView control that displays the folder contents
 *----------------------------------------------------------------------------*/
STDMETHODIMP CSwishView::CreateViewWindow( IShellView *psvPrevious, 
										   LPCFOLDERSETTINGS pFolderSettings, 
										   IShellBrowser *pShellBrowser, 
										   RECT *prcView, HWND *phWnd )
{
	ATLTRACE("CSwishView::CreateViewWindow called\n");

	// Save information passed by Explorer
	m_FolderSettings = *pFolderSettings;
	m_spShellBrowser = pShellBrowser;
	m_spShellBrowser->GetWindow(&m_hWndParent);
	
	// Create window
    if (Create(m_hWndParent, *prcView) == NULL)
	{
		UNREACHABLE
		return E_FAIL;
	}

	ATLTRACE("SwishView window created\n");

	*phWnd = m_hWnd;

	return S_OK;
}

/*------------------------------------------------------------------------------
 * CSwishView::DestroyViewWindow : IShellView
 * Clean up all states that represent the view, including the window 
 * and any other associated resources.
 *----------------------------------------------------------------------------*/
STDMETHODIMP CSwishView::DestroyViewWindow()
{
	ATLTRACE("CSwishView::DestroyViewWindow called\n");

	UIActivate ( SVUIA_DEACTIVATE );

	DestroyWindow();

	return S_OK;
}


/*------------------------------------------------------------------------------
 * CSwishView::GetCurrentInfo : IShellView
 * Get the current folder settings.
 * Prior to switching views, Windows Explorer will call this method to request 
 * the current FOLDERSETTINGS values to pass them to the next folder view object
 *----------------------------------------------------------------------------*/
STDMETHODIMP CSwishView::GetCurrentInfo( LPFOLDERSETTINGS lpfs )
{
	ATLTRACE("CSwishView::GetCurrentInfo called\n");

	*lpfs = m_FolderSettings;
	return S_OK;
}

/*------------------------------------------------------------------------------
 * CSwishView::Refresh : IShellView
 * User selected Refresh from the View menu or pressed the F5 key. 
 * Update the folder view display by reloading data from remote SFTP system
 *----------------------------------------------------------------------------*/
STDMETHODIMP CSwishView::Refresh()
{
	ATLTRACE("CSwishView::Refresh called\n");

	// TODO: replace stub
	return S_OK;
}

/*------------------------------------------------------------------------------
 * CSwishView::SaveViewState : IShellView
 * Saves the Shell's view settings.
 * The current state can be restored during a subsequent browsing session.
 *----------------------------------------------------------------------------*/
STDMETHODIMP CSwishView::SaveViewState()
{
	ATLTRACE("CSwishView::SaveViewState called\n");

	IStream *pViewStateStream;
	m_spShellBrowser->GetViewStateStream(STGM_WRITE, &pViewStateStream);
	pViewStateStream->AddRef();

	// TODO: use this stream to save view data

	pViewStateStream->Release();

	return S_OK;
}

/*------------------------------------------------------------------------------
 * CSwishView::UIActivate : IShellView
 * Track folder view state: activated with/without focus, deactivated.
 * The activation state of the view window is changed by an external event.
 * This is not caused by the Shell view itself, for example, if the TAB key was
 * pressed when the tree had the focus, the view should be given the focus.
 *----------------------------------------------------------------------------*/
STDMETHODIMP CSwishView::UIActivate( UINT uState )
{
	ATLTRACE("CSwishView::UIActivate called\n");

/*
SVUIA_ACTIVATE_FOCUS
    Microsoft Windows Explorer has just created the view window with the input focus. This means the Shell view should be able to set menu items appropriate for the focused state.
SVUIA_ACTIVATE_NOFOCUS
    The Shell view is losing the input focus, or it has just been created without the input focus. The Shell view should be able to set menu items appropriate for the nonfocused state. This means no selection-specific items should be added. 
SVUIA_DEACTIVATE
    Windows Explorer is about to destroy the Shell view window. The Shell view should remove all extended user interfaces. These are typically merged menus and merged modeless pop-up windows.
	*/

	return S_OK;
}
/////////////////////////////////////////////////////////////////////////////
// Message handlers

LRESULT CSwishView::OnCreate( UINT uMsg, WPARAM wParam, 
                              LPARAM lParam, BOOL& bHandled )
{
	ATLTRACE("CSwishView::OnCreate called\n");

HWND hwndList;
DWORD dwListStyles = WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER |
                       LVS_SINGLESEL | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS;
DWORD dwListExStyles = WS_EX_CLIENTEDGE;
DWORD dwListExtendedStyles = LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP;

    // Set the list view's display style (large/small/list/report) based on
    // the FOLDERSETTINGS we were given in CreateViewWindow().

    switch ( m_FolderSettings.ViewMode )
	{
        case FVM_ICON:      dwListStyles |= LVS_ICON;      break;
        case FVM_SMALLICON: dwListStyles |= LVS_SMALLICON; break;
        case FVM_LIST:      dwListStyles |= LVS_LIST;      break;
        case FVM_DETAILS:   dwListStyles |= LVS_REPORT;    break;
		case FVM_THUMBNAIL: dwListStyles |= LVS_ICON;      break;
		case FVM_TILE: 	    dwListStyles |= LVS_ICON;      break;
		case FVM_THUMBSTRIP:dwListStyles |= LVS_ICON;      break;
		default: UNREACHABLE; break;
	}

    // Create the list control.  Note that m_hWnd (inherited from CWindowImpl)
    // has already been set to the container window's handle.

    hwndList = CreateWindowEx ( dwListExStyles, WC_LISTVIEW, NULL, dwListStyles,
                                0, 0, 0, 0, m_hWnd, (HMENU) sm_uListID, 
                                _Module.GetModuleInstance(), 0 );

    if ( NULL == hwndList )
        return -1;

    // Attach m_wndList to the list control and initialize styles, image
    // lists, etc.

    m_wndList.Attach ( hwndList );

    m_wndList.SetExtendedListViewStyle ( dwListExtendedStyles );

    m_wndList.SetImageList ( g_ImglistLarge, LVSIL_NORMAL );
    m_wndList.SetImageList ( g_ImglistSmall, LVSIL_SMALL );

    if ( dwListStyles & LVS_REPORT )
    {
        m_wndList.InsertColumn ( 0, _T("Drive"), LVCFMT_LEFT, 0, 0 );
        m_wndList.InsertColumn ( 1, _T("Volume Name"), LVCFMT_LEFT, 0, 1 );
        m_wndList.InsertColumn ( 2, _T("Free Space"), LVCFMT_LEFT, 0, 2 );
        m_wndList.InsertColumn ( 3, _T("Total Space"), LVCFMT_LEFT, 0, 3 );
    }

    //FillList();

    return 0;
}


// CSwishView

