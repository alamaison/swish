// SwishView.h : Declaration of the CSwishView

#ifndef SWISHVIEW_H
#define SWISHVIEW_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "stdafx.h"
#include "resource.h"       // main symbols

#include "SwishFolder.h"

// ISwishView
[
	object,
	uuid("b816a83b-5022-11dc-9153-0090f5284f85"),
	helpstring("ISwishView Interface"),
	pointer_default(unique)
]
__interface ISwishView : IUnknown
{
};


// CSwishView
[
	coclass,
	default(ISwishView),
	threading(apartment),
	vi_progid("Swish.SwishView"),
	progid("Swish.SwishView.1"),
	version(1.0),
	uuid("b816a83c-5022-11dc-9153-0090f5284f85"),
	helpstring("SwishView Class")
]
class ATL_NO_VTABLE CSwishView :
	public CFrameWindowImpl<CSwishView>,
	public ISwishView,
	public IShellView
{
public:
	DECLARE_FRAME_WND_CLASS(_T("SwishView Window Class"), NULL);
	//DECLARE_WND_SUPERCLASS(NULL, CListViewCtrl::GetWndClassName())

	//DECLARE_WND_CLASS(_T("SwishView Window Class"))

	CSwishView()
	{
		ATLTRACE("CSwishView constructor called\n");
	}

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct()
	{
		return S_OK;
	}

	void FinalRelease()
	{
	}

BEGIN_MSG_MAP(CSwishView)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
/*
    MESSAGE_HANDLER(WM_SIZE, OnSize)
    MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
    MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)

    NOTIFY_CODE_HANDLER(LVN_DELETEITEM, OnListDeleteitem)
    NOTIFY_CODE_HANDLER(HDN_ITEMCLICK, OnHeaderItemclick)
    NOTIFY_CODE_HANDLER(NM_SETFOCUS, OnListSetfocus)

    COMMAND_ID_HANDLER(IDC_SYS_PROPERTIES, OnSystemProperties)
    COMMAND_ID_HANDLER(IDC_EXPLORE_DRIVE, OnExploreDrive)
    COMMAND_ID_HANDLER(IDC_ABOUT_SIMPLENS, OnAbout)
    MESSAGE_HANDLER(WM_INITMENUPOPUP, OnInitMenuPopup)
    MESSAGE_HANDLER(WM_MENUSELECT, OnMenuSelect) */
	CHAIN_MSG_MAP(CFrameWindowImpl<CSwishView>)
END_MSG_MAP()

    // IOleWindow
    STDMETHOD(GetWindow)( HWND* phwnd );
    STDMETHOD(ContextSensitiveHelp)( BOOL )
        { return E_NOTIMPL; }

	// IShellView
	STDMETHOD(CreateViewWindow)( IShellView *psvPrevious, LPCFOLDERSETTINGS pfs,
		IShellBrowser *psb, RECT *prcView, HWND *phWnd );
	STDMETHOD(DestroyViewWindow)();
	STDMETHOD(GetCurrentInfo)( LPFOLDERSETTINGS lpfs );
	STDMETHOD(Refresh)();
	STDMETHOD(SaveViewState)();
	STDMETHOD(TranslateAccelerator)( LPMSG lpmsg )
		{ ATLTRACE("CSwishView::TranslateAccelerator called\n"); return S_FALSE; }
	STDMETHOD(AddPropertySheetPages)(DWORD, LPFNADDPROPSHEETPAGE, LPARAM)
        { ATLTRACE("CSwishView::AddPropertySheetPages called\n"); return E_NOTIMPL; }
    STDMETHOD(EnableModeless)(BOOL)
        { ATLTRACE("CSwishView::EnableModeless called\n"); return E_NOTIMPL; }
    STDMETHOD(GetItemObject)(UINT, REFIID, void**)
        { ATLTRACE("CSwishView::GetItemObject called\n"); return E_NOTIMPL; }
    STDMETHOD(SelectItem)(LPCITEMIDLIST, UINT)
        { ATLTRACE("CSwishView::SelectItem called\n"); return E_NOTIMPL; }
	STDMETHOD(UIActivate)( UINT uState );

    // Message handlers
    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

	static UINT sm_uListID;

    HRESULT _init( CSwishFolder *pContainingFolder )
    {
		ATLASSERT(pContainingFolder);

        m_pSwishFolder = pContainingFolder;

        if (m_pSwishFolder != NULL)
            m_pSwishFolder->AddRef();

        return S_OK;
    }

private:
	// Saved information passed by Explorer
	FOLDERSETTINGS m_FolderSettings;
	CComPtr<IShellBrowser> m_spShellBrowser;
	CSwishFolder *m_pSwishFolder;

	// Windows
	HWND m_hWndParent;
	CContainedWindowT<WTL::CListViewCtrl> m_wndList;
};

#endif // SWISHVIEW_H
