#include "stdafx.h"
#include "PCTMaxApp.h"
#include "PCTMaxAppDlg.h"
#include <commdlg.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// ---------------------------------------------------------------------------
// Log callbacks
// ---------------------------------------------------------------------------

static CPCTMaxAppDlg* g_pDlg = nullptr;

static CString Utf8ToWide(const char* utf8)
{
    if (!utf8 || !*utf8) return CString();
    int len = ::MultiByteToWideChar(CP_UTF8, 0, utf8, -1, nullptr, 0);
    if (len <= 0) return CString();
    CString result;
    ::MultiByteToWideChar(CP_UTF8, 0, utf8, -1, result.GetBuffer(len), len);
    result.ReleaseBuffer();
    return result;
}

static void WINAPI PCTLogCallback(int level, const char* source, const char* message)
{
    if (g_pDlg && ::IsWindow(g_pDlg->GetSafeHwnd()))
    {
        static const char* levelNames[] = { "DEBUG", "INFO", "WARN", "ERROR" };
        const char* lvl = (level >= 0 && level <= 3) ? levelNames[level] : "???";
        CString* pMsg = new CString();
        pMsg->Format(_T("[PCT][%s] %s"), (LPCTSTR)Utf8ToWide(lvl),
                     (LPCTSTR)Utf8ToWide(message));
        g_pDlg->PostMessage(WM_LOG_MESSAGE, 0, (LPARAM)pMsg);
    }
}

static void WINAPI OSW1LogCallback(int level, const char* source, const char* message)
{
    if (g_pDlg && ::IsWindow(g_pDlg->GetSafeHwnd()))
    {
        static const char* levelNames[] = { "DEBUG", "INFO", "WARN", "ERROR" };
        const char* lvl = (level >= 0 && level <= 3) ? levelNames[level] : "???";
        CString* pMsg = new CString();
        pMsg->Format(_T("[OSW1][%s] %s"), (LPCTSTR)Utf8ToWide(lvl),
                     (LPCTSTR)Utf8ToWide(message));
        g_pDlg->PostMessage(WM_LOG_MESSAGE, 0, (LPARAM)pMsg);
    }
}

static void WINAPI OSW2LogCallback(int level, const char* source, const char* message)
{
    if (g_pDlg && ::IsWindow(g_pDlg->GetSafeHwnd()))
    {
        static const char* levelNames[] = { "DEBUG", "INFO", "WARN", "ERROR" };
        const char* lvl = (level >= 0 && level <= 3) ? levelNames[level] : "???";
        CString* pMsg = new CString();
        pMsg->Format(_T("[OSW2][%s] %s"), (LPCTSTR)Utf8ToWide(lvl),
                     (LPCTSTR)Utf8ToWide(message));
        g_pDlg->PostMessage(WM_LOG_MESSAGE, 0, (LPARAM)pMsg);
    }
}

// ---------------------------------------------------------------------------
// Message map
// ---------------------------------------------------------------------------

BEGIN_MESSAGE_MAP(CPCTMaxAppDlg, CDialogEx)
    ON_WM_CLOSE()
    ON_BN_CLICKED(IDC_BTN_CONNECT, &CPCTMaxAppDlg::OnBnClickedConnect)
    ON_BN_CLICKED(IDC_BTN_DISCONNECT, &CPCTMaxAppDlg::OnBnClickedDisconnect)
    ON_BN_CLICKED(IDC_BTN_ZEROING, &CPCTMaxAppDlg::OnBnClickedZeroing)
    ON_BN_CLICKED(IDC_BTN_MEASURE, &CPCTMaxAppDlg::OnBnClickedMeasure)
    ON_BN_CLICKED(IDC_BTN_CONTINUOUS, &CPCTMaxAppDlg::OnBnClickedContinuous)
    ON_BN_CLICKED(IDC_BTN_STOP, &CPCTMaxAppDlg::OnBnClickedStop)
    ON_BN_CLICKED(IDC_BTN_CLEAR_LOG, &CPCTMaxAppDlg::OnBnClickedClearLog)
    ON_BN_CLICKED(IDC_CHECK_OVERRIDE, &CPCTMaxAppDlg::OnBnClickedOverride)
    ON_BN_CLICKED(IDC_BTN_SCREENSHOT, &CPCTMaxAppDlg::OnBnClickedScreenshot)
    ON_BN_CLICKED(IDC_BTN_SAVE_CSV, &CPCTMaxAppDlg::OnBnClickedSaveCsv)
    ON_MESSAGE(WM_LOG_MESSAGE, &CPCTMaxAppDlg::OnLogMessage)
    ON_MESSAGE(WM_WORKER_DONE, &CPCTMaxAppDlg::OnWorkerDone)
END_MESSAGE_MAP()

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

CPCTMaxAppDlg::CPCTMaxAppDlg(CWnd* pParent)
    : CDialogEx(IDD_PCTMAXAPP_DIALOG, pParent)
    , m_bBusy(false)
    , m_bStopRequested(false)
    , m_bPctConnected(false)
    , m_bOswConnected(false)
    , m_bOsw2Connected(false)
{
}

CPCTMaxAppDlg::~CPCTMaxAppDlg()
{
    g_pDlg = nullptr;
}

void CPCTMaxAppDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT_PCT_DLL, m_editPctDll);
    DDX_Control(pDX, IDC_EDIT_OSW_DLL, m_editOswDll);
    DDX_Control(pDX, IDC_EDIT_IP, m_editIP);
    DDX_Control(pDX, IDC_EDIT_PCT_PORT, m_editPctPort);
    DDX_Control(pDX, IDC_EDIT_OSW_PORT, m_editOswPort);
    DDX_Control(pDX, IDC_EDIT_OSW2_PORT, m_editOsw2Port);
    DDX_Control(pDX, IDC_EDIT_CH_FROM, m_editChFrom);
    DDX_Control(pDX, IDC_EDIT_CH_TO, m_editChTo);
    DDX_Control(pDX, IDC_CHECK_1310, m_check1310);
    DDX_Control(pDX, IDC_CHECK_1550, m_check1550);
    DDX_Control(pDX, IDC_EDIT_OSW_DEVICE_NUM, m_editOswDeviceNum);
    DDX_Control(pDX, IDC_EDIT_OSW2_DEVICE_NUM, m_editOsw2DeviceNum);
    DDX_Control(pDX, IDC_RADIO_OSW1_ONLY, m_radioOsw1Only);
    DDX_Control(pDX, IDC_RADIO_OSW_BOTH, m_radioOswBoth);
    DDX_Control(pDX, IDC_CHECK_OVERRIDE, m_checkOverride);
    DDX_Control(pDX, IDC_EDIT_IL_VALUE, m_editILValue);
    DDX_Control(pDX, IDC_EDIT_LENGTH_VALUE, m_editLengthValue);
    DDX_Control(pDX, IDC_LIST_RESULTS, m_listResults);
    DDX_Control(pDX, IDC_EDIT_LOG, m_editLog);
}

// ---------------------------------------------------------------------------
// Initialization
// ---------------------------------------------------------------------------

BOOL CPCTMaxAppDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    g_pDlg = this;

    // Defaults
    m_editPctDll.SetWindowText(_T("UDL.ViaviPCT.dll"));
    m_editOswDll.SetWindowText(_T("UDL.ViaviOSW.dll"));
    m_editIP.SetWindowText(_T("10.14.20.91"));
    m_editPctPort.SetWindowText(_T("8301"));
    m_editOswPort.SetWindowText(_T("8201"));
    m_editOsw2Port.SetWindowText(_T("8202"));

    m_editChFrom.SetWindowText(_T("1"));
    m_editChTo.SetWindowText(_T("8"));
    m_check1310.SetCheck(BST_CHECKED);
    m_check1550.SetCheck(BST_CHECKED);
    m_editOswDeviceNum.SetWindowText(_T("1"));
    m_editOsw2DeviceNum.SetWindowText(_T("1"));

    m_radioOsw1Only.SetCheck(BST_CHECKED);

    m_checkOverride.SetCheck(BST_CHECKED);
    m_editILValue.SetWindowText(_T("0.1"));
    m_editLengthValue.SetWindowText(_T("3.0"));

    // Results list columns
    m_listResults.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_listResults.InsertColumn(0, _T("CH"), LVCFMT_CENTER, 35);
    m_listResults.InsertColumn(1, _T("WL (nm)"), LVCFMT_CENTER, 60);
    m_listResults.InsertColumn(2, _T("IL (dB)"), LVCFMT_RIGHT, 70);
    m_listResults.InsertColumn(3, _T("ORL (dB)"), LVCFMT_RIGHT, 70);
    m_listResults.InsertColumn(4, _T("Zone1 (dB)"), LVCFMT_RIGHT, 75);
    m_listResults.InsertColumn(5, _T("Zone2 (dB)"), LVCFMT_RIGHT, 75);
    m_listResults.InsertColumn(6, _T("Length (m)"), LVCFMT_RIGHT, 70);
    m_listResults.InsertColumn(7, _T("Power (dBm)"), LVCFMT_RIGHT, 80);

    EnableControls();
    AppendLog(_T("PCTMax Control started. MAP 300 | PCTmax style."));
    AppendLog(_T("1) Enter IP address of MAP chassis"));
    AppendLog(_T("2) Click 'Connect All' to load DLLs and connect PCT + OSW"));
    AppendLog(_T("3) Run Zeroing, then Measure"));

    return TRUE;
}

void CPCTMaxAppDlg::OnOK() {}

void CPCTMaxAppDlg::OnCancel()
{
    if (m_bBusy)
    {
        MessageBox(_T("An operation is in progress. Please wait."),
                   _T("Busy"), MB_OK | MB_ICONINFORMATION);
        return;
    }
    CDialogEx::OnCancel();
}

void CPCTMaxAppDlg::OnClose()
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
// Connect All -- load DLLs and connect both PCT and OSW in one click
// ---------------------------------------------------------------------------

