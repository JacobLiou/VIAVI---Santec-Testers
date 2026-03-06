#include "stdafx.h"
#include "SantecApp.h"
#include "SantecAppDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// ---------------------------------------------------------------------------
// 全局日志回调 -- 将驱动日志通过 PostMessage 转发到对话框
// ---------------------------------------------------------------------------

static CSantecAppDlg* g_pDlg = nullptr;

static void WINAPI RLMLogCallback(int level, const char* source, const char* message)
{
    if (g_pDlg && ::IsWindow(g_pDlg->GetSafeHwnd()))
    {
        static const char* levelNames[] = { "DEBUG", "INFO", "WARN", "ERROR" };
        const char* lvl = (level >= 0 && level <= 3) ? levelNames[level] : "???";
        CString* pMsg = new CString();
        pMsg->Format(_T("[RLM][%S][%S] %S"), lvl, source, message);
        g_pDlg->PostMessage(WM_LOG_MESSAGE, 0, (LPARAM)pMsg);
    }
}

static void WINAPI OSXLogCallback(int level, const char* source, const char* message)
{
    if (g_pDlg && ::IsWindow(g_pDlg->GetSafeHwnd()))
    {
        static const char* levelNames[] = { "DEBUG", "INFO", "WARN", "ERROR" };
        const char* lvl = (level >= 0 && level <= 3) ? levelNames[level] : "???";
        CString* pMsg = new CString();
        pMsg->Format(_T("[OSX][%S][%S] %S"), lvl, source, message);
        g_pDlg->PostMessage(WM_LOG_MESSAGE, 0, (LPARAM)pMsg);
    }
}

// ---------------------------------------------------------------------------
// 消息映射
// ---------------------------------------------------------------------------

BEGIN_MESSAGE_MAP(CSantecAppDlg, CDialogEx)
    ON_WM_CLOSE()
    // RLM
    ON_BN_CLICKED(IDC_BTN_LOAD_RLM, &CSantecAppDlg::OnBnClickedLoadRlm)
    ON_BN_CLICKED(IDC_BTN_CONNECT_RLM, &CSantecAppDlg::OnBnClickedConnectRlm)
    ON_BN_CLICKED(IDC_BTN_DISCONNECT_RLM, &CSantecAppDlg::OnBnClickedDisconnectRlm)
    // OSX
    ON_BN_CLICKED(IDC_BTN_LOAD_OSX, &CSantecAppDlg::OnBnClickedLoadOsx)
    ON_BN_CLICKED(IDC_BTN_CONNECT_OSX, &CSantecAppDlg::OnBnClickedConnectOsx)
    ON_BN_CLICKED(IDC_BTN_DISCONNECT_OSX, &CSantecAppDlg::OnBnClickedDisconnectOsx)
    // Operations
    ON_BN_CLICKED(IDC_BTN_ZEROING, &CSantecAppDlg::OnBnClickedZeroing)
    ON_BN_CLICKED(IDC_BTN_MEASURE, &CSantecAppDlg::OnBnClickedMeasure)
    ON_BN_CLICKED(IDC_BTN_CONTINUOUS, &CSantecAppDlg::OnBnClickedContinuous)
    ON_BN_CLICKED(IDC_BTN_STOP, &CSantecAppDlg::OnBnClickedStop)
    ON_BN_CLICKED(IDC_BTN_CLEAR_LOG, &CSantecAppDlg::OnBnClickedClearLog)
    ON_BN_CLICKED(IDC_CHECK_OVERRIDE, &CSantecAppDlg::OnBnClickedOverride)
    // Custom messages
    ON_MESSAGE(WM_LOG_MESSAGE, &CSantecAppDlg::OnLogMessage)
    ON_MESSAGE(WM_WORKER_DONE, &CSantecAppDlg::OnWorkerDone)
END_MESSAGE_MAP()

// ---------------------------------------------------------------------------
// 构造 / 析构
// ---------------------------------------------------------------------------

CSantecAppDlg::CSantecAppDlg(CWnd* pParent)
    : CDialogEx(IDD_SANTECAPP_DIALOG, pParent)
    , m_bBusy(false)
    , m_bStopRequested(false)
    , m_bRlmConnected(false)
    , m_bOsxConnected(false)
{
}

