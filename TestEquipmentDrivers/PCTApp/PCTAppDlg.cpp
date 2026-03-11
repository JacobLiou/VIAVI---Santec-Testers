#include "stdafx.h"
#include "PCTApp.h"
#include "PCTAppDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// ---------------------------------------------------------------------------
// Global log callback
// ---------------------------------------------------------------------------

static CPCTAppDlg* g_pDlg = nullptr;

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

static void WINAPI PCT4LogCB(int level, const char* source, const char* message)
{
    if (g_pDlg && ::IsWindow(g_pDlg->GetSafeHwnd()))
    {
        static const char* levelNames[] = { "DEBUG", "INFO", "WARN", "ERROR" };
        const char* lvl = (level >= 0 && level <= 3) ? levelNames[level] : "???";
        CString* pMsg = new CString();
        pMsg->Format(_T("[%s][%s] %s"), (LPCTSTR)Utf8ToWide(lvl),
                     (LPCTSTR)Utf8ToWide(source), (LPCTSTR)Utf8ToWide(message));
        g_pDlg->PostMessage(WM_LOG_MESSAGE, 0, (LPARAM)pMsg);
    }
}

// ---------------------------------------------------------------------------
// Message map
// ---------------------------------------------------------------------------

BEGIN_MESSAGE_MAP(CPCTAppDlg, CDialogEx)
    ON_WM_CLOSE()
    ON_BN_CLICKED(IDC_BTN_LOAD, &CPCTAppDlg::OnBnClickedLoad)
    ON_BN_CLICKED(IDC_BTN_CONNECT, &CPCTAppDlg::OnBnClickedConnect)
    ON_BN_CLICKED(IDC_BTN_DISCONNECT, &CPCTAppDlg::OnBnClickedDisconnect)
    ON_BN_CLICKED(IDC_BTN_ZEROING, &CPCTAppDlg::OnBnClickedZeroing)
    ON_BN_CLICKED(IDC_BTN_MEASURE, &CPCTAppDlg::OnBnClickedMeasure)
    ON_BN_CLICKED(IDC_BTN_STOP, &CPCTAppDlg::OnBnClickedStop)
    ON_BN_CLICKED(IDC_BTN_CLEAR_LOG, &CPCTAppDlg::OnBnClickedClearLog)
    ON_MESSAGE(WM_LOG_MESSAGE, &CPCTAppDlg::OnLogMessage)
    ON_MESSAGE(WM_WORKER_DONE, &CPCTAppDlg::OnWorkerDone)
END_MESSAGE_MAP()

// ---------------------------------------------------------------------------
// Construction / Destruction
// ---------------------------------------------------------------------------

CPCTAppDlg::CPCTAppDlg(CWnd* pParent)
    : CDialogEx(IDD_PCTAPP_DIALOG, pParent)
    , m_bBusy(false)
    , m_bStopRequested(false)
    , m_bConnected(false)
{
}

CPCTAppDlg::~CPCTAppDlg()
{
    g_pDlg = nullptr;
}

void CPCTAppDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT_DLL_PATH, m_editDllPath);
    DDX_Control(pDX, IDC_COMBO_ADDR, m_comboAddr);
    DDX_Control(pDX, IDC_EDIT_PORT, m_editPort);
    DDX_Control(pDX, IDC_CHECK_1310, m_check1310);
    DDX_Control(pDX, IDC_CHECK_1550, m_check1550);
    DDX_Control(pDX, IDC_EDIT_OPM, m_editOpmIndex);
    DDX_Control(pDX, IDC_COMBO_CONN_MODE, m_comboConnMode);
    DDX_Control(pDX, IDC_EDIT_LAUNCH, m_editLaunchPort);
    DDX_Control(pDX, IDC_EDIT_CH_FROM, m_editChFrom);
    DDX_Control(pDX, IDC_EDIT_CH_TO, m_editChTo);
    DDX_Control(pDX, IDC_EDIT_AVG_TIME, m_editAvgTime);
    DDX_Control(pDX, IDC_LIST_RESULTS, m_listResults);
    DDX_Control(pDX, IDC_EDIT_LOG, m_editLog);
}

// ---------------------------------------------------------------------------
// OnInitDialog
// ---------------------------------------------------------------------------

