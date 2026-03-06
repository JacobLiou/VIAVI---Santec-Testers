#include "stdafx.h"
#include "ViaviApp.h"
#include "ViaviAppDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// ---------------------------------------------------------------------------
// 全局日志回调 -- 将驱动日志通过 PostMessage 转发到对话框
// ---------------------------------------------------------------------------

static CViaviAppDlg* g_pDlg = nullptr;

static void WINAPI PCTLogCallback(int level, const char* source, const char* message)
{
    if (g_pDlg && ::IsWindow(g_pDlg->GetSafeHwnd()))
    {
        static const char* levelNames[] = { "DEBUG", "INFO", "WARN", "ERROR" };
        const char* lvl = (level >= 0 && level <= 3) ? levelNames[level] : "???";
        CString* pMsg = new CString();
        pMsg->Format(_T("[PCT][%S][%S] %S"), lvl, source, message);
        g_pDlg->PostMessage(WM_LOG_MESSAGE, 0, (LPARAM)pMsg);
    }
}

static void WINAPI OSWLogCallback(int level, const char* source, const char* message)
{
    if (g_pDlg && ::IsWindow(g_pDlg->GetSafeHwnd()))
    {
        static const char* levelNames[] = { "DEBUG", "INFO", "WARN", "ERROR" };
        const char* lvl = (level >= 0 && level <= 3) ? levelNames[level] : "???";
        CString* pMsg = new CString();
        pMsg->Format(_T("[OSW][%S][%S] %S"), lvl, source, message);
        g_pDlg->PostMessage(WM_LOG_MESSAGE, 0, (LPARAM)pMsg);
    }
}

// ---------------------------------------------------------------------------
// 消息映射
// ---------------------------------------------------------------------------

BEGIN_MESSAGE_MAP(CViaviAppDlg, CDialogEx)
    ON_WM_CLOSE()
    // PCT
    ON_BN_CLICKED(IDC_BTN_LOAD_PCT, &CViaviAppDlg::OnBnClickedLoadPct)
    ON_BN_CLICKED(IDC_BTN_ENUMERATE_PCT, &CViaviAppDlg::OnBnClickedEnumeratePct)
    ON_BN_CLICKED(IDC_BTN_CONNECT_PCT, &CViaviAppDlg::OnBnClickedConnectPct)
    ON_BN_CLICKED(IDC_BTN_DISCONNECT_PCT, &CViaviAppDlg::OnBnClickedDisconnectPct)
    // OSW
    ON_BN_CLICKED(IDC_BTN_LOAD_OSW, &CViaviAppDlg::OnBnClickedLoadOsw)
    ON_BN_CLICKED(IDC_BTN_ENUMERATE_OSW, &CViaviAppDlg::OnBnClickedEnumerateOsw)
    ON_BN_CLICKED(IDC_BTN_CONNECT_OSW, &CViaviAppDlg::OnBnClickedConnectOsw)
    ON_BN_CLICKED(IDC_BTN_DISCONNECT_OSW, &CViaviAppDlg::OnBnClickedDisconnectOsw)
    // Operations
    ON_BN_CLICKED(IDC_BTN_ZEROING, &CViaviAppDlg::OnBnClickedZeroing)
    ON_BN_CLICKED(IDC_BTN_MEASURE, &CViaviAppDlg::OnBnClickedMeasure)
    ON_BN_CLICKED(IDC_BTN_CONTINUOUS, &CViaviAppDlg::OnBnClickedContinuous)
    ON_BN_CLICKED(IDC_BTN_STOP, &CViaviAppDlg::OnBnClickedStop)
    ON_BN_CLICKED(IDC_BTN_CLEAR_LOG, &CViaviAppDlg::OnBnClickedClearLog)
    ON_BN_CLICKED(IDC_CHECK_OVERRIDE, &CViaviAppDlg::OnBnClickedOverride)
    // Custom messages
    ON_MESSAGE(WM_LOG_MESSAGE, &CViaviAppDlg::OnLogMessage)
    ON_MESSAGE(WM_WORKER_DONE, &CViaviAppDlg::OnWorkerDone)
END_MESSAGE_MAP()

// ---------------------------------------------------------------------------
// 构造 / 析构
// ---------------------------------------------------------------------------

CViaviAppDlg::CViaviAppDlg(CWnd* pParent)
    : CDialogEx(IDD_VIAVIAPP_DIALOG, pParent)
    , m_bBusy(false)
    , m_bStopRequested(false)
    , m_bPctConnected(false)
    , m_bOswConnected(false)
{
}

CViaviAppDlg::~CViaviAppDlg()
{
    g_pDlg = nullptr;
}

void CViaviAppDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    // PCT
    DDX_Control(pDX, IDC_EDIT_PCT_DLL, m_editPctDll);
    DDX_Control(pDX, IDC_COMBO_PCT_CONN_MODE, m_comboPctConnMode);
    DDX_Control(pDX, IDC_COMBO_PCT_ADDR, m_comboPctAddr);
    DDX_Control(pDX, IDC_EDIT_PCT_PORT, m_editPctPort);
    // OSW
    DDX_Control(pDX, IDC_EDIT_OSW_DLL, m_editOswDll);
    DDX_Control(pDX, IDC_COMBO_OSW_CONN_MODE, m_comboOswConnMode);
    DDX_Control(pDX, IDC_COMBO_OSW_ADDR, m_comboOswAddr);
    DDX_Control(pDX, IDC_EDIT_OSW_PORT, m_editOswPort);
    // Test Config
    DDX_Control(pDX, IDC_CHECK_1310, m_check1310);
    DDX_Control(pDX, IDC_CHECK_1550, m_check1550);
    DDX_Control(pDX, IDC_EDIT_CH_FROM, m_editChFrom);
    DDX_Control(pDX, IDC_EDIT_CH_TO, m_editChTo);
    DDX_Control(pDX, IDC_EDIT_OSW_DEVICE_NUM, m_editOswDeviceNum);
    DDX_Control(pDX, IDC_CHECK_OVERRIDE, m_checkOverride);
    DDX_Control(pDX, IDC_EDIT_IL_VALUE, m_editILValue);
    DDX_Control(pDX, IDC_EDIT_LENGTH_VALUE, m_editLengthValue);
    // Results / Log
    DDX_Control(pDX, IDC_LIST_RESULTS, m_listResults);
    DDX_Control(pDX, IDC_EDIT_LOG, m_editLog);
}

// ---------------------------------------------------------------------------
// 初始化
// ---------------------------------------------------------------------------

BOOL CViaviAppDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    g_pDlg = this;

    // PCT defaults
    m_editPctDll.SetWindowText(_T("UDL.ViaviPCT.dll"));
    m_comboPctConnMode.AddString(_T("USB (VISA)"));
    m_comboPctConnMode.AddString(_T("TCP (Ethernet)"));
    m_comboPctConnMode.SetCurSel(1);
    m_editPctPort.SetWindowText(_T("8301"));

    // OSW defaults
    m_editOswDll.SetWindowText(_T("UDL.ViaviOSW.dll"));
    m_comboOswConnMode.AddString(_T("USB (VISA)"));
    m_comboOswConnMode.AddString(_T("TCP (Ethernet)"));
    m_comboOswConnMode.SetCurSel(1);
    m_editOswPort.SetWindowText(_T("8203"));

    // Test config defaults
    m_check1310.SetCheck(BST_CHECKED);
    m_check1550.SetCheck(BST_CHECKED);
    m_editChFrom.SetWindowText(_T("1"));
    m_editChTo.SetWindowText(_T("4"));
    m_editOswDeviceNum.SetWindowText(_T("1"));

    m_checkOverride.SetCheck(BST_CHECKED);
    m_editILValue.SetWindowText(_T("0.1"));
    m_editLengthValue.SetWindowText(_T("3.0"));

    // Results list columns
    m_listResults.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_listResults.InsertColumn(0, _T("CH"), LVCFMT_CENTER, 40);
    m_listResults.InsertColumn(1, _T("WL (nm)"), LVCFMT_CENTER, 65);
    m_listResults.InsertColumn(2, _T("IL (dB)"), LVCFMT_RIGHT, 75);
    m_listResults.InsertColumn(3, _T("ORL (dB)"), LVCFMT_RIGHT, 75);
    m_listResults.InsertColumn(4, _T("Zone1 (dB)"), LVCFMT_RIGHT, 80);
    m_listResults.InsertColumn(5, _T("Zone2 (dB)"), LVCFMT_RIGHT, 80);
    m_listResults.InsertColumn(6, _T("Length (m)"), LVCFMT_RIGHT, 75);
    m_listResults.InsertColumn(7, _T("Power (dBm)"), LVCFMT_RIGHT, 85);

    EnableControls();
    AppendLog(_T("Viavi PCT/OSW Debug Tool started."));
    AppendLog(_T("PCT (mORL-A1) handles measurement. OSW (mOSW-C1) handles channel switching."));
    AppendLog(_T("Load DLLs, connect both devices, then run Zeroing / Measure."));

    return TRUE;
}

void CViaviAppDlg::OnOK() {}

void CViaviAppDlg::OnCancel()
{
    if (m_bBusy)
    {
        MessageBox(_T("An operation is in progress. Please wait."),
                   _T("Busy"), MB_OK | MB_ICONINFORMATION);
        return;
    }
    CDialogEx::OnCancel();
}

void CViaviAppDlg::OnClose()
{
    if (m_bBusy)
    {
        MessageBox(_T("An operation is in progress. Please wait."),
                   _T("Busy"), MB_OK | MB_ICONINFORMATION);
        return;
    }
    CDialogEx::OnClose();
}

// ---------------------------------------------------------------------------
// PCT DLL 加载
// ---------------------------------------------------------------------------

void CViaviAppDlg::OnBnClickedLoadPct()
{
    if (m_pctLoader.IsDllLoaded())
    {
        if (m_pctLoader.GetDriverHandle())
        {
            m_pctLoader.Disconnect();
            m_pctLoader.DestroyDriver();
        }
        m_pctLoader.UnloadDll();
        m_bPctConnected = false;
        AppendLog(_T("[PCT] DLL unloaded."));
        EnableControls();
        return;
    }

    CString path;
    m_editPctDll.GetWindowText(path);
    if (path.IsEmpty()) { AppendLog(_T("Please enter PCT DLL path.")); return; }

    CStringA pathA(path);
    if (m_pctLoader.LoadDll(pathA.GetString()))
    {
        m_pctLoader.SetLogCallback(PCTLogCallback);
        AppendLog(_T("[PCT] DLL loaded successfully."));
    }
    else
    {
        CString msg; msg.Format(_T("[PCT] Failed to load: %s"), (LPCTSTR)path);
        AppendLog(msg);
    }
    EnableControls();
}

