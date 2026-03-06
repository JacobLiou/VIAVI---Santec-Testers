#include "stdafx.h"
#include "GMSApp.h"
#include "GMSAppDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// ---------------------------------------------------------------------------
// 全局日志回调 -- 将驱动日志消息通过 PostMessage 转发到对话框
// ---------------------------------------------------------------------------

static CGMSAppDlg* g_pDlg = nullptr;

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

// ---------------------------------------------------------------------------
// 消息映射
// ---------------------------------------------------------------------------

BEGIN_MESSAGE_MAP(CGMSAppDlg, CDialogEx)
    ON_WM_CLOSE()
    ON_BN_CLICKED(IDC_BTN_LOAD_RLM, &CGMSAppDlg::OnBnClickedLoadRlm)
    ON_BN_CLICKED(IDC_BTN_ENUMERATE, &CGMSAppDlg::OnBnClickedEnumerate)
    ON_BN_CLICKED(IDC_BTN_CONNECT, &CGMSAppDlg::OnBnClickedConnect)
    ON_BN_CLICKED(IDC_BTN_DISCONNECT, &CGMSAppDlg::OnBnClickedDisconnect)
    ON_BN_CLICKED(IDC_BTN_ZEROING, &CGMSAppDlg::OnBnClickedZeroing)
    ON_BN_CLICKED(IDC_BTN_MEASURE, &CGMSAppDlg::OnBnClickedMeasure)
    ON_BN_CLICKED(IDC_BTN_CONTINUOUS, &CGMSAppDlg::OnBnClickedContinuous)
    ON_BN_CLICKED(IDC_BTN_STOP, &CGMSAppDlg::OnBnClickedStop)
    ON_BN_CLICKED(IDC_BTN_CLEAR_LOG, &CGMSAppDlg::OnBnClickedClearLog)
    ON_BN_CLICKED(IDC_CHECK_OVERRIDE, &CGMSAppDlg::OnBnClickedOverride)
    ON_CBN_SELCHANGE(IDC_COMBO_CONN_MODE, &CGMSAppDlg::OnCbnSelchangeConnMode)
    ON_MESSAGE(WM_LOG_MESSAGE, &CGMSAppDlg::OnLogMessage)
    ON_MESSAGE(WM_WORKER_DONE, &CGMSAppDlg::OnWorkerDone)
END_MESSAGE_MAP()

// ---------------------------------------------------------------------------
// 构造 / 析构
// ---------------------------------------------------------------------------

CGMSAppDlg::CGMSAppDlg(CWnd* pParent)
    : CDialogEx(IDD_GMSAPP_DIALOG, pParent)
    , m_bBusy(false)
    , m_bStopRequested(false)
    , m_bRlmConnected(false)
{
}

CGMSAppDlg::~CGMSAppDlg()
{
    g_pDlg = nullptr;
}

void CGMSAppDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT_RLM_DLL, m_editRlmDll);
    DDX_Control(pDX, IDC_COMBO_CONN_MODE, m_comboConnMode);
    DDX_Control(pDX, IDC_COMBO_RLM_ADDR, m_comboRlmAddr);
    DDX_Control(pDX, IDC_EDIT_RLM_PORT, m_editRlmPort);
    DDX_Control(pDX, IDC_CHECK_1310, m_check1310);
    DDX_Control(pDX, IDC_CHECK_1550, m_check1550);
    DDX_Control(pDX, IDC_EDIT_CH_FROM, m_editChFrom);
    DDX_Control(pDX, IDC_EDIT_CH_TO, m_editChTo);
    DDX_Control(pDX, IDC_COMBO_SWITCH_NUM, m_comboSwitchNum);
    DDX_Control(pDX, IDC_CHECK_OVERRIDE, m_checkOverride);
    DDX_Control(pDX, IDC_EDIT_IL_VALUE, m_editILValue);
    DDX_Control(pDX, IDC_EDIT_LENGTH_VALUE, m_editLengthValue);
    DDX_Control(pDX, IDC_LIST_RESULTS, m_listResults);
    DDX_Control(pDX, IDC_EDIT_LOG, m_editLog);
}

// ---------------------------------------------------------------------------
// 初始化
// ---------------------------------------------------------------------------

