#include "stdafx.h"
#include "DriverTestApp3.h"
#include "DriverTestApp3Dlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static CDriverTestApp3Dlg* g_pDlg = nullptr;

static void WINAPI GlobalLogCallback(int level, const char* source, const char* message)
{
    if (g_pDlg && ::IsWindow(g_pDlg->GetSafeHwnd()))
    {
        static const char* levelNames[] = { "DEBUG", "INFO", "WARN", "ERROR" };
        const char* lvl = (level >= 0 && level <= 3) ? levelNames[level] : "???";

        CString* pMsg = new CString();
        pMsg->Format(_T("[%S][%S] %S"), lvl, source, message);
        g_pDlg->PostMessage(WM_LOG_MESSAGE, 0, (LPARAM)pMsg);
    }
}

BEGIN_MESSAGE_MAP(CDriverTestApp3Dlg, CDialogEx)
    ON_WM_CLOSE()
    ON_BN_CLICKED(IDC_BTN_LOAD_DLL, &CDriverTestApp3Dlg::OnBnClickedLoadDll)
    ON_BN_CLICKED(IDC_BTN_UNLOAD_DLL, &CDriverTestApp3Dlg::OnBnClickedUnloadDll)
    ON_BN_CLICKED(IDC_BTN_CONNECT, &CDriverTestApp3Dlg::OnBnClickedConnect)
    ON_BN_CLICKED(IDC_BTN_DISCONNECT, &CDriverTestApp3Dlg::OnBnClickedDisconnect)
    ON_BN_CLICKED(IDC_BTN_INITIALIZE, &CDriverTestApp3Dlg::OnBnClickedInitialize)
    ON_BN_CLICKED(IDC_BTN_CONFIGURE_ORL, &CDriverTestApp3Dlg::OnBnClickedConfigureOrl)
    ON_BN_CLICKED(IDC_BTN_TAKE_REFERENCE, &CDriverTestApp3Dlg::OnBnClickedTakeReference)
    ON_BN_CLICKED(IDC_BTN_TAKE_MEASUREMENT, &CDriverTestApp3Dlg::OnBnClickedTakeMeasurement)
    ON_BN_CLICKED(IDC_BTN_GET_RESULTS, &CDriverTestApp3Dlg::OnBnClickedGetResults)
    ON_BN_CLICKED(IDC_BTN_RUN_FULL_TEST, &CDriverTestApp3Dlg::OnBnClickedRunFullTest)
    ON_BN_CLICKED(IDC_BTN_CLEAR_LOG, &CDriverTestApp3Dlg::OnBnClickedClearLog)
    ON_CBN_SELCHANGE(IDC_COMBO_DEVICE_TYPE, &CDriverTestApp3Dlg::OnCbnSelchangeDeviceType)
    ON_BN_CLICKED(IDC_CHECK_OVERRIDE, &CDriverTestApp3Dlg::OnBnClickedOverride)
    ON_MESSAGE(WM_LOG_MESSAGE, &CDriverTestApp3Dlg::OnLogMessage)
    ON_MESSAGE(WM_WORKER_DONE, &CDriverTestApp3Dlg::OnWorkerDone)
END_MESSAGE_MAP()

CDriverTestApp3Dlg::CDriverTestApp3Dlg(CWnd* pParent)
    : CDialogEx(IDD_DRIVERTESTAPP3_DIALOG, pParent)
    , m_bBusy(false)
{
}

CDriverTestApp3Dlg::~CDriverTestApp3Dlg()
{
    g_pDlg = nullptr;
}