// ---------------------------------------------------------------------------
// OSW DLL 加载
// ---------------------------------------------------------------------------

void CViaviAppDlg::OnBnClickedLoadOsw()
{
    if (m_oswLoader.IsDllLoaded())
    {
        if (m_oswLoader.GetDriverHandle())
        {
            m_oswLoader.Disconnect();
            m_oswLoader.DestroyDriver();
        }
        m_oswLoader.UnloadDll();
        m_bOswConnected = false;
        AppendLog(_T("[OSW] DLL unloaded."));
        EnableControls();
        return;
    }

    CString path;
    m_editOswDll.GetWindowText(path);
    if (path.IsEmpty()) { AppendLog(_T("Please enter OSW DLL path.")); return; }

    CStringA pathA(path);
    if (m_oswLoader.LoadDll(pathA.GetString()))
    {
        m_oswLoader.SetLogCallback(OSWLogCallback);
        AppendLog(_T("[OSW] DLL loaded successfully."));
    }
    else
    {
        CString msg; msg.Format(_T("[OSW] Failed to load: %s"), (LPCTSTR)path);
        AppendLog(msg);
    }
    EnableControls();
}

// ---------------------------------------------------------------------------
// VISA 枚举
// ---------------------------------------------------------------------------

void CViaviAppDlg::OnBnClickedEnumeratePct()
{
    if (!IsPctVisaMode())
    {
        AppendLog(_T("[PCT] VISA enumeration is only available in USB (VISA) mode."));
        return;
    }

    m_comboPctAddr.ResetContent();

    char buf[4096] = { 0 };
    int found = 0;
    if (m_pctLoader.IsDllLoaded() && m_pctLoader.HasVisaSupport())
        found = m_pctLoader.EnumerateVisaResources(buf, sizeof(buf));

    if (found > 0)
    {
        CString all(buf);
        int pos = 0;
        CString token = all.Tokenize(_T(";"), pos);
        while (!token.IsEmpty())
        {
            token.Trim();
            if (!token.IsEmpty())
                m_comboPctAddr.AddString(token);
            token = all.Tokenize(_T(";"), pos);
        }
        CString msg; msg.Format(_T("[PCT] Found %d VISA resource(s)."), found);
        AppendLog(msg);
        if (m_comboPctAddr.GetCount() > 0)
            m_comboPctAddr.SetCurSel(0);
    }
    else
    {
        AppendLog(_T("[PCT] No VISA resources found."));
    }
}

void CViaviAppDlg::OnBnClickedEnumerateOsw()
{
    if (!IsOswVisaMode())
    {
        AppendLog(_T("[OSW] VISA enumeration is only available in USB (VISA) mode."));
        return;
    }

    m_comboOswAddr.ResetContent();

    char buf[4096] = { 0 };
    int found = 0;
    if (m_oswLoader.IsDllLoaded() && m_oswLoader.HasVisaSupport())
        found = m_oswLoader.EnumerateVisaResources(buf, sizeof(buf));

    if (found > 0)
    {
        CString all(buf);
        int pos = 0;
        CString token = all.Tokenize(_T(";"), pos);
        while (!token.IsEmpty())
        {
            token.Trim();
            if (!token.IsEmpty())
                m_comboOswAddr.AddString(token);
            token = all.Tokenize(_T(";"), pos);
        }
        CString msg; msg.Format(_T("[OSW] Found %d VISA resource(s)."), found);
        AppendLog(msg);
        if (m_comboOswAddr.GetCount() > 0)
            m_comboOswAddr.SetCurSel(0);
    }
    else
    {
        AppendLog(_T("[OSW] No VISA resources found."));
    }
}

// ---------------------------------------------------------------------------
// PCT 连接 / 断开
// ---------------------------------------------------------------------------

void CViaviAppDlg::OnBnClickedConnectPct()
{
    CString pctAddr;
    m_comboPctAddr.GetWindowText(pctAddr);
    if (pctAddr.IsEmpty())
    {
        AppendLog(_T("[PCT] Please specify address."));
        return;
    }

    if (m_pctLoader.GetDriverHandle())
    {
        m_pctLoader.Disconnect();
        m_pctLoader.DestroyDriver();
    }
    m_bPctConnected = false;

    CStringA addrA(pctAddr);
    std::string addrStr(addrA.GetString());
    bool useVisa = IsPctVisaMode();

    CString portStr;
    m_editPctPort.GetWindowText(portStr);
    int port = _ttoi(portStr);
    if (port <= 0) port = 8301;

    CViaviPCTDllLoader* pPct = &m_pctLoader;
    bool* pConn = &m_bPctConnected;

    bool created = false;
    if (useVisa)
    {
        created = pPct->CreateDriverEx("viavi", addrStr.c_str(), 0, 0, 2);
        AppendLog(_T("[PCT] Creating driver (USB VISA)..."));
    }
    else
    {
        created = pPct->CreateDriver("viavi", addrStr.c_str(), port, 0);
        CString msg; msg.Format(_T("[PCT] Creating driver (TCP %s:%d)..."), (LPCTSTR)pctAddr, port);
        AppendLog(msg);
    }

    if (!created)
    {
        AppendLog(_T("[PCT] Failed to create driver."));
        return;
    }

    RunAsync(_T("Connecting PCT..."), [pPct, pConn]() -> WorkerResult*
    {
        WorkerResult* r = new WorkerResult();
        CString log;

        BOOL ok = pPct->Connect();
        if (ok)
        {
            *pConn = true;
            pPct->Initialize();
            log += _T("[PCT] Connected and initialized.\r\n");

            PCTDeviceInfo info = {};
            if (pPct->GetDeviceInfo(&info))
            {
                CString infoMsg;
                infoMsg.Format(_T("[PCT] SN: %S  FW: %S  Desc: %S"),
                               info.serialNumber, info.firmwareVersion, info.description);
                log += infoMsg;
            }
        }
        else
        {
            log += _T("[PCT] Connection FAILED.");
        }

        r->success = *pConn;
        r->logMessage = log;
        r->statusText = *pConn ? _T("PCT Connected") : _T("PCT Connection Failed");
        return r;
    });
}