BOOL CGMSAppDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    g_pDlg = this;

    m_editRlmDll.SetWindowText(_T("UDL.SantecRLM.dll"));

    m_comboConnMode.AddString(_T("USB (VISA)"));
    m_comboConnMode.AddString(_T("TCP (Ethernet)"));
    m_comboConnMode.SetCurSel(0);

    m_editRlmPort.SetWindowText(_T("5025"));

    m_check1310.SetCheck(BST_CHECKED);
    m_check1550.SetCheck(BST_CHECKED);

    m_editChFrom.SetWindowText(_T("1"));
    m_editChTo.SetWindowText(_T("4"));

    // SW1 = 第一个外部开关, SW2 = 第二个外部开关
    m_comboSwitchNum.AddString(_T("SW1"));
    m_comboSwitchNum.AddString(_T("SW2"));
    m_comboSwitchNum.SetCurSel(0);

    m_checkOverride.SetCheck(BST_CHECKED);
    m_editILValue.SetWindowText(_T("0.1"));
    m_editLengthValue.SetWindowText(_T("3.0"));

    m_listResults.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_listResults.InsertColumn(0, _T("CH"), LVCFMT_CENTER, 40);
    m_listResults.InsertColumn(1, _T("WL (nm)"), LVCFMT_CENTER, 65);
    m_listResults.InsertColumn(2, _T("IL (dB)"), LVCFMT_RIGHT, 75);
    m_listResults.InsertColumn(3, _T("RL (dB)"), LVCFMT_RIGHT, 75);
    m_listResults.InsertColumn(4, _T("RLA (dB)"), LVCFMT_RIGHT, 75);
    m_listResults.InsertColumn(5, _T("RLB (dB)"), LVCFMT_RIGHT, 75);
    m_listResults.InsertColumn(6, _T("RL Total (dB)"), LVCFMT_RIGHT, 85);
    m_listResults.InsertColumn(7, _T("Length (m)"), LVCFMT_RIGHT, 75);

    EnableControls();
    AppendLog(_T("GMS Application started (Integrated Mode)."));
    AppendLog(_T("In integrated mode, PC connects to RLM only. RLM controls OSX switch via USB A port."));
    AppendLog(_T("Load DLL, select connection mode (USB VISA / TCP), then connect."));

    return TRUE;
}

void CGMSAppDlg::OnOK() {}

void CGMSAppDlg::OnCancel()
{
    if (m_bBusy)
    {
        MessageBox(_T("An operation is in progress. Please wait."),
                   _T("Busy"), MB_OK | MB_ICONINFORMATION);
        return;
    }
    CDialogEx::OnCancel();
}

void CGMSAppDlg::OnClose()
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
// DLL 加载
// ---------------------------------------------------------------------------

void CGMSAppDlg::OnBnClickedLoadRlm()
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
// VISA 枚举
// ---------------------------------------------------------------------------

void CGMSAppDlg::OnBnClickedEnumerate()
{
    if (!IsVisaMode())
    {
        AppendLog(_T("VISA enumeration is only available in USB (VISA) mode."));
        return;
    }

    m_comboRlmAddr.ResetContent();

    char buf[4096] = { 0 };
    int found = 0;

    if (m_rlmLoader.IsDllLoaded() && m_rlmLoader.HasVisaSupport())
    {
        found = m_rlmLoader.EnumerateVisaResources(buf, sizeof(buf));
    }

    if (found > 0)
    {
        CString all(buf);
        int pos = 0;
        CString token = all.Tokenize(_T(";"), pos);
        while (!token.IsEmpty())
        {
            token.Trim();
            if (!token.IsEmpty())
                m_comboRlmAddr.AddString(token);
            token = all.Tokenize(_T(";"), pos);
        }

        CString msg; msg.Format(_T("Found %d VISA resource(s). Select the RLM device."), found);
        AppendLog(msg);

        if (m_comboRlmAddr.GetCount() > 0)
            m_comboRlmAddr.SetCurSel(0);
    }
    else
    {
        AppendLog(_T("No VISA resources found. Make sure R&S VISA is installed and the RLM is connected via USB."));
    }
}

// ---------------------------------------------------------------------------
// 连接 / 断开
// ---------------------------------------------------------------------------