void CPCTMaxAppDlg::OnBnClickedConnect()
{
    CString pctDll, oswDll, ip, pctPortStr, oswPortStr, osw2PortStr;
    m_editPctDll.GetWindowText(pctDll);
    m_editOswDll.GetWindowText(oswDll);
    m_editIP.GetWindowText(ip);
    m_editPctPort.GetWindowText(pctPortStr);
    m_editOswPort.GetWindowText(oswPortStr);
    m_editOsw2Port.GetWindowText(osw2PortStr);

    if (ip.IsEmpty()) { AppendLog(_T("Please enter IP address.")); return; }

    int pctPort = _ttoi(pctPortStr);
    int oswPort = _ttoi(oswPortStr);
    int osw2Port = _ttoi(osw2PortStr);
    if (pctPort <= 0) pctPort = 8301;
    if (oswPort <= 0) oswPort = 8201;
    if (osw2Port <= 0) osw2Port = 8202;

    bool dualOsw = IsDualOswMode();

    // Load PCT DLL
    if (!m_pctLoader.IsDllLoaded())
    {
        CStringA pathA(pctDll);
        if (m_pctLoader.LoadDll(pathA.GetString()))
        {
            m_pctLoader.SetLogCallback(PCTLogCallback);
            AppendLog(_T("[PCT] DLL loaded."));
        }
        else
        {
            AppendLog(_T("[PCT] Failed to load DLL."));
            return;
        }
    }

    // Load OSW DLL (shared for OSW1)
    if (!m_oswLoader.IsDllLoaded())
    {
        CStringA pathA(oswDll);
        if (m_oswLoader.LoadDll(pathA.GetString()))
        {
            m_oswLoader.SetLogCallback(OSW1LogCallback);
            AppendLog(_T("[OSW1] DLL loaded."));
        }
        else
        {
            AppendLog(_T("[OSW1] Failed to load DLL. Continuing with PCT only."));
        }
    }

    // Load OSW2 DLL (same DLL, separate instance)
    if (dualOsw && !m_osw2Loader.IsDllLoaded())
    {
        CStringA pathA(oswDll);
        if (m_osw2Loader.LoadDll(pathA.GetString()))
        {
            m_osw2Loader.SetLogCallback(OSW2LogCallback);
            AppendLog(_T("[OSW2] DLL loaded."));
        }
        else
        {
            AppendLog(_T("[OSW2] Failed to load DLL."));
        }
    }

    // Create drivers (TCP)
    CStringA ipA(ip);
    std::string ipStr(ipA.GetString());

    if (m_pctLoader.GetDriverHandle())
    {
        m_pctLoader.Disconnect();
        m_pctLoader.DestroyDriver();
    }
    m_bPctConnected = false;

    if (!m_pctLoader.CreateDriver("viavi", ipStr.c_str(), pctPort, 0))
    {
        AppendLog(_T("[PCT] Failed to create driver."));
        return;
    }

    if (m_oswLoader.IsDllLoaded())
    {
        if (m_oswLoader.GetDriverHandle())
        {
            m_oswLoader.Disconnect();
            m_oswLoader.DestroyDriver();
        }
        m_bOswConnected = false;
        m_oswLoader.CreateDriver(ipStr.c_str(), oswPort);
    }

    if (dualOsw && m_osw2Loader.IsDllLoaded())
    {
        if (m_osw2Loader.GetDriverHandle())
        {
            m_osw2Loader.Disconnect();
            m_osw2Loader.DestroyDriver();
        }
        m_bOsw2Connected = false;
        m_osw2Loader.CreateDriver(ipStr.c_str(), osw2Port);
    }

    CString statusMsg;
    if (dualOsw)
        statusMsg.Format(_T("Connecting to %s (PCT:%d, OSW1:%d, OSW2:%d)..."),
                         (LPCTSTR)ip, pctPort, oswPort, osw2Port);
    else
        statusMsg.Format(_T("Connecting to %s (PCT:%d, OSW1:%d)..."),
                         (LPCTSTR)ip, pctPort, oswPort);
    AppendLog(statusMsg);

    CViaviPCTDllLoader* pPct = &m_pctLoader;
    CViaviOSWDllLoader* pOsw = &m_oswLoader;
    CViaviOSWDllLoader* pOsw2 = &m_osw2Loader;
    bool* pPctConn = &m_bPctConnected;
    bool* pOswConn = &m_bOswConnected;
    bool* pOsw2Conn = &m_bOsw2Connected;
    bool osw1Loaded = m_oswLoader.IsDllLoaded();
    bool osw2Loaded = dualOsw && m_osw2Loader.IsDllLoaded();

    RunAsync(_T("Connecting..."),
        [pPct, pOsw, pOsw2, pPctConn, pOswConn, pOsw2Conn,
         osw1Loaded, osw2Loaded]() -> WorkerResult*
    {
        WorkerResult* r = new WorkerResult();
        CString log;

        // Connect PCT
        BOOL pctOk = pPct->Connect();
        if (pctOk)
        {
            *pPctConn = true;
            pPct->Initialize();
            log += _T("[PCT] Connected and initialized.\r\n");

            PCTDeviceInfo info = {};
            if (pPct->GetDeviceInfo(&info))
            {
                CString infoMsg;
                infoMsg.Format(_T("[PCT] SN: %s  FW: %s  Desc: %s\r\n"),
                               (LPCTSTR)Utf8ToWide(info.serialNumber),
                               (LPCTSTR)Utf8ToWide(info.firmwareVersion),
                               (LPCTSTR)Utf8ToWide(info.description));
                log += infoMsg;
            }
        }
        else
        {
            log += _T("[PCT] Connection FAILED.\r\n");
        }

        // Connect OSW1
        if (osw1Loaded && pOsw->GetDriverHandle())
        {
            BOOL oswOk = pOsw->Connect();
            if (oswOk)
            {
                *pOswConn = true;
                log += _T("[OSW1] Connected.\r\n");

                OSWDeviceInfo oswInfo = {};
                if (pOsw->GetDeviceInfo(&oswInfo))
                {
                    CString infoMsg;
                    infoMsg.Format(_T("[OSW1] SN: %s  FW: %s\r\n"),
                                   (LPCTSTR)Utf8ToWide(oswInfo.serialNumber),
                                   (LPCTSTR)Utf8ToWide(oswInfo.firmwareVersion));
                    log += infoMsg;
                }

                int devCount = pOsw->GetDeviceCount();
                CString devMsg;
                devMsg.Format(_T("[OSW1] Device count: %d"), devCount);
                log += devMsg;
                for (int d = 1; d <= devCount; ++d)
                {
                    OSWSwitchInfo swInfo = {};
                    if (pOsw->GetSwitchInfo(d, &swInfo))
                    {
                        CString swMsg;
                        swMsg.Format(_T("\r\n[OSW1] Dev %d: %d ch, current=%d"),
                                     d, swInfo.channelCount, swInfo.currentChannel);
                        log += swMsg;
                    }
                }
                log += _T("\r\n");
            }
            else
            {
                log += _T("[OSW1] Connection FAILED.\r\n");
            }
        }

        // Connect OSW2
        if (osw2Loaded && pOsw2->GetDriverHandle())
        {
            BOOL osw2Ok = pOsw2->Connect();
            if (osw2Ok)
            {
                *pOsw2Conn = true;
                log += _T("[OSW2] Connected.\r\n");

                OSWDeviceInfo osw2Info = {};
                if (pOsw2->GetDeviceInfo(&osw2Info))
                {
                    CString infoMsg;
                    infoMsg.Format(_T("[OSW2] SN: %s  FW: %s\r\n"),
                                   (LPCTSTR)Utf8ToWide(osw2Info.serialNumber),
                                   (LPCTSTR)Utf8ToWide(osw2Info.firmwareVersion));
                    log += infoMsg;
                }

                int devCount = pOsw2->GetDeviceCount();
                CString devMsg;
                devMsg.Format(_T("[OSW2] Device count: %d"), devCount);
                log += devMsg;
                for (int d = 1; d <= devCount; ++d)
                {
                    OSWSwitchInfo swInfo = {};
                    if (pOsw2->GetSwitchInfo(d, &swInfo))
                    {
                        CString swMsg;
                        swMsg.Format(_T("\r\n[OSW2] Dev %d: %d ch, current=%d"),
                                     d, swInfo.channelCount, swInfo.currentChannel);
                        log += swMsg;
                    }
                }
                log += _T("\r\n");
            }
            else
            {
                log += _T("[OSW2] Connection FAILED.\r\n");
            }
        }

        r->success = *pPctConn;
        r->logMessage = log;
        if (*pPctConn && *pOswConn && *pOsw2Conn)
            r->statusText = _T("Status: PCT + OSW1 + OSW2 Connected");
        else if (*pPctConn && *pOswConn)
            r->statusText = _T("Status: PCT + OSW1 Connected");
        else if (*pPctConn)
            r->statusText = _T("Status: PCT Connected (OSW not available)");
        else
            r->statusText = _T("Status: Connection Failed");
        return r;
    });
}