void CViaviAppDlg::OnBnClickedDisconnectPct()
{
    CViaviPCTDllLoader* pPct = &m_pctLoader;
    bool* pConn = &m_bPctConnected;

    RunAsync(_T("Disconnecting PCT..."), [pPct, pConn]() -> WorkerResult*
    {
        WorkerResult* r = new WorkerResult();
        if (pPct->GetDriverHandle())
        {
            pPct->Disconnect();
            pPct->DestroyDriver();
        }
        *pConn = false;
        r->success = true;
        r->logMessage = _T("[PCT] Disconnected.");
        r->statusText = _T("PCT Disconnected");
        return r;
    });
}

// ---------------------------------------------------------------------------
// OSW 连接 / 断开
// ---------------------------------------------------------------------------

void CViaviAppDlg::OnBnClickedConnectOsw()
{
    CString oswAddr;
    m_comboOswAddr.GetWindowText(oswAddr);
    if (oswAddr.IsEmpty())
    {
        AppendLog(_T("[OSW] Please specify address."));
        return;
    }

    if (m_oswLoader.GetDriverHandle())
    {
        m_oswLoader.Disconnect();
        m_oswLoader.DestroyDriver();
    }
    m_bOswConnected = false;

    CStringA addrA(oswAddr);
    std::string addrStr(addrA.GetString());
    bool useVisa = IsOswVisaMode();

    CString portStr;
    m_editOswPort.GetWindowText(portStr);
    int port = _ttoi(portStr);
    if (port <= 0) port = 8203;

    CViaviOSWDllLoader* pOsw = &m_oswLoader;
    bool* pConn = &m_bOswConnected;

    bool created = false;
    if (useVisa)
    {
        created = pOsw->CreateDriverEx(addrStr.c_str(), 0, 2);
        AppendLog(_T("[OSW] Creating driver (USB VISA)..."));
    }
    else
    {
        created = pOsw->CreateDriver(addrStr.c_str(), port);
        CString msg; msg.Format(_T("[OSW] Creating driver (TCP %s:%d)..."), (LPCTSTR)oswAddr, port);
        AppendLog(msg);
    }

    if (!created)
    {
        AppendLog(_T("[OSW] Failed to create driver."));
        return;
    }

    RunAsync(_T("Connecting OSW..."), [pOsw, pConn]() -> WorkerResult*
    {
        WorkerResult* r = new WorkerResult();
        CString log;

        BOOL ok = pOsw->Connect();
        if (ok)
        {
            *pConn = true;
            log += _T("[OSW] Connected.\r\n");

            OSWDeviceInfo info = {};
            if (pOsw->GetDeviceInfo(&info))
            {
                CString infoMsg;
                infoMsg.Format(_T("[OSW] SN: %S  FW: %S  Desc: %S"),
                               info.serialNumber, info.firmwareVersion, info.description);
                log += infoMsg;
            }

            int devCount = pOsw->GetDeviceCount();
            CString devMsg;
            devMsg.Format(_T("\r\n[OSW] Device count: %d"), devCount);
            log += devMsg;

            for (int d = 1; d <= devCount; ++d)
            {
                OSWSwitchInfo swInfo = {};
                if (pOsw->GetSwitchInfo(d, &swInfo))
                {
                    CString swMsg;
                    swMsg.Format(_T("\r\n[OSW] Device %d: %d channels, current=%d"),
                                 d, swInfo.channelCount, swInfo.currentChannel);
                    log += swMsg;
                }
            }
        }
        else
        {
            log += _T("[OSW] Connection FAILED.");
        }

        r->success = *pConn;
        r->logMessage = log;
        r->statusText = *pConn ? _T("OSW Connected") : _T("OSW Connection Failed");
        return r;
    });
}

void CViaviAppDlg::OnBnClickedDisconnectOsw()
{
    CViaviOSWDllLoader* pOsw = &m_oswLoader;
    bool* pConn = &m_bOswConnected;

    RunAsync(_T("Disconnecting OSW..."), [pOsw, pConn]() -> WorkerResult*
    {
        WorkerResult* r = new WorkerResult();
        if (pOsw->GetDriverHandle())
        {
            pOsw->Disconnect();
            pOsw->DestroyDriver();
        }
        *pConn = false;
        r->success = true;
        r->logMessage = _T("[OSW] Disconnected.");
        r->statusText = _T("OSW Disconnected");
        return r;
    });
}

// ---------------------------------------------------------------------------
// 清零（参考测量）
// ---------------------------------------------------------------------------

