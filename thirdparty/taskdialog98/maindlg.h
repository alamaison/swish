// maindlg.h : interface of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINDLG_H__DA2FA2FB_17F8_4507_9DAE_D279641E8337__INCLUDED_)
#define AFX_MAINDLG_H__DA2FA2FB_17F8_4507_9DAE_D279641E8337__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


#if _WIN32_WINNT < 0x0600

inline int AtlTaskDialog(HWND hWndParent, 
                         ATL::_U_STRINGorID WindowTitle, ATL::_U_STRINGorID MainInstructionText, ATL::_U_STRINGorID ContentText, 
                         TASKDIALOG_COMMON_BUTTON_FLAGS dwCommonButtons = 0U, ATL::_U_STRINGorID Icon = (LPCTSTR)NULL)
{
   int nRet = -1;
   typedef HRESULT (STDAPICALLTYPE *PFN_TaskDialog)(HWND hwndParent, HINSTANCE hInstance, PCWSTR pszWindowTitle, PCWSTR pszMainInstruction, PCWSTR pszContent, TASKDIALOG_COMMON_BUTTON_FLAGS dwCommonButtons, PCWSTR pszIcon, int* pnButton);
   HMODULE m_hCommCtrlDLL = ::LoadLibrary(_T("comctl32.dll"));
   if(m_hCommCtrlDLL != NULL)
   {
      PFN_TaskDialog pfnTaskDialog = (PFN_TaskDialog)::GetProcAddress(m_hCommCtrlDLL, "TaskDialog");
      if(pfnTaskDialog != NULL)
      {
         USES_CONVERSION;
         HRESULT hRet = pfnTaskDialog(hWndParent, ModuleHelper::GetResourceInstance(), 
            IS_INTRESOURCE(WindowTitle.m_lpstr) ? (LPCWSTR) WindowTitle.m_lpstr : T2CW(WindowTitle.m_lpstr), 
            IS_INTRESOURCE(MainInstructionText.m_lpstr) ? (LPCWSTR) MainInstructionText.m_lpstr : T2CW(MainInstructionText.m_lpstr), 
            IS_INTRESOURCE(ContentText.m_lpstr) ?  (LPCWSTR) ContentText.m_lpstr : T2CW(ContentText.m_lpstr), 
            dwCommonButtons, 
            IS_INTRESOURCE(Icon.m_lpstr) ? (LPCWSTR) Icon.m_lpstr : T2CW(Icon.m_lpstr),
            &nRet);
         ATLVERIFY(SUCCEEDED(hRet));
      }

      ::FreeLibrary(m_hCommCtrlDLL);
   }
   return nRet;
}

#endif // _WIN32_WINNT < 0x0600

inline int AtlTaskDialogIndirect(TASKDIALOGCONFIG* pTask, int* pnButton = NULL, int* pnRadioButton = NULL, BOOL* pfVerificationFlagChecked = NULL)
{
   // This allows apps to run on older versions of Windows
   typedef HRESULT (STDAPICALLTYPE *PFN_TaskDialogIndirect)(const TASKDIALOGCONFIG* pTaskConfig, int* pnButton, int* pnRadioButton, BOOL* pfVerificationFlagChecked);

   HRESULT hRet = E_UNEXPECTED;
   HMODULE m_hCommCtrlDLL = ::LoadLibrary(_T("comctl32.dll"));
   if(m_hCommCtrlDLL != NULL)
   {
      PFN_TaskDialogIndirect pfnTaskDialogIndirect = (PFN_TaskDialogIndirect)::GetProcAddress(m_hCommCtrlDLL, "TaskDialogIndirect");
      if(pfnTaskDialogIndirect != NULL)
         hRet = pfnTaskDialogIndirect(pTask, pnButton, pnRadioButton, pfVerificationFlagChecked);

      ::FreeLibrary(m_hCommCtrlDLL);
   }
   return hRet;
}



class CMainDlg : public CDialogImpl<CMainDlg>
{
public:
   enum { IDD = IDD_MAINDLG };

