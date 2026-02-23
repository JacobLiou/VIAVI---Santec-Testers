#include "stdafx.h"
#include "DriverTestApp.h"
#include "DriverTestAppDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace EquipmentDriver;

// Store dialog pointer for log callback
static CDriverTestAppDlg* g_pDlg = nullptr;

BEGIN_MESSAGE_MAP(CDriverTestAppDlg, CDialogEx)
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
END_MESSAGE_MAP()

CDriverTestAppDlg::CDriverTestAppDlg(CWnd* pParent)
    : CDialogEx(IDD_DRIVERTESTAPP_DIALOG, pParent)
    , m_pDriver(nullptr)
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

    // Setup log callback to route driver logs to the UI
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

    // Wavelength defaults
    m_check1310.SetCheck(BST_CHECKED);
    m_check1550.SetCheck(BST_CHECKED);

    // Channel defaults
    m_editChFrom.SetWindowText(_T("1"));
    m_editChTo.SetWindowText(_T("12"));

    // ORL method combo
    m_comboOrlMethod.AddString(_T("Integration (1)"));
    m_comboOrlMethod.AddString(_T("Discrete (2)"));
    m_comboOrlMethod.SetCurSel(1); // Discrete by default

    // ORL origin combo
    m_comboOrlOrigin.AddString(_T("A+B from DUT start (1)"));
    m_comboOrlOrigin.AddString(_T("A+B from DUT end (2)"));
    m_comboOrlOrigin.AddString(_T("A start / B end (3)"));
    m_comboOrlOrigin.SetCurSel(0);

    // ORL offsets
    m_editAOffset.SetWindowText(_T("-0.5"));
    m_editBOffset.SetWindowText(_T("0.5"));

    // Reference defaults
    m_checkOverride.SetCheck(BST_CHECKED);
    m_editILValue.SetWindowText(_T("0.1"));
    m_editLengthValue.SetWindowText(_T("3.0"));

    // Setup results list columns
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
// Log handling
// ---------------------------------------------------------------------------

LRESULT CDriverTestAppDlg::OnLogMessage(WPARAM wParam, LPARAM lParam)
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
// Connection
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

        AppendLog(_T("Connecting..."));
        UpdateStatus(_T("Connecting..."));

        if (m_pDriver->Connect())
        {
            UpdateStatus(_T("Connected"));
            EnableControls(true);
            AppendLog(_T("Connection successful."));
        }
        else
        {
            UpdateStatus(_T("Failed"));
            AppendLog(_T("Connection failed."));
        }
    }
    catch (const std::exception& e)
    {
        CString msg;
        msg.Format(_T("Connection error: %S"), e.what());
        AppendLog(msg);
        UpdateStatus(_T("Error"));
    }
}

void CDriverTestAppDlg::OnBnClickedDisconnect()
{
    if (m_pDriver)
    {
        m_pDriver->Disconnect();
        AppendLog(_T("Disconnected."));
    }
    UpdateStatus(_T("Disconnected"));
    EnableControls(false);
}

void CDriverTestAppDlg::OnBnClickedInitialize()
{
    if (!m_pDriver) return;
    try
    {
        if (m_pDriver->Initialize())
            AppendLog(_T("Initialization successful."));
        else
            AppendLog(_T("Initialization failed."));
    }
    catch (const std::exception& e)
    {
        CString msg;
        msg.Format(_T("Init error: %S"), e.what());
        AppendLog(msg);
    }
}

// ---------------------------------------------------------------------------
// ORL Configuration (VIAVI specific)
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

    try
    {
        ORLConfig cfg;
        cfg.channel = 1;
        cfg.method = static_cast<ORLMethod>(m_comboOrlMethod.GetCurSel() + 1);
        cfg.origin = static_cast<ORLOrigin>(m_comboOrlOrigin.GetCurSel() + 1);
        cfg.aOffset = _ttof(aStr);
        cfg.bOffset = _ttof(bStr);

        viavi->ConfigureORL(cfg);
        AppendLog(_T("ORL configuration applied."));
    }
    catch (const std::exception& e)
    {
        CString msg;
        msg.Format(_T("ORL config error: %S"), e.what());
        AppendLog(msg);
    }
}

