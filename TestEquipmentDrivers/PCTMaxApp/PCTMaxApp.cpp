#include "stdafx.h"
#include "PCTMaxApp.h"
#include "PCTMaxAppDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CPCTMaxApp, CWinApp)
END_MESSAGE_MAP()

CPCTMaxApp theApp;

CPCTMaxApp::CPCTMaxApp()
    : m_gdiplusToken(0)
{
}

BOOL CPCTMaxApp::InitInstance()
{
    INITCOMMONCONTROLSEX InitCtrls;
    InitCtrls.dwSize = sizeof(InitCtrls);
    InitCtrls.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&InitCtrls);

    CWinApp::InitInstance();
    AfxEnableControlContainer();

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);

    CPCTMaxAppDlg dlg;
    m_pMainWnd = &dlg;
    dlg.DoModal();

    return FALSE;
}

int CPCTMaxApp::ExitInstance()
{
    if (m_gdiplusToken)
        Gdiplus::GdiplusShutdown(m_gdiplusToken);

    WSACleanup();
    return CWinApp::ExitInstance();
}