CSantecAppDlg::~CSantecAppDlg()
{
    g_pDlg = nullptr;
}

void CSantecAppDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    // RLM
    DDX_Control(pDX, IDC_EDIT_RLM_DLL, m_editRlmDll);
    DDX_Control(pDX, IDC_EDIT_RLM_IP, m_editRlmIp);
    DDX_Control(pDX, IDC_EDIT_RLM_PORT, m_editRlmPort);
    // OSX
    DDX_Control(pDX, IDC_EDIT_OSX_DLL, m_editOsxDll);
    DDX_Control(pDX, IDC_EDIT_OSX_IP, m_editOsxIp);
    DDX_Control(pDX, IDC_EDIT_OSX_PORT, m_editOsxPort);
    // Test Config
    DDX_Control(pDX, IDC_CHECK_1310, m_check1310);
    DDX_Control(pDX, IDC_CHECK_1550, m_check1550);
    DDX_Control(pDX, IDC_EDIT_CH_FROM, m_editChFrom);
    DDX_Control(pDX, IDC_EDIT_CH_TO, m_editChTo);
    DDX_Control(pDX, IDC_EDIT_MODULE_INDEX, m_editModuleIndex);
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

BOOL CSantecAppDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    g_pDlg = this;

    // RLM defaults
    m_editRlmDll.SetWindowText(_T("UDL.SantecRLM.dll"));
    m_editRlmIp.SetWindowText(_T("127.0.0.1"));
    m_editRlmPort.SetWindowText(_T("5025"));

    // OSX defaults
    m_editOsxDll.SetWindowText(_T("UDL.SantecOSX.dll"));
    m_editOsxIp.SetWindowText(_T("127.0.0.1"));
    m_editOsxPort.SetWindowText(_T("5025"));

    // Test config defaults
    m_check1310.SetCheck(BST_CHECKED);
    m_check1550.SetCheck(BST_CHECKED);
    m_editChFrom.SetWindowText(_T("1"));
    m_editChTo.SetWindowText(_T("4"));
    m_editModuleIndex.SetWindowText(_T("0"));

    m_checkOverride.SetCheck(BST_CHECKED);
    m_editILValue.SetWindowText(_T("0.1"));
    m_editLengthValue.SetWindowText(_T("3.0"));

    // Results list columns (RLM specific: RLA, RLB, RLTotal)
    m_listResults.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_listResults.InsertColumn(0, _T("CH"), LVCFMT_CENTER, 40);
    m_listResults.InsertColumn(1, _T("WL (nm)"), LVCFMT_CENTER, 65);
    m_listResults.InsertColumn(2, _T("IL (dB)"), LVCFMT_RIGHT, 75);
    m_listResults.InsertColumn(3, _T("RL (dB)"), LVCFMT_RIGHT, 75);
    m_listResults.InsertColumn(4, _T("RLA (dB)"), LVCFMT_RIGHT, 80);
    m_listResults.InsertColumn(5, _T("RLB (dB)"), LVCFMT_RIGHT, 80);
    m_listResults.InsertColumn(6, _T("RLTotal (dB)"), LVCFMT_RIGHT, 90);
    m_listResults.InsertColumn(7, _T("Length (m)"), LVCFMT_RIGHT, 75);

    EnableControls();
    AppendLog(_T("Santec RLM/OSX Debug Tool started. (TCP Mode)"));
    AppendLog(_T("RLM (RL1 Series) handles measurement. OSX (OSX-100 Series) handles channel switching."));
    AppendLog(_T("Load DLLs, connect both devices, then run Zeroing / Measure."));

    return TRUE;
}

void CSantecAppDlg::OnOK() {}

void CSantecAppDlg::OnCancel()
{
    if (m_bBusy)
    {
        MessageBox(_T("An operation is in progress. Please wait."),
                   _T("Busy"), MB_OK | MB_ICONINFORMATION);
        return;
    }
    CDialogEx::OnCancel();
}

void CSantecAppDlg::OnClose()
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
// RLM DLL 加载
// ---------------------------------------------------------------------------

