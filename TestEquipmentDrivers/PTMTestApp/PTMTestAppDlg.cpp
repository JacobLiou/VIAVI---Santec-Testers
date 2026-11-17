#include "stdafx.h"
#include "PTMTestApp.h"
#include "PTMTestAppDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CPTMTestAppDlg* CPTMTestAppDlg::s_instance = nullptr;

CPTMTestAppDlg::CPTMTestAppDlg(CWnd* pParent)
    : CDialogEx(IDD_PTMTESTAPP_DIALOG, pParent)
{
    s_instance = this;
}

void CPTMTestAppDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT_DLL_PATH, m_editDllPath);
    DDX_Control(pDX, IDC_COMBO_CONN_MODE, m_comboConnMode);
    DDX_Control(pDX, IDC_COMBO_ADDRESS, m_comboAddress);
    DDX_Control(pDX, IDC_EDIT_PORT, m_editPort);
    DDX_Control(pDX, IDC_STATIC_STATUS, m_staticStatus);
    DDX_Control(pDX, IDC_EDIT_CHANNELS, m_editChannels);
    DDX_Control(pDX, IDC_EDIT_DETECT_CH, m_editDetectCh);
    DDX_Control(pDX, IDC_CHECK_LOCAL, m_checkLocal);
    DDX_Control(pDX, IDC_EDIT_SCPI, m_editScpi);
    DDX_Control(pDX, IDC_LIST_RESULTS, m_listResults);
    DDX_Control(pDX, IDC_EDIT_RAW, m_editRaw);
    DDX_Control(pDX, IDC_EDIT_LOG, m_editLog);
}

BEGIN_MESSAGE_MAP(CPTMTestAppDlg, CDialogEx)
    ON_WM_CLOSE()
    ON_BN_CLICKED(IDC_BTN_LOAD_DLL, &CPTMTestAppDlg::OnBnClickedLoadDll)
    ON_BN_CLICKED(IDC_BTN_UNLOAD_DLL, &CPTMTestAppDlg::OnBnClickedUnloadDll)
    ON_BN_CLICKED(IDC_BTN_CONNECT, &CPTMTestAppDlg::OnBnClickedConnect)
    ON_BN_CLICKED(IDC_BTN_DISCONNECT, &CPTMTestAppDlg::OnBnClickedDisconnect)
    ON_BN_CLICKED(IDC_BTN_INITIALIZE, &CPTMTestAppDlg::OnBnClickedInitialize)
    ON_BN_CLICKED(IDC_BTN_ENUMERATE, &CPTMTestAppDlg::OnBnClickedEnumerate)
    ON_BN_CLICKED(IDC_BTN_MEASURE, &CPTMTestAppDlg::OnBnClickedMeasure)
    ON_BN_CLICKED(IDC_BTN_GET_RESULTS, &CPTMTestAppDlg::OnBnClickedGetResults)
    ON_BN_CLICKED(IDC_BTN_DETECT, &CPTMTestAppDlg::OnBnClickedDetect)
    ON_BN_CLICKED(IDC_BTN_VFL, &CPTMTestAppDlg::OnBnClickedVfl)
    ON_BN_CLICKED(IDC_BTN_LASER_OFF, &CPTMTestAppDlg::OnBnClickedLaserOff)
    ON_BN_CLICKED(IDC_CHECK_LOCAL, &CPTMTestAppDlg::OnBnClickedLocal)
    ON_BN_CLICKED(IDC_BTN_SEND_SCPI, &CPTMTestAppDlg::OnBnClickedSendScpi)
    ON_BN_CLICKED(IDC_BTN_CLEAR_LOG, &CPTMTestAppDlg::OnBnClickedClearLog)
    ON_CBN_SELCHANGE(IDC_COMBO_CONN_MODE, &CPTMTestAppDlg::OnCbnSelchangeConnMode)
    ON_MESSAGE(WM_LOG_MESSAGE, &CPTMTestAppDlg::OnLogMessage)
END_MESSAGE_MAP()

BOOL CPTMTestAppDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    SetIcon(AfxGetApp()->LoadStandardIcon(IDI_APPLICATION), TRUE);

    m_editDllPath.SetWindowText(_T("UDL.SantecPTM.dll"));
    m_comboConnMode.AddString(_T("TCP"));
    m_comboConnMode.AddString(_T("USB/VISA"));
    m_comboConnMode.SetCurSel(0);
    m_comboAddress.SetWindowText(_T("127.0.0.1"));
    m_editPort.SetWindowText(_T("5025"));
    m_editChannels.SetWindowText(_T("0"));
    m_editDetectCh.SetWindowText(_T("1"));
    m_editScpi.SetWindowText(_T("*IDN?"));

    m_listResults.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_listResults.InsertColumn(0, _T("Output"), LVCFMT_LEFT, 80);
    m_listResults.InsertColumn(1, _T("Input"), LVCFMT_LEFT, 80);
    m_listResults.InsertColumn(2, _T("Map"), LVCFMT_LEFT, 100);

    EnableConnectionControls(false);
    EnableDeviceControls(false);
    AppendLog(_T("Ready. Load UDL.SantecPTM.dll then connect to PTMSimulator (127.0.0.1:5025)."));
    return TRUE;
}