void CPCTMaxAppDlg::OnBnClickedDisconnect()
{
    CViaviPCTDllLoader* pPct = &m_pctLoader;
    CViaviOSWDllLoader* pOsw = &m_oswLoader;
    CViaviOSWDllLoader* pOsw2 = &m_osw2Loader;
    bool* pPctConn = &m_bPctConnected;
    bool* pOswConn = &m_bOswConnected;
    bool* pOsw2Conn = &m_bOsw2Connected;

    RunAsync(_T("Disconnecting..."),
        [pPct, pOsw, pOsw2, pPctConn, pOswConn, pOsw2Conn]() -> WorkerResult*
    {
        WorkerResult* r = new WorkerResult();
        if (pOsw2->GetDriverHandle())
        {
            pOsw2->Disconnect();
            pOsw2->DestroyDriver();
        }
        *pOsw2Conn = false;

        if (pOsw->GetDriverHandle())
        {
            pOsw->Disconnect();
            pOsw->DestroyDriver();
        }
        *pOswConn = false;

        if (pPct->GetDriverHandle())
        {
            pPct->Disconnect();
            pPct->DestroyDriver();
        }
        *pPctConn = false;

        r->success = true;
        r->logMessage = _T("Disconnected all devices.");
        r->statusText = _T("Status: Not Connected");
        return r;
    });
}

// ---------------------------------------------------------------------------
// Zeroing (Reference)
// ---------------------------------------------------------------------------