void CGMSAppDlg::OnBnClickedConnect()
{
    CString rlmAddr;
    m_comboRlmAddr.GetWindowText(rlmAddr);

    if (rlmAddr.IsEmpty())
    {
        AppendLog(_T("Please specify RLM address."));
        return;
    }

    if (m_rlmLoader.GetDriverHandle())
    {
        m_rlmLoader.Disconnect();
        m_rlmLoader.DestroyDriver();
    }
    m_bRlmConnected = false;

    CStringA rlmAddrA(rlmAddr);
    std::string rlmStr(rlmAddrA.GetString());

    bool useVisa = IsVisaMode();

    CString rlmPortStr;
    m_editRlmPort.GetWindowText(rlmPortStr);
    int rlmPort = _ttoi(rlmPortStr);
    if (rlmPort <= 0) rlmPort = 5025;

    int switchNum = GetSelectedSwitchNum();

    CSantecRLMDllLoader* pRlm = &m_rlmLoader;

    bool rlmCreated = false;
    if (useVisa)
    {
        rlmCreated = pRlm->CreateDriverEx("santec", rlmStr.c_str(), 0, 0, 2);
        AppendLog(_T("[RLM] Creating driver (USB VISA)..."));
    }
    else
    {
        rlmCreated = pRlm->CreateDriver("santec", rlmStr.c_str(), rlmPort, 0);
        CString msg; msg.Format(_T("[RLM] Creating driver (TCP %s:%d)..."), (LPCTSTR)rlmAddr, rlmPort);
        AppendLog(msg);
    }

    if (!rlmCreated)
    {
        AppendLog(_T("[RLM] Failed to create driver."));
        return;
    }

    AppendLog(_T("Connecting..."));

    bool* pRlmConn = &m_bRlmConnected;

    RunAsync(_T("Connecting..."), [pRlm, pRlmConn, switchNum]() -> WorkerResult*
    {
        WorkerResult* r = new WorkerResult();
        CString log;

        BOOL rlmOk = pRlm->Connect();
        if (rlmOk)
        {
            *pRlmConn = true;
            pRlm->Initialize();
            log += _T("[RLM] Connected and initialized.\r\n");

            // 查询外部开关信息以验证 OSX 是否连接到 RLM 的 USB A 端口
            char swInfo[256] = { 0 };
            if (pRlm->GetSwitchInfo(switchNum, swInfo, sizeof(swInfo)))
            {
                CString infoMsg;
                infoMsg.Format(_T("[RLM] External switch SW%d info: %S\r\n"), switchNum, swInfo);
                log += infoMsg;
            }
            else
            {
                CString warnMsg;
                warnMsg.Format(_T("[RLM] Warning: Could not query SW%d info. External switch may not be connected.\r\n"), switchNum);
                log += warnMsg;
            }
        }
        else
        {
            log += _T("[RLM] Connection FAILED.\r\n");
        }

        r->success = *pRlmConn;
        r->logMessage = log;
        r->statusText = *pRlmConn ? _T("RLM Connected (Integrated Mode)") : _T("Connection Failed");

        return r;
    });
}

void CGMSAppDlg::OnBnClickedDisconnect()
{
    CSantecRLMDllLoader* pRlm = &m_rlmLoader;
    bool* pRlmConn = &m_bRlmConnected;

    RunAsync(_T("Disconnecting..."), [pRlm, pRlmConn]() -> WorkerResult*
    {
        WorkerResult* r = new WorkerResult();
        if (pRlm->GetDriverHandle())
        {
            pRlm->Disconnect();
            pRlm->DestroyDriver();
        }
        *pRlmConn = false;
        r->success = true;
        r->logMessage = _T("Disconnected.");
        r->statusText = _T("Disconnected");
        return r;
    });
}

// ---------------------------------------------------------------------------
// 清零（参考测量）
// ---------------------------------------------------------------------------