BOOL CPCTAppDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    g_pDlg = this;

    m_editDllPath.SetWindowText(_T("UDL.ViaviPCT4All.dll"));
    m_comboAddr.AddString(_T("172.16.154.87"));
    m_comboAddr.SetCurSel(0);
    m_editPort.SetWindowText(_T("8301"));

    m_check1310.SetCheck(BST_CHECKED);
    m_check1550.SetCheck(BST_CHECKED);
    m_editOpmIndex.SetWindowText(_T("1"));
    m_comboConnMode.AddString(_T("1 - Single MTJ"));
    m_comboConnMode.AddString(_T("2 - Dual MTJ"));
    m_comboConnMode.SetCurSel(0);
    m_editLaunchPort.SetWindowText(_T("1"));
    m_editChFrom.SetWindowText(_T("1"));
    m_editChTo.SetWindowText(_T("12"));
    m_editAvgTime.SetWindowText(_T("5"));

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
    AppendLog(_T("PCTApp - Viavi MAP PCT All-in-One Test Tool"));
    AppendLog(_T("Uses UDL.ViaviPCT4All driver (includes built-in optical switch control)."));
    AppendLog(_T("Flow: Load DLL -> Connect -> Zeroing -> Measure."));

    return TRUE;
}

void CPCTAppDlg::OnOK() {}

void CPCTAppDlg::OnCancel()
{
    if (m_bBusy)
    {
        MessageBox(_T("An operation is in progress. Please wait."),
                   _T("Busy"), MB_OK | MB_ICONINFORMATION);
        return;
    }
    CDialogEx::OnCancel();
}

void CPCTAppDlg::OnClose()
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
// DLL Load / Unload
// ---------------------------------------------------------------------------

void CPCTAppDlg::OnBnClickedLoad()
{
    if (m_loader.IsDllLoaded())
    {
        if (m_loader.GetDriverHandle())
        {
            m_loader.Disconnect();
            m_loader.DestroyDriver();
        }
        m_loader.UnloadDll();
        m_bConnected = false;
        AppendLog(_T("DLL unloaded."));
        EnableControls();
        return;
    }

    CString path;
    m_editDllPath.GetWindowText(path);
    if (path.IsEmpty()) { AppendLog(_T("Please enter DLL path.")); return; }

    CStringA pathA(path);
    if (m_loader.LoadDll(pathA.GetString()))
    {
        m_loader.SetLogCallback(PCT4LogCB);
        AppendLog(_T("DLL loaded successfully."));
    }
    else
    {
        CString msg; msg.Format(_T("Failed to load: %s"), (LPCTSTR)path);
        AppendLog(msg);
    }
    EnableControls();
}

// ---------------------------------------------------------------------------
// Connect / Disconnect
// ---------------------------------------------------------------------------

void CPCTAppDlg::OnBnClickedConnect()
{
    CString addr;
    m_comboAddr.GetWindowText(addr);
    if (addr.IsEmpty()) { AppendLog(_T("Please specify address.")); return; }

    if (m_loader.GetDriverHandle())
    {
        m_loader.Disconnect();
        m_loader.DestroyDriver();
    }
    m_bConnected = false;

    CStringA addrA(addr);
    std::string addrStr(addrA.GetString());

    CString portStr;
    m_editPort.GetWindowText(portStr);
    int port = _ttoi(portStr);
    if (port <= 0) port = 8301;

    bool created = m_loader.CreateDriver("viavi", addrStr.c_str(), port, 0);
    CString msg; msg.Format(_T("Creating driver (TCP %s:%d)..."), (LPCTSTR)addr, port);
    AppendLog(msg);

    if (!created) { AppendLog(_T("Failed to create driver.")); return; }

    CPCT4AllDllLoader* pLoader = &m_loader;
    bool* pConn = &m_bConnected;

    RunAsync(_T("Connecting..."), [pLoader, pConn]() -> WorkerResult*
    {
        WorkerResult* r = new WorkerResult();
        CString log;

        BOOL ok = pLoader->Connect();
        if (ok)
        {
            *pConn = true;
            pLoader->Initialize();
            log += _T("Connected and initialized.\r\n");

            PCT4IdentificationInfo idn = {};
            if (pLoader->GetIdentification(&idn))
            {
                CString idnMsg;
                idnMsg.Format(_T("  IDN: %s, %s, SN: %s, FW: %s"),
                    (LPCTSTR)Utf8ToWide(idn.manufacturer),
                    (LPCTSTR)Utf8ToWide(idn.platform),
                    (LPCTSTR)Utf8ToWide(idn.serialNumber),
                    (LPCTSTR)Utf8ToWide(idn.firmwareVersion));
                log += idnMsg + _T("\r\n");
            }

            PCT4CassetteInfo ci = {};
            if (pLoader->GetCassetteInfo(&ci))
            {
                CString ciMsg;
                ciMsg.Format(_T("  Module: SN=%s  FW=%s  Desc=%s"),
                    (LPCTSTR)Utf8ToWide(ci.serialNumber),
                    (LPCTSTR)Utf8ToWide(ci.firmwareVersion),
                    (LPCTSTR)Utf8ToWide(ci.description));
                log += ciMsg;
            }
        }
        else
        {
            log += _T("Connection FAILED.");
        }

        r->success = *pConn;
        r->logMessage = log;
        r->statusText = *pConn ? _T("Connected") : _T("Connection Failed");
        return r;
    });
}

