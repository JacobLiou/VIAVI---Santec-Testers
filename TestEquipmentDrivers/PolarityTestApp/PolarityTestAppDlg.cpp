#include "stdafx.h"
#include "PolarityTestApp.h"
#include "PolarityTestAppDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CPolarityTestAppDlg::CPolarityTestAppDlg(CWnd* pParent)
    : CDialogEx(IDD_POLARITYTESTAPP_DIALOG, pParent)
    , m_opened(false)
{
}

void CPolarityTestAppDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT_DLL_PATH, m_editDllPath);
    DDX_Control(pDX, IDC_EDIT_STATION, m_editStation);
    DDX_Control(pDX, IDC_EDIT_RLM_SERIAL, m_editRlmSerial);
    DDX_Control(pDX, IDC_EDIT_RDSP_SERIAL, m_editRdspSerial);
    DDX_Control(pDX, IDC_EDIT_PROFILE, m_editProfile);
    DDX_Control(pDX, IDC_STATIC_STATUS, m_staticStatus);
    DDX_Control(pDX, IDC_LIST_RESULTS, m_listResults);
    DDX_Control(pDX, IDC_EDIT_RAW, m_editRaw);
    DDX_Control(pDX, IDC_EDIT_LOG, m_editLog);
}

BEGIN_MESSAGE_MAP(CPolarityTestAppDlg, CDialogEx)
    ON_WM_CLOSE()
    ON_BN_CLICKED(IDC_BTN_LOAD_DLL, &CPolarityTestAppDlg::OnBnClickedLoadDll)
    ON_BN_CLICKED(IDC_BTN_UNLOAD_DLL, &CPolarityTestAppDlg::OnBnClickedUnloadDll)
    ON_BN_CLICKED(IDC_BTN_CREATE, &CPolarityTestAppDlg::OnBnClickedCreate)
    ON_BN_CLICKED(IDC_BTN_DESTROY, &CPolarityTestAppDlg::OnBnClickedDestroy)
    ON_BN_CLICKED(IDC_BTN_OPEN, &CPolarityTestAppDlg::OnBnClickedOpen)
    ON_BN_CLICKED(IDC_BTN_CLOSE, &CPolarityTestAppDlg::OnBnClickedClose)
    ON_BN_CLICKED(IDC_BTN_SNAPSHOT, &CPolarityTestAppDlg::OnBnClickedSnapshot)
    ON_BN_CLICKED(IDC_BTN_RUN_MAPPING, &CPolarityTestAppDlg::OnBnClickedRunMapping)
    ON_BN_CLICKED(IDC_BTN_CANCEL, &CPolarityTestAppDlg::OnBnClickedCancel)
    ON_BN_CLICKED(IDC_BTN_GET_RESULTS, &CPolarityTestAppDlg::OnBnClickedGetResults)
    ON_BN_CLICKED(IDC_BTN_CLEAR_LOG, &CPolarityTestAppDlg::OnBnClickedClearLog)
END_MESSAGE_MAP()

BOOL CPolarityTestAppDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    SetIcon(AfxGetApp()->LoadStandardIcon(IDI_APPLICATION), TRUE);

    m_editDllPath.SetWindowText(_T("UDL.SantecPolarity.dll"));
    m_editStation.SetWindowText(_T("STATION-1"));
    m_editRlmSerial.SetWindowText(_T(""));
    m_editRdspSerial.SetWindowText(_T(""));
    m_editProfile.SetWindowText(_T(""));

    m_listResults.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_listResults.InsertColumn(0, _T("In"), LVCFMT_LEFT, 40);
    m_listResults.InsertColumn(1, _T("SW"), LVCFMT_LEFT, 40);
    m_listResults.InsertColumn(2, _T("Expect"), LVCFMT_LEFT, 50);
    m_listResults.InsertColumn(3, _T("Observe"), LVCFMT_LEFT, 55);
    m_listResults.InsertColumn(4, _T("Light"), LVCFMT_LEFT, 45);
    m_listResults.InsertColumn(5, _T("Status"), LVCFMT_LEFT, 90);

    EnableAfterLoad(false);
    AppendLog(_T("Ready. Load UDL.SantecPolarity.dll (same folder as deps), then Create → Open → Run Mapping."));
    return TRUE;
}