void CPCTMaxAppDlg::OnBnClickedZeroing()
{
    if (!m_pctLoader.GetDriverHandle() || !m_bPctConnected) return;

    std::vector<double> wavelengths = GetSelectedWavelengths();
    std::vector<int> channels = GetSelectedChannels();
    int oswDeviceNum = GetOswDeviceNum();
    int osw2DeviceNum = GetOsw2DeviceNum();
    bool useOsw1 = m_bOswConnected && (channels.size() > 1);
    bool useOsw2 = IsDualOswMode() && m_bOsw2Connected && (channels.size() > 1);

    BOOL bOverride = (m_checkOverride.GetCheck() == BST_CHECKED);
    CString ilStr, lenStr;
    m_editILValue.GetWindowText(ilStr);
    m_editLengthValue.GetWindowText(lenStr);
    double ilValue = _ttof(ilStr);
    double lengthValue = _ttof(lenStr);

    AppendLog(_T("=== Zeroing (Reference) Started ==="));

    CViaviPCTDllLoader* pPct = &m_pctLoader;
    CViaviOSWDllLoader* pOsw = &m_oswLoader;
    CViaviOSWDllLoader* pOsw2 = &m_osw2Loader;
    std::atomic<bool>* pStop = &m_bStopRequested;
    m_bStopRequested = false;

    RunAsync(_T("Zeroing..."),
        [pPct, pOsw, pOsw2, wavelengths, channels, oswDeviceNum, osw2DeviceNum,
         useOsw1, useOsw2, bOverride, ilValue, lengthValue, pStop]() -> WorkerResult*
    {
        WorkerResult* r = new WorkerResult();
        CString log;
        bool useOsw = useOsw1 || useOsw2;

        try
        {
            std::vector<double> wl = wavelengths;
            pPct->ConfigureWavelengths(wl.data(), (int)wl.size());

            if (!useOsw)
            {
                std::vector<int> ch = channels;
                pPct->ConfigureChannels(ch.data(), (int)ch.size());
                BOOL ok = pPct->TakeReference(bOverride, ilValue, lengthValue);
                log.Format(_T("Reference %s (no OSW switching)."),
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
                    chLog.Format(_T("  CH%d: switching..."), ch);
                    log += chLog;

                    if (useOsw1)
                    {
                        pOsw->SwitchChannel(oswDeviceNum, ch);
                        pOsw->WaitForIdle(5000);
                    }
                    if (useOsw2)
                    {
                        pOsw2->SwitchChannel(osw2DeviceNum, ch);
                        pOsw2->WaitForIdle(5000);
                    }

                    std::vector<int> singleCh = { ch };
                    pPct->ConfigureChannels(singleCh.data(), 1);

                    BOOL ok = pPct->TakeReference(bOverride, ilValue, lengthValue);
                    log += ok ? _T(" OK\r\n") : _T(" FAILED\r\n");
                    if (!ok) allOk = false;
                }
                r->success = allOk;
            }

            r->logMessage = _T("=== Zeroing Complete ===\r\n") + log;
            r->statusText = r->success
                ? _T("Status: Zeroing Done - Ready to Measure")
                : _T("Status: Zeroing Failed");
        }
        catch (...)
        {
            r->logMessage = _T("Zeroing error.");
            r->statusText = _T("Status: Zeroing Error");
        }
        return r;
    });
}

// ---------------------------------------------------------------------------
// Measure
// ---------------------------------------------------------------------------

void CPCTMaxAppDlg::OnBnClickedMeasure()
{
    if (!m_pctLoader.GetDriverHandle() || !m_bPctConnected) return;

    std::vector<double> wavelengths = GetSelectedWavelengths();
    std::vector<int> channels = GetSelectedChannels();
    int oswDeviceNum = GetOswDeviceNum();
    int osw2DeviceNum = GetOsw2DeviceNum();
    bool useOsw1 = m_bOswConnected && (channels.size() > 1);
    bool useOsw2 = IsDualOswMode() && m_bOsw2Connected && (channels.size() > 1);

    AppendLog(_T("=== Measurement Started ==="));

    CViaviPCTDllLoader* pPct = &m_pctLoader;
    CViaviOSWDllLoader* pOsw = &m_oswLoader;
    CViaviOSWDllLoader* pOsw2 = &m_osw2Loader;
    std::atomic<bool>* pStop = &m_bStopRequested;
    m_bStopRequested = false;

    RunAsync(_T("Measuring..."),
        [pPct, pOsw, pOsw2, wavelengths, channels,
         oswDeviceNum, osw2DeviceNum, useOsw1, useOsw2, pStop]() -> WorkerResult*
    {
        WorkerResult* r = new WorkerResult();
        CString log;
        bool useOsw = useOsw1 || useOsw2;

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
                msg.Format(_T("Measurement %s. %d result(s)."),
                           ok ? _T("OK") : _T("FAILED"), (int)allResults.size());
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
                    chLog.Format(_T("  CH%d: switching..."), ch);
                    log += chLog;

                    if (useOsw1)
                    {
                        pOsw->SwitchChannel(oswDeviceNum, ch);
                        pOsw->WaitForIdle(5000);
                    }
                    if (useOsw2)
                    {
                        pOsw2->SwitchChannel(osw2DeviceNum, ch);
                        pOsw2->WaitForIdle(5000);
                    }

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
                        msg.Format(_T(" %d result(s)\r\n"), count);
                        log += msg;
                    }
                    else
                    {
                        log += _T(" FAILED\r\n");
                        allOk = false;
                    }
                }
                r->success = allOk;
            }

            r->results = allResults;
            r->hasResults = true;

            CString summary;
            summary.Format(_T("=== Measurement Complete: %d result(s) ==="),
                           (int)allResults.size());
            r->logMessage = summary + _T("\r\n") + log;
            r->statusText = r->success
                ? _T("Status: Measurement Done")
                : _T("Status: Measurement Failed");
        }
        catch (...)
        {
            r->logMessage = _T("Measurement error.");
            r->statusText = _T("Status: Measurement Error");
        }
        return r;
    });
}