void CViaviAppDlg::OnBnClickedZeroing()
{
    if (!m_pctLoader.GetDriverHandle() || !m_bPctConnected) return;

    std::vector<double> wavelengths = GetSelectedWavelengths();
    std::vector<int> channels = GetSelectedChannels();
    int oswDeviceNum = GetOswDeviceNum();
    bool useOsw = m_bOswConnected && (channels.size() > 1);

    BOOL bOverride = (m_checkOverride.GetCheck() == BST_CHECKED);
    CString ilStr, lenStr;
    m_editILValue.GetWindowText(ilStr);
    m_editLengthValue.GetWindowText(lenStr);
    double ilValue = _ttof(ilStr);
    double lengthValue = _ttof(lenStr);

    AppendLog(_T("=== Zeroing (Reference) Started ==="));

    CViaviPCTDllLoader* pPct = &m_pctLoader;
    CViaviOSWDllLoader* pOsw = &m_oswLoader;
    std::atomic<bool>* pStop = &m_bStopRequested;
    m_bStopRequested = false;

    RunAsync(_T("Zeroing..."),
        [pPct, pOsw, wavelengths, channels, oswDeviceNum, useOsw,
         bOverride, ilValue, lengthValue, pStop]() -> WorkerResult*
    {
        WorkerResult* r = new WorkerResult();
        CString log;

        try
        {
            std::vector<double> wl = wavelengths;
            pPct->ConfigureWavelengths(wl.data(), (int)wl.size());

            if (!useOsw)
            {
                std::vector<int> ch = channels;
                pPct->ConfigureChannels(ch.data(), (int)ch.size());
                BOOL ok = pPct->TakeReference(bOverride, ilValue, lengthValue);
                log.Format(_T("Reference %s (single channel, no OSW switching)."),
                           ok ? _T("completed") : _T("FAILED"));
                r->success = (ok != FALSE);
            }
            else
            {
                bool allOk = true;
                for (size_t i = 0; i < channels.size(); ++i)
                {
                    if (pStop->load()) { log += _T("\r\nStopped by user."); break; }

                    int ch = channels[i];

                    CString chLog;
                    chLog.Format(_T("  CH%d: OSW switching device %d -> channel %d ..."),
                                 ch, oswDeviceNum, ch);
                    log += chLog;

                    pOsw->SwitchChannel(oswDeviceNum, ch);
                    pOsw->WaitForIdle(5000);

                    std::vector<int> singleCh = { ch };
                    pPct->ConfigureChannels(singleCh.data(), 1);

                    BOOL ok = pPct->TakeReference(bOverride, ilValue, lengthValue);
                    if (ok)
                        log += _T(" Reference OK\r\n");
                    else
                    {
                        log += _T(" Reference FAILED\r\n");
                        allOk = false;
                    }
                }
                r->success = allOk;
            }

            r->logMessage = _T("=== Zeroing Complete ===\r\n") + log;
            r->statusText = r->success ? _T("Zeroing Done - Ready") : _T("Zeroing Failed");
        }
        catch (...)
        {
            r->logMessage = _T("Zeroing error.");
            r->statusText = _T("Zeroing Error");
        }
        return r;
    });
}

// ---------------------------------------------------------------------------
// 测量
// ---------------------------------------------------------------------------

void CViaviAppDlg::OnBnClickedMeasure()
{
    if (!m_pctLoader.GetDriverHandle() || !m_bPctConnected) return;

    std::vector<double> wavelengths = GetSelectedWavelengths();
    std::vector<int> channels = GetSelectedChannels();
    int oswDeviceNum = GetOswDeviceNum();
    bool useOsw = m_bOswConnected && (channels.size() > 1);

    AppendLog(_T("=== Measurement Started ==="));

    CViaviPCTDllLoader* pPct = &m_pctLoader;
    CViaviOSWDllLoader* pOsw = &m_oswLoader;
    std::atomic<bool>* pStop = &m_bStopRequested;
    m_bStopRequested = false;

    RunAsync(_T("Measuring..."),
        [pPct, pOsw, wavelengths, channels, oswDeviceNum, useOsw, pStop]() -> WorkerResult*
    {
        WorkerResult* r = new WorkerResult();
        CString log;

        try
        {
            std::vector<double> wl = wavelengths;
            pPct->ConfigureWavelengths(wl.data(), (int)wl.size());

            std::vector<PCTMeasurementResult> allResults;

            if (!useOsw)
            {
                std::vector<int> ch = channels;
                pPct->ConfigureChannels(ch.data(), (int)ch.size());

                BOOL ok = pPct->TakeMeasurement();
                if (ok)
                {
                    PCTMeasurementResult buf[256];
                    int count = pPct->GetResults(buf, 256);
                    allResults.assign(buf, buf + count);
                }
                CString msg;
                msg.Format(_T("Measurement %s. Got %d result(s)."),
                           ok ? _T("completed") : _T("FAILED"), (int)allResults.size());
                log += msg;
                r->success = (ok != FALSE);
            }
            else
            {
                bool allOk = true;
                for (size_t i = 0; i < channels.size(); ++i)
                {
                    if (pStop->load()) { log += _T("\r\nStopped by user."); break; }

                    int ch = channels[i];
                    CString chLog;
                    chLog.Format(_T("  CH%d: OSW switching ..."), ch);
                    log += chLog;

                    pOsw->SwitchChannel(oswDeviceNum, ch);
                    pOsw->WaitForIdle(5000);

                    std::vector<int> singleCh = { ch };
                    pPct->ConfigureChannels(singleCh.data(), 1);

                    BOOL ok = pPct->TakeMeasurement();
                    if (ok)
                    {
                        PCTMeasurementResult buf[256];
                        int count = pPct->GetResults(buf, 256);
                        for (int j = 0; j < count; ++j)
                            allResults.push_back(buf[j]);
                        CString msg;
                        msg.Format(_T(" Measured, %d result(s)\r\n"), count);
                        log += msg;
                    }
                    else
                    {
                        log += _T(" Measurement FAILED\r\n");
                        allOk = false;
                    }
                }
                r->success = allOk;
            }

            r->results = allResults;
            r->hasResults = true;

            CString summary;
            summary.Format(_T("=== Measurement Complete: %d total result(s) ==="),
                           (int)allResults.size());
            r->logMessage = summary + _T("\r\n") + log;
            r->statusText = r->success ? _T("Measurement Done - Ready") : _T("Measurement Failed");
        }
        catch (...)
        {
            r->logMessage = _T("Measurement error.");
            r->statusText = _T("Measurement Error");
        }
        return r;
    });
}

