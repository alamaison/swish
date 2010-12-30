// TaskDialogTest.cpp : main source file for TaskDialogTest.exe
//

#include "stdafx.h"

#include <atlframe.h>
#include <atlctrls.h>
#include <atldlgs.h>
#include <atlctrlw.h>

#include "resource.h"

#include "maindlg.h"

CAppModule _Module;


int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR /*lpstrCmdLine*/, int /*nCmdShow*/)
{
   HRESULT hRes = ::CoInitialize(NULL);
   ATLASSERT(SUCCEEDED(hRes));

   ::DefWindowProc(NULL, 0, 0, 0L);

   AtlInitCommonControls(ICC_COOL_CLASSES | ICC_BAR_CLASSES);

   hRes = _Module.Init(NULL, hInstance);
   ATLASSERT(SUCCEEDED(hRes));

   int nRet = 0;
   {
      CMainDlg dlgMain;
      nRet = dlgMain.DoModal();
   }

   _Module.Term();
   ::CoUninitialize();

   return nRet;
}
