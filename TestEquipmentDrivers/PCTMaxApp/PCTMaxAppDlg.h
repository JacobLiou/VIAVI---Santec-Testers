#pragma once

#include "resource.h"
#include "ViaviPCTDllLoader.h"
#include "ViaviOSWDllLoader.h"
#include <vector>
#include <thread>
#include <functional>
#include <atomic>

#define WM_LOG_MESSAGE  (WM_USER + 100)
#define WM_WORKER_DONE  (WM_USER + 101)

class CPCTMaxAppDlg : public CDialogEx
{
public:
    CPCTMaxAppDlg(CWnd* pParent = NULL);
    virtual ~CPCTMaxAppDlg();

    enum { IDD = IDD_PCTMAXAPP_DIALOG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK() override;
    virtual void OnCancel() override;

    DECLARE_MESSAGE_MAP()

    afx_msg void OnClose();
    afx_msg void OnBnClickedConnect();
    afx_msg void OnBnClickedDisconnect();
    afx_msg void OnBnClickedZeroing();
    afx_msg void OnBnClickedMeasure();
    afx_msg void OnBnClickedContinuous();
    afx_msg void OnBnClickedStop();
    afx_msg void OnBnClickedClearLog();
    afx_msg void OnBnClickedOverride();
    afx_msg void OnBnClickedScreenshot();
    afx_msg void OnBnClickedSaveCsv();
    afx_msg LRESULT OnLogMessage(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnWorkerDone(WPARAM wParam, LPARAM lParam);

private:
    void AppendLog(const CString& text);
    void UpdateStatus(const CString& status);
    void EnableControls();
    void SetBusy(bool busy, const CString& statusText = _T(""));

    void PopulateResultsList(const std::vector<PCTMeasurementResult>& results);
    std::vector<double> GetSelectedWavelengths();
    std::vector<int> GetSelectedChannels();
    int GetOswDeviceNum();
    int GetOsw2DeviceNum();
    bool IsDualOswMode();

    struct WorkerResult
    {
        bool success;
        bool hasResults;
        CString statusText;
        CString logMessage;
        std::vector<PCTMeasurementResult> results;
        WorkerResult() : success(false), hasResults(false) {}
    };

    void RunAsync(const CString& operationName, std::function<WorkerResult*()> task);

    bool SaveWindowAsPng(HWND hWnd, const CString& filePath);
    static int GetEncoderClsid(const WCHAR* format, CLSID* pClsid);

    // Connection UI
    CEdit       m_editPctDll;
    CEdit       m_editOswDll;
    CEdit       m_editIP;
    CEdit       m_editPctPort;
    CEdit       m_editOswPort;
    CEdit       m_editOsw2Port;

    // Setup UI
    CEdit       m_editChFrom;
    CEdit       m_editChTo;
    CButton     m_check1310;
    CButton     m_check1550;
    CEdit       m_editOswDeviceNum;
    CEdit       m_editOsw2DeviceNum;
    CButton     m_radioOsw1Only;
    CButton     m_radioOswBoth;
    CButton     m_checkOverride;
    CEdit       m_editILValue;
    CEdit       m_editLengthValue;

    // Results / Log
    CListCtrl   m_listResults;
    CEdit       m_editLog;

    // Loaders
    CViaviPCTDllLoader m_pctLoader;
    CViaviOSWDllLoader m_oswLoader;
    CViaviOSWDllLoader m_osw2Loader;

    bool m_bBusy;
    std::atomic<bool> m_bStopRequested;
    bool m_bPctConnected;
    bool m_bOswConnected;
    bool m_bOsw2Connected;
};