// ---------------------------------------------------------------------------
// 连续测试（清零 + 测量循环）
// ---------------------------------------------------------------------------

void CViaviAppDlg::OnBnClickedContinuous()
{
    if (!m_pctLoader.GetDriverHandle() || !m_bPctConnected) return;

    std::vector<double> wavelengths = GetSelectedWavelengths();
    std::vector<int> channels = GetSelectedChannels();
    int oswDeviceNum = GetOswDeviceNum();
    bool useOsw = m_bOswConnected && (channels.size() > 1);

    BOOL bOverride = (m_checkOverride.GetCheck() == BST_CHECKED);
    CString ilStr, lenStr;
    m_editILValue.GetWindowText(ilStr);
    m_editLengthValue.GetWindowText(lenStr);
    double ilValue = _ttof(ilStr);
    double lengthValue = _ttof(lenStr);

    AppendLog(_T("=== Continuous Test Started (press STOP to end) ==="));
    m_bStopRequested = false;

    CViaviPCTDllLoader* pPct = &m_pctLoader;
    CViaviOSWDllLoader* pOsw = &m_oswLoader;
    std::atomic<bool>* pStop = &m_bStopRequested;

    RunAsync(_T("Continuous Test..."),
        [pPct, pOsw, wavelengths, channels, oswDeviceNum, useOsw,
         bOverride, ilValue, lengthValue, pStop]() -> WorkerResult*
    {
        WorkerResult* r = new WorkerResult();
        CString log;
        int iteration = 0;
        std::vector<PCTMeasurementResult> allResults;

        try
        {
            std::vector<double> wl = wavelengths;
            pPct->ConfigureWavelengths(wl.data(), (int)wl.size());

            while (!pStop->load())
            {
                ++iteration;
                CString iterLog;
                iterLog.Format(_T("--- Iteration %d ---\r\n"), iteration);
                log += iterLog;

                // Phase 1: 清零
                if (!useOsw)
                {
                    std::vector<int> ch = channels;
                    pPct->ConfigureChannels(ch.data(), (int)ch.size());
                    pPct->TakeReference(bOverride, ilValue, lengthValue);
                }
                else
                {
                    for (size_t i = 0; i < channels.size() && !pStop->load(); ++i)
                    {
                        int ch = channels[i];
                        pOsw->SwitchChannel(oswDeviceNum, ch);
                        pOsw->WaitForIdle(5000);
                        std::vector<int> singleCh = { ch };
                        pPct->ConfigureChannels(singleCh.data(), 1);
                        pPct->TakeReference(bOverride, ilValue, lengthValue);
                    }
                }

                if (pStop->load()) break;
                log += _T("  Zeroing done.\r\n");

                // Phase 2: 测量
                allResults.clear();
                if (!useOsw)
                {
                    std::vector<int> ch = channels;
                    pPct->ConfigureChannels(ch.data(), (int)ch.size());
                    if (pPct->TakeMeasurement())
                    {
                        PCTMeasurementResult buf[256];
                        int count = pPct->GetResults(buf, 256);
                        allResults.assign(buf, buf + count);
                    }
                }
                else
                {
                    for (size_t i = 0; i < channels.size() && !pStop->load(); ++i)
                    {
                        int ch = channels[i];
                        pOsw->SwitchChannel(oswDeviceNum, ch);
                        pOsw->WaitForIdle(5000);
                        std::vector<int> singleCh = { ch };
                        pPct->ConfigureChannels(singleCh.data(), 1);
                        if (pPct->TakeMeasurement())
                        {
                            PCTMeasurementResult buf[256];
                            int count = pPct->GetResults(buf, 256);
                            for (int j = 0; j < count; ++j)
                                allResults.push_back(buf[j]);
                        }
                    }
                }

                CString measLog;
                measLog.Format(_T("  Measurement done: %d result(s).\r\n"),
                               (int)allResults.size());
                log += measLog;
            }

            r->results = allResults;
            r->hasResults = true;
            r->success = true;

            CString summary;
            summary.Format(_T("=== Continuous Test Stopped after %d iteration(s) ==="),
                           iteration);
            r->logMessage = summary + _T("\r\n") + log;
            r->statusText = _T("Continuous Test Done");
        }
        catch (...)
        {
            r->logMessage = _T("Continuous test error.");
            r->statusText = _T("Continuous Test Error");
        }
        return r;
    });
}

void CViaviAppDlg::OnBnClickedStop()
{
    m_bStopRequested = true;
    AppendLog(_T("Stop requested... waiting for current operation to finish."));
}

