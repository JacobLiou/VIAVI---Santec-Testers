#include "stdafx.h"
#include "PTMTestApp.h"
#include "PTMTestAppDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CPTMTestApp, CWinApp)
END_MESSAGE_MAP()

CPTMTestApp theApp;

CPTMTestApp::CPTMTestApp()
{
}

BOOL CPTMTestApp::InitInstance()
{
    INITCOMMONCONTROLSEX InitCtrls;
    InitCtrls.dwSize = sizeof(InitCtrls);
    InitCtrls.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&InitCtrls);

    CWinApp::InitInstance();
    AfxEnableControlContainer();

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    CPTMTestAppDlg dlg;
    m_pMainWnd = &dlg;
    dlg.DoModal();

    WSACleanup();
    return FALSE;
}
