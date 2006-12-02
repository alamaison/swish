#include <atlbase.h>        // Base ATL classes
#include <atlwin.h>         // ATL windowing classes

class CSwishWindow : public CWindowImpl<CSwishWindow, CWindow, CFrameWinTraits>
{
public:
    DECLARE_WND_CLASS(_T("Swish Window Class"))
 
    BEGIN_MSG_MAP(CSwishWindow)
        MESSAGE_HANDLER(WM_CLOSE, OnClose)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        COMMAND_ID_HANDLER(IDC_ABOUT, OnAbout)
    END_MSG_MAP()
 
    LRESULT OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        DestroyWindow();
        return 0;
    }
 
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        PostQuitMessage(0);
        return 0;
    }
 
    LRESULT OnAbout(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
    {
        MessageBox ( _T("Sample ATL window"), _T("About MyWindow") );
        return 0;
    }
};