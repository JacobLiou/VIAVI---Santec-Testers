#include "stdafx.h"
#include "GMSApp.h"
#include "GMSAppDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CGMSApp, CWinApp)
END_MESSAGE_MAP()

CGMSApp theApp;

CGMSApp::CGMSApp()
{
}

BOOL CGMSApp::InitInstance()
{
    INITCOMMONCONTROLSEX InitCtrls;
    InitCtrls.dwSize = sizeof(InitCtrls);
    InitCtrls.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&InitCtrls);

    CWinApp::InitInstance();
    AfxEnableControlContainer();

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    CGMSAppDlg dlg;
    m_pMainWnd = &dlg;
    dlg.DoModal();

    WSACleanup();
    return FALSE;
}