void CPCTAppDlg::OnBnClickedDisconnect()
{
    CPCT4AllDllLoader* pLoader = &m_loader;
    bool* pConn = &m_bConnected;

    RunAsync(_T("Disconnecting..."), [pLoader, pConn]() -> WorkerResult*
    {
        WorkerResult* r = new WorkerResult();
        if (pLoader->GetDriverHandle())
        {
            pLoader->Disconnect();
            pLoader->DestroyDriver();
        }
        *pConn = false;
        r->success = true;
        r->logMessage = _T("Disconnected.");
        r->statusText = _T("Disconnected");
        return r;
    });
}

// ---------------------------------------------------------------------------
// Helpers: build SCPI parameter strings from UI
// ---------------------------------------------------------------------------

std::string CPCTAppDlg::BuildSourceList()
{
    std::string wl;
    if (m_check1310.GetCheck() == BST_CHECKED) wl += "1310";
    if (m_check1550.GetCheck() == BST_CHECKED)
    {
        if (!wl.empty()) wl += ",";
        wl += "1550";
    }
    if (wl.empty()) wl = "1310,1550";
    return wl;
}

std::string CPCTAppDlg::BuildPathListChannels()
{
    CString fromStr, toStr;
    m_editChFrom.GetWindowText(fromStr);
    m_editChTo.GetWindowText(toStr);
    int from = _ttoi(fromStr);
    int to = _ttoi(toStr);
    if (from < 1) from = 1;
    if (to < from) to = from;

    char buf[64];
    sprintf_s(buf, "%d-%d", from, to);
    return std::string(buf);
}

// ---------------------------------------------------------------------------
// Zeroing (Reference Measurement)
//
// Flow from Programming Guide:
//   SENSe:FUNCtion 0       -> Reference mode
//   SENSe:OPM <n>          -> Select OPM
//   SOURce:LIST <wl>       -> Source wavelengths
//   PATH:CONNection <m>    -> Single/Dual MTJ
//   PATH:LAUNch <p>        -> Launch port
//   PATH:LIST <sw>,<ch>    -> SW channels
//   MEASure:STARt
//   Poll MEASure:STATe? until != 2 (BUSY)
//   If state == 3 -> fault
// ---------------------------------------------------------------------------