void CSantecAppDlg::OnBnClickedLoadRlm()
{
    if (m_rlmLoader.IsDllLoaded())
    {
        if (m_rlmLoader.GetDriverHandle())
        {
            m_rlmLoader.Disconnect();
            m_rlmLoader.DestroyDriver();
        }
        m_rlmLoader.UnloadDll();
        m_bRlmConnected = false;
        AppendLog(_T("[RLM] DLL unloaded."));
        EnableControls();
        return;
    }

    CString path;
    m_editRlmDll.GetWindowText(path);
    if (path.IsEmpty()) { AppendLog(_T("Please enter RLM DLL path.")); return; }

    CStringA pathA(path);
    if (m_rlmLoader.LoadDll(pathA.GetString()))
    {
        m_rlmLoader.SetLogCallback(RLMLogCallback);
        AppendLog(_T("[RLM] DLL loaded successfully."));
    }
    else
    {
        CString msg; msg.Format(_T("[RLM] Failed to load: %s"), (LPCTSTR)path);
        AppendLog(msg);
    }
    EnableControls();
}

// ---------------------------------------------------------------------------
// OSX DLL 加载
// ---------------------------------------------------------------------------

void CSantecAppDlg::OnBnClickedLoadOsx()
{
    if (m_osxLoader.IsDllLoaded())
    {
        if (m_osxLoader.GetDriverHandle())
        {
            m_osxLoader.Disconnect();
            m_osxLoader.DestroyDriver();
        }
        m_osxLoader.UnloadDll();
        m_bOsxConnected = false;
        AppendLog(_T("[OSX] DLL unloaded."));
        EnableControls();
        return;
    }

    CString path;
    m_editOsxDll.GetWindowText(path);
    if (path.IsEmpty()) { AppendLog(_T("Please enter OSX DLL path.")); return; }

    CStringA pathA(path);
    if (m_osxLoader.LoadDll(pathA.GetString()))
    {
        m_osxLoader.SetLogCallback(OSXLogCallback);
        AppendLog(_T("[OSX] DLL loaded successfully."));
    }
    else
    {
        CString msg; msg.Format(_T("[OSX] Failed to load: %s"), (LPCTSTR)path);
        AppendLog(msg);
    }
    EnableControls();
}

// ---------------------------------------------------------------------------
// RLM 连接 / 断开
// ---------------------------------------------------------------------------

void CSantecAppDlg::OnBnClickedConnectRlm()
{
    CString ipStr;
    m_editRlmIp.GetWindowText(ipStr);
    if (ipStr.IsEmpty())
    {
        AppendLog(_T("[RLM] Please specify IP address."));
        return;
    }

    if (m_rlmLoader.GetDriverHandle())
    {
        m_rlmLoader.Disconnect();
        m_rlmLoader.DestroyDriver();
    }
    m_bRlmConnected = false;

    CStringA ipA(ipStr);
    std::string ip(ipA.GetString());

    CString portStr;
    m_editRlmPort.GetWindowText(portStr);
    int port = _ttoi(portStr);
    if (port <= 0) port = 5025;

    bool created = m_rlmLoader.CreateDriver("santec", ip.c_str(), port, 0);
    {
        CString msg; msg.Format(_T("[RLM] Creating driver (TCP %s:%d)..."), (LPCTSTR)ipStr, port);
        AppendLog(msg);
    }

    if (!created)
    {
        AppendLog(_T("[RLM] Failed to create driver."));
        return;
    }

    CSantecRLMTcpLoader* pRlm = &m_rlmLoader;
    bool* pConn = &m_bRlmConnected;

    RunAsync(_T("Connecting RLM..."), [pRlm, pConn]() -> WorkerResult*
    {
        WorkerResult* r = new WorkerResult();
        CString log;

        BOOL ok = pRlm->Connect();
        if (ok)
        {
            *pConn = true;
            pRlm->Initialize();
            log += _T("[RLM] Connected and initialized.\r\n");

            RLMDeviceInfo info = {};
            if (pRlm->GetDeviceInfo(&info))
            {
                CString infoMsg;
                infoMsg.Format(_T("[RLM] %S / %S  SN: %S  FW: %S"),
                               info.manufacturer, info.model,
                               info.serialNumber, info.firmwareVersion);
                log += infoMsg;
            }
        }
        else
        {
            log += _T("[RLM] Connection FAILED.");
        }

        r->success = *pConn;
        r->logMessage = log;
        r->statusText = *pConn ? _T("RLM Connected") : _T("RLM Connection Failed");
        return r;
    });
}