void CPolarityTestAppDlg::OnClose()
{
    m_opened = false;
    m_loader.Destroy();
    m_loader.UnloadDll();
    CDialogEx::OnClose();
}

void CPolarityTestAppDlg::AppendLog(const CString& text)
{
    CString existing;
    m_editLog.GetWindowText(existing);
    if (!existing.IsEmpty()) existing += _T("\r\n");
    existing += text;
    m_editLog.SetWindowText(existing);
    m_editLog.LineScroll(m_editLog.GetLineCount());
}

void CPolarityTestAppDlg::UpdateStatus(const CString& status)
{
    m_staticStatus.SetWindowText(status);
}

void CPolarityTestAppDlg::EnableAfterLoad(bool loaded)
{
    GetDlgItem(IDC_BTN_UNLOAD_DLL)->EnableWindow(loaded);
    GetDlgItem(IDC_BTN_CREATE)->EnableWindow(loaded);
    if (!loaded)
    {
        EnableAfterCreate(false);
    }
}

void CPolarityTestAppDlg::EnableAfterCreate(bool created)
{
    GetDlgItem(IDC_BTN_DESTROY)->EnableWindow(created);
    GetDlgItem(IDC_BTN_OPEN)->EnableWindow(created);
    if (!created)
    {
        m_opened = false;
        EnableAfterOpen(false);
    }
}

void CPolarityTestAppDlg::EnableAfterOpen(bool opened)
{
    m_opened = opened;
    GetDlgItem(IDC_BTN_CLOSE)->EnableWindow(opened);
    GetDlgItem(IDC_BTN_SNAPSHOT)->EnableWindow(opened);
    GetDlgItem(IDC_BTN_RUN_MAPPING)->EnableWindow(opened);
    GetDlgItem(IDC_BTN_CANCEL)->EnableWindow(opened);
    GetDlgItem(IDC_BTN_GET_RESULTS)->EnableWindow(opened);
}

CString CPolarityTestAppDlg::StatusToString(SP_STATUS st)
{
    CString s;
    s.Format(_T("%d"), (int)st);
    return s;
}

CString CPolarityTestAppDlg::FiberStatusToString(SP_FIBER_STATUS st)
{
    switch (st)
    {
    case SP_FIBER_PASS: return _T("PASS");
    case SP_FIBER_WRONG_MAPPING: return _T("WRONG_MAP");
    case SP_FIBER_MISSING: return _T("MISSING");
    case SP_FIBER_UNEXPECTED: return _T("UNEXPECTED");
    case SP_FIBER_DUPLICATE: return _T("DUPLICATE");
    case SP_FIBER_DARK_ALLOWED: return _T("DARK_OK");
    case SP_FIBER_AMBIGUOUS: return _T("AMBIGUOUS");
    case SP_FIBER_ACQUISITION_ERROR: return _T("ACQ_ERR");
    default: return _T("?");
    }
}

CStringA CPolarityTestAppDlg::GetUtf8(CEdit& edit)
{
    CString w;
    edit.GetWindowText(w);
    if (w.IsEmpty()) return CStringA();
    const int n = WideCharToMultiByte(CP_UTF8, 0, w, w.GetLength(), NULL, 0, NULL, NULL);
    if (n <= 0) return CStringA();
    CStringA utf8;
    char* p = utf8.GetBuffer(n);
    WideCharToMultiByte(CP_UTF8, 0, w, w.GetLength(), p, n, NULL, NULL);
    utf8.ReleaseBuffer(n);
    return utf8;
}