void CGMSAppDlg::OnBnClickedZeroing()
{
    if (!m_rlmLoader.GetDriverHandle() || !m_bRlmConnected) return;

    std::vector<double> wavelengths = GetSelectedWavelengths();
    std::vector<int> channels = GetSelectedChannels();
    int switchNum = GetSelectedSwitchNum();
    bool useSwitch = (channels.size() > 1);

    BOOL bOverride = (m_checkOverride.GetCheck() == BST_CHECKED);
    CString ilStr, lenStr;
    m_editILValue.GetWindowText(ilStr);
    m_editLengthValue.GetWindowText(lenStr);
    double ilValue = _ttof(ilStr);
    double lengthValue = _ttof(lenStr);

    AppendLog(_T("=== Zeroing (Reference) Started ==="));

    CSantecRLMDllLoader* pRlm = &m_rlmLoader;
    std::atomic<bool>* pStop = &m_bStopRequested;
    m_bStopRequested = false;

    RunAsync(_T("Zeroing..."),
        [pRlm, wavelengths, channels, switchNum, useSwitch,
         bOverride, ilValue, lengthValue, pStop]() -> WorkerResult*
    {
        WorkerResult* r = new WorkerResult();
        CString log;

        try
        {
            std::vector<double> wl = wavelengths;
            pRlm->ConfigureWavelengths(wl.data(), (int)wl.size());

            if (!useSwitch)
            {
                std::vector<int> ch = channels;
                pRlm->ConfigureChannels(ch.data(), (int)ch.size());
                BOOL ok = pRlm->TakeReference(bOverride, ilValue, lengthValue);
                log.Format(_T("Reference %s (single channel, no switch needed)."),
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
                    chLog.Format(_T("  CH%d: Switching SW%d -> channel %d ..."), ch, switchNum, ch);
                    log += chLog;

                    pRlm->SetSwitchChannel(switchNum, ch);
                    Sleep(500);

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

void CGMSAppDlg::OnBnClickedMeasure()
{
    if (!m_rlmLoader.GetDriverHandle() || !m_bRlmConnected) return;

    std::vector<double> wavelengths = GetSelectedWavelengths();
    std::vector<int> channels = GetSelectedChannels();
    int switchNum = GetSelectedSwitchNum();
    bool useSwitch = (channels.size() > 1);

    AppendLog(_T("=== Measurement Started ==="));

    CSantecRLMDllLoader* pRlm = &m_rlmLoader;
    std::atomic<bool>* pStop = &m_bStopRequested;
    m_bStopRequested = false;

    RunAsync(_T("Measuring..."),
        [pRlm, wavelengths, channels, switchNum, useSwitch, pStop]() -> WorkerResult*
    {
        WorkerResult* r = new WorkerResult();
        CString log;

        try
        {
            std::vector<double> wl = wavelengths;
            pRlm->ConfigureWavelengths(wl.data(), (int)wl.size());

            std::vector<DriverMeasurementResult> allResults;

            if (!useSwitch)
            {
                std::vector<int> ch = channels;
                pRlm->ConfigureChannels(ch.data(), (int)ch.size());

                BOOL ok = pRlm->TakeMeasurement();
                if (ok)
                {
                    DriverMeasurementResult buf[256];
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
                bool allOk = true;
                for (size_t i = 0; i < channels.size(); ++i)
                {
                    if (pStop->load()) { log += _T("\r\nStopped by user."); break; }

                    int ch = channels[i];
                    CString chLog;
                    chLog.Format(_T("  CH%d: Switching SW%d ..."), ch, switchNum);
                    log += chLog;

                    pRlm->SetSwitchChannel(switchNum, ch);
                    Sleep(500);

                    std::vector<int> singleCh = { ch };
                    pRlm->ConfigureChannels(singleCh.data(), 1);

                    BOOL ok = pRlm->TakeMeasurement();
                    if (ok)
                    {
                        DriverMeasurementResult buf[256];
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
            summary.Format(_T("=== Measurement Complete: %d total result(s) ==="), (int)allResults.size());
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

void CGMSAppDlg::OnBnClickedContinuous()
{
    if (!m_rlmLoader.GetDriverHandle() || !m_bRlmConnected) return;

    std::vector<double> wavelengths = GetSelectedWavelengths();
    std::vector<int> channels = GetSelectedChannels();
    int switchNum = GetSelectedSwitchNum();
    bool useSwitch = (channels.size() > 1);

    BOOL bOverride = (m_checkOverride.GetCheck() == BST_CHECKED);
    CString ilStr, lenStr;
    m_editILValue.GetWindowText(ilStr);
    m_editLengthValue.GetWindowText(lenStr);
    double ilValue = _ttof(ilStr);
    double lengthValue = _ttof(lenStr);

    AppendLog(_T("=== Continuous Test Started (press STOP to end) ==="));
    m_bStopRequested = false;

    CSantecRLMDllLoader* pRlm = &m_rlmLoader;
    std::atomic<bool>* pStop = &m_bStopRequested;

    RunAsync(_T("Continuous Test..."),
        [pRlm, wavelengths, channels, switchNum, useSwitch,
         bOverride, ilValue, lengthValue, pStop]() -> WorkerResult*
    {
        WorkerResult* r = new WorkerResult();
        CString log;
        int iteration = 0;
        std::vector<DriverMeasurementResult> allResults;

        try
        {
            std::vector<double> wl = wavelengths;
            pRlm->ConfigureWavelengths(wl.data(), (int)wl.size());

            while (!pStop->load())
            {
                ++iteration;
                CString iterLog;
                iterLog.Format(_T("--- Iteration %d ---\r\n"), iteration);
                log += iterLog;

                // Phase 1: 清零
                if (!useSwitch)
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
                        pRlm->SetSwitchChannel(switchNum, ch);
                        Sleep(500);
                        std::vector<int> singleCh = { ch };
                        pRlm->ConfigureChannels(singleCh.data(), 1);
                        pRlm->TakeReference(bOverride, ilValue, lengthValue);
                    }
                }

                if (pStop->load()) break;
                log += _T("  Zeroing done.\r\n");

                // Phase 2: 测量
                allResults.clear();
                if (!useSwitch)
                {
                    std::vector<int> ch = channels;
                    pRlm->ConfigureChannels(ch.data(), (int)ch.size());
                    if (pRlm->TakeMeasurement())
                    {
                        DriverMeasurementResult buf[256];
                        int count = pRlm->GetResults(buf, 256);
                        allResults.assign(buf, buf + count);
                    }
                }
                else
                {
                    for (size_t i = 0; i < channels.size() && !pStop->load(); ++i)
                    {
                        int ch = channels[i];
                        pRlm->SetSwitchChannel(switchNum, ch);
                        Sleep(500);
                        std::vector<int> singleCh = { ch };
                        pRlm->ConfigureChannels(singleCh.data(), 1);
                        if (pRlm->TakeMeasurement())
                        {
                            DriverMeasurementResult buf[256];
                            int count = pRlm->GetResults(buf, 256);
                            for (int j = 0; j < count; ++j)
                                allResults.push_back(buf[j]);
                        }
                    }
                }

                CString measLog;
                measLog.Format(_T("  Measurement done: %d result(s).\r\n"), (int)allResults.size());
                log += measLog;
            }

            r->results = allResults;
            r->hasResults = true;
            r->success = true;

            CString summary;
            summary.Format(_T("=== Continuous Test Stopped after %d iteration(s) ==="), iteration);
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

void CGMSAppDlg::OnBnClickedStop()
{
    m_bStopRequested = true;
    AppendLog(_T("Stop requested..."));
}

// ---------------------------------------------------------------------------
// Override 复选框
// ---------------------------------------------------------------------------

void CGMSAppDlg::OnBnClickedOverride()
{
    BOOL enabled = (m_checkOverride.GetCheck() == BST_CHECKED);
    m_editILValue.EnableWindow(enabled);
    m_editLengthValue.EnableWindow(enabled);
}

// ---------------------------------------------------------------------------
// 异步工作线程基础设施
// ---------------------------------------------------------------------------

void CGMSAppDlg::RunAsync(const CString& operationName,
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

LRESULT CGMSAppDlg::OnWorkerDone(WPARAM, LPARAM lParam)
{
    WorkerResult* result = reinterpret_cast<WorkerResult*>(lParam);
    if (result)
    {
        if (!result->logMessage.IsEmpty())
            AppendLog(result->logMessage);
        if (result->hasResults)
            PopulateResultsList(result->results);
        if (!result->statusText.IsEmpty())
            UpdateStatus(result->statusText);
        delete result;
    }
    SetBusy(false);
    return 0;
}

// ---------------------------------------------------------------------------
// UI 状态管理
// ---------------------------------------------------------------------------

void CGMSAppDlg::SetBusy(bool busy, const CString& statusText)
{
    m_bBusy = busy;
    if (busy && !statusText.IsEmpty())
        UpdateStatus(statusText);

    EnableControls();
}

void CGMSAppDlg::EnableControls()
{
    bool rlmLoaded = m_rlmLoader.IsDllLoaded();
    bool anyConnected = m_bRlmConnected;
    bool visa = IsVisaMode();

    GetDlgItem(IDC_EDIT_RLM_DLL)->EnableWindow(!m_bBusy && !rlmLoaded);
    SetDlgItemText(IDC_BTN_LOAD_RLM, rlmLoaded ? _T("Unload") : _T("Load"));
    GetDlgItem(IDC_BTN_LOAD_RLM)->EnableWindow(!m_bBusy);

    GetDlgItem(IDC_COMBO_CONN_MODE)->EnableWindow(!m_bBusy && !anyConnected);

    GetDlgItem(IDC_BTN_ENUMERATE)->EnableWindow(!m_bBusy && visa && rlmLoaded);
    GetDlgItem(IDC_COMBO_RLM_ADDR)->EnableWindow(!m_bBusy && rlmLoaded && !anyConnected);
    GetDlgItem(IDC_EDIT_RLM_PORT)->EnableWindow(!m_bBusy && !visa && !anyConnected);

    GetDlgItem(IDC_BTN_CONNECT)->EnableWindow(!m_bBusy && rlmLoaded && !anyConnected);
    GetDlgItem(IDC_BTN_DISCONNECT)->EnableWindow(!m_bBusy && anyConnected);

    GetDlgItem(IDC_CHECK_1310)->EnableWindow(!m_bBusy);
    GetDlgItem(IDC_CHECK_1550)->EnableWindow(!m_bBusy);
    GetDlgItem(IDC_EDIT_CH_FROM)->EnableWindow(!m_bBusy);
    GetDlgItem(IDC_EDIT_CH_TO)->EnableWindow(!m_bBusy);
    GetDlgItem(IDC_COMBO_SWITCH_NUM)->EnableWindow(!m_bBusy);
    GetDlgItem(IDC_CHECK_OVERRIDE)->EnableWindow(!m_bBusy);

    BOOL overrideOn = (m_checkOverride.GetCheck() == BST_CHECKED);
    GetDlgItem(IDC_EDIT_IL_VALUE)->EnableWindow(!m_bBusy && overrideOn);
    GetDlgItem(IDC_EDIT_LENGTH_VALUE)->EnableWindow(!m_bBusy && overrideOn);

    GetDlgItem(IDC_BTN_ZEROING)->EnableWindow(!m_bBusy && anyConnected);
    GetDlgItem(IDC_BTN_MEASURE)->EnableWindow(!m_bBusy && anyConnected);
    GetDlgItem(IDC_BTN_CONTINUOUS)->EnableWindow(!m_bBusy && anyConnected);
    GetDlgItem(IDC_BTN_STOP)->EnableWindow(m_bBusy);
}

// ---------------------------------------------------------------------------
// 日志 / 状态
// ---------------------------------------------------------------------------

LRESULT CGMSAppDlg::OnLogMessage(WPARAM, LPARAM lParam)
{
    CString* pMsg = reinterpret_cast<CString*>(lParam);
    if (pMsg) { AppendLog(*pMsg); delete pMsg; }
    return 0;
}

void CGMSAppDlg::AppendLog(const CString& text)
{
    CString current;
    m_editLog.GetWindowText(current);
    if (!current.IsEmpty()) current += _T("\r\n");
    current += text;
    m_editLog.SetWindowText(current);
    m_editLog.LineScroll(m_editLog.GetLineCount());
}

void CGMSAppDlg::UpdateStatus(const CString& status)
{
    SetDlgItemText(IDC_STATIC_STATUS, status);
}

void CGMSAppDlg::OnBnClickedClearLog()
{
    m_editLog.SetWindowText(_T(""));
}

// ---------------------------------------------------------------------------
// 结果列表
// ---------------------------------------------------------------------------

void CGMSAppDlg::PopulateResultsList(const std::vector<DriverMeasurementResult>& results)
{
    m_listResults.DeleteAllItems();

    for (size_t i = 0; i < results.size(); ++i)
    {
        const DriverMeasurementResult& res = results[i];
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
// 连接模式
// ---------------------------------------------------------------------------

bool CGMSAppDlg::IsVisaMode()
{
    return (m_comboConnMode.GetCurSel() == 0);
}

void CGMSAppDlg::OnCbnSelchangeConnMode()
{
    bool visa = IsVisaMode();
    AppendLog(visa ? _T("Switched to USB (VISA) mode.") : _T("Switched to TCP (Ethernet) mode."));
    EnableControls();
}

// ---------------------------------------------------------------------------
// 辅助函数
// ---------------------------------------------------------------------------

std::vector<double> CGMSAppDlg::GetSelectedWavelengths()
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

std::vector<int> CGMSAppDlg::GetSelectedChannels()
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

int CGMSAppDlg::GetSelectedSwitchNum()
{
    int sel = m_comboSwitchNum.GetCurSel();
    return (sel >= 0) ? (sel + 1) : 1;
}
