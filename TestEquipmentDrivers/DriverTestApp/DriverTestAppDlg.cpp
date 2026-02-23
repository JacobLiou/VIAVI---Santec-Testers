#include "stdafx.h"
#include "DriverTestApp.h"
#include "DriverTestAppDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace EquipmentDriver;

static CDriverTestAppDlg* g_pDlg = nullptr;

BEGIN_MESSAGE_MAP(CDriverTestAppDlg, CDialogEx)
    ON_WM_CLOSE()
    ON_BN_CLICKED(IDC_BTN_CONNECT, &CDriverTestAppDlg::OnBnClickedConnect)
    ON_BN_CLICKED(IDC_BTN_DISCONNECT, &CDriverTestAppDlg::OnBnClickedDisconnect)
    ON_BN_CLICKED(IDC_BTN_INITIALIZE, &CDriverTestAppDlg::OnBnClickedInitialize)
    ON_BN_CLICKED(IDC_BTN_CONFIGURE_ORL, &CDriverTestAppDlg::OnBnClickedConfigureOrl)
    ON_BN_CLICKED(IDC_BTN_TAKE_REFERENCE, &CDriverTestAppDlg::OnBnClickedTakeReference)
    ON_BN_CLICKED(IDC_BTN_TAKE_MEASUREMENT, &CDriverTestAppDlg::OnBnClickedTakeMeasurement)
    ON_BN_CLICKED(IDC_BTN_GET_RESULTS, &CDriverTestAppDlg::OnBnClickedGetResults)
    ON_BN_CLICKED(IDC_BTN_RUN_FULL_TEST, &CDriverTestAppDlg::OnBnClickedRunFullTest)
    ON_BN_CLICKED(IDC_BTN_CLEAR_LOG, &CDriverTestAppDlg::OnBnClickedClearLog)
    ON_CBN_SELCHANGE(IDC_COMBO_DEVICE_TYPE, &CDriverTestAppDlg::OnCbnSelchangeDeviceType)
    ON_BN_CLICKED(IDC_CHECK_OVERRIDE, &CDriverTestAppDlg::OnBnClickedOverride)
    ON_MESSAGE(WM_LOG_MESSAGE, &CDriverTestAppDlg::OnLogMessage)
    ON_MESSAGE(WM_WORKER_DONE, &CDriverTestAppDlg::OnWorkerDone)
END_MESSAGE_MAP()

CDriverTestAppDlg::CDriverTestAppDlg(CWnd* pParent)
    : CDialogEx(IDD_DRIVERTESTAPP_DIALOG, pParent)
    , m_pDriver(nullptr)
    , m_bBusy(false)
{
}

CDriverTestAppDlg::~CDriverTestAppDlg()
{
    g_pDlg = nullptr;
    if (m_pDriver)
    {
        CDriverFactory::Destroy(m_pDriver);
        m_pDriver = nullptr;
    }
}

void CDriverTestAppDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
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

BOOL CDriverTestAppDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    g_pDlg = this;

    CLogger::SetGlobalCallback(
        [](LogLevel level, const std::string& source, const std::string& message)
    {
        if (g_pDlg && ::IsWindow(g_pDlg->GetSafeHwnd()))
        {
            static const char* levelNames[] = { "DEBUG", "INFO", "WARN", "ERROR" };
            const char* lvl = (level >= 0 && level <= 3) ? levelNames[level] : "???";

            CString* pMsg = new CString();
            pMsg->Format(_T("[%S][%S] %S"), lvl, source.c_str(), message.c_str());
            g_pDlg->PostMessage(WM_LOG_MESSAGE, 0, (LPARAM)pMsg);
        }
    });

    // Device type combo
    m_comboDeviceType.AddString(_T("VIAVI MAP300 PCT"));
    m_comboDeviceType.AddString(_T("Santec"));
    m_comboDeviceType.SetCurSel(0);

    // Default values
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
    m_listResults.InsertColumn(0, _T("Channel"), LVCFMT_CENTER, 60);
    m_listResults.InsertColumn(1, _T("Wavelength (nm)"), LVCFMT_CENTER, 90);
    m_listResults.InsertColumn(2, _T("IL (dB)"), LVCFMT_RIGHT, 70);
    m_listResults.InsertColumn(3, _T("RL (dB)"), LVCFMT_RIGHT, 70);

    EnableControls(false);
    AppendLog(_T("Application started. Select device type and connect."));

    return TRUE;
}

