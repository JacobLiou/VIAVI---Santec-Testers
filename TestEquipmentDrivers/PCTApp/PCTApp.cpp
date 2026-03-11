#include "stdafx.h"
#include "PCTApp.h"
#include "PCTAppDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CPCTApp, CWinApp)
END_MESSAGE_MAP()

CPCTApp theApp;

CPCTApp::CPCTApp()
{
}

BOOL CPCTApp::InitInstance()
{
    INITCOMMONCONTROLSEX InitCtrls;
    InitCtrls.dwSize = sizeof(InitCtrls);
    InitCtrls.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&InitCtrls);

    CWinApp::InitInstance();
    AfxEnableControlContainer();

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    CPCTAppDlg dlg;
    m_pMainWnd = &dlg;
    dlg.DoModal();

    WSACleanup();
    return FALSE;
}