// ---------------------------------------------------------------------------
// Override 复选框
// ---------------------------------------------------------------------------

void CViaviAppDlg::OnBnClickedOverride()
{
    BOOL enabled = (m_checkOverride.GetCheck() == BST_CHECKED);
    m_editILValue.EnableWindow(enabled);
    m_editLengthValue.EnableWindow(enabled);
}

// ---------------------------------------------------------------------------
// 异步工作线程基础设施
// ---------------------------------------------------------------------------

void CViaviAppDlg::RunAsync(const CString& operationName,
                              std::function<WorkerResult*()> task)
{
    if (m_bBusy)
    {
        AppendLog(_T("Another operation is in progress."));
        return;
    }

    SetBusy(true, operationName);

    HWND hWnd = GetSafeHwnd();
    std::thread worker([hWnd, task]()
    {
        WorkerResult* result = nullptr;
        try { result = task(); }
        catch (...)
        {
            result = new WorkerResult();
            result->logMessage = _T("Unexpected internal error.");
            result->statusText = _T("Error");
        }

        if (::IsWindow(hWnd))
            ::PostMessage(hWnd, WM_WORKER_DONE, 0, (LPARAM)result);
        else
            delete result;
    });
    worker.detach();
}

LRESULT CViaviAppDlg::OnWorkerDone(WPARAM, LPARAM lParam)
{
    WorkerResult* result = reinterpret_cast<WorkerResult*>(lParam);
    if (result)
    {
        if (!result->logMessage.IsEmpty())
            AppendLog(result->logMessage);
        if (result->hasResults)
            PopulateResultsList(result->results);
        delete result;
    }
    SetBusy(false);
    return 0;
}

// ---------------------------------------------------------------------------
// UI 状态管理
// ---------------------------------------------------------------------------

void CViaviAppDlg::SetBusy(bool busy, const CString& statusText)
{
    m_bBusy = busy;
    EnableControls();
}

void CViaviAppDlg::EnableControls()
{
    bool pctLoaded = m_pctLoader.IsDllLoaded();
    bool oswLoaded = m_oswLoader.IsDllLoaded();
    bool pctVisa = IsPctVisaMode();
    bool oswVisa = IsOswVisaMode();
    bool bothConnected = m_bPctConnected && m_bOswConnected;
    bool pctOnly = m_bPctConnected;

    // PCT connection group
    GetDlgItem(IDC_EDIT_PCT_DLL)->EnableWindow(!m_bBusy && !pctLoaded);
    SetDlgItemText(IDC_BTN_LOAD_PCT, pctLoaded ? _T("Unload") : _T("Load"));
    GetDlgItem(IDC_BTN_LOAD_PCT)->EnableWindow(!m_bBusy);

    GetDlgItem(IDC_COMBO_PCT_CONN_MODE)->EnableWindow(!m_bBusy && !m_bPctConnected);
    GetDlgItem(IDC_BTN_ENUMERATE_PCT)->EnableWindow(!m_bBusy && pctVisa && pctLoaded);
    GetDlgItem(IDC_COMBO_PCT_ADDR)->EnableWindow(!m_bBusy && pctLoaded && !m_bPctConnected);
    GetDlgItem(IDC_EDIT_PCT_PORT)->EnableWindow(!m_bBusy && !pctVisa && !m_bPctConnected);
    GetDlgItem(IDC_BTN_CONNECT_PCT)->EnableWindow(!m_bBusy && pctLoaded && !m_bPctConnected);
    GetDlgItem(IDC_BTN_DISCONNECT_PCT)->EnableWindow(!m_bBusy && m_bPctConnected);

    // OSW connection group
    GetDlgItem(IDC_EDIT_OSW_DLL)->EnableWindow(!m_bBusy && !oswLoaded);
    SetDlgItemText(IDC_BTN_LOAD_OSW, oswLoaded ? _T("Unload") : _T("Load"));
    GetDlgItem(IDC_BTN_LOAD_OSW)->EnableWindow(!m_bBusy);

    GetDlgItem(IDC_COMBO_OSW_CONN_MODE)->EnableWindow(!m_bBusy && !m_bOswConnected);
    GetDlgItem(IDC_BTN_ENUMERATE_OSW)->EnableWindow(!m_bBusy && oswVisa && oswLoaded);
    GetDlgItem(IDC_COMBO_OSW_ADDR)->EnableWindow(!m_bBusy && oswLoaded && !m_bOswConnected);
    GetDlgItem(IDC_EDIT_OSW_PORT)->EnableWindow(!m_bBusy && !oswVisa && !m_bOswConnected);
    GetDlgItem(IDC_BTN_CONNECT_OSW)->EnableWindow(!m_bBusy && oswLoaded && !m_bOswConnected);
    GetDlgItem(IDC_BTN_DISCONNECT_OSW)->EnableWindow(!m_bBusy && m_bOswConnected);

    // Test configuration
    GetDlgItem(IDC_CHECK_1310)->EnableWindow(!m_bBusy);
    GetDlgItem(IDC_CHECK_1550)->EnableWindow(!m_bBusy);
    GetDlgItem(IDC_EDIT_CH_FROM)->EnableWindow(!m_bBusy);
    GetDlgItem(IDC_EDIT_CH_TO)->EnableWindow(!m_bBusy);
    GetDlgItem(IDC_EDIT_OSW_DEVICE_NUM)->EnableWindow(!m_bBusy);
    GetDlgItem(IDC_CHECK_OVERRIDE)->EnableWindow(!m_bBusy);

    BOOL overrideOn = (m_checkOverride.GetCheck() == BST_CHECKED);
    GetDlgItem(IDC_EDIT_IL_VALUE)->EnableWindow(!m_bBusy && overrideOn);
    GetDlgItem(IDC_EDIT_LENGTH_VALUE)->EnableWindow(!m_bBusy && overrideOn);

    // Operations: require at least PCT connected
    GetDlgItem(IDC_BTN_ZEROING)->EnableWindow(!m_bBusy && pctOnly);
    GetDlgItem(IDC_BTN_MEASURE)->EnableWindow(!m_bBusy && pctOnly);
    GetDlgItem(IDC_BTN_CONTINUOUS)->EnableWindow(!m_bBusy && pctOnly);
    GetDlgItem(IDC_BTN_STOP)->EnableWindow(m_bBusy);

    // Update status labels
    UpdatePctStatus(m_bPctConnected ? _T("Connected") : _T("Not Connected"));
    UpdateOswStatus(m_bOswConnected ? _T("Connected") : _T("Not Connected"));
}