void CPCTAppDlg::OnBnClickedZeroing()
{
    if (!m_loader.GetDriverHandle() || !m_bConnected) return;

    std::string sourceList = BuildSourceList();
    std::string pathChannels = BuildPathListChannels();

    CString opmStr, launchStr, avgStr;
    m_editOpmIndex.GetWindowText(opmStr);
    m_editLaunchPort.GetWindowText(launchStr);
    m_editAvgTime.GetWindowText(avgStr);
    int opmIndex = _ttoi(opmStr);
    int launchPort = _ttoi(launchStr);
    int avgTime = _ttoi(avgStr);
    int connMode = m_comboConnMode.GetCurSel() + 1;

    if (opmIndex < 1) opmIndex = 1;
    if (launchPort < 1) launchPort = 1;
    if (avgTime < 1) avgTime = 5;

    AppendLog(_T("=== Zeroing (Reference) Started ==="));

    CPCT4AllDllLoader* pLoader = &m_loader;
    std::atomic<bool>* pStop = &m_bStopRequested;
    m_bStopRequested = false;

    RunAsync(_T("Zeroing..."),
        [pLoader, sourceList, pathChannels, opmIndex, launchPort,
         avgTime, connMode, pStop]() -> WorkerResult*
    {
        WorkerResult* r = new WorkerResult();
        CString log;

        try
        {
            // -- Measurement Setup --
            pLoader->SetFunction(0);
            log += _T("  SENSe:FUNCtion 0 (Reference)\r\n");

            char opmCmd[64];
            sprintf_s(opmCmd, ":SENS:OPM %d", opmIndex);
            pLoader->SendWrite(opmCmd);
            CString opmMsg; opmMsg.Format(_T("  SENSe:OPM %d\r\n"), opmIndex);
            log += opmMsg;

            pLoader->SetSourceList(sourceList.c_str());
            CString slMsg; slMsg.Format(_T("  SOURce:LIST %s\r\n"), (LPCTSTR)Utf8ToWide(sourceList.c_str()));
            log += slMsg;

            pLoader->SetConnection(connMode);
            CString cmMsg; cmMsg.Format(_T("  PATH:CONNection %d\r\n"), connMode);
            log += cmMsg;

            pLoader->SetLaunch(launchPort);
            CString lpMsg; lpMsg.Format(_T("  PATH:LAUNch %d\r\n"), launchPort);
            log += lpMsg;

            char plCmd[128];
            sprintf_s(plCmd, "%s", pathChannels.c_str());
            pLoader->SetPathList(1, plCmd);
            CString plMsg; plMsg.Format(_T("  PATH:LIST 1,%s\r\n"), (LPCTSTR)Utf8ToWide(pathChannels.c_str()));
            log += plMsg;

            pLoader->SetAveragingTime(avgTime);

            // -- Acquisition --
            pLoader->StartMeasurement();
            log += _T("  MEASure:STARt\r\n");

            int state = 2;
            int pollCount = 0;
            while (!pStop->load())
            {
                ::Sleep(500);
                state = pLoader->GetMeasurementState();
                ++pollCount;

                if (state != 2) break;
                if (pollCount > 600) break;  // 5 min timeout
            }

            if (pStop->load())
            {
                pLoader->StopMeasurement();
                log += _T("  Stopped by user.\r\n");
                r->success = false;
                r->statusText = _T("Zeroing Stopped");
            }
            else if (state == 3)
            {
                log += _T("  MEASure:STATe = 3 (FAULT) - Measurement Error!\r\n");
                char errMsg[256] = {};
                pLoader->CheckError(errMsg, sizeof(errMsg));
                CString errStr; errStr.Format(_T("  Error: %s\r\n"), (LPCTSTR)Utf8ToWide(errMsg));
                log += errStr;
                r->success = false;
                r->statusText = _T("Zeroing Failed (Fault)");
            }
            else if (state == 1)
            {
                log += _T("  MEASure:STATe = 1 (IDLE) - Reference OK\r\n");
                r->success = true;
                r->statusText = _T("Zeroing Done - Ready");
            }
            else
            {
                CString stMsg; stMsg.Format(_T("  MEASure:STATe = %d (timeout or unknown)\r\n"), state);
                log += stMsg;
                r->success = false;
                r->statusText = _T("Zeroing Timeout");
            }

            r->logMessage = _T("=== Zeroing Complete ===\r\n") + log;
        }
        catch (...)
        {
            r->logMessage = _T("Zeroing: unexpected error.");
            r->statusText = _T("Zeroing Error");
        }
        return r;
    });
}

// ---------------------------------------------------------------------------
// DUT Measurement
//
// Flow from Programming Guide:
//   SENSe:FUNCtion 1       -> DUT mode
//   (same setup as zeroing)
//   MEASure:STARt
//   Poll MEASure:STATe? until != 2
//   For ch = startCh to endCh:
//     MEASure:ALL? ch       -> parse results
// ---------------------------------------------------------------------------