// ---------------------------------------------------------------------------
// Continuous (Zeroing + Measure loop)
// ---------------------------------------------------------------------------

void CPCTMaxAppDlg::OnBnClickedContinuous()
{
    if (!m_pctLoader.GetDriverHandle() || !m_bPctConnected) return;

    std::vector<double> wavelengths = GetSelectedWavelengths();
    std::vector<int> channels = GetSelectedChannels();
    int oswDeviceNum = GetOswDeviceNum();
    int osw2DeviceNum = GetOsw2DeviceNum();
    bool useOsw1 = m_bOswConnected && (channels.size() > 1);
    bool useOsw2 = IsDualOswMode() && m_bOsw2Connected && (channels.size() > 1);

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
    CViaviOSWDllLoader* pOsw2 = &m_osw2Loader;
    std::atomic<bool>* pStop = &m_bStopRequested;

    RunAsync(_T("Continuous Test..."),
        [pPct, pOsw, pOsw2, wavelengths, channels,
         oswDeviceNum, osw2DeviceNum, useOsw1, useOsw2,
         bOverride, ilValue, lengthValue, pStop]() -> WorkerResult*
    {
        WorkerResult* r = new WorkerResult();
        CString log;
        int iteration = 0;
        std::vector<PCTMeasurementResult> allResults;
        bool useOsw = useOsw1 || useOsw2;

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

                // Zeroing phase
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
                        if (useOsw1)
                        {
                            pOsw->SwitchChannel(oswDeviceNum, ch);
                            pOsw->WaitForIdle(5000);
                        }
                        if (useOsw2)
                        {
                            pOsw2->SwitchChannel(osw2DeviceNum, ch);
                            pOsw2->WaitForIdle(5000);
                        }
                        std::vector<int> singleCh = { ch };
                        pPct->ConfigureChannels(singleCh.data(), 1);
                        pPct->TakeReference(bOverride, ilValue, lengthValue);
                    }
                }

                if (pStop->load()) break;
                log += _T("  Zeroing done.\r\n");

                // Measurement phase
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
                        if (useOsw1)
                        {
                            pOsw->SwitchChannel(oswDeviceNum, ch);
                            pOsw->WaitForIdle(5000);
                        }
                        if (useOsw2)
                        {
                            pOsw2->SwitchChannel(osw2DeviceNum, ch);
                            pOsw2->WaitForIdle(5000);
                        }
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
                measLog.Format(_T("  Measured: %d result(s).\r\n"),
                               (int)allResults.size());
                log += measLog;
            }

            r->results = allResults;
            r->hasResults = true;
            r->success = true;

            CString summary;
            summary.Format(_T("=== Continuous Test Stopped (%d iterations) ==="),
                           iteration);
            r->logMessage = summary + _T("\r\n") + log;
            r->statusText = _T("Status: Continuous Test Done");
        }
        catch (...)
        {
            r->logMessage = _T("Continuous test error.");
            r->statusText = _T("Status: Error");
        }
        return r;
    });
}

void CPCTMaxAppDlg::OnBnClickedStop()
{
    m_bStopRequested = true;
    m_pctLoader.AbortMeasurement();
    AppendLog(_T("Stop requested - aborting measurement."));
}

void CPCTMaxAppDlg::OnBnClickedOverride()
{
    BOOL enabled = (m_checkOverride.GetCheck() == BST_CHECKED);
    m_editILValue.EnableWindow(enabled);
    m_editLengthValue.EnableWindow(enabled);
}

// ---------------------------------------------------------------------------
// Screenshot (Primary Feature)
// ---------------------------------------------------------------------------

int CPCTMaxAppDlg::GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
    UINT num = 0, size = 0;
    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0) return -1;

    Gdiplus::ImageCodecInfo* pInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
    if (!pInfo) return -1;

    Gdiplus::GetImageEncoders(num, size, pInfo);
    for (UINT i = 0; i < num; ++i)
    {
        if (wcscmp(pInfo[i].MimeType, format) == 0)
        {
            *pClsid = pInfo[i].Clsid;
            free(pInfo);
            return i;
        }
    }
    free(pInfo);
    return -1;
}