void CDriverTestApp3Dlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT_DLL_PATH, m_editDllPath);
    DDX_Control(pDX, IDC_COMBO_DEVICE_TYPE, m_comboDeviceType);
    DDX_Control(pDX, IDC_EDIT_IP, m_editIP);
    DDX_Control(pDX, IDC_EDIT_PORT, m_editPort);
    DDX_Control(pDX, IDC_EDIT_SLOT, m_editSlot);
    DDX_Control(pDX, IDC_CHECK_1310, m_check1310);
    DDX_Control(pDX, IDC_CHECK_1550, m_check1550);
    DDX_Control(pDX, IDC_EDIT_CH_FROM, m_editChFrom);
    DDX_Control(pDX, IDC_EDIT_CH_TO, m_editChTo);
    DDX_Control(pDX, IDC_COMBO_ORL_METHOD, m_comboOrlMethod);
    DDX_Control(pDX, IDC_COMBO_ORL_ORIGIN, m_comboOrlOrigin);
    DDX_Control(pDX, IDC_EDIT_A_OFFSET, m_editAOffset);
    DDX_Control(pDX, IDC_EDIT_B_OFFSET, m_editBOffset);
    DDX_Control(pDX, IDC_CHECK_OVERRIDE, m_checkOverride);
    DDX_Control(pDX, IDC_EDIT_IL_VALUE, m_editILValue);
    DDX_Control(pDX, IDC_EDIT_LENGTH_VALUE, m_editLengthValue);
    DDX_Control(pDX, IDC_LIST_RESULTS, m_listResults);
    DDX_Control(pDX, IDC_EDIT_LOG, m_editLog);
}

BOOL CDriverTestApp3Dlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    g_pDlg = this;

    m_editDllPath.SetWindowText(_T("UDL.ViaviNSantecTester.dll"));

    m_comboDeviceType.AddString(_T("VIAVI MAP300 PCT"));
    m_comboDeviceType.AddString(_T("Santec"));
    m_comboDeviceType.SetCurSel(0);

    m_editIP.SetWindowText(_T("10.14.132.194"));
    m_editPort.SetWindowText(_T("8301"));
    m_editSlot.SetWindowText(_T("1"));

    m_check1310.SetCheck(BST_CHECKED);
    m_check1550.SetCheck(BST_CHECKED);

    m_editChFrom.SetWindowText(_T("1"));
    m_editChTo.SetWindowText(_T("12"));

    m_comboOrlMethod.AddString(_T("Integration (1)"));
    m_comboOrlMethod.AddString(_T("Discrete (2)"));
    m_comboOrlMethod.SetCurSel(1);

    m_comboOrlOrigin.AddString(_T("A+B from DUT start (1)"));
    m_comboOrlOrigin.AddString(_T("A+B from DUT end (2)"));
    m_comboOrlOrigin.AddString(_T("A start / B end (3)"));
    m_comboOrlOrigin.SetCurSel(0);

    m_editAOffset.SetWindowText(_T("-0.5"));
    m_editBOffset.SetWindowText(_T("0.5"));

    m_checkOverride.SetCheck(BST_CHECKED);
    m_editILValue.SetWindowText(_T("0.1"));
    m_editLengthValue.SetWindowText(_T("3.0"));

    m_listResults.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_listResults.InsertColumn(0, _T("Channel"), LVCFMT_CENTER, 55);
    m_listResults.InsertColumn(1, _T("WL (nm)"), LVCFMT_CENTER, 60);
    m_listResults.InsertColumn(2, _T("IL (dB)"), LVCFMT_RIGHT, 65);
    m_listResults.InsertColumn(3, _T("RLA (dB)"), LVCFMT_RIGHT, 65);
    m_listResults.InsertColumn(4, _T("RLB (dB)"), LVCFMT_RIGHT, 65);
    m_listResults.InsertColumn(5, _T("RLTOTAL (dB)"), LVCFMT_RIGHT, 80);
    m_listResults.InsertColumn(6, _T("Length (m)"), LVCFMT_RIGHT, 70);

    EnableConnectionControls(false);
    EnableControls(false);
    AppendLog(_T("Application started. Load DLL first, then select device type and connect."));

    return TRUE;
}

// ---------------------------------------------------------------------------
// 防止意外关闭对话框
// ---------------------------------------------------------------------------

void CDriverTestApp3Dlg::OnOK()
{
}