void CPCTAppDlg::OnBnClickedMeasure()
{
    if (!m_loader.GetDriverHandle() || !m_bConnected) return;

    std::string sourceList = BuildSourceList();
    std::string pathChannels = BuildPathListChannels();

    CString opmStr, launchStr, avgStr, fromStr, toStr;
    m_editOpmIndex.GetWindowText(opmStr);
    m_editLaunchPort.GetWindowText(launchStr);
    m_editAvgTime.GetWindowText(avgStr);
    m_editChFrom.GetWindowText(fromStr);
    m_editChTo.GetWindowText(toStr);
    int opmIndex = _ttoi(opmStr);
    int launchPort = _ttoi(launchStr);
    int avgTime = _ttoi(avgStr);
    int connMode = m_comboConnMode.GetCurSel() + 1;
    int chFrom = _ttoi(fromStr);
    int chTo = _ttoi(toStr);

    if (opmIndex < 1) opmIndex = 1;
    if (launchPort < 1) launchPort = 1;
    if (avgTime < 1) avgTime = 5;
    if (chFrom < 1) chFrom = 1;
    if (chTo < chFrom) chTo = chFrom;

    AppendLog(_T("=== DUT Measurement Started ==="));

    CPCT4AllDllLoader* pLoader = &m_loader;
    std::atomic<bool>* pStop = &m_bStopRequested;
    m_bStopRequested = false;

    RunAsync(_T("Measuring..."),
        [pLoader, sourceList, pathChannels, opmIndex, launchPort,
         avgTime, connMode, chFrom, chTo, pStop]() -> WorkerResult*
    {
        WorkerResult* r = new WorkerResult();
        CString log;

        try
        {
            // -- Measurement Setup --
            pLoader->SetFunction(1);
            log += _T("  SENSe:FUNCtion 1 (DUT)\r\n");

            char opmCmd[64];
            sprintf_s(opmCmd, ":SENS:OPM %d", opmIndex);
            pLoader->SendWrite(opmCmd);

            pLoader->SetSourceList(sourceList.c_str());
            pLoader->SetConnection(connMode);
            pLoader->SetLaunch(launchPort);
            pLoader->SetPathList(1, pathChannels.c_str());
            pLoader->SetAveragingTime(avgTime);

            CString setupMsg;
            setupMsg.Format(_T("  Setup: OPM=%d, Source=%s, Conn=%d, Launch=%d, SW1=%s, Avg=%ds\r\n"),
                opmIndex, (LPCTSTR)Utf8ToWide(sourceList.c_str()),
                connMode, launchPort,
                (LPCTSTR)Utf8ToWide(pathChannels.c_str()), avgTime);
            log += setupMsg;

            // -- Acquisition --
            pLoader->StartMeasurement();
            log += _T("  MEASure:STARt\r\n");

            int state = 2;
            int pollCount = 0;
            while (!pStop->load())
            {
                ::Sleep(500);
                state = pLoader->GetMeasurementState();
                ++pollCount;
                if (state != 2) break;
                if (pollCount > 600) break;
            }

            if (pStop->load())
            {
                pLoader->StopMeasurement();
                log += _T("  Stopped by user.\r\n");
                r->success = false;
                r->statusText = _T("Measurement Stopped");
                r->logMessage = _T("=== Measurement Stopped ===\r\n") + log;
                return r;
            }

            if (state == 3)
            {
                log += _T("  MEASure:STATe = 3 (FAULT)\r\n");
                char errMsg[256] = {};
                pLoader->CheckError(errMsg, sizeof(errMsg));
                CString errStr; errStr.Format(_T("  Error: %s\r\n"), (LPCTSTR)Utf8ToWide(errMsg));
                log += errStr;
                r->success = false;
                r->statusText = _T("Measurement Failed (Fault)");
                r->logMessage = _T("=== Measurement Failed ===\r\n") + log;
                return r;
            }

            if (state != 1)
            {
                CString stMsg; stMsg.Format(_T("  MEASure:STATe = %d (timeout)\r\n"), state);
                log += stMsg;
                r->success = false;
                r->statusText = _T("Measurement Timeout");
                r->logMessage = _T("=== Measurement Timeout ===\r\n") + log;
                return r;
            }

            log += _T("  MEASure:STATe = 1 (IDLE) - Acquisition complete.\r\n");

            // -- Get Results --
            std::vector<MeasResult> allResults;

            for (int ch = chFrom; ch <= chTo; ++ch)
            {
                if (pStop->load()) break;

                char queryCmd[64];
                sprintf_s(queryCmd, ":MEAS:ALL? %d", ch);
                char response[4096] = {};
                BOOL ok = pLoader->SendCommand(queryCmd, response, sizeof(response));

                if (ok && response[0] != '\0')
                {
                    std::vector<MeasResult> chResults =
                        ParseMeasureAllResponse(std::string(response), ch);
                    allResults.insert(allResults.end(), chResults.begin(), chResults.end());

                    CString resMsg;
                    resMsg.Format(_T("  CH %d: %d result(s) - raw: %s\r\n"),
                        ch, (int)chResults.size(),
                        (LPCTSTR)Utf8ToWide(response));
                    log += resMsg;
                }
                else
                {
                    CString failMsg;
                    failMsg.Format(_T("  CH %d: no data\r\n"), ch);
                    log += failMsg;
                }
            }

            r->success = true;
            r->hasResults = true;
            r->results = allResults;

            CString summary;
            summary.Format(_T("=== Measurement Complete: %d result(s) ===\r\n"),
                           (int)allResults.size());
            r->logMessage = summary + log;
            r->statusText = _T("Measurement Done - Ready");
        }
        catch (...)
        {
            r->logMessage = _T("Measurement: unexpected error.");
            r->statusText = _T("Measurement Error");
        }
        return r;
    });
}