bool CPCTMaxAppDlg::SaveWindowAsPng(HWND hWnd, const CString& filePath)
{
    CRect rc;
    ::GetWindowRect(hWnd, &rc);
    int w = rc.Width();
    int h = rc.Height();

    HDC hScreen = ::GetDC(NULL);
    HDC hMemDC = ::CreateCompatibleDC(hScreen);
    HBITMAP hBmp = ::CreateCompatibleBitmap(hScreen, w, h);
    HGDIOBJ hOld = ::SelectObject(hMemDC, hBmp);

    ::PrintWindow(hWnd, hMemDC, 0);

    ::SelectObject(hMemDC, hOld);

    Gdiplus::Bitmap bitmap(hBmp, NULL);
    CLSID pngClsid;
    if (GetEncoderClsid(L"image/png", &pngClsid) < 0)
    {
        ::DeleteObject(hBmp);
        ::DeleteDC(hMemDC);
        ::ReleaseDC(NULL, hScreen);
        return false;
    }

    Gdiplus::Status status = bitmap.Save(filePath, &pngClsid, NULL);

    // Copy to clipboard
    if (::OpenClipboard(hWnd))
    {
        ::EmptyClipboard();
        ::SetClipboardData(CF_BITMAP, hBmp);
        ::CloseClipboard();
    }
    else
    {
        ::DeleteObject(hBmp);
    }

    ::DeleteDC(hMemDC);
    ::ReleaseDC(NULL, hScreen);
    return (status == Gdiplus::Ok);
}

void CPCTMaxAppDlg::OnBnClickedScreenshot()
{
    SYSTEMTIME st;
    GetLocalTime(&st);
    CString fileName;
    fileName.Format(_T("PCTMax_%04d%02d%02d_%02d%02d%02d.png"),
                    st.wYear, st.wMonth, st.wDay,
                    st.wHour, st.wMinute, st.wSecond);

    TCHAR exePath[MAX_PATH] = {};
    ::GetModuleFileName(NULL, exePath, MAX_PATH);
    ::PathRemoveFileSpec(exePath);

    CString fullPath;
    fullPath.Format(_T("%s\\%s"), exePath, (LPCTSTR)fileName);

    if (SaveWindowAsPng(GetSafeHwnd(), fullPath))
    {
        CString msg;
        msg.Format(_T("Screenshot saved: %s (also copied to clipboard)"), (LPCTSTR)fullPath);
        AppendLog(msg);
    }
    else
    {
        AppendLog(_T("Screenshot failed."));
    }
}

// ---------------------------------------------------------------------------
// Save CSV
// ---------------------------------------------------------------------------

void CPCTMaxAppDlg::OnBnClickedSaveCsv()
{
    int count = m_listResults.GetItemCount();
    if (count == 0)
    {
        AppendLog(_T("No results to save."));
        return;
    }

    CFileDialog dlg(FALSE, _T("csv"), _T("PCTMax_Results"),
                    OFN_OVERWRITEPROMPT, _T("CSV Files (*.csv)|*.csv||"));
    if (dlg.DoModal() != IDOK) return;

    CString path = dlg.GetPathName();
    CStdioFile file;
    if (!file.Open(path, CFile::modeCreate | CFile::modeWrite | CFile::typeText))
    {
        AppendLog(_T("Failed to create CSV file."));
        return;
    }

    file.WriteString(_T("CH,WL(nm),IL(dB),ORL(dB),Zone1(dB),Zone2(dB),Length(m),Power(dBm)\n"));

    for (int i = 0; i < count; ++i)
    {
        CString line;
        line.Format(_T("%s,%s,%s,%s,%s,%s,%s,%s\n"),
                    (LPCTSTR)m_listResults.GetItemText(i, 0),
                    (LPCTSTR)m_listResults.GetItemText(i, 1),
                    (LPCTSTR)m_listResults.GetItemText(i, 2),
                    (LPCTSTR)m_listResults.GetItemText(i, 3),
                    (LPCTSTR)m_listResults.GetItemText(i, 4),
                    (LPCTSTR)m_listResults.GetItemText(i, 5),
                    (LPCTSTR)m_listResults.GetItemText(i, 6),
                    (LPCTSTR)m_listResults.GetItemText(i, 7));
        file.WriteString(line);
    }
    file.Close();

    CString msg;
    msg.Format(_T("CSV saved: %s"), (LPCTSTR)path);
    AppendLog(msg);
}

// ---------------------------------------------------------------------------
// Async worker infrastructure
// ---------------------------------------------------------------------------

void CPCTMaxAppDlg::RunAsync(const CString& operationName,
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
            result->statusText = _T("Status: Error");
        }

        if (::IsWindow(hWnd))
            ::PostMessage(hWnd, WM_WORKER_DONE, 0, (LPARAM)result);
        else
            delete result;
    });
    worker.detach();
}

LRESULT CPCTMaxAppDlg::OnWorkerDone(WPARAM, LPARAM lParam)
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
// UI state management
// ---------------------------------------------------------------------------

void CPCTMaxAppDlg::SetBusy(bool busy, const CString& statusText)
{
    m_bBusy = busy;
    if (!statusText.IsEmpty())
        UpdateStatus(statusText);
    EnableControls();
}

