#include "stdafx.h"
#include "SantecApp.h"
#include "SantecAppDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CSantecApp, CWinApp)
END_MESSAGE_MAP()

CSantecApp theApp;

CSantecApp::CSantecApp()
{
}

BOOL CSantecApp::InitInstance()
{
    INITCOMMONCONTROLSEX InitCtrls;
    InitCtrls.dwSize = sizeof(InitCtrls);
    InitCtrls.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&InitCtrls);

    CWinApp::InitInstance();
    AfxEnableControlContainer();

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    CSantecAppDlg dlg;
    m_pMainWnd = &dlg;
    dlg.DoModal();

    WSACleanup();
    return FALSE;
}