// ---------------------------------------------------------------------------
// Stop
// ---------------------------------------------------------------------------

void CPCTAppDlg::OnBnClickedStop()
{
    m_bStopRequested = true;
    if (m_loader.GetDriverHandle())
        m_loader.StopMeasurement();
    AppendLog(_T("Stop requested - sending :MEASure:STOP"));
}

// ---------------------------------------------------------------------------
// Parse :MEAS:ALL? response
//
// Response format (comma-separated per wavelength):
//   IL, ORL, ORL_Zone1, ORL_Zone2, Length, Power, ...
// Multiple wavelengths are concatenated.
// ---------------------------------------------------------------------------

std::vector<CPCTAppDlg::MeasResult> CPCTAppDlg::ParseMeasureAllResponse(
    const std::string& response, int channel)
{
    std::vector<MeasResult> results;

    std::vector<double> values;
    std::istringstream iss(response);
    std::string token;
    while (std::getline(iss, token, ','))
    {
        try { values.push_back(std::stod(token)); }
        catch (...) { values.push_back(0.0); }
    }

    // Each wavelength block has 6 values: IL, ORL, Zone1, Zone2, Length, Power
    const int FIELDS_PER_WL = 6;

    if (values.size() >= (size_t)FIELDS_PER_WL)
    {
        size_t numWL = values.size() / FIELDS_PER_WL;
        double wlList[] = { 1310.0, 1550.0, 1490.0, 1625.0 };

        for (size_t w = 0; w < numWL && w < 4; ++w)
        {
            size_t base = w * FIELDS_PER_WL;
            MeasResult m;
            m.channel = channel;
            m.wavelength = (w < 4) ? wlList[w] : 0.0;
            m.insertionLoss = values[base + 0];
            m.returnLoss = values[base + 1];
            m.orlZone1 = values[base + 2];
            m.orlZone2 = values[base + 3];
            m.dutLength = values[base + 4];
            m.power = values[base + 5];
            results.push_back(m);
        }
    }
    else if (!values.empty())
    {
        MeasResult m;
        m.channel = channel;
        m.wavelength = 0.0;
        m.insertionLoss = values.size() > 0 ? values[0] : 0.0;
        m.returnLoss = values.size() > 1 ? values[1] : 0.0;
        m.orlZone1 = values.size() > 2 ? values[2] : 0.0;
        m.orlZone2 = values.size() > 3 ? values[3] : 0.0;
        m.dutLength = values.size() > 4 ? values[4] : 0.0;
        m.power = values.size() > 5 ? values[5] : 0.0;
        results.push_back(m);
    }

    return results;
}

