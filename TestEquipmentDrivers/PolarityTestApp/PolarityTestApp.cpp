#include "stdafx.h"
#include "PolarityTestApp.h"
#include "PolarityTestAppDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CPolarityTestApp, CWinApp)
END_MESSAGE_MAP()

CPolarityTestApp theApp;

CPolarityTestApp::CPolarityTestApp()
{
}

BOOL CPolarityTestApp::InitInstance()
{
    INITCOMMONCONTROLSEX InitCtrls;
    InitCtrls.dwSize = sizeof(InitCtrls);
    InitCtrls.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&InitCtrls);

    CWinApp::InitInstance();
    AfxEnableControlContainer();

    CPolarityTestAppDlg dlg;
    m_pMainWnd = &dlg;
    dlg.DoModal();
    return FALSE;
}