// ---------------------------------------------------------------------------
// Reference
// ---------------------------------------------------------------------------

void CDriverTestAppDlg::OnBnClickedTakeReference()
{
    if (!m_pDriver) return;

    try
    {
        // Configure wavelengths and channels first
        m_pDriver->ConfigureWavelengths(GetSelectedWavelengths());
        m_pDriver->ConfigureChannels(GetSelectedChannels());

        ReferenceConfig cfg;
        cfg.useOverride = (m_checkOverride.GetCheck() == BST_CHECKED);

        CString ilStr, lenStr;
        m_editILValue.GetWindowText(ilStr);
        m_editLengthValue.GetWindowText(lenStr);
        cfg.ilValue = _ttof(ilStr);
        cfg.lengthValue = _ttof(lenStr);

        AppendLog(_T("Taking reference..."));
        if (m_pDriver->TakeReference(cfg))
            AppendLog(_T("Reference completed."));
        else
            AppendLog(_T("Reference FAILED."));
    }
    catch (const std::exception& e)
    {
        CString msg;
        msg.Format(_T("Reference error: %S"), e.what());
        AppendLog(msg);
    }
}

// ---------------------------------------------------------------------------
// Measurement
// ---------------------------------------------------------------------------

void CDriverTestAppDlg::OnBnClickedTakeMeasurement()
{
    if (!m_pDriver) return;
    try
    {
        AppendLog(_T("Taking measurement..."));
        if (m_pDriver->TakeMeasurement())
            AppendLog(_T("Measurement completed."));
        else
            AppendLog(_T("Measurement FAILED."));
    }
    catch (const std::exception& e)
    {
        CString msg;
        msg.Format(_T("Measurement error: %S"), e.what());
        AppendLog(msg);
    }
}

// ---------------------------------------------------------------------------
// Get Results
// ---------------------------------------------------------------------------

void CDriverTestAppDlg::OnBnClickedGetResults()
{
    if (!m_pDriver) return;
    try
    {
        std::vector<MeasurementResult> results = m_pDriver->GetResults();
        PopulateResultsList(results);
        CString msg;
        msg.Format(_T("Retrieved %d results."), (int)results.size());
        AppendLog(msg);
    }
    catch (const std::exception& e)
    {
        CString msg;
        msg.Format(_T("Get results error: %S"), e.what());
        AppendLog(msg);
    }
}

// ---------------------------------------------------------------------------
// Run Full Test
// ---------------------------------------------------------------------------

void CDriverTestAppDlg::OnBnClickedRunFullTest()
{
    if (!m_pDriver) return;
    try
    {
        ReferenceConfig refCfg;
        refCfg.useOverride = (m_checkOverride.GetCheck() == BST_CHECKED);

        CString ilStr, lenStr;
        m_editILValue.GetWindowText(ilStr);
        m_editLengthValue.GetWindowText(lenStr);
        refCfg.ilValue = _ttof(ilStr);
        refCfg.lengthValue = _ttof(lenStr);

        AppendLog(_T("=== Running Full Test ==="));
        std::vector<MeasurementResult> results = m_pDriver->RunFullTest(
            GetSelectedWavelengths(),
            GetSelectedChannels(),
            true,
            refCfg);

        PopulateResultsList(results);

        CString msg;
        msg.Format(_T("=== Full Test Complete: %d results ==="), (int)results.size());
        AppendLog(msg);
    }
    catch (const std::exception& e)
    {
        CString msg;
        msg.Format(_T("Full test error: %S"), e.what());
        AppendLog(msg);
    }
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