// ---------------------------------------------------------------------------
// Async worker infrastructure
// ---------------------------------------------------------------------------

void CPCTAppDlg::RunAsync(const CString& operationName,
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

LRESULT CPCTAppDlg::OnWorkerDone(WPARAM, LPARAM lParam)
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
// UI state management
// ---------------------------------------------------------------------------

void CPCTAppDlg::SetBusy(bool busy, const CString& statusText)
{
    m_bBusy = busy;
    EnableControls();
}

void CPCTAppDlg::EnableControls()
{
    bool loaded = m_loader.IsDllLoaded();

    GetDlgItem(IDC_EDIT_DLL_PATH)->EnableWindow(!m_bBusy && !loaded);
    SetDlgItemText(IDC_BTN_LOAD, loaded ? _T("Unload") : _T("Load"));
    GetDlgItem(IDC_BTN_LOAD)->EnableWindow(!m_bBusy);
    GetDlgItem(IDC_COMBO_ADDR)->EnableWindow(!m_bBusy && loaded && !m_bConnected);
    GetDlgItem(IDC_EDIT_PORT)->EnableWindow(!m_bBusy && !m_bConnected);
    GetDlgItem(IDC_BTN_CONNECT)->EnableWindow(!m_bBusy && loaded && !m_bConnected);
    GetDlgItem(IDC_BTN_DISCONNECT)->EnableWindow(!m_bBusy && m_bConnected);

    GetDlgItem(IDC_CHECK_1310)->EnableWindow(!m_bBusy);
    GetDlgItem(IDC_CHECK_1550)->EnableWindow(!m_bBusy);
    GetDlgItem(IDC_EDIT_OPM)->EnableWindow(!m_bBusy);
    GetDlgItem(IDC_COMBO_CONN_MODE)->EnableWindow(!m_bBusy);
    GetDlgItem(IDC_EDIT_LAUNCH)->EnableWindow(!m_bBusy);
    GetDlgItem(IDC_EDIT_CH_FROM)->EnableWindow(!m_bBusy);
    GetDlgItem(IDC_EDIT_CH_TO)->EnableWindow(!m_bBusy);
    GetDlgItem(IDC_EDIT_AVG_TIME)->EnableWindow(!m_bBusy);

    GetDlgItem(IDC_BTN_ZEROING)->EnableWindow(!m_bBusy && m_bConnected);
    GetDlgItem(IDC_BTN_MEASURE)->EnableWindow(!m_bBusy && m_bConnected);
    GetDlgItem(IDC_BTN_STOP)->EnableWindow(m_bBusy);

    SetDlgItemText(IDC_STATIC_STATUS, m_bConnected ? _T("Connected") : _T("Not Connected"));
}

// ---------------------------------------------------------------------------
// Log / Results
// ---------------------------------------------------------------------------

LRESULT CPCTAppDlg::OnLogMessage(WPARAM, LPARAM lParam)
{
    CString* pMsg = reinterpret_cast<CString*>(lParam);
    if (pMsg) { AppendLog(*pMsg); delete pMsg; }
    return 0;
}

void CPCTAppDlg::AppendLog(const CString& text)
{
    CString current;
    m_editLog.GetWindowText(current);
    if (!current.IsEmpty()) current += _T("\r\n");
    current += text;
    m_editLog.SetWindowText(current);
    m_editLog.LineScroll(m_editLog.GetLineCount());
}

void CPCTAppDlg::OnBnClickedClearLog()
{
    m_editLog.SetWindowText(_T(""));
}

void CPCTAppDlg::PopulateResultsList(const std::vector<MeasResult>& results)
{
    m_listResults.DeleteAllItems();

    for (size_t i = 0; i < results.size(); ++i)
    {
        const MeasResult& res = results[i];
        CString chStr, wlStr, ilStr, orlStr, z1Str, z2Str, lenStr, pwrStr;
        chStr.Format(_T("%d"), res.channel);
        wlStr.Format(_T("%.0f"), res.wavelength);
        ilStr.Format(_T("%.3f"), res.insertionLoss);
        orlStr.Format(_T("%.2f"), res.returnLoss);
        z1Str.Format(_T("%.2f"), res.orlZone1);
        z2Str.Format(_T("%.2f"), res.orlZone2);
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