void CPTMTestAppDlg::OnClose()
{
    m_loader.SetLogCallback(nullptr);
    m_loader.DestroyDriver();
    m_loader.UnloadDll();
    s_instance = nullptr;
    CDialogEx::OnClose();
}

void CPTMTestAppDlg::AppendLog(const CString& text)
{
    CString existing;
    m_editLog.GetWindowText(existing);
    if (!existing.IsEmpty()) existing += _T("\r\n");
    existing += text;
    m_editLog.SetWindowText(existing);
    m_editLog.LineScroll(m_editLog.GetLineCount());
}

void CPTMTestAppDlg::UpdateStatus(const CString& status)
{
    m_staticStatus.SetWindowText(status);
}

void CPTMTestAppDlg::EnableConnectionControls(bool dllLoaded)
{
    GetDlgItem(IDC_BTN_CONNECT)->EnableWindow(dllLoaded);
    GetDlgItem(IDC_BTN_ENUMERATE)->EnableWindow(dllLoaded);
    GetDlgItem(IDC_BTN_UNLOAD_DLL)->EnableWindow(dllLoaded);
}

void CPTMTestAppDlg::EnableDeviceControls(bool connected)
{
    GetDlgItem(IDC_BTN_DISCONNECT)->EnableWindow(connected);
    GetDlgItem(IDC_BTN_INITIALIZE)->EnableWindow(connected);
    GetDlgItem(IDC_BTN_MEASURE)->EnableWindow(connected);
    GetDlgItem(IDC_BTN_GET_RESULTS)->EnableWindow(connected);
    GetDlgItem(IDC_BTN_DETECT)->EnableWindow(connected);
    GetDlgItem(IDC_BTN_VFL)->EnableWindow(connected);
    GetDlgItem(IDC_BTN_LASER_OFF)->EnableWindow(connected);
    GetDlgItem(IDC_CHECK_LOCAL)->EnableWindow(connected);
    GetDlgItem(IDC_BTN_SEND_SCPI)->EnableWindow(connected);
}

bool CPTMTestAppDlg::IsVisaMode()
{
    return m_comboConnMode.GetCurSel() == 1;
}

void CPTMTestAppDlg::PopulateResults(const PTMPolarityResult* results, int count)
{
    m_listResults.DeleteAllItems();
    for (int i = 0; i < count; ++i)
    {
        CString out, in, map;
        out.Format(_T("%d"), results[i].outputChannel);
        in.Format(_T("%d"), results[i].inputChannel);
        map.Format(_T("%d -> %d"), results[i].outputChannel, results[i].inputChannel);
        int idx = m_listResults.InsertItem(i, out);
        m_listResults.SetItemText(idx, 1, in);
        m_listResults.SetItemText(idx, 2, map);
    }
}

void WINAPI CPTMTestAppDlg::LogThunk(int level, const char* source, const char* message)
{
    if (!s_instance || !::IsWindow(s_instance->GetSafeHwnd())) return;
    CString* p = new CString;
    p->Format(_T("[%d] %S: %S"), level, source ? source : "", message ? message : "");
    s_instance->PostMessage(WM_LOG_MESSAGE, 0, reinterpret_cast<LPARAM>(p));
}

LRESULT CPTMTestAppDlg::OnLogMessage(WPARAM, LPARAM lParam)
{
    CString* p = reinterpret_cast<CString*>(lParam);
    if (p)
    {
        AppendLog(*p);
        delete p;
    }
    return 0;
}

void CPTMTestAppDlg::OnBnClickedLoadDll()
{
    CString path;
    m_editDllPath.GetWindowText(path);
    CT2A pathA(path);
    if (!m_loader.LoadDll(pathA))
    {
        AppendLog(_T("Failed to load DLL."));
        UpdateStatus(_T("Load failed"));
        return;
    }
    m_loader.SetLogCallback(&CPTMTestAppDlg::LogThunk);
    EnableConnectionControls(true);
    UpdateStatus(_T("DLL loaded"));
    AppendLog(_T("DLL loaded."));
}

void CPTMTestAppDlg::OnBnClickedUnloadDll()
{
    m_loader.SetLogCallback(nullptr);
    m_loader.UnloadDll();
    EnableConnectionControls(false);
    EnableDeviceControls(false);
    UpdateStatus(_T("DLL Not Loaded"));
    AppendLog(_T("DLL unloaded."));
}

void CPTMTestAppDlg::OnCbnSelchangeConnMode()
{
    GetDlgItem(IDC_EDIT_PORT)->EnableWindow(!IsVisaMode());
}