void CSantecAppDlg::OnBnClickedDisconnectRlm()
{
    CSantecRLMTcpLoader* pRlm = &m_rlmLoader;
    bool* pConn = &m_bRlmConnected;

    RunAsync(_T("Disconnecting RLM..."), [pRlm, pConn]() -> WorkerResult*
    {
        WorkerResult* r = new WorkerResult();
        if (pRlm->GetDriverHandle())
        {
            pRlm->Disconnect();
            pRlm->DestroyDriver();
        }
        *pConn = false;
        r->success = true;
        r->logMessage = _T("[RLM] Disconnected.");
        r->statusText = _T("RLM Disconnected");
        return r;
    });
}

// ---------------------------------------------------------------------------
// OSX 连接 / 断开
// ---------------------------------------------------------------------------

void CSantecAppDlg::OnBnClickedConnectOsx()
{
    CString ipStr;
    m_editOsxIp.GetWindowText(ipStr);
    if (ipStr.IsEmpty())
    {
        AppendLog(_T("[OSX] Please specify IP address."));
        return;
    }

    if (m_osxLoader.GetDriverHandle())
    {
        m_osxLoader.Disconnect();
        m_osxLoader.DestroyDriver();
    }
    m_bOsxConnected = false;

    CStringA ipA(ipStr);
    std::string ip(ipA.GetString());

    CString portStr;
    m_editOsxPort.GetWindowText(portStr);
    int port = _ttoi(portStr);
    if (port <= 0) port = 5025;

    bool created = m_osxLoader.CreateDriver(ip.c_str(), port);
    {
        CString msg; msg.Format(_T("[OSX] Creating driver (TCP %s:%d)..."), (LPCTSTR)ipStr, port);
        AppendLog(msg);
    }

    if (!created)
    {
        AppendLog(_T("[OSX] Failed to create driver."));
        return;
    }

    CSantecOSXTcpLoader* pOsx = &m_osxLoader;
    bool* pConn = &m_bOsxConnected;

    RunAsync(_T("Connecting OSX..."), [pOsx, pConn]() -> WorkerResult*
    {
        WorkerResult* r = new WorkerResult();
        CString log;

        BOOL ok = pOsx->Connect();
        if (ok)
        {
            *pConn = true;
            log += _T("[OSX] Connected.\r\n");

            OSXDeviceInfo info = {};
            if (pOsx->GetDeviceInfo(&info))
            {
                CString infoMsg;
                infoMsg.Format(_T("[OSX] %S / %S  SN: %S  FW: %S"),
                               info.manufacturer, info.model,
                               info.serialNumber, info.firmwareVersion);
                log += infoMsg;
            }

            int modCount = pOsx->GetModuleCount();
            CString modMsg;
            modMsg.Format(_T("\r\n[OSX] Module count: %d"), modCount);
            log += modMsg;

            for (int m = 0; m < modCount; ++m)
            {
                OSXModuleInfo mi = {};
                if (pOsx->GetModuleInfo(m, &mi))
                {
                    CString mInfo;
                    mInfo.Format(_T("\r\n[OSX] Module %d: %S  channels=%d  current=%d"),
                                 m, mi.catalogEntry, mi.channelCount, mi.currentChannel);
                    log += mInfo;
                }
            }
        }
        else
        {
            log += _T("[OSX] Connection FAILED.");
        }

        r->success = *pConn;
        r->logMessage = log;
        r->statusText = *pConn ? _T("OSX Connected") : _T("OSX Connection Failed");
        return r;
    });
}

void CSantecAppDlg::OnBnClickedDisconnectOsx()
{
    CSantecOSXTcpLoader* pOsx = &m_osxLoader;
    bool* pConn = &m_bOsxConnected;

    RunAsync(_T("Disconnecting OSX..."), [pOsx, pConn]() -> WorkerResult*
    {
        WorkerResult* r = new WorkerResult();
        if (pOsx->GetDriverHandle())
        {
            pOsx->Disconnect();
            pOsx->DestroyDriver();
        }
        *pConn = false;
        r->success = true;
        r->logMessage = _T("[OSX] Disconnected.");
        r->statusText = _T("OSX Disconnected");
        return r;
    });
}