// ---------------------------------------------------------------------------
// 日志 / 状态
// ---------------------------------------------------------------------------

LRESULT CViaviAppDlg::OnLogMessage(WPARAM, LPARAM lParam)
{
    CString* pMsg = reinterpret_cast<CString*>(lParam);
    if (pMsg) { AppendLog(*pMsg); delete pMsg; }
    return 0;
}

void CViaviAppDlg::AppendLog(const CString& text)
{
    CString current;
    m_editLog.GetWindowText(current);
    if (!current.IsEmpty()) current += _T("\r\n");
    current += text;
    m_editLog.SetWindowText(current);
    m_editLog.LineScroll(m_editLog.GetLineCount());
}

void CViaviAppDlg::UpdatePctStatus(const CString& status)
{
    SetDlgItemText(IDC_STATIC_PCT_STATUS, status);
}

void CViaviAppDlg::UpdateOswStatus(const CString& status)
{
    SetDlgItemText(IDC_STATIC_OSW_STATUS, status);
}

void CViaviAppDlg::OnBnClickedClearLog()
{
    m_editLog.SetWindowText(_T(""));
}

// ---------------------------------------------------------------------------
// 结果列表
// ---------------------------------------------------------------------------

void CViaviAppDlg::PopulateResultsList(const std::vector<PCTMeasurementResult>& results)
{
    m_listResults.DeleteAllItems();

    for (size_t i = 0; i < results.size(); ++i)
    {
        const PCTMeasurementResult& res = results[i];
        CString chStr, wlStr, ilStr, orlStr, z1Str, z2Str, lenStr, pwrStr;
        chStr.Format(_T("%d"), res.channel);
        wlStr.Format(_T("%.0f"), res.wavelength);
        ilStr.Format(_T("%.4f"), res.insertionLoss);
        orlStr.Format(_T("%.2f"), res.returnLoss);
        z1Str.Format(_T("%.2f"), res.returnLossZone1);
        z2Str.Format(_T("%.2f"), res.returnLossZone2);
        lenStr.Format(_T("%.2f"), res.dutLength);
        pwrStr.Format(_T("%.2f"), res.power);

        int idx = m_listResults.InsertItem(static_cast<int>(i), chStr);
        m_listResults.SetItemText(idx, 1, wlStr);
        m_listResults.SetItemText(idx, 2, ilStr);
        m_listResults.SetItemText(idx, 3, orlStr);
        m_listResults.SetItemText(idx, 4, z1Str);
        m_listResults.SetItemText(idx, 5, z2Str);
        m_listResults.SetItemText(idx, 6, lenStr);
        m_listResults.SetItemText(idx, 7, pwrStr);
    }
}

// ---------------------------------------------------------------------------
// 连接模式
// ---------------------------------------------------------------------------

bool CViaviAppDlg::IsPctVisaMode()
{
    return (m_comboPctConnMode.GetCurSel() == 0);
}

bool CViaviAppDlg::IsOswVisaMode()
{
    return (m_comboOswConnMode.GetCurSel() == 0);
}

// ---------------------------------------------------------------------------
// 辅助函数
// ---------------------------------------------------------------------------

std::vector<double> CViaviAppDlg::GetSelectedWavelengths()
{
    std::vector<double> wl;
    if (m_check1310.GetCheck() == BST_CHECKED) wl.push_back(1310.0);
    if (m_check1550.GetCheck() == BST_CHECKED) wl.push_back(1550.0);
    if (wl.empty())
    {
        AppendLog(_T("Warning: No wavelength selected, defaulting to 1310+1550."));
        wl.push_back(1310.0);
        wl.push_back(1550.0);
    }
    return wl;
}

std::vector<int> CViaviAppDlg::GetSelectedChannels()
{
    CString fromStr, toStr;
    m_editChFrom.GetWindowText(fromStr);
    m_editChTo.GetWindowText(toStr);

    int from = _ttoi(fromStr);
    int to = _ttoi(toStr);
    if (from < 1) from = 1;
    if (to < from) to = from;

    std::vector<int> ch;
    for (int i = from; i <= to; ++i)
        ch.push_back(i);
    return ch;
}

int CViaviAppDlg::GetOswDeviceNum()
{
    CString str;
    m_editOswDeviceNum.GetWindowText(str);
    int num = _ttoi(str);
    return (num >= 1) ? num : 1;
}