void CPolarityTestAppDlg::PopulateResults(const SP_FIBER_RESULT* results, uint32_t count)
{
    m_listResults.DeleteAllItems();
    for (uint32_t i = 0; i < count; ++i)
    {
        CString t;
        t.Format(_T("%d"), results[i].inputFiber);
        const int row = m_listResults.InsertItem(i, t);
        t.Format(_T("%d"), results[i].switchChannel);
        m_listResults.SetItemText(row, 1, t);
        t.Format(_T("%d"), results[i].expectedOutput);
        m_listResults.SetItemText(row, 2, t);
        t.Format(_T("%d"), results[i].observedOutput);
        m_listResults.SetItemText(row, 3, t);
        m_listResults.SetItemText(row, 4, results[i].lightDetected ? _T("Y") : _T("N"));
        m_listResults.SetItemText(row, 5, FiberStatusToString(results[i].status));
    }
}

void CPolarityTestAppDlg::OnBnClickedLoadDll()
{
    CString path;
    m_editDllPath.GetWindowText(path);
    CStringA pathA(path);
    if (!m_loader.LoadDll(pathA))
    {
        AppendLog(_T("LoadLibrary / GetProcAddress failed. Check path and that SP_* exports exist."));
        UpdateStatus(_T("Load failed"));
        EnableAfterLoad(false);
        return;
    }
    UpdateStatus(_T("DLL Loaded"));
    EnableAfterLoad(true);
    AppendLog(_T("Loaded UDL.SantecPolarity.dll"));
}

void CPolarityTestAppDlg::OnBnClickedUnloadDll()
{
    m_opened = false;
    m_loader.Destroy();
    m_loader.UnloadDll();
    EnableAfterLoad(false);
    UpdateStatus(_T("DLL Not Loaded"));
    AppendLog(_T("Unloaded DLL"));
}

void CPolarityTestAppDlg::OnBnClickedCreate()
{
    const SP_STATUS st = m_loader.Create();
    if (st != SP_OK)
    {
        AppendLog(_T("SP_Create failed: ") + StatusToString(st));
        return;
    }
    EnableAfterCreate(true);
    UpdateStatus(_T("Created"));
    AppendLog(_T("SP_Create OK"));
}

void CPolarityTestAppDlg::OnBnClickedDestroy()
{
    m_loader.Destroy();
    EnableAfterCreate(false);
    UpdateStatus(_T("Destroyed"));
    AppendLog(_T("SP_Destroy OK"));
}

void CPolarityTestAppDlg::OnBnClickedOpen()
{
    if (!m_loader.HasHandle())
    {
        AppendLog(_T("Create handle first."));
        return;
    }

    const CStringA station = GetUtf8(m_editStation);
    const CStringA rlm = GetUtf8(m_editRlmSerial);
    const CStringA rdsp = GetUtf8(m_editRdspSerial);
    const CStringA profile = GetUtf8(m_editProfile);

    SP_OPEN_OPTIONS opt = {};
    opt.structSize = sizeof(opt);
    opt.version = 1;
    opt.stationIdUtf8 = station.IsEmpty() ? NULL : (const char*)station;
    opt.rlmSerialUtf8 = rlm.IsEmpty() ? NULL : (const char*)rlm;
    opt.rdspSerialUtf8 = rdsp.IsEmpty() ? NULL : (const char*)rdsp;
    opt.profilePathUtf8 = profile.IsEmpty() ? NULL : (const char*)profile;
    opt.connectTimeoutMs = 15000;
    opt.operationTimeoutMs = 30000;
    opt.mappingTimeoutMs = 120000;

    const SP_STATUS st = m_loader.Open(&opt);
    if (st != SP_OK)
    {
        CString msg;
        msg.Format(_T("SP_Open failed: %d  %hs"), (int)st, m_loader.GetLastErrorUtf8().c_str());
        AppendLog(msg);
        EnableAfterOpen(false);
        return;
    }
    EnableAfterOpen(true);
    UpdateStatus(_T("Opened"));
    AppendLog(_T("SP_Open OK"));
}

