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
    DDX_Control(pDX, IDC_CHECK_1450, m_check1450);
    DDX_Control(pDX, IDC_CHECK_1550, m_check1550);
    DDX_Control(pDX, IDC_CHECK_1625, m_check1625);
    DDX_Control(pDX, IDC_RADIO_J1, m_radioJ1);
    DDX_Control(pDX, IDC_RADIO_J2, m_radioJ2);
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
    m_check1450.SetCheck(BST_UNCHECKED);
    m_check1550.SetCheck(BST_CHECKED);
    m_check1625.SetCheck(BST_UNCHECKED);
    m_radioJ1.SetCheck(BST_CHECKED);
    m_editChFrom.SetWindowText(_T("1"));
    m_editChTo.SetWindowText(_T("24"));
    m_editAvgTime.SetWindowText(_T("2"));

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
    auto append = [&](const char* nm, CButton& chk) {
        if (chk.GetCheck() == BST_CHECKED) {
            if (!wl.empty()) wl += ",";
            wl += nm;
        }
    };
    append("1310", m_check1310);
    append("1450", m_check1450);
    append("1550", m_check1550);
    append("1625", m_check1625);
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
// Helper: parse a comma-separated response with wavelength markers into
// a vector of doubles per wavelength. Returns map[wlIndex] = value.
// Response format: "WL1,val1,...,WL2,val2,..." or just "val1,val2,..."
// ---------------------------------------------------------------------------

static std::vector<double> ParsePerWLResponse(const std::string& response)
{
    std::vector<double> vals;
    std::istringstream iss(response);
    std::string token;
    while (std::getline(iss, token, ','))
    {
        if (token.empty() || token.find_first_not_of(" \t") == std::string::npos)
            vals.push_back(NAN);
        else
        {
            try { vals.push_back(std::stod(token)); }
            catch (...) { vals.push_back(NAN); }
        }
    }
    return vals;
}

// ---------------------------------------------------------------------------
// Helper: fetch complete results for a channel range.
// Queries :MEAS:ALL?, :MEAS:POW?, :MEAS:LENG? and merges them.
// ---------------------------------------------------------------------------