// ---------------------------------------------------------------------------
// Prevent accidental dialog close
// ---------------------------------------------------------------------------

void CDriverTestAppDlg::OnOK()
{
    // Block Enter key from closing the dialog
}

void CDriverTestAppDlg::OnCancel()
{
    if (m_bBusy)
    {
        MessageBox(_T("An operation is in progress. Please wait for it to complete."),
                   _T("Busy"), MB_OK | MB_ICONINFORMATION);
        return;
    }
    CDialogEx::OnCancel();
}

void CDriverTestAppDlg::OnClose()
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
// Async worker infrastructure
// ---------------------------------------------------------------------------

void CDriverTestAppDlg::RunAsync(const CString& operationName,
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

LRESULT CDriverTestAppDlg::OnWorkerDone(WPARAM /*wParam*/, LPARAM lParam)
{
    WorkerResult* result = reinterpret_cast<WorkerResult*>(lParam);
    if (result)
    {
        if (!result->logMessage.IsEmpty())
            AppendLog(result->logMessage);

        if (!result->statusText.IsEmpty())
            UpdateStatus(result->statusText);

        if (result->hasResults)
            PopulateResultsList(result->results);

        delete result;
    }

    SetBusy(false);
    return 0;
}

// ---------------------------------------------------------------------------
// Busy state management
// ---------------------------------------------------------------------------

void CDriverTestAppDlg::SetBusy(bool busy, const CString& statusText)
{
    m_bBusy = busy;

    if (busy)
    {
        static const int ids[] = {
            IDC_BTN_CONNECT, IDC_BTN_DISCONNECT, IDC_BTN_INITIALIZE,
            IDC_BTN_CONFIGURE_ORL, IDC_BTN_TAKE_REFERENCE, IDC_BTN_TAKE_MEASUREMENT,
            IDC_BTN_GET_RESULTS, IDC_BTN_RUN_FULL_TEST,
            IDC_COMBO_DEVICE_TYPE, IDC_EDIT_IP, IDC_EDIT_PORT, IDC_EDIT_SLOT,
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
        bool connected = m_pDriver && m_pDriver->IsConnected();
        EnableControls(connected);

        // Parameter controls are always usable when not busy
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
// Log handling
// ---------------------------------------------------------------------------

LRESULT CDriverTestAppDlg::OnLogMessage(WPARAM /*wParam*/, LPARAM lParam)
{
    CString* pMsg = reinterpret_cast<CString*>(lParam);
    if (pMsg)
    {
        AppendLog(*pMsg);
        delete pMsg;
    }
    return 0;
}

void CDriverTestAppDlg::AppendLog(const CString& text)
{
    CString current;
    m_editLog.GetWindowText(current);
    if (!current.IsEmpty())
        current += _T("\r\n");
    current += text;
    m_editLog.SetWindowText(current);
    m_editLog.LineScroll(m_editLog.GetLineCount());
}

void CDriverTestAppDlg::UpdateStatus(const CString& status)
{
    SetDlgItemText(IDC_STATIC_STATUS, status);
}

void CDriverTestAppDlg::EnableControls(bool connected)
{
    GetDlgItem(IDC_BTN_CONNECT)->EnableWindow(!connected);
    GetDlgItem(IDC_BTN_DISCONNECT)->EnableWindow(connected);
    GetDlgItem(IDC_BTN_INITIALIZE)->EnableWindow(connected);
    GetDlgItem(IDC_COMBO_DEVICE_TYPE)->EnableWindow(!connected);
    GetDlgItem(IDC_EDIT_IP)->EnableWindow(!connected);
    GetDlgItem(IDC_EDIT_PORT)->EnableWindow(!connected);
    GetDlgItem(IDC_EDIT_SLOT)->EnableWindow(!connected);

    GetDlgItem(IDC_BTN_CONFIGURE_ORL)->EnableWindow(connected);
    GetDlgItem(IDC_BTN_TAKE_REFERENCE)->EnableWindow(connected);
    GetDlgItem(IDC_BTN_TAKE_MEASUREMENT)->EnableWindow(connected);
    GetDlgItem(IDC_BTN_GET_RESULTS)->EnableWindow(connected);
    GetDlgItem(IDC_BTN_RUN_FULL_TEST)->EnableWindow(connected);
}

// ---------------------------------------------------------------------------
// Device type change
// ---------------------------------------------------------------------------

void CDriverTestAppDlg::OnCbnSelchangeDeviceType()
{
    int sel = m_comboDeviceType.GetCurSel();
    if (sel == 0) // VIAVI
    {
        m_editPort.SetWindowText(_T("8301"));
        m_editSlot.SetWindowText(_T("1"));
        GetDlgItem(IDC_EDIT_SLOT)->EnableWindow(TRUE);
        GetDlgItem(IDC_BTN_CONFIGURE_ORL)->ShowWindow(SW_SHOW);
    }
    else // Santec
    {
        m_editPort.SetWindowText(_T("5000"));
        m_editSlot.SetWindowText(_T("0"));
        GetDlgItem(IDC_EDIT_SLOT)->EnableWindow(FALSE);
        GetDlgItem(IDC_BTN_CONFIGURE_ORL)->ShowWindow(SW_HIDE);
    }
}

void CDriverTestAppDlg::OnBnClickedOverride()
{
    BOOL enabled = (m_checkOverride.GetCheck() == BST_CHECKED);
    m_editILValue.EnableWindow(enabled);
    m_editLengthValue.EnableWindow(enabled);
}

// ---------------------------------------------------------------------------
// Connection (async)
// ---------------------------------------------------------------------------

void CDriverTestAppDlg::OnBnClickedConnect()
{
    if (m_pDriver)
    {
        CDriverFactory::Destroy(m_pDriver);
        m_pDriver = nullptr;
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

    try
    {
        if (sel == 0)
            m_pDriver = CDriverFactory::Create("viavi", ipStr, port, slot);
        else
            m_pDriver = CDriverFactory::Create("santec", ipStr, port, slot);
    }
    catch (const std::exception& e)
    {
        CString msg;
        msg.Format(_T("Failed to create driver: %S"), e.what());
        AppendLog(msg);
        return;
    }

    AppendLog(_T("Connecting..."));

    IEquipmentDriver* pDriver = m_pDriver;
    RunAsync(_T("Connecting..."), [pDriver]() -> WorkerResult*
    {
        WorkerResult* r = new WorkerResult();
        try
        {
            r->success = pDriver->Connect();
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
        catch (const std::exception& e)
        {
            CString msg;
            msg.Format(_T("Connection error: %S"), e.what());
            r->logMessage = msg;
            r->statusText = _T("Error");
        }
        return r;
    });
}

void CDriverTestAppDlg::OnBnClickedDisconnect()
{
    if (!m_pDriver) return;

    IEquipmentDriver* pDriver = m_pDriver;
    RunAsync(_T("Disconnecting..."), [pDriver]() -> WorkerResult*
    {
        WorkerResult* r = new WorkerResult();
        pDriver->Disconnect();
        r->success = true;
        r->logMessage = _T("Disconnected.");
        r->statusText = _T("Disconnected");
        return r;
    });
}

// ---------------------------------------------------------------------------
// Initialize (async)
// ---------------------------------------------------------------------------

void CDriverTestAppDlg::OnBnClickedInitialize()
{
    if (!m_pDriver) return;

    IEquipmentDriver* pDriver = m_pDriver;
    RunAsync(_T("Initializing..."), [pDriver]() -> WorkerResult*
    {
        WorkerResult* r = new WorkerResult();
        try
        {
            r->success = pDriver->Initialize();
            r->logMessage = r->success
                ? _T("Initialization successful.")
                : _T("Initialization failed.");
        }
        catch (const std::exception& e)
        {
            CString msg;
            msg.Format(_T("Init error: %S"), e.what());
            r->logMessage = msg;
        }
        return r;
    });
}

// ---------------------------------------------------------------------------
// ORL Configuration (async, VIAVI specific)
// ---------------------------------------------------------------------------

void CDriverTestAppDlg::OnBnClickedConfigureOrl()
{
    if (!m_pDriver) return;

    CViaviPCTDriver* viavi = dynamic_cast<CViaviPCTDriver*>(m_pDriver);
    if (!viavi)
    {
        AppendLog(_T("ORL configuration is only available for VIAVI devices."));
        return;
    }

    CString aStr, bStr;
    m_editAOffset.GetWindowText(aStr);
    m_editBOffset.GetWindowText(bStr);

    ORLConfig cfg;
    cfg.channel = 1;
    cfg.method = static_cast<ORLMethod>(m_comboOrlMethod.GetCurSel() + 1);
    cfg.origin = static_cast<ORLOrigin>(m_comboOrlOrigin.GetCurSel() + 1);
    cfg.aOffset = _ttof(aStr);
    cfg.bOffset = _ttof(bStr);

    RunAsync(_T("Configuring ORL..."), [viavi, cfg]() -> WorkerResult*
    {
        WorkerResult* r = new WorkerResult();
        try
        {
            viavi->ConfigureORL(cfg);
            r->success = true;
            r->logMessage = _T("ORL configuration applied.");
        }
        catch (const std::exception& e)
        {
            CString msg;
            msg.Format(_T("ORL config error: %S"), e.what());
            r->logMessage = msg;
        }
        return r;
    });
}

// ---------------------------------------------------------------------------
// Reference (async)
// ---------------------------------------------------------------------------

void CDriverTestAppDlg::OnBnClickedTakeReference()
{
    if (!m_pDriver) return;

    std::vector<double> wavelengths = GetSelectedWavelengths();
    std::vector<int> channels = GetSelectedChannels();

    ReferenceConfig cfg;
    cfg.useOverride = (m_checkOverride.GetCheck() == BST_CHECKED);
    CString ilStr, lenStr;
    m_editILValue.GetWindowText(ilStr);
    m_editLengthValue.GetWindowText(lenStr);
    cfg.ilValue = _ttof(ilStr);
    cfg.lengthValue = _ttof(lenStr);

    AppendLog(_T("Taking reference..."));

    IEquipmentDriver* pDriver = m_pDriver;
    RunAsync(_T("Taking reference..."), [pDriver, wavelengths, channels, cfg]() -> WorkerResult*
    {
        WorkerResult* r = new WorkerResult();
        try
        {
            pDriver->ConfigureWavelengths(wavelengths);
            pDriver->ConfigureChannels(channels);
            r->success = pDriver->TakeReference(cfg);
            r->logMessage = r->success
                ? _T("Reference completed.")
                : _T("Reference FAILED.");
        }
        catch (const std::exception& e)
        {
            CString msg;
            msg.Format(_T("Reference error: %S"), e.what());
            r->logMessage = msg;
        }
        return r;
    });
}

// ---------------------------------------------------------------------------
// Measurement (async)
// ---------------------------------------------------------------------------

void CDriverTestAppDlg::OnBnClickedTakeMeasurement()
{
    if (!m_pDriver) return;

    AppendLog(_T("Taking measurement..."));

    IEquipmentDriver* pDriver = m_pDriver;
    RunAsync(_T("Measuring..."), [pDriver]() -> WorkerResult*
    {
        WorkerResult* r = new WorkerResult();
        try
        {
            r->success = pDriver->TakeMeasurement();
            r->logMessage = r->success
                ? _T("Measurement completed.")
                : _T("Measurement FAILED.");
        }
        catch (const std::exception& e)
        {
            CString msg;
            msg.Format(_T("Measurement error: %S"), e.what());
            r->logMessage = msg;
        }
        return r;
    });
}

// ---------------------------------------------------------------------------
// Get Results (async)
// ---------------------------------------------------------------------------

void CDriverTestAppDlg::OnBnClickedGetResults()
{
    if (!m_pDriver) return;

    IEquipmentDriver* pDriver = m_pDriver;
    RunAsync(_T("Retrieving results..."), [pDriver]() -> WorkerResult*
    {
        WorkerResult* r = new WorkerResult();
        try
        {
            r->results = pDriver->GetResults();
            r->hasResults = true;
            r->success = true;
            CString msg;
            msg.Format(_T("Retrieved %d results."), (int)r->results.size());
            r->logMessage = msg;
        }
        catch (const std::exception& e)
        {
            CString msg;
            msg.Format(_T("Get results error: %S"), e.what());
            r->logMessage = msg;
        }
        return r;
    });
}

// ---------------------------------------------------------------------------
// Run Full Test (async)
// ---------------------------------------------------------------------------

void CDriverTestAppDlg::OnBnClickedRunFullTest()
{
    if (!m_pDriver) return;

    std::vector<double> wavelengths = GetSelectedWavelengths();
    std::vector<int> channels = GetSelectedChannels();

    ReferenceConfig refCfg;
    refCfg.useOverride = (m_checkOverride.GetCheck() == BST_CHECKED);
    CString ilStr, lenStr;
    m_editILValue.GetWindowText(ilStr);
    m_editLengthValue.GetWindowText(lenStr);
    refCfg.ilValue = _ttof(ilStr);
    refCfg.lengthValue = _ttof(lenStr);

    AppendLog(_T("=== Running Full Test ==="));

    IEquipmentDriver* pDriver = m_pDriver;
    RunAsync(_T("Running full test..."), [pDriver, wavelengths, channels, refCfg]() -> WorkerResult*
    {
        WorkerResult* r = new WorkerResult();
        try
        {
            r->results = pDriver->RunFullTest(wavelengths, channels, true, refCfg);
            r->hasResults = true;
            r->success = true;
            CString msg;
            msg.Format(_T("=== Full Test Complete: %d results ==="), (int)r->results.size());
            r->logMessage = msg;
        }
        catch (const std::exception& e)
        {
            CString msg;
            msg.Format(_T("Full test error: %S"), e.what());
            r->logMessage = msg;
        }
        return r;
    });
}

// ---------------------------------------------------------------------------
// Clear Log
// ---------------------------------------------------------------------------

void CDriverTestAppDlg::OnBnClickedClearLog()
{
    m_editLog.SetWindowText(_T(""));
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

std::vector<double> CDriverTestAppDlg::GetSelectedWavelengths()
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

std::vector<int> CDriverTestAppDlg::GetSelectedChannels()
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

void CDriverTestAppDlg::PopulateResultsList(const std::vector<MeasurementResult>& results)
{
    m_listResults.DeleteAllItems();

    for (size_t i = 0; i < results.size(); ++i)
    {
        const MeasurementResult& r = results[i];
        CString chStr, wlStr, ilStr, rlStr;
        chStr.Format(_T("%d"), r.channel);
        wlStr.Format(_T("%.0f"), r.wavelength);
        ilStr.Format(_T("%.4f"), r.insertionLoss);
        rlStr.Format(_T("%.4f"), r.returnLoss);

        int idx = m_listResults.InsertItem(static_cast<int>(i), chStr);
        m_listResults.SetItemText(idx, 1, wlStr);
        m_listResults.SetItemText(idx, 2, ilStr);
        m_listResults.SetItemText(idx, 3, rlStr);
    }
}