void CPolarityTestAppDlg::OnBnClickedClose()
{
    const SP_STATUS st = m_loader.Close();
    EnableAfterOpen(false);
    CString msg;
    msg.Format(_T("SP_Close -> %d"), (int)st);
    AppendLog(msg);
    UpdateStatus(_T("Closed"));
}

void CPolarityTestAppDlg::OnBnClickedSnapshot()
{
    SP_HARDWARE_SNAPSHOT snap = {};
    snap.structSize = sizeof(snap);
    snap.version = 1;
    const SP_STATUS st = m_loader.GetHardwareSnapshot(&snap);
    if (st != SP_OK)
    {
        CString msg;
        msg.Format(_T("SP_GetHardwareSnapshot failed: %d  %hs"), (int)st, m_loader.GetLastErrorUtf8().c_str());
        AppendLog(msg);
        return;
    }
    CString raw;
    raw.Format(
        _T("RLM=%hs/%hs fw=%hs out=%d | OSX=%hs sw=%d ch=%d | RDSP=%hs/%hs wl=%d rem=%d"),
        snap.rlmPartNumber, snap.rlmSerial, snap.rlmFirmware, snap.rlmOutputCount,
        snap.osxPartNumber, snap.switchNumber, snap.switchChannelCount,
        snap.rdspPartNumber, snap.rdspSerial, snap.polarityWavelengthNm, snap.remoteDetectorIndex);
    m_editRaw.SetWindowText(raw);
    AppendLog(_T("Snapshot OK"));
}

void CPolarityTestAppDlg::OnBnClickedRunMapping()
{
    AppendLog(_T("SP_RunMapping starting (blocks until done — use a spare UI if needed)..."));
    UpdateStatus(_T("Mapping..."));
    const SP_STATUS st = m_loader.RunMapping();
    CString msg;
    msg.Format(_T("SP_RunMapping -> %d  %hs"), (int)st, m_loader.GetLastErrorUtf8().c_str());
    AppendLog(msg);
    UpdateStatus(st == SP_OK ? _T("Mapping done") : _T("Mapping failed"));
    if (st == SP_OK)
        OnBnClickedGetResults();
}

void CPolarityTestAppDlg::OnBnClickedCancel()
{
    const SP_STATUS st = m_loader.Cancel();
    CString msg;
    msg.Format(_T("SP_Cancel -> %d"), (int)st);
    AppendLog(msg);
}

void CPolarityTestAppDlg::OnBnClickedGetResults()
{
    SP_RESULT_SUMMARY summary = {};
    summary.structSize = sizeof(summary);
    summary.version = 1;
    SP_STATUS st = m_loader.GetSummary(&summary);
    if (st != SP_OK)
    {
        CString msg;
        msg.Format(_T("SP_GetSummary failed: %d"), (int)st);
        AppendLog(msg);
        return;
    }

    CString raw;
    raw.Format(_T("pass=%d fibers=%u ok=%u fail=%u diag=%u elapsedMs=%llu"),
        summary.overallPass, summary.fiberCount, summary.passCount, summary.failCount,
        summary.diagnosticImageCount, (unsigned long long)summary.elapsedMs);
    m_editRaw.SetWindowText(raw);

    uint32_t count = 0;
    st = m_loader.GetFiberResultCount(&count);
    if (st != SP_OK || count == 0)
    {
        PopulateResults(NULL, 0);
        AppendLog(_T("No fiber results"));
        return;
    }

    std::vector<SP_FIBER_RESULT> buf(count);
    uint32_t written = 0;
    st = m_loader.GetFiberResults(buf.data(), count, &written);
    if (st != SP_OK)
    {
        AppendLog(_T("SP_GetFiberResults failed"));
        return;
    }
    PopulateResults(buf.data(), written);
    CString msg;
    msg.Format(_T("Got %u fiber results"), written);
    AppendLog(msg);
}

void CPolarityTestAppDlg::OnBnClickedClearLog()
{
    m_editLog.SetWindowText(_T(""));
}