// ---------------------------------------------------------------------------
// 清零（参考测量）
// ---------------------------------------------------------------------------

void CSantecAppDlg::OnBnClickedZeroing()
{
    if (!m_rlmLoader.GetDriverHandle() || !m_bRlmConnected) return;

    std::vector<double> wavelengths = GetSelectedWavelengths();
    std::vector<int> channels = GetSelectedChannels();
    int moduleIndex = GetModuleIndex();
    bool useOsx = m_bOsxConnected && (channels.size() > 1);

    BOOL bOverride = (m_checkOverride.GetCheck() == BST_CHECKED);
    CString ilStr, lenStr;
    m_editILValue.GetWindowText(ilStr);
    m_editLengthValue.GetWindowText(lenStr);
    double ilValue = _ttof(ilStr);
    double lengthValue = _ttof(lenStr);

    AppendLog(_T("=== Zeroing (Reference) Started ==="));

    CSantecRLMTcpLoader* pRlm = &m_rlmLoader;
    CSantecOSXTcpLoader* pOsx = &m_osxLoader;
    std::atomic<bool>* pStop = &m_bStopRequested;
    m_bStopRequested = false;

    RunAsync(_T("Zeroing..."),
        [pRlm, pOsx, wavelengths, channels, moduleIndex, useOsx,
         bOverride, ilValue, lengthValue, pStop]() -> WorkerResult*
    {
        WorkerResult* r = new WorkerResult();
        CString log;

        try
        {
            std::vector<double> wl = wavelengths;
            pRlm->ConfigureWavelengths(wl.data(), (int)wl.size());

            if (!useOsx)
            {
                std::vector<int> ch = channels;
                pRlm->ConfigureChannels(ch.data(), (int)ch.size());
                BOOL ok = pRlm->TakeReference(bOverride, ilValue, lengthValue);
                log.Format(_T("Reference %s (single channel, no OSX switching)."),
                           ok ? _T("completed") : _T("FAILED"));
                r->success = (ok != FALSE);
            }
            else
            {
                pOsx->SelectModule(moduleIndex);

                bool allOk = true;
                for (size_t i = 0; i < channels.size(); ++i)
                {
                    if (pStop->load()) { log += _T("\r\nStopped by user."); break; }

                    int ch = channels[i];

                    CString chLog;
                    chLog.Format(_T("  CH%d: OSX module %d -> channel %d ..."),
                                 ch, moduleIndex, ch);
                    log += chLog;

                    pOsx->SwitchChannel(ch);
                    pOsx->WaitForOperation(5000);

                    std::vector<int> singleCh = { ch };
                    pRlm->ConfigureChannels(singleCh.data(), 1);

                    BOOL ok = pRlm->TakeReference(bOverride, ilValue, lengthValue);
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

void CSantecAppDlg::OnBnClickedMeasure()
{
    if (!m_rlmLoader.GetDriverHandle() || !m_bRlmConnected) return;

    std::vector<double> wavelengths = GetSelectedWavelengths();
    std::vector<int> channels = GetSelectedChannels();
    int moduleIndex = GetModuleIndex();
    bool useOsx = m_bOsxConnected && (channels.size() > 1);

    AppendLog(_T("=== Measurement Started ==="));

    CSantecRLMTcpLoader* pRlm = &m_rlmLoader;
    CSantecOSXTcpLoader* pOsx = &m_osxLoader;
    std::atomic<bool>* pStop = &m_bStopRequested;
    m_bStopRequested = false;

    RunAsync(_T("Measuring..."),
        [pRlm, pOsx, wavelengths, channels, moduleIndex, useOsx, pStop]() -> WorkerResult*
    {
        WorkerResult* r = new WorkerResult();
        CString log;

        try
        {
            std::vector<double> wl = wavelengths;
            pRlm->ConfigureWavelengths(wl.data(), (int)wl.size());

            std::vector<RLMMeasurementResult> allResults;

            if (!useOsx)
            {
                std::vector<int> ch = channels;
                pRlm->ConfigureChannels(ch.data(), (int)ch.size());

                BOOL ok = pRlm->TakeMeasurement();
                if (ok)
                {
                    RLMMeasurementResult buf[256];
                    int count = pRlm->GetResults(buf, 256);
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
                pOsx->SelectModule(moduleIndex);

                bool allOk = true;
                for (size_t i = 0; i < channels.size(); ++i)
                {
                    if (pStop->load()) { log += _T("\r\nStopped by user."); break; }

                    int ch = channels[i];
                    CString chLog;
                    chLog.Format(_T("  CH%d: OSX switching ..."), ch);
                    log += chLog;

                    pOsx->SwitchChannel(ch);
                    pOsx->WaitForOperation(5000);

                    std::vector<int> singleCh = { ch };
                    pRlm->ConfigureChannels(singleCh.data(), 1);

                    BOOL ok = pRlm->TakeMeasurement();
                    if (ok)
                    {
                        RLMMeasurementResult buf[256];
                        int count = pRlm->GetResults(buf, 256);
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

void CSantecAppDlg::OnBnClickedContinuous()
{
    if (!m_rlmLoader.GetDriverHandle() || !m_bRlmConnected) return;

    std::vector<double> wavelengths = GetSelectedWavelengths();
    std::vector<int> channels = GetSelectedChannels();
    int moduleIndex = GetModuleIndex();
    bool useOsx = m_bOsxConnected && (channels.size() > 1);

    BOOL bOverride = (m_checkOverride.GetCheck() == BST_CHECKED);
    CString ilStr, lenStr;
    m_editILValue.GetWindowText(ilStr);
    m_editLengthValue.GetWindowText(lenStr);
    double ilValue = _ttof(ilStr);
    double lengthValue = _ttof(lenStr);

    AppendLog(_T("=== Continuous Test Started (press STOP to end) ==="));
    m_bStopRequested = false;

    CSantecRLMTcpLoader* pRlm = &m_rlmLoader;
    CSantecOSXTcpLoader* pOsx = &m_osxLoader;
    std::atomic<bool>* pStop = &m_bStopRequested;

    RunAsync(_T("Continuous Test..."),
        [pRlm, pOsx, wavelengths, channels, moduleIndex, useOsx,
         bOverride, ilValue, lengthValue, pStop]() -> WorkerResult*
    {
        WorkerResult* r = new WorkerResult();
        CString log;
        int iteration = 0;
        std::vector<RLMMeasurementResult> allResults;

        try
        {
            std::vector<double> wl = wavelengths;
            pRlm->ConfigureWavelengths(wl.data(), (int)wl.size());

            if (useOsx)
                pOsx->SelectModule(moduleIndex);

            while (!pStop->load())
            {
                ++iteration;
                CString iterLog;
                iterLog.Format(_T("--- Iteration %d ---\r\n"), iteration);
                log += iterLog;

                // Phase 1: 清零
                if (!useOsx)
                {
                    std::vector<int> ch = channels;
                    pRlm->ConfigureChannels(ch.data(), (int)ch.size());
                    pRlm->TakeReference(bOverride, ilValue, lengthValue);
                }
                else
                {
                    for (size_t i = 0; i < channels.size() && !pStop->load(); ++i)
                    {
                        int ch = channels[i];
                        pOsx->SwitchChannel(ch);
                        pOsx->WaitForOperation(5000);
                        std::vector<int> singleCh = { ch };
                        pRlm->ConfigureChannels(singleCh.data(), 1);
                        pRlm->TakeReference(bOverride, ilValue, lengthValue);
                    }
                }

                if (pStop->load()) break;
                log += _T("  Zeroing done.\r\n");

                // Phase 2: 测量
                allResults.clear();
                if (!useOsx)
                {
                    std::vector<int> ch = channels;
                    pRlm->ConfigureChannels(ch.data(), (int)ch.size());
                    if (pRlm->TakeMeasurement())
                    {
                        RLMMeasurementResult buf[256];
                        int count = pRlm->GetResults(buf, 256);
                        allResults.assign(buf, buf + count);
                    }
                }
                else
                {
                    for (size_t i = 0; i < channels.size() && !pStop->load(); ++i)
                    {
                        int ch = channels[i];
                        pOsx->SwitchChannel(ch);
                        pOsx->WaitForOperation(5000);
                        std::vector<int> singleCh = { ch };
                        pRlm->ConfigureChannels(singleCh.data(), 1);
                        if (pRlm->TakeMeasurement())
                        {
                            RLMMeasurementResult buf[256];
                            int count = pRlm->GetResults(buf, 256);
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

void CSantecAppDlg::OnBnClickedStop()
{
    m_bStopRequested = true;
    AppendLog(_T("Stop requested... waiting for current operation to finish."));
}

// ---------------------------------------------------------------------------
// Override 复选框
// ---------------------------------------------------------------------------

void CSantecAppDlg::OnBnClickedOverride()
{
    BOOL enabled = (m_checkOverride.GetCheck() == BST_CHECKED);
    m_editILValue.EnableWindow(enabled);
    m_editLengthValue.EnableWindow(enabled);
}

// ---------------------------------------------------------------------------
// 异步工作线程基础设施
// ---------------------------------------------------------------------------

void CSantecAppDlg::RunAsync(const CString& operationName,
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

LRESULT CSantecAppDlg::OnWorkerDone(WPARAM, LPARAM lParam)
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

void CSantecAppDlg::SetBusy(bool busy, const CString& statusText)
{
    m_bBusy = busy;
    EnableControls();
}

void CSantecAppDlg::EnableControls()
{
    bool rlmLoaded = m_rlmLoader.IsDllLoaded();
    bool osxLoaded = m_osxLoader.IsDllLoaded();
    bool rlmOnly = m_bRlmConnected;

    // RLM connection group
    GetDlgItem(IDC_EDIT_RLM_DLL)->EnableWindow(!m_bBusy && !rlmLoaded);
    SetDlgItemText(IDC_BTN_LOAD_RLM, rlmLoaded ? _T("Unload") : _T("Load"));
    GetDlgItem(IDC_BTN_LOAD_RLM)->EnableWindow(!m_bBusy);

    GetDlgItem(IDC_EDIT_RLM_IP)->EnableWindow(!m_bBusy && rlmLoaded && !m_bRlmConnected);
    GetDlgItem(IDC_EDIT_RLM_PORT)->EnableWindow(!m_bBusy && rlmLoaded && !m_bRlmConnected);
    GetDlgItem(IDC_BTN_CONNECT_RLM)->EnableWindow(!m_bBusy && rlmLoaded && !m_bRlmConnected);
    GetDlgItem(IDC_BTN_DISCONNECT_RLM)->EnableWindow(!m_bBusy && m_bRlmConnected);

    // OSX connection group
    GetDlgItem(IDC_EDIT_OSX_DLL)->EnableWindow(!m_bBusy && !osxLoaded);
    SetDlgItemText(IDC_BTN_LOAD_OSX, osxLoaded ? _T("Unload") : _T("Load"));
    GetDlgItem(IDC_BTN_LOAD_OSX)->EnableWindow(!m_bBusy);

    GetDlgItem(IDC_EDIT_OSX_IP)->EnableWindow(!m_bBusy && osxLoaded && !m_bOsxConnected);
    GetDlgItem(IDC_EDIT_OSX_PORT)->EnableWindow(!m_bBusy && osxLoaded && !m_bOsxConnected);
    GetDlgItem(IDC_BTN_CONNECT_OSX)->EnableWindow(!m_bBusy && osxLoaded && !m_bOsxConnected);
    GetDlgItem(IDC_BTN_DISCONNECT_OSX)->EnableWindow(!m_bBusy && m_bOsxConnected);

    // Test configuration
    GetDlgItem(IDC_CHECK_1310)->EnableWindow(!m_bBusy);
    GetDlgItem(IDC_CHECK_1550)->EnableWindow(!m_bBusy);
    GetDlgItem(IDC_EDIT_CH_FROM)->EnableWindow(!m_bBusy);
    GetDlgItem(IDC_EDIT_CH_TO)->EnableWindow(!m_bBusy);
    GetDlgItem(IDC_EDIT_MODULE_INDEX)->EnableWindow(!m_bBusy);
    GetDlgItem(IDC_CHECK_OVERRIDE)->EnableWindow(!m_bBusy);

    BOOL overrideOn = (m_checkOverride.GetCheck() == BST_CHECKED);
    GetDlgItem(IDC_EDIT_IL_VALUE)->EnableWindow(!m_bBusy && overrideOn);
    GetDlgItem(IDC_EDIT_LENGTH_VALUE)->EnableWindow(!m_bBusy && overrideOn);

    // Operations: require at least RLM connected
    GetDlgItem(IDC_BTN_ZEROING)->EnableWindow(!m_bBusy && rlmOnly);
    GetDlgItem(IDC_BTN_MEASURE)->EnableWindow(!m_bBusy && rlmOnly);
    GetDlgItem(IDC_BTN_CONTINUOUS)->EnableWindow(!m_bBusy && rlmOnly);
    GetDlgItem(IDC_BTN_STOP)->EnableWindow(m_bBusy);

    // Update status labels
    UpdateRlmStatus(m_bRlmConnected ? _T("Connected") : _T("Not Connected"));
    UpdateOsxStatus(m_bOsxConnected ? _T("Connected") : _T("Not Connected"));
}

// ---------------------------------------------------------------------------
// 日志 / 状态
// ---------------------------------------------------------------------------

LRESULT CSantecAppDlg::OnLogMessage(WPARAM, LPARAM lParam)
{
    CString* pMsg = reinterpret_cast<CString*>(lParam);
    if (pMsg) { AppendLog(*pMsg); delete pMsg; }
    return 0;
}

void CSantecAppDlg::AppendLog(const CString& text)
{
    CString current;
    m_editLog.GetWindowText(current);
    if (!current.IsEmpty()) current += _T("\r\n");
    current += text;
    m_editLog.SetWindowText(current);
    m_editLog.LineScroll(m_editLog.GetLineCount());
}

void CSantecAppDlg::UpdateRlmStatus(const CString& status)
{
    SetDlgItemText(IDC_STATIC_RLM_STATUS, status);
}

void CSantecAppDlg::UpdateOsxStatus(const CString& status)
{
    SetDlgItemText(IDC_STATIC_OSX_STATUS, status);
}

void CSantecAppDlg::OnBnClickedClearLog()
{
    m_editLog.SetWindowText(_T(""));
}

// ---------------------------------------------------------------------------
// 结果列表
// ---------------------------------------------------------------------------

void CSantecAppDlg::PopulateResultsList(const std::vector<RLMMeasurementResult>& results)
{
    m_listResults.DeleteAllItems();

    for (size_t i = 0; i < results.size(); ++i)
    {
        const RLMMeasurementResult& res = results[i];
        CString chStr, wlStr, ilStr, rlStr, rlaStr, rlbStr, rltStr, lenStr;
        chStr.Format(_T("%d"), res.channel);
        wlStr.Format(_T("%.0f"), res.wavelength);
        ilStr.Format(_T("%.4f"), res.insertionLoss);
        rlStr.Format(_T("%.2f"), res.returnLoss);
        rlaStr.Format(_T("%.2f"), res.returnLossA);
        rlbStr.Format(_T("%.2f"), res.returnLossB);
        rltStr.Format(_T("%.2f"), res.returnLossTotal);
        lenStr.Format(_T("%.2f"), res.dutLength);

        int idx = m_listResults.InsertItem(static_cast<int>(i), chStr);
        m_listResults.SetItemText(idx, 1, wlStr);
        m_listResults.SetItemText(idx, 2, ilStr);
        m_listResults.SetItemText(idx, 3, rlStr);
        m_listResults.SetItemText(idx, 4, rlaStr);
        m_listResults.SetItemText(idx, 5, rlbStr);
        m_listResults.SetItemText(idx, 6, rltStr);
        m_listResults.SetItemText(idx, 7, lenStr);
    }
}

// ---------------------------------------------------------------------------
// 辅助函数
// ---------------------------------------------------------------------------

std::vector<double> CSantecAppDlg::GetSelectedWavelengths()
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

std::vector<int> CSantecAppDlg::GetSelectedChannels()
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

int CSantecAppDlg::GetModuleIndex()
{
    CString str;
    m_editModuleIndex.GetWindowText(str);
    int idx = _ttoi(str);
    return (idx >= 0) ? idx : 0;
}