   BEGIN_MSG_MAP(CMainDlg)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
      COMMAND_ID_HANDLER(IDOK, OnClose)
      COMMAND_ID_HANDLER(IDCANCEL, OnClose)
      COMMAND_ID_HANDLER(IDC_BUTTON1, OnTest1)
      COMMAND_ID_HANDLER(IDC_BUTTON2, OnTest2)
      COMMAND_ID_HANDLER(IDC_BUTTON3, OnTest3)
      COMMAND_ID_HANDLER(IDC_BUTTON4, OnTest4)
      COMMAND_ID_HANDLER(IDC_BUTTON5, OnTest5)
      COMMAND_ID_HANDLER(IDC_BUTTON6, OnTest6)
      COMMAND_ID_HANDLER(IDC_BUTTON7, OnTest7)
      COMMAND_ID_HANDLER(IDC_BUTTON8, OnTest8)
      COMMAND_ID_HANDLER(IDC_BUTTON9, OnTest9)
      COMMAND_ID_HANDLER(IDC_BUTTON10, OnTest10)
      COMMAND_ID_HANDLER(IDC_BUTTON11, OnTest11)
      COMMAND_ID_HANDLER(IDC_BUTTON12, OnTest12)
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      CenterWindow();

      HICON hIcon = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), 
         IMAGE_ICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
      SetIcon(hIcon, TRUE);
      HICON hIconSmall = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), 
         IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
      SetIcon(hIconSmall, FALSE);

      return TRUE;
   }

   LRESULT OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      CSimpleDialog<IDD_ABOUTBOX, FALSE> dlg;
      dlg.DoModal();
      return 0;
   }

   LRESULT OnClose(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      EndDialog(wID);
      return 0;
   }

   LRESULT OnTest1(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      LPCTSTR pstrWindowTitle = _T("Window Title");
      LPCTSTR pstrInstructionsText = _T("Test Case 1");
      LPCTSTR pstrContentText1 = _T("This is Bjarke's Task Dialog");
      LPCTSTR pstrContentText2 = _T("This is the Windows Vista Task Dialog");
      int iRes = 0;
      Task98Dialog(m_hWnd, _Module.GetResourceInstance(), pstrWindowTitle,pstrInstructionsText, pstrContentText1, TDCBF_YES_BUTTON | TDCBF_OK_BUTTON | TDCBF_NO_BUTTON | TDCBF_CANCEL_BUTTON | TDCBF_CLOSE_BUTTON | TDCBF_RETRY_BUTTON, MAKEINTRESOURCE(IDR_MAINFRAME), &iRes);
      AtlTaskDialog(m_hWnd, pstrWindowTitle, pstrInstructionsText, pstrContentText2, TDCBF_YES_BUTTON | TDCBF_OK_BUTTON | TDCBF_NO_BUTTON | TDCBF_CANCEL_BUTTON | TDCBF_CLOSE_BUTTON | TDCBF_RETRY_BUTTON, MAKEINTRESOURCE(IDR_MAINFRAME));
      return 0;
   }

   LRESULT OnTest2(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      LPCTSTR pstrWindowTitle = _T("Window Title");
      LPCTSTR pstrInstructionsText = _T("Click on the button below");
      LPCTSTR pstrContentText = _T("Choose a button. Do the right thing and read this multi-line entry. Can there be any more?? I don't know. Maybe there is.");
      int iRes = 0;
      Task98Dialog(m_hWnd, _Module.GetResourceInstance(), pstrWindowTitle,pstrInstructionsText, pstrContentText, TDCBF_OK_BUTTON, MAKEINTRESOURCE(IDR_MAINFRAME), &iRes);
      AtlTaskDialog(m_hWnd, pstrWindowTitle, pstrInstructionsText, pstrContentText, TDCBF_OK_BUTTON, MAKEINTRESOURCE(IDR_MAINFRAME));
      return 0;
   }

   LRESULT OnTest3(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      TASKDIALOGCONFIG cfg = { 0 };
      cfg.cbSize = sizeof(cfg);
      cfg.hInstance = _Module.GetResourceInstance();
      cfg.pszWindowTitle = L"Window Title";
      cfg.pszMainIcon = MAKEINTRESOURCEW(IDR_MAINFRAME);
      cfg.pszContent = L"This is the contents";
      cfg.dwCommonButtons = TDCBF_YES_BUTTON | TDCBF_OK_BUTTON | TDCBF_NO_BUTTON | TDCBF_CANCEL_BUTTON | TDCBF_CLOSE_BUTTON | TDCBF_RETRY_BUTTON;
      TASKDIALOG_BUTTON buttons[] = {
         { 100, L"Button #1" },
         { 101, L"Button #2\nText Below" },
      };
      cfg.pButtons = buttons;
      cfg.cButtons = 2;
      cfg.nDefaultButton = 101;
      int iRes, iRadio, iVerify;
      Task98DialogIndirect(&cfg, &iRes, &iRadio, &iVerify);
      AtlTaskDialogIndirect(&cfg, &iRes, &iRadio, &iVerify);
      return 0;
   }   

   LRESULT OnTest4(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      TASKDIALOGCONFIG cfg = { 0 };
      cfg.cbSize = sizeof(cfg);
      cfg.hInstance = _Module.GetResourceInstance();
      cfg.pszWindowTitle = L"Window Title";
      cfg.pszMainIcon = TD_WARNING_ICON;
      cfg.pszMainInstruction = L"This is a test";
      cfg.pszContent = MAKEINTRESOURCEW(IDS_TASKDLG_CANCEL);
      cfg.dwCommonButtons = TDCBF_YES_BUTTON | TDCBF_OK_BUTTON | TDCBF_NO_BUTTON | TDCBF_CANCEL_BUTTON | TDCBF_CLOSE_BUTTON | TDCBF_RETRY_BUTTON;
      TASKDIALOG_BUTTON buttons[] = {
         { 100, L"Button #1" },
         { 101, L"Button #2\nText Below" },
      };
      cfg.pButtons = buttons;
      cfg.cButtons = 2;
      cfg.nDefaultButton = 101;
      TASKDIALOG_BUTTON radios[] = {
         { 200, L"Radio #1" },
         { 201, L"Radio #2\nText Below" },
         { 202, L"Radio #3" },
      };
      cfg.pRadioButtons = radios;
      cfg.nDefaultRadioButton = 202;
      cfg.cRadioButtons = 3;
      int iRes, iRadio, iVerify;
      Task98DialogIndirect(&cfg, &iRes, &iRadio, &iVerify);
      AtlTaskDialogIndirect(&cfg, &iRes, &iRadio, &iVerify);
      return 0;
   }   

   LRESULT OnTest5(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      TASKDIALOGCONFIG cfg = { 0 };
      cfg.cbSize = sizeof(cfg);
      cfg.hwndParent = m_hWnd;
      cfg.hInstance = _Module.GetResourceInstance();
      cfg.pszWindowTitle = L"Window Title";
      cfg.pszMainIcon = TD_ERROR_ICON;
      cfg.pszMainInstruction = L"This is another test\nThere are 3 lines\nof instruction text here.";
      cfg.pszContent = L"This is the contents of yet another test. Testing the verifaction checkbox below.";
      cfg.dwCommonButtons = TDCBF_YES_BUTTON | TDCBF_OK_BUTTON | TDCBF_NO_BUTTON | TDCBF_CANCEL_BUTTON | TDCBF_CLOSE_BUTTON | TDCBF_RETRY_BUTTON;
      TASKDIALOG_BUTTON buttons[] = {
         { 100, L"Button #1" },
         { 101, L"Button #2\nText Below" },
      };
      cfg.pButtons = buttons;
      cfg.cButtons = 2;
      cfg.nDefaultButton = IDNO;
      cfg.pszVerificationText = L"Verifcation text. This is a very long text, so maybe it will wrap.";
      cfg.dwFlags = TDF_POSITION_RELATIVE_TO_WINDOW;
      int iRes, iRadio, iVerify;
      Task98DialogIndirect(&cfg, &iRes, &iRadio, &iVerify);
      AtlTaskDialogIndirect(&cfg, &iRes, &iRadio, &iVerify);
      return 0;
   }   

   LRESULT OnTest6(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      TASKDIALOGCONFIG cfg = { 0 };
      cfg.cbSize = sizeof(cfg);
      cfg.hwndParent = m_hWnd;
      cfg.hInstance = _Module.GetResourceInstance();
      cfg.pszWindowTitle = L"Window Title";
      cfg.pszMainIcon = TD_ERROR_ICON;
      cfg.pszMainInstruction = L"This is another test";
      cfg.pszContent = L"This is the contents of yet another test. Testing the verifaction checkbox below.";
      cfg.dwCommonButtons = TDCBF_OK_BUTTON | TDCBF_CANCEL_BUTTON;
      TASKDIALOG_BUTTON buttons[] = {
         { 100, L"Button #1" },
         { 101, L"Button #2\nText Below" },
         { 102, L"Button #3\nThis is a longer line of text which nothing really interesting in it. Lets see how long it can be.\nLine 2" },
         { 103, L"Button #4" },
      };
      cfg.pButtons = buttons;
      cfg.cButtons = 4;
      cfg.nDefaultButton = 101;
      cfg.pszVerificationText = L"Verifcation text.";
      cfg.dwFlags = TDF_POSITION_RELATIVE_TO_WINDOW | TDF_USE_COMMAND_LINKS;
      int iRes, iRadio, iVerify;
      Task98DialogIndirect(&cfg, &iRes, &iRadio, &iVerify);
      AtlTaskDialogIndirect(&cfg, &iRes, &iRadio, &iVerify);
      return 0;
   }

   LRESULT OnTest7(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      TASKDIALOGCONFIG cfg = { 0 };
      cfg.cbSize = sizeof(cfg);
      cfg.hwndParent = m_hWnd;
      cfg.hInstance = _Module.GetResourceInstance();
      cfg.pszWindowTitle = L"Window Title";
      cfg.pszMainIcon = TD_ERROR_ICON;
      cfg.pszMainInstruction = L"This is another test. The Main Instruction label can also be rather long and span multiple lines.";
      cfg.pszContent = L"This is the contents of yet another long label. Testing the verifaction checkbox below. This line is longer than the others.";
      cfg.dwCommonButtons = TDCBF_OK_BUTTON | TDCBF_CANCEL_BUTTON;
      TASKDIALOG_BUTTON buttons[] = {
         { 100, L"Button #1" },
         { 101, L"Button #2" },
      };
      cfg.pButtons = buttons;
      cfg.cButtons = 2;
      cfg.nDefaultButton = 101;
      TASKDIALOG_BUTTON radios[] = {
         { 200, L"Radio #1" },
         { 201, L"Radio #2\nText Below" },
         { 202, L"Radio #3. This is a rather long radio button text label which will span multiple lines." },
      };
      cfg.pRadioButtons = radios;
      cfg.nDefaultRadioButton = 202;
      cfg.cRadioButtons = 3;
      cfg.pszVerificationText = L"Verifcation text.";
      cfg.pszFooter = L"Footer Text";
      cfg.pszFooterIcon = MAKEINTRESOURCEW(IDR_MAINFRAME);
      cfg.pszExpandedControlText = NULL;
      cfg.pszCollapsedControlText = NULL;
      cfg.pszExpandedInformation = L"Expanded information text here...";
      cfg.dwFlags = TDF_POSITION_RELATIVE_TO_WINDOW | TDF_USE_COMMAND_LINKS;
      int iRes, iRadio, iVerify;
      Task98DialogIndirect(&cfg, &iRes, &iRadio, &iVerify);
      AtlTaskDialogIndirect(&cfg, &iRes, &iRadio, &iVerify);
      return 0;
   }   

   LRESULT OnTest8(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      TASKDIALOGCONFIG cfg = { 0 };
      cfg.cbSize = sizeof(cfg);
      cfg.hwndParent = m_hWnd;
      cfg.hInstance = _Module.GetResourceInstance();
      cfg.pszWindowTitle = L"Window Title";
      cfg.pszMainIcon = TD_ERROR_ICON;
      cfg.pszMainInstruction = L"This is another test";
      cfg.pszContent = L"This is the contents of yet another test with a <a href=\"link1\">link</a>.";
      cfg.dwCommonButtons = TDCBF_OK_BUTTON | TDCBF_CANCEL_BUTTON;
      TASKDIALOG_BUTTON buttons[] = {
         { 100, L"Button Label for Control #1" },
         { 101, L"Button Label for Control #2" },
      };
      cfg.pButtons = buttons;
      cfg.cButtons = 2;
      cfg.nDefaultButton = 101;
      TASKDIALOG_BUTTON radios[] = {
         { 200, L"Radio #1\nText Below Radio button #1" },
         { 201, L"Radio #2\nText Below Radio button #2" },
         { 202, L"Radio #3\nText Below Radio button #3" },
      };
      cfg.pRadioButtons = radios;
      cfg.cRadioButtons = 3;
      cfg.pszVerificationText = L"Verifcation text. This is a long text with\ntwo lines.";
      cfg.pszFooter = L"Footer Text with a <a href=\"link1\">link</a>.";
      //cfg.hFooterIcon = ::LoadIcon(NULL, MAKEINTRESOURCE(IDI_WINLOGO));
      cfg.hFooterIcon = (HICON) ::LoadImage(NULL, MAKEINTRESOURCE(IDI_ASTERISK), IMAGE_ICON, 16, 16, LR_LOADTRANSPARENT | LR_SHARED);
      cfg.pszExpandedControlText = L"Collapse Control Text\nWith an extra line. Wohoo.";
      cfg.pszCollapsedControlText = L"Expand Control Text";
      cfg.pszExpandedInformation = L"Expanded information text here with a <a id=\"link1\">link</a>.";
      cfg.dwFlags = TDF_POSITION_RELATIVE_TO_WINDOW | TDF_USE_COMMAND_LINKS_NO_ICON | TDF_EXPAND_FOOTER_AREA | TDF_USE_HICON_FOOTER | TDF_ENABLE_HYPERLINKS;
      int iRes, iRadio, iVerify;
      Task98DialogIndirect(&cfg, &iRes, &iRadio, &iVerify);
      AtlTaskDialogIndirect(&cfg, &iRes, &iRadio, &iVerify);
      return 0;
   }

   static HRESULT CALLBACK TaskDialogCallback9(HWND hWnd, UINT msg, WPARAM wParam, LPARAM /*lParam*/, LONG_PTR /*lpRefData*/)
   {
      switch( msg ) {
      case TDN_TIMER:
         ::SendMessage(hWnd, TDM_SET_PROGRESS_BAR_POS, wParam / 30, 0L);
         if( wParam / 30 >= 100 ) ::SendMessage(hWnd, TDM_CLICK_BUTTON, IDOK, 0L);
         break;
      }
      return 0;
   }

   LRESULT OnTest9(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      TASKDIALOGCONFIG cfg = { 0 };
      cfg.cbSize = sizeof(cfg);
      cfg.hwndParent = m_hWnd;
      cfg.hInstance = _Module.GetResourceInstance();
      cfg.pszWindowTitle = L"Window Title";
      cfg.pszMainIcon = TD_ERROR_ICON;
      cfg.pszMainInstruction = L"This is Progress Bar test";
      cfg.pszContent = L"This is the content text above the Progress Bar.";
      cfg.dwCommonButtons = TDCBF_OK_BUTTON | TDCBF_CANCEL_BUTTON;
      cfg.nDefaultButton = IDOK;
      cfg.pfCallback = TaskDialogCallback9;
      cfg.dwFlags = TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SHOW_PROGRESS_BAR | TDF_CALLBACK_TIMER;
      int iRes, iRadio, iVerify;
      Task98DialogIndirect(&cfg, &iRes, &iRadio, &iVerify);
      AtlTaskDialogIndirect(&cfg, &iRes, &iRadio, &iVerify);
      return 0;
   }

   static HRESULT CALLBACK TaskDialogCallback10(HWND hWnd, UINT msg, WPARAM wParam, LPARAM /*lParam*/, LONG_PTR /*lpRefData*/)
   {
      switch( msg ) {
      case TDN_DIALOG_CONSTRUCTED:
         ::SendMessage(hWnd, TDM_ENABLE_BUTTON, 101, 0L);
         ::SendMessage(hWnd, TDM_ENABLE_BUTTON, IDCANCEL, 0L);
         ::SendMessage(hWnd, TDM_ENABLE_RADIO_BUTTON, 201, 0L);
         ::SendMessage(hWnd, TDM_SET_MARQUEE_PROGRESS_BAR, 1, 0L);
         ::SendMessage(hWnd, TDM_SET_PROGRESS_BAR_MARQUEE, 1, 30L);
         break;
      case TDN_BUTTON_CLICKED:
         if( wParam == 100 ) {
            ::SendMessage(hWnd, TDM_CLICK_RADIO_BUTTON, 202, 0L);
            return S_FALSE;
         }
         return S_OK;
      }
      return 0;
   }

   LRESULT OnTest10(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      TASKDIALOGCONFIG cfg = { 0 };
      cfg.cbSize = sizeof(cfg);
      cfg.hwndParent = m_hWnd;
      cfg.hInstance = _Module.GetResourceInstance();
      cfg.pszWindowTitle = L"Window Title";
      cfg.hMainIcon = ::LoadIcon(NULL, MAKEINTRESOURCE(IDI_WINLOGO));
      cfg.pszMainInstruction = L"This is Progress Bar test";
      cfg.pszContent = L"This is the content text above the Progress Bar.";
      cfg.dwCommonButtons = TDCBF_OK_BUTTON | TDCBF_CANCEL_BUTTON;
      cfg.pfCallback = TaskDialogCallback10;
      TASKDIALOG_BUTTON buttons[] = {
         { 100, L"Not clickable" },
         { 101, L"Disabled" },
      };
      cfg.pButtons = buttons;
      cfg.cButtons = 2;
      cfg.nDefaultButton = 101;
      TASKDIALOG_BUTTON radios[] = {
         { 200, L"Radio #1\nText Below Radio button #1. This button as a very very long text line which should wrap the text to several lines I hope. It will test the sizing of the dialog." },
         { 201, L"Radio #2\nText Below Radio button #2" },
         { 202, L"Radio #3\nText Below Radio button #3. This is another long line which will wrap to the second line only." },
      };
      cfg.pRadioButtons = radios;
      cfg.cRadioButtons = 3;
      cfg.dwFlags = TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SHOW_PROGRESS_BAR | TDF_USE_HICON_MAIN;
      int iRes, iRadio, iVerify;
      Task98DialogIndirect(&cfg, &iRes, &iRadio, &iVerify);
      AtlTaskDialogIndirect(&cfg, &iRes, &iRadio, &iVerify);
      return 0;
   }  

   class CTask98Dialog11 : public CTask98DialogImpl<CTask98Dialog11>
   {
   public:
      int DoModal(HWND hWnd = NULL)
      {
         m_cfg.hwndParent = hWnd;
         m_cfg.pszWindowTitle = L"Window Title";
         m_cfg.hMainIcon = ::LoadIcon(NULL, MAKEINTRESOURCE(IDI_WINLOGO));
         m_cfg.pszMainInstruction = L"This is Progress Bar test";
         m_cfg.pszContent = L"This is the content text above the Progress Bar.";
         m_cfg.dwCommonButtons = TDCBF_OK_BUTTON | TDCBF_CANCEL_BUTTON;
         m_cfg.nDefaultButton = IDOK;
         TASKDIALOG_BUTTON buttons[] = {
            { 100, L"Not clickable" },
            { 101, L"Disabled" },
         };
         m_cfg.pButtons = buttons;
         m_cfg.cButtons = 2;
         m_cfg.nDefaultButton = 101;
         TASKDIALOG_BUTTON radios[] = {
            { 200, L"Radio #1\nText Below Radio button #1." },
            { 201, L"Radio #2\nText Below Radio button #2." },
            { 202, L"Radio #3\nText Below Radio button #3." },
         };
         m_cfg.pRadioButtons = radios;
         m_cfg.cRadioButtons = 3;
         m_cfg.dwFlags = TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SHOW_PROGRESS_BAR | TDF_USE_HICON_MAIN | TDF_CALLBACK_TIMER;
         return CTask98DialogImpl<CTask98Dialog11>::DoModal(hWnd);
      }

      void OnCreated()
      {
         EnableButton(IDCANCEL, FALSE);
         EnableButton(101, FALSE);
         EnableRadioButton(201, FALSE);
         ClickRadioButton(202);
      }
      BOOL OnTimer(UINT wTime)
      {
         SetProgressBarPos(wTime / 30);
         return FALSE;
      }
      BOOL OnButtonClicked(int nID)
      {
         if( nID == 100 ) return TRUE;
         return FALSE;
      }
   };

   LRESULT OnTest11(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      CTask98Dialog11 dlg;
      dlg.DoModal();
      return 0;
   }  

   LRESULT OnTest12(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      TASKDIALOGCONFIG cfg = { 0 };
      cfg.cbSize = sizeof(cfg);
      cfg.hInstance = _Module.GetResourceInstance();
      cfg.pszWindowTitle = L"Window Title";
      cfg.pszMainIcon = MAKEINTRESOURCEW(IDR_MAINFRAME);
      cfg.pszContent = L"This is the contents.\nhttp://www.viksoe.dk/code/testing_a_really.long.url.html?with=argument&that=makes&it&even=1&longer_than_the&screen=no&anditjustkeepgoingandgoing\nLine3\nLine4\nLine5";
      cfg.dwCommonButtons = TDCBF_OK_BUTTON | TDCBF_CANCEL_BUTTON;
      int iRes, iRadio, iVerify;
      Task98DialogIndirect(&cfg, &iRes, &iRadio, &iVerify);
      AtlTaskDialogIndirect(&cfg, &iRes, &iRadio, &iVerify);
      return 0;
   }   
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINDLG_H__DA2FA2FB_17F8_4507_9DAE_D279641E8337__INCLUDED_)