void CDriverTestApp3Dlg::OnCancel()
{
    if (m_bBusy)
    {
        MessageBox(_T("An operation is in progress. Please wait for it to complete."),
                   _T("Busy"), MB_OK | MB_ICONINFORMATION);
        return;
    }
    CDialogEx::OnCancel();
}

void CDriverTestApp3Dlg::OnClose()
{
    if (m_bBusy)
    {
        MessageBox(_T("An operation is in progress. Please wait for it to complete."),
                   _T("Busy"), MB_OK | MB_ICONINFORMATION);
        return;
    }
    CDialogEx::OnClose();
}

// ---------------------------------------------------------------------------
// DLL 加载 / 卸载
// ---------------------------------------------------------------------------

void CDriverTestApp3Dlg::OnBnClickedLoadDll()
{
    CString dllPath;
    m_editDllPath.GetWindowText(dllPath);

    if (dllPath.IsEmpty())
    {
        AppendLog(_T("Please enter a DLL path."));
        return;
    }

    CStringA pathA(dllPath);
    bool ok = m_loader.LoadDll(pathA.GetString());

    if (ok)
    {
        m_loader.SetLogCallback(GlobalLogCallback);
        AppendLog(_T("DLL loaded successfully."));
        UpdateStatus(_T("DLL Loaded - Disconnected"));
        EnableConnectionControls(true);
    }
    else
    {
        CString msg;
        msg.Format(_T("Failed to load DLL: %s"), (LPCTSTR)dllPath);
        AppendLog(msg);
    }
}

void CDriverTestApp3Dlg::OnBnClickedUnloadDll()
{
    if (m_loader.GetDriverHandle())
    {
        m_loader.Disconnect();
        m_loader.DestroyDriver();
    }

    m_loader.UnloadDll();
    AppendLog(_T("DLL unloaded."));
    UpdateStatus(_T("DLL Not Loaded"));
    EnableConnectionControls(false);
    EnableControls(false);
}

// ---------------------------------------------------------------------------
// 异步工作线程基础设施
// ---------------------------------------------------------------------------