static std::vector<CPCTAppDlg::MeasResult> FetchChannelResults(
    CPCT4AllDllLoader* pLoader, int chFrom, int chTo,
    std::atomic<bool>* pStop, CString& log)
{
    std::vector<CPCTAppDlg::MeasResult> allResults;

    for (int ch = chFrom; ch <= chTo; ++ch)
    {
        if (pStop->load()) break;

        char queryCmd[64];
        sprintf_s(queryCmd, ":MEAS:ALL? %d", ch);
        char response[4096] = {};
        BOOL ok = pLoader->SendCommand(queryCmd, response, sizeof(response));

        if (ok && response[0] != '\0')
        {
            std::vector<CPCTAppDlg::MeasResult> chResults =
                CPCTAppDlg::ParseMeasureAllResponse(std::string(response), ch);
            allResults.insert(allResults.end(), chResults.begin(), chResults.end());

            CString resMsg;
            resMsg.Format(_T("  CH %d: %d result(s)\r\n"), ch, (int)chResults.size());
            log += resMsg;
        }
        else
        {
            CString failMsg;
            failMsg.Format(_T("  CH %d: no data\r\n"), ch);
            log += failMsg;
        }
    }

    // Query Power and Length for all channels and merge
    char powResp[8192] = {};
    char lenResp[8192] = {};
    pLoader->SendCommand(":MEAS:POW?", powResp, sizeof(powResp));
    pLoader->SendCommand(":MEAS:LENG?", lenResp, sizeof(lenResp));

    if (powResp[0] != '\0')
    {
        std::vector<double> powVals = ParsePerWLResponse(std::string(powResp));
        // Power response may have WL markers or be flat values per channel/WL
        // Try to match: strip WL markers, collect only data values
        std::vector<double> powData;
        for (size_t i = 0; i < powVals.size(); ++i)
        {
            if (!std::isnan(powVals[i]) && powVals[i] >= 1000.0)
                continue;  // skip wavelength markers
            powData.push_back(powVals[i]);
        }
        // If no WL markers found, use all values as-is
        if (powData.empty()) powData = powVals;

        for (size_t i = 0; i < allResults.size() && i < powData.size(); ++i)
            allResults[i].power = powData[i];
    }

    if (lenResp[0] != '\0')
    {
        std::vector<double> lenVals = ParsePerWLResponse(std::string(lenResp));
        std::vector<double> lenData;
        for (size_t i = 0; i < lenVals.size(); ++i)
        {
            if (!std::isnan(lenVals[i]) && lenVals[i] >= 1000.0)
                continue;
            lenData.push_back(lenVals[i]);
        }
        if (lenData.empty()) lenData = lenVals;

        for (size_t i = 0; i < allResults.size() && i < lenData.size(); ++i)
            allResults[i].dutLength = lenData[i];
    }

    return allResults;
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

    CString avgStr, fromStr, toStr;
    m_editAvgTime.GetWindowText(avgStr);
    m_editChFrom.GetWindowText(fromStr);
    m_editChTo.GetWindowText(toStr);
    int launchPort = (m_radioJ2.GetCheck() == BST_CHECKED) ? 2 : 1;
    int avgTime = _ttoi(avgStr);
    int chFrom = _ttoi(fromStr);
    int chTo = _ttoi(toStr);

    if (avgTime < 1) avgTime = 5;
    if (chFrom < 1) chFrom = 1;
    if (chTo < chFrom) chTo = chFrom;

    AppendLog(_T("=== Zeroing (Reference) Started ==="));

    CPCT4AllDllLoader* pLoader = &m_loader;
    std::atomic<bool>* pStop = &m_bStopRequested;
    m_bStopRequested = false;

    RunAsync(_T("Zeroing..."),
        [pLoader, sourceList, pathChannels, launchPort,
         avgTime, chFrom, chTo, pStop]() -> WorkerResult*
    {
        WorkerResult* r = new WorkerResult();
        CString log;

        try
        {
            // -- Measurement Setup --
            pLoader->SetFunction(0);
            log += _T("  SENSe:FUNCtion 0 (Reference)\r\n");

            pLoader->SendWrite(":SENS:OPM 1");
            log += _T("  SENSe:OPM 1\r\n");

            pLoader->SetSourceList(sourceList.c_str());
            CString slMsg; slMsg.Format(_T("  SOURce:LIST %s\r\n"), (LPCTSTR)Utf8ToWide(sourceList.c_str()));
            log += slMsg;

            pLoader->SetConnection(1);
            log += _T("  PATH:CONNection 1 (Single MTJ)\r\n");

            pLoader->SetLaunch(launchPort);
            CString lpMsg; lpMsg.Format(_T("  PATH:LAUNch %d (J%d)\r\n"), launchPort, launchPort);
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

                std::vector<MeasResult> allResults =
                    FetchChannelResults(pLoader, chFrom, chTo, pStop, log);

                r->success = true;
                r->hasResults = true;
                r->results = allResults;
                CString summary;
                summary.Format(_T("  Zeroing results: %d entries\r\n"), (int)allResults.size());
                log += summary;
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

    CString avgStr, fromStr, toStr;
    m_editAvgTime.GetWindowText(avgStr);
    m_editChFrom.GetWindowText(fromStr);
    m_editChTo.GetWindowText(toStr);
    int launchPort = (m_radioJ2.GetCheck() == BST_CHECKED) ? 2 : 1;
    int avgTime = _ttoi(avgStr);
    int chFrom = _ttoi(fromStr);
    int chTo = _ttoi(toStr);

    if (avgTime < 1) avgTime = 5;
    if (chFrom < 1) chFrom = 1;
    if (chTo < chFrom) chTo = chFrom;

    AppendLog(_T("=== DUT Measurement Started ==="));

    CPCT4AllDllLoader* pLoader = &m_loader;
    std::atomic<bool>* pStop = &m_bStopRequested;
    m_bStopRequested = false;

    RunAsync(_T("Measuring..."),
        [pLoader, sourceList, pathChannels, launchPort,
         avgTime, chFrom, chTo, pStop]() -> WorkerResult*
    {
        WorkerResult* r = new WorkerResult();
        CString log;

        try
        {
            // -- Measurement Setup --
            pLoader->SetFunction(1);
            log += _T("  SENSe:FUNCtion 1 (DUT)\r\n");

            pLoader->SendWrite(":SENS:OPM 1");

            pLoader->SetSourceList(sourceList.c_str());
            pLoader->SetConnection(1);
            pLoader->SetLaunch(launchPort);
            pLoader->SetPathList(1, pathChannels.c_str());
            pLoader->SetAveragingTime(avgTime);

            CString setupMsg;
            setupMsg.Format(_T("  Setup: OPM=1, Source=%s, Conn=1(Single), Launch=J%d, SW1=%s, Avg=%ds\r\n"),
                (LPCTSTR)Utf8ToWide(sourceList.c_str()),
                launchPort,
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

            // -- Get Results (ALL + Power + Length) --
            std::vector<MeasResult> allResults =
                FetchChannelResults(pLoader, chFrom, chTo, pStop, log);

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
        if (token.empty() || token.find_first_not_of(" \t") == std::string::npos)
            values.push_back(NAN);
        else
        {
            try { values.push_back(std::stod(token)); }
            catch (...) { values.push_back(NAN); }
        }
    }

    // Response format: WL1,IL1,ORL1,Zone1_1,Zone2_1,WL2,IL2,ORL2,Zone1_2,Zone2_2,...
    // Detect wavelength markers (>= 1000 nm) and split into per-WL blocks
    std::vector<size_t> wlIndices;
    for (size_t i = 0; i < values.size(); ++i)
    {
        if (!std::isnan(values[i]) && values[i] >= 1000.0)
            wlIndices.push_back(i);
    }

    for (size_t w = 0; w < wlIndices.size(); ++w)
    {
        size_t start = wlIndices[w];
        size_t end = (w + 1 < wlIndices.size()) ? wlIndices[w + 1] : values.size();

        MeasResult m;
        m.channel = channel;
        m.wavelength    = values[start];
        m.insertionLoss = (start + 1 < end) ? values[start + 1] : NAN;
        m.returnLoss    = (start + 2 < end) ? values[start + 2] : NAN;
        m.orlZone1      = (start + 3 < end) ? values[start + 3] : NAN;
        m.orlZone2      = (start + 4 < end) ? values[start + 4] : NAN;
        m.dutLength     = (start + 5 < end) ? values[start + 5] : NAN;
        m.power         = (start + 6 < end) ? values[start + 6] : NAN;

        results.push_back(m);
    }

    if (wlIndices.empty() && !values.empty())
    {
        MeasResult m;
        m.channel = channel;
        m.wavelength = NAN;
        m.insertionLoss = values.size() > 0 ? values[0] : NAN;
        m.returnLoss = values.size() > 1 ? values[1] : NAN;
        m.orlZone1 = values.size() > 2 ? values[2] : NAN;
        m.orlZone2 = values.size() > 3 ? values[3] : NAN;
        m.dutLength = values.size() > 4 ? values[4] : NAN;
        m.power = values.size() > 5 ? values[5] : NAN;
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
    GetDlgItem(IDC_CHECK_1450)->EnableWindow(!m_bBusy);
    GetDlgItem(IDC_CHECK_1550)->EnableWindow(!m_bBusy);
    GetDlgItem(IDC_CHECK_1625)->EnableWindow(!m_bBusy);
    GetDlgItem(IDC_RADIO_J1)->EnableWindow(!m_bBusy);
    GetDlgItem(IDC_RADIO_J2)->EnableWindow(!m_bBusy);
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

static CString FmtVal(double v, const TCHAR* fmt)
{
    if (std::isnan(v)) return _T("-");
    CString s; s.Format(fmt, v); return s;
}

void CPCTAppDlg::PopulateResultsList(const std::vector<MeasResult>& results)
{
    m_listResults.DeleteAllItems();

    for (size_t i = 0; i < results.size(); ++i)
    {
        const MeasResult& res = results[i];
        CString chStr;
        chStr.Format(_T("%d"), res.channel);

        int idx = m_listResults.InsertItem(static_cast<int>(i), chStr);
        m_listResults.SetItemText(idx, 1, FmtVal(res.wavelength, _T("%.0f")));
        m_listResults.SetItemText(idx, 2, FmtVal(res.insertionLoss, _T("%.3f")));
        m_listResults.SetItemText(idx, 3, FmtVal(res.returnLoss, _T("%.2f")));
        m_listResults.SetItemText(idx, 4, FmtVal(res.orlZone1, _T("%.2f")));
        m_listResults.SetItemText(idx, 5, FmtVal(res.orlZone2, _T("%.2f")));
        m_listResults.SetItemText(idx, 6, FmtVal(res.dutLength, _T("%.2f")));
        m_listResults.SetItemText(idx, 7, FmtVal(res.power, _T("%.2f")));
    }
}
