#include "stdafx.h"
#include "DriverTestApp3.h"
#include "DriverTestApp3Dlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CDriverTestApp3App, CWinApp)
END_MESSAGE_MAP()

CDriverTestApp3App theApp;

CDriverTestApp3App::CDriverTestApp3App()
{
}

BOOL CDriverTestApp3App::InitInstance()
{
    INITCOMMONCONTROLSEX InitCtrls;
    InitCtrls.dwSize = sizeof(InitCtrls);
    InitCtrls.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&InitCtrls);

    CWinApp::InitInstance();
    AfxEnableControlContainer();

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    CDriverTestApp3Dlg dlg;
    m_pMainWnd = &dlg;
    dlg.DoModal();

    WSACleanup();

    return FALSE;
}