void CPTMTestAppDlg::OnBnClickedEnumerate()
{
    char buf[4096] = {};
    int n = m_loader.EnumerateVisaResources(buf, sizeof(buf));
    m_comboAddress.ResetContent();
    if (n <= 0 || buf[0] == '\0')
    {
        AppendLog(_T("No VISA USB resources found."));
        return;
    }
    CString all(buf);
    int start = 0;
    while (start >= 0)
    {
        CString item = all.Tokenize(_T(";"), start);
        if (!item.IsEmpty())
            m_comboAddress.AddString(item);
    }
    if (m_comboAddress.GetCount() > 0)
        m_comboAddress.SetCurSel(0);
    CString msg;
    msg.Format(_T("Found %d VISA resource(s)."), n);
    AppendLog(msg);
}

void CPTMTestAppDlg::OnBnClickedConnect()
{
    CString addr, portStr;
    m_comboAddress.GetWindowText(addr);
    m_editPort.GetWindowText(portStr);
    int port = _ttoi(portStr);
    CT2A addrA(addr);

    bool ok = false;
    if (IsVisaMode())
        ok = m_loader.CreateDriverEx("ptm", addrA, 0, 0, 2);
    else
        ok = m_loader.CreateDriver("ptm", addrA, port, 0);

    if (!ok)
    {
        AppendLog(_T("CreateDriver failed."));
        return;
    }
    if (!m_loader.Connect())
    {
        AppendLog(_T("Connect failed."));
        m_loader.DestroyDriver();
        return;
    }

    PTMDeviceInfo info = {};
    if (m_loader.GetDeviceInfo(&info))
    {
        CString msg;
        msg.Format(_T("Connected: %S / %S / %S"), info.manufacturer, info.model, info.serialNumber);
        AppendLog(msg);
    }
    EnableDeviceControls(true);
    UpdateStatus(_T("Connected"));
}

void CPTMTestAppDlg::OnBnClickedDisconnect()
{
    m_loader.Disconnect();
    m_loader.DestroyDriver();
    EnableDeviceControls(false);
    UpdateStatus(_T("Disconnected"));
    AppendLog(_T("Disconnected."));
}

void CPTMTestAppDlg::OnBnClickedInitialize()
{
    if (!m_loader.Initialize())
    {
        AppendLog(_T("Initialize failed."));
        return;
    }
    int ch = m_loader.GetChannelCount();
    CString msg;
    msg.Format(_T("Initialized. Channels=%d"), ch);
    AppendLog(msg);
    CString chStr;
    chStr.Format(_T("%d"), ch);
    m_editChannels.SetWindowText(chStr);
}

void CPTMTestAppDlg::OnBnClickedMeasure()
{
    CString chStr;
    m_editChannels.GetWindowText(chStr);
    int n = _ttoi(chStr);
    if (!m_loader.TakeMeasurement(n))
    {
        AppendLog(_T("TakeMeasurement failed."));
        return;
    }
    AppendLog(_T("Measurement complete."));
    OnBnClickedGetResults();
}

void CPTMTestAppDlg::OnBnClickedGetResults()
{
    PTMPolarityResult results[288];
    int count = m_loader.GetResults(results, 288);
    PopulateResults(results, count);

    char raw[4096] = {};
    if (m_loader.GetRawResult(raw, sizeof(raw)))
        m_editRaw.SetWindowText(CString(raw));

    CString msg;
    msg.Format(_T("Got %d channel mapping(s)."), count);
    AppendLog(msg);
}

void CPTMTestAppDlg::OnBnClickedDetect()
{
    CString chStr;
    m_editDetectCh.GetWindowText(chStr);
    int ch = _ttoi(chStr);
    int det = m_loader.Detect(ch);
    CString msg;
    msg.Format(_T("DETECT%d? -> %d"), ch, det);
    AppendLog(msg);
}

void CPTMTestAppDlg::OnBnClickedVfl()
{
    CString chStr;
    m_editDetectCh.GetWindowText(chStr);
    int ch = _ttoi(chStr);
    if (m_loader.SetVfl(ch, 1))
        AppendLog(_T("VFL enabled."));
    else
        AppendLog(_T("VFL failed."));
}

void CPTMTestAppDlg::OnBnClickedLaserOff()
{
    if (m_loader.LaserOff())
        AppendLog(_T("LAS:OFF sent."));
}

void CPTMTestAppDlg::OnBnClickedLocal()
{
    BOOL local = m_checkLocal.GetCheck() == BST_CHECKED;
    m_loader.SetLocalMode(local);
    AppendLog(local ? _T("Local mode ON") : _T("Local mode OFF"));
}

void CPTMTestAppDlg::OnBnClickedSendScpi()
{
    CString cmd;
    m_editScpi.GetWindowText(cmd);
    CT2A cmdA(cmd);
    char resp[4096] = {};
    if (!m_loader.SendCommand(cmdA, resp, sizeof(resp)))
    {
        AppendLog(_T("SendCommand failed."));
        return;
    }
    CString msg;
    msg.Format(_T("SCPI '%s' -> '%S'"), cmd, resp);
    AppendLog(msg);
}

void CPTMTestAppDlg::OnBnClickedClearLog()
{
    m_editLog.SetWindowText(_T(""));
}