void CDriverTestApp3Dlg::RunAsync(const CString& operationName,
                                   std::function<WorkerResult*()> task)
{
    if (m_bBusy)
    {
        AppendLog(_T("Another operation is in progress. Please wait."));
        return;
    }

    SetBusy(true, operationName);

    HWND hWnd = GetSafeHwnd();
    std::thread worker([hWnd, task]()
    {
        WorkerResult* result = nullptr;
        try
        {
            result = task();
        }
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

LRESULT CDriverTestApp3Dlg::OnWorkerDone(WPARAM /*wParam*/, LPARAM lParam)
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
        else
        {
            bool connected = m_loader.IsConnected() != FALSE;
            UpdateStatus(connected ? _T("Connected - Ready") : _T("Disconnected"));
        }

        delete result;
    }

    SetBusy(false);
    return 0;
}

// ---------------------------------------------------------------------------
// 忙碌状态管理
// ---------------------------------------------------------------------------

void CDriverTestApp3Dlg::SetBusy(bool busy, const CString& statusText)
{
    m_bBusy = busy;

    if (busy)
    {
        static const int ids[] = {
            IDC_BTN_LOAD_DLL, IDC_BTN_UNLOAD_DLL,
            IDC_BTN_CONNECT, IDC_BTN_DISCONNECT, IDC_BTN_INITIALIZE,
            IDC_BTN_CONFIGURE_ORL, IDC_BTN_TAKE_REFERENCE, IDC_BTN_TAKE_MEASUREMENT,
            IDC_BTN_GET_RESULTS, IDC_BTN_RUN_FULL_TEST,
            IDC_COMBO_DEVICE_TYPE, IDC_EDIT_IP, IDC_EDIT_PORT, IDC_EDIT_SLOT,
            IDC_EDIT_DLL_PATH,
            IDC_CHECK_1310, IDC_CHECK_1550, IDC_EDIT_CH_FROM, IDC_EDIT_CH_TO,
            IDC_COMBO_ORL_METHOD, IDC_COMBO_ORL_ORIGIN, IDC_EDIT_A_OFFSET, IDC_EDIT_B_OFFSET,
            IDC_CHECK_OVERRIDE, IDC_EDIT_IL_VALUE, IDC_EDIT_LENGTH_VALUE
        };

        for (int id : ids)
        {
            CWnd* pWnd = GetDlgItem(id);
            if (pWnd) pWnd->EnableWindow(FALSE);
        }

        if (!statusText.IsEmpty())
            UpdateStatus(statusText);
    }
    else
    {
        bool dllLoaded = m_loader.IsDllLoaded();
        bool connected = dllLoaded && m_loader.IsConnected() != FALSE;

        EnableConnectionControls(dllLoaded);
        EnableControls(connected);

        GetDlgItem(IDC_CHECK_1310)->EnableWindow(TRUE);
        GetDlgItem(IDC_CHECK_1550)->EnableWindow(TRUE);
        GetDlgItem(IDC_EDIT_CH_FROM)->EnableWindow(TRUE);
        GetDlgItem(IDC_EDIT_CH_TO)->EnableWindow(TRUE);
        GetDlgItem(IDC_COMBO_ORL_METHOD)->EnableWindow(TRUE);
        GetDlgItem(IDC_COMBO_ORL_ORIGIN)->EnableWindow(TRUE);
        GetDlgItem(IDC_EDIT_A_OFFSET)->EnableWindow(TRUE);
        GetDlgItem(IDC_EDIT_B_OFFSET)->EnableWindow(TRUE);
        GetDlgItem(IDC_CHECK_OVERRIDE)->EnableWindow(TRUE);

        BOOL overrideOn = (m_checkOverride.GetCheck() == BST_CHECKED);
        GetDlgItem(IDC_EDIT_IL_VALUE)->EnableWindow(overrideOn);
        GetDlgItem(IDC_EDIT_LENGTH_VALUE)->EnableWindow(overrideOn);
    }
}

// ---------------------------------------------------------------------------
// 日志处理
// ---------------------------------------------------------------------------

LRESULT CDriverTestApp3Dlg::OnLogMessage(WPARAM /*wParam*/, LPARAM lParam)
{
    CString* pMsg = reinterpret_cast<CString*>(lParam);
    if (pMsg)
    {
        AppendLog(*pMsg);
        delete pMsg;
    }
    return 0;
}

void CDriverTestApp3Dlg::AppendLog(const CString& text)
{
    CString current;
    m_editLog.GetWindowText(current);
    if (!current.IsEmpty())
        current += _T("\r\n");
    current += text;
    m_editLog.SetWindowText(current);
    m_editLog.LineScroll(m_editLog.GetLineCount());
}

void CDriverTestApp3Dlg::UpdateStatus(const CString& status)
{
    SetDlgItemText(IDC_STATIC_STATUS, status);
}

void CDriverTestApp3Dlg::EnableConnectionControls(bool dllLoaded)
{
    GetDlgItem(IDC_EDIT_DLL_PATH)->EnableWindow(!dllLoaded);
    GetDlgItem(IDC_BTN_LOAD_DLL)->EnableWindow(!dllLoaded);
    GetDlgItem(IDC_BTN_UNLOAD_DLL)->EnableWindow(dllLoaded);

    if (!dllLoaded)
    {
        GetDlgItem(IDC_BTN_CONNECT)->EnableWindow(FALSE);
        GetDlgItem(IDC_BTN_DISCONNECT)->EnableWindow(FALSE);
        GetDlgItem(IDC_BTN_INITIALIZE)->EnableWindow(FALSE);
        GetDlgItem(IDC_COMBO_DEVICE_TYPE)->EnableWindow(FALSE);
        GetDlgItem(IDC_EDIT_IP)->EnableWindow(FALSE);
        GetDlgItem(IDC_EDIT_PORT)->EnableWindow(FALSE);
        GetDlgItem(IDC_EDIT_SLOT)->EnableWindow(FALSE);
    }
    else
    {
        bool connected = m_loader.IsConnected() != FALSE;
        GetDlgItem(IDC_BTN_CONNECT)->EnableWindow(!connected);
        GetDlgItem(IDC_BTN_DISCONNECT)->EnableWindow(connected);
        GetDlgItem(IDC_BTN_INITIALIZE)->EnableWindow(connected);
        GetDlgItem(IDC_COMBO_DEVICE_TYPE)->EnableWindow(!connected);
        GetDlgItem(IDC_EDIT_IP)->EnableWindow(!connected);
        GetDlgItem(IDC_EDIT_PORT)->EnableWindow(!connected);
        GetDlgItem(IDC_EDIT_SLOT)->EnableWindow(!connected);
    }
}

void CDriverTestApp3Dlg::EnableControls(bool connected)
{
    GetDlgItem(IDC_BTN_CONNECT)->EnableWindow(m_loader.IsDllLoaded() && !connected);
    GetDlgItem(IDC_BTN_DISCONNECT)->EnableWindow(connected);
    GetDlgItem(IDC_BTN_INITIALIZE)->EnableWindow(connected);
    GetDlgItem(IDC_COMBO_DEVICE_TYPE)->EnableWindow(m_loader.IsDllLoaded() && !connected);
    GetDlgItem(IDC_EDIT_IP)->EnableWindow(m_loader.IsDllLoaded() && !connected);
    GetDlgItem(IDC_EDIT_PORT)->EnableWindow(m_loader.IsDllLoaded() && !connected);
    GetDlgItem(IDC_EDIT_SLOT)->EnableWindow(m_loader.IsDllLoaded() && !connected);

    GetDlgItem(IDC_BTN_CONFIGURE_ORL)->EnableWindow(connected);
    GetDlgItem(IDC_BTN_TAKE_REFERENCE)->EnableWindow(connected);
    GetDlgItem(IDC_BTN_TAKE_MEASUREMENT)->EnableWindow(connected);
    GetDlgItem(IDC_BTN_GET_RESULTS)->EnableWindow(connected);
    GetDlgItem(IDC_BTN_RUN_FULL_TEST)->EnableWindow(connected);
}

// ---------------------------------------------------------------------------
// 设备类型切换
// ---------------------------------------------------------------------------

void CDriverTestApp3Dlg::OnCbnSelchangeDeviceType()
{
    int sel = m_comboDeviceType.GetCurSel();
    if (sel == 0)
    {
        m_editPort.SetWindowText(_T("8301"));
        m_editSlot.SetWindowText(_T("1"));
        GetDlgItem(IDC_EDIT_SLOT)->EnableWindow(TRUE);
        GetDlgItem(IDC_BTN_CONFIGURE_ORL)->ShowWindow(SW_SHOW);
    }
    else
    {
        m_editPort.SetWindowText(_T("5025"));
        m_editSlot.SetWindowText(_T("0"));
        GetDlgItem(IDC_EDIT_SLOT)->EnableWindow(FALSE);
        GetDlgItem(IDC_BTN_CONFIGURE_ORL)->ShowWindow(SW_HIDE);
    }
}

void CDriverTestApp3Dlg::OnBnClickedOverride()
{
    BOOL enabled = (m_checkOverride.GetCheck() == BST_CHECKED);
    m_editILValue.EnableWindow(enabled);
    m_editLengthValue.EnableWindow(enabled);
}

// ---------------------------------------------------------------------------
// 连接（异步）-- 使用动态加载器 C API
// ---------------------------------------------------------------------------

void CDriverTestApp3Dlg::OnBnClickedConnect()
{
    if (m_loader.GetDriverHandle())
    {
        m_loader.DestroyDriver();
    }

    CString ip, portStr, slotStr;
    m_editIP.GetWindowText(ip);
    m_editPort.GetWindowText(portStr);
    m_editSlot.GetWindowText(slotStr);

    int port = _ttoi(portStr);
    int slot = _ttoi(slotStr);
    int sel = m_comboDeviceType.GetCurSel();

    CStringA ipA(ip);
    std::string ipStr(ipA.GetString());
    std::string typeStr = (sel == 0) ? "viavi" : "santec";

    bool created = m_loader.CreateDriver(typeStr.c_str(), ipStr.c_str(), port, slot);
    if (!created)
    {
        AppendLog(_T("Failed to create driver instance."));
        return;
    }

    AppendLog(_T("Connecting..."));

    CViaviSantecDllLoader* pLoader = &m_loader;
    RunAsync(_T("Connecting..."), [pLoader]() -> WorkerResult*
    {
        WorkerResult* r = new WorkerResult();
        try
        {
            r->success = pLoader->Connect() != FALSE;
            if (r->success)
            {
                r->logMessage = _T("Connection successful.");
                r->statusText = _T("Connected");
            }
            else
            {
                r->logMessage = _T("Connection failed.");
                r->statusText = _T("Connection Failed");
            }
        }
        catch (...)
        {
            r->logMessage = _T("Connection error.");
            r->statusText = _T("Error");
        }
        return r;
    });
}

void CDriverTestApp3Dlg::OnBnClickedDisconnect()
{
    if (!m_loader.GetDriverHandle()) return;

    CViaviSantecDllLoader* pLoader = &m_loader;
    RunAsync(_T("Disconnecting..."), [pLoader]() -> WorkerResult*
    {
        WorkerResult* r = new WorkerResult();
        pLoader->Disconnect();
        r->success = true;
        r->logMessage = _T("Disconnected.");
        r->statusText = _T("Disconnected");
        return r;
    });
}

// ---------------------------------------------------------------------------
// 初始化（异步）
// ---------------------------------------------------------------------------

void CDriverTestApp3Dlg::OnBnClickedInitialize()
{
    if (!m_loader.GetDriverHandle()) return;

    CViaviSantecDllLoader* pLoader = &m_loader;
    RunAsync(_T("Initializing..."), [pLoader]() -> WorkerResult*
    {
        WorkerResult* r = new WorkerResult();
        try
        {
            r->success = pLoader->Initialize() != FALSE;
            r->logMessage = r->success
                ? _T("Initialization successful.")
                : _T("Initialization failed.");
            r->statusText = r->success
                ? _T("Initialized - Ready")
                : _T("Initialization Failed");
        }
        catch (...)
        {
            r->logMessage = _T("Init error.");
            r->statusText = _T("Init Error");
        }
        return r;
    });
}

// ---------------------------------------------------------------------------
// ORL 配置（异步）-- 通过 C API ConfigureORL
// ---------------------------------------------------------------------------

void CDriverTestApp3Dlg::OnBnClickedConfigureOrl()
{
    if (!m_loader.GetDriverHandle()) return;

    CString aStr, bStr;
    m_editAOffset.GetWindowText(aStr);
    m_editBOffset.GetWindowText(bStr);

    int channel = 1;
    int method = m_comboOrlMethod.GetCurSel() + 1;
    int origin = m_comboOrlOrigin.GetCurSel() + 1;
    double aOffset = _ttof(aStr);
    double bOffset = _ttof(bStr);

    CViaviSantecDllLoader* pLoader = &m_loader;
    RunAsync(_T("Configuring ORL..."),
        [pLoader, channel, method, origin, aOffset, bOffset]() -> WorkerResult*
    {
        WorkerResult* r = new WorkerResult();
        try
        {
            r->success = pLoader->ConfigureORL(channel, method, origin, aOffset, bOffset) != FALSE;
            r->logMessage = r->success
                ? _T("ORL configuration applied.")
                : _T("ORL configuration failed.");
            r->statusText = r->success
                ? _T("ORL Configured - Ready")
                : _T("ORL Config Failed");
        }
        catch (...)
        {
            r->logMessage = _T("ORL config error.");
            r->statusText = _T("ORL Config Error");
        }
        return r;
    });
}

// ---------------------------------------------------------------------------
// 参考（异步）-- 配置波长/通道后进行参考测量
// ---------------------------------------------------------------------------

void CDriverTestApp3Dlg::OnBnClickedTakeReference()
{
    if (!m_loader.GetDriverHandle()) return;

    std::vector<double> wavelengths = GetSelectedWavelengths();
    std::vector<int> channels = GetSelectedChannels();

    BOOL bOverride = (m_checkOverride.GetCheck() == BST_CHECKED);
    CString ilStr, lenStr;
    m_editILValue.GetWindowText(ilStr);
    m_editLengthValue.GetWindowText(lenStr);
    double ilValue = _ttof(ilStr);
    double lengthValue = _ttof(lenStr);

    AppendLog(_T("Taking reference..."));

    CViaviSantecDllLoader* pLoader = &m_loader;
    RunAsync(_T("Taking reference..."),
        [pLoader, wavelengths, channels, bOverride, ilValue, lengthValue]() -> WorkerResult*
    {
        WorkerResult* r = new WorkerResult();
        try
        {
            std::vector<double> wl = wavelengths;
            std::vector<int> ch = channels;
            pLoader->ConfigureWavelengths(wl.data(), (int)wl.size());
            pLoader->ConfigureChannels(ch.data(), (int)ch.size());
            r->success = pLoader->TakeReference(bOverride, ilValue, lengthValue) != FALSE;
            r->logMessage = r->success
                ? _T("Reference completed.")
                : _T("Reference FAILED.");
            r->statusText = r->success
                ? _T("Reference Done - Ready")
                : _T("Reference Failed");
        }
        catch (...)
        {
            r->logMessage = _T("Reference error.");
            r->statusText = _T("Reference Error");
        }
        return r;
    });
}

// ---------------------------------------------------------------------------
// 测量（异步）
// ---------------------------------------------------------------------------

void CDriverTestApp3Dlg::OnBnClickedTakeMeasurement()
{
    if (!m_loader.GetDriverHandle()) return;

    AppendLog(_T("Taking measurement..."));

    CViaviSantecDllLoader* pLoader = &m_loader;
    RunAsync(_T("Measuring..."), [pLoader]() -> WorkerResult*
    {
        WorkerResult* r = new WorkerResult();
        try
        {
            r->success = pLoader->TakeMeasurement() != FALSE;
            r->logMessage = r->success
                ? _T("Measurement completed.")
                : _T("Measurement FAILED.");
            r->statusText = r->success
                ? _T("Measurement Done - Ready")
                : _T("Measurement Failed");
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
// 获取结果（异步）
// ---------------------------------------------------------------------------

void CDriverTestApp3Dlg::OnBnClickedGetResults()
{
    if (!m_loader.GetDriverHandle()) return;

    CViaviSantecDllLoader* pLoader = &m_loader;
    RunAsync(_T("Retrieving results..."), [pLoader]() -> WorkerResult*
    {
        WorkerResult* r = new WorkerResult();
        try
        {
            DriverMeasurementResult buf[256];
            int count = pLoader->GetResults(buf, 256);
            r->results.assign(buf, buf + count);
            r->hasResults = true;
            r->success = true;
            CString msg;
            msg.Format(_T("Retrieved %d results."), count);
            r->logMessage = msg;
            r->statusText = _T("Results Retrieved - Ready");
        }
        catch (...)
        {
            r->logMessage = _T("Get results error.");
            r->statusText = _T("Get Results Error");
        }
        return r;
    });
}

// ---------------------------------------------------------------------------
// 运行完整测试（异步）-- 分解为：参考 + 测量 + 获取结果
// ---------------------------------------------------------------------------

void CDriverTestApp3Dlg::OnBnClickedRunFullTest()
{
    if (!m_loader.GetDriverHandle()) return;

    std::vector<double> wavelengths = GetSelectedWavelengths();
    std::vector<int> channels = GetSelectedChannels();

    BOOL bOverride = (m_checkOverride.GetCheck() == BST_CHECKED);
    CString ilStr, lenStr;
    m_editILValue.GetWindowText(ilStr);
    m_editLengthValue.GetWindowText(lenStr);
    double ilValue = _ttof(ilStr);
    double lengthValue = _ttof(lenStr);

    AppendLog(_T("=== Running Full Test ==="));

    CViaviSantecDllLoader* pLoader = &m_loader;
    RunAsync(_T("Running full test..."),
        [pLoader, wavelengths, channels, bOverride, ilValue, lengthValue]() -> WorkerResult*
    {
        WorkerResult* r = new WorkerResult();
        try
        {
            std::vector<double> wl = wavelengths;
            std::vector<int> ch = channels;
            pLoader->ConfigureWavelengths(wl.data(), (int)wl.size());
            pLoader->ConfigureChannels(ch.data(), (int)ch.size());

            BOOL refOk = pLoader->TakeReference(bOverride, ilValue, lengthValue);
            if (!refOk)
            {
                r->logMessage = _T("Full test: Reference FAILED.");
                r->statusText = _T("Full Test - Reference Failed");
                return r;
            }

            BOOL measOk = pLoader->TakeMeasurement();
            if (!measOk)
            {
                r->logMessage = _T("Full test: Measurement FAILED.");
                r->statusText = _T("Full Test - Measurement Failed");
                return r;
            }

            DriverMeasurementResult buf[256];
            int count = pLoader->GetResults(buf, 256);
            r->results.assign(buf, buf + count);
            r->hasResults = true;
            r->success = true;

            CString msg;
            msg.Format(_T("=== Full Test Complete: %d results ==="), count);
            r->logMessage = msg;
            r->statusText = _T("Full Test Done - Ready");
        }
        catch (...)
        {
            r->logMessage = _T("Full test error.");
            r->statusText = _T("Full Test Error");
        }
        return r;
    });
}

// ---------------------------------------------------------------------------
// 清除日志
// ---------------------------------------------------------------------------

void CDriverTestApp3Dlg::OnBnClickedClearLog()
{
    m_editLog.SetWindowText(_T(""));
}

// ---------------------------------------------------------------------------
// 辅助函数
// ---------------------------------------------------------------------------

std::vector<double> CDriverTestApp3Dlg::GetSelectedWavelengths()
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

std::vector<int> CDriverTestApp3Dlg::GetSelectedChannels()
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

void CDriverTestApp3Dlg::PopulateResultsList(const std::vector<DriverMeasurementResult>& results)
{
    m_listResults.DeleteAllItems();

    for (size_t i = 0; i < results.size(); ++i)
    {
        const DriverMeasurementResult& r = results[i];
        CString chStr, wlStr, ilStr, rlaStr, rlbStr, rltStr, lenStr;
        chStr.Format(_T("%d"), r.channel);
        wlStr.Format(_T("%.0f"), r.wavelength);
        ilStr.Format(_T("%.4f"), r.insertionLoss);
        rlaStr.Format(_T("%.2f"), r.returnLossA);
        rlbStr.Format(_T("%.2f"), r.returnLossB);
        rltStr.Format(_T("%.2f"), r.returnLossTotal);
        lenStr.Format(_T("%.2f"), r.dutLength);

        int idx = m_listResults.InsertItem(static_cast<int>(i), chStr);
        m_listResults.SetItemText(idx, 1, wlStr);
        m_listResults.SetItemText(idx, 2, ilStr);
        m_listResults.SetItemText(idx, 3, rlaStr);
        m_listResults.SetItemText(idx, 4, rlbStr);
        m_listResults.SetItemText(idx, 5, rltStr);
        m_listResults.SetItemText(idx, 6, lenStr);
    }
}
