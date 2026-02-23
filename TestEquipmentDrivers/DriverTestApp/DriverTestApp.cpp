#include "stdafx.h"
#include "DriverTestApp.h"
#include "DriverTestAppDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CDriverTestAppApp, CWinApp)
END_MESSAGE_MAP()

CDriverTestAppApp theApp;

CDriverTestAppApp::CDriverTestAppApp()
{
}

BOOL CDriverTestAppApp::InitInstance()
{
    INITCOMMONCONTROLSEX InitCtrls;
    InitCtrls.dwSize = sizeof(InitCtrls);
    InitCtrls.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&InitCtrls);

    CWinApp::InitInstance();
    AfxEnableControlContainer();

    // Initialize Winsock
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    CDriverTestAppDlg dlg;
    m_pMainWnd = &dlg;
    dlg.DoModal();

    WSACleanup();

    return FALSE;
}