void CPCTMaxAppDlg::EnableControls()
{
    bool pctConnected = m_bPctConnected;

    // Connection
    bool anyConnected = m_bPctConnected || m_bOswConnected || m_bOsw2Connected;
    GetDlgItem(IDC_EDIT_PCT_DLL)->EnableWindow(!m_bBusy && !m_bPctConnected);
    GetDlgItem(IDC_EDIT_OSW_DLL)->EnableWindow(!m_bBusy && !m_bOswConnected);
    GetDlgItem(IDC_EDIT_IP)->EnableWindow(!m_bBusy && !m_bPctConnected);
    GetDlgItem(IDC_EDIT_PCT_PORT)->EnableWindow(!m_bBusy && !m_bPctConnected);
    GetDlgItem(IDC_EDIT_OSW_PORT)->EnableWindow(!m_bBusy && !m_bOswConnected);
    GetDlgItem(IDC_EDIT_OSW2_PORT)->EnableWindow(!m_bBusy && !m_bOsw2Connected);
    GetDlgItem(IDC_BTN_CONNECT)->EnableWindow(!m_bBusy && !m_bPctConnected);
    GetDlgItem(IDC_BTN_DISCONNECT)->EnableWindow(!m_bBusy && anyConnected);

    // Setup
    GetDlgItem(IDC_EDIT_CH_FROM)->EnableWindow(!m_bBusy);
    GetDlgItem(IDC_EDIT_CH_TO)->EnableWindow(!m_bBusy);
    GetDlgItem(IDC_CHECK_1310)->EnableWindow(!m_bBusy);
    GetDlgItem(IDC_CHECK_1550)->EnableWindow(!m_bBusy);
    GetDlgItem(IDC_RADIO_OSW1_ONLY)->EnableWindow(!m_bBusy);
    GetDlgItem(IDC_RADIO_OSW_BOTH)->EnableWindow(!m_bBusy);
    GetDlgItem(IDC_EDIT_OSW_DEVICE_NUM)->EnableWindow(!m_bBusy);
    GetDlgItem(IDC_EDIT_OSW2_DEVICE_NUM)->EnableWindow(!m_bBusy);
    GetDlgItem(IDC_CHECK_OVERRIDE)->EnableWindow(!m_bBusy);

    BOOL overrideOn = (m_checkOverride.GetCheck() == BST_CHECKED);
    GetDlgItem(IDC_EDIT_IL_VALUE)->EnableWindow(!m_bBusy && overrideOn);
    GetDlgItem(IDC_EDIT_LENGTH_VALUE)->EnableWindow(!m_bBusy && overrideOn);

    // Actions
    GetDlgItem(IDC_BTN_ZEROING)->EnableWindow(!m_bBusy && pctConnected);
    GetDlgItem(IDC_BTN_MEASURE)->EnableWindow(!m_bBusy && pctConnected);
    GetDlgItem(IDC_BTN_CONTINUOUS)->EnableWindow(!m_bBusy && pctConnected);
    GetDlgItem(IDC_BTN_STOP)->EnableWindow(m_bBusy);

    // Bottom buttons
    GetDlgItem(IDC_BTN_SCREENSHOT)->EnableWindow(!m_bBusy);
    GetDlgItem(IDC_BTN_SAVE_CSV)->EnableWindow(!m_bBusy);
}

// ---------------------------------------------------------------------------
// Log / Status
// ---------------------------------------------------------------------------

LRESULT CPCTMaxAppDlg::OnLogMessage(WPARAM, LPARAM lParam)
{
    CString* pMsg = reinterpret_cast<CString*>(lParam);
    if (pMsg) { AppendLog(*pMsg); delete pMsg; }
    return 0;
}

void CPCTMaxAppDlg::AppendLog(const CString& text)
{
    CString current;
    m_editLog.GetWindowText(current);
    if (!current.IsEmpty()) current += _T("\r\n");
    current += text;
    m_editLog.SetWindowText(current);
    m_editLog.LineScroll(m_editLog.GetLineCount());
}

void CPCTMaxAppDlg::UpdateStatus(const CString& status)
{
    SetDlgItemText(IDC_STATIC_STATUS, status);
}

void CPCTMaxAppDlg::OnBnClickedClearLog()
{
    m_editLog.SetWindowText(_T(""));
}

// ---------------------------------------------------------------------------
// Results list
// ---------------------------------------------------------------------------

void CPCTMaxAppDlg::PopulateResultsList(const std::vector<PCTMeasurementResult>& results)
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
// Helpers
// ---------------------------------------------------------------------------

std::vector<double> CPCTMaxAppDlg::GetSelectedWavelengths()
{
    std::vector<double> wl;
    if (m_check1310.GetCheck() == BST_CHECKED) wl.push_back(1310.0);
    if (m_check1550.GetCheck() == BST_CHECKED) wl.push_back(1550.0);
    if (wl.empty())
    {
        AppendLog(_T("No wavelength selected, defaulting to 1310+1550."));
        wl.push_back(1310.0);
        wl.push_back(1550.0);
    }
    return wl;
}

std::vector<int> CPCTMaxAppDlg::GetSelectedChannels()
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

int CPCTMaxAppDlg::GetOswDeviceNum()
{
    CString str;
    m_editOswDeviceNum.GetWindowText(str);
    int num = _ttoi(str);
    return (num >= 1) ? num : 1;
}

int CPCTMaxAppDlg::GetOsw2DeviceNum()
{
    CString str;
    m_editOsw2DeviceNum.GetWindowText(str);
    int num = _ttoi(str);
    return (num >= 1) ? num : 1;
}

bool CPCTMaxAppDlg::IsDualOswMode()
{
    return (m_radioOswBoth.GetCheck() == BST_CHECKED);
}
