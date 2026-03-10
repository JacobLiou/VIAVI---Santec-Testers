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

class CViaviAppDlg : public CDialogEx
{
public:
    CViaviAppDlg(CWnd* pParent = NULL);
    virtual ~CViaviAppDlg();

    enum { IDD = IDD_VIAVIAPP_DIALOG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK() override;
    virtual void OnCancel() override;

    DECLARE_MESSAGE_MAP()

    afx_msg void OnClose();

    // PCT
    afx_msg void OnBnClickedLoadPct();
    afx_msg void OnBnClickedEnumeratePct();
    afx_msg void OnBnClickedConnectPct();
    afx_msg void OnBnClickedDisconnectPct();

    // OSW1
    afx_msg void OnBnClickedLoadOsw();
    afx_msg void OnBnClickedEnumerateOsw();
    afx_msg void OnBnClickedConnectOsw();
    afx_msg void OnBnClickedDisconnectOsw();

    // OSW2
    afx_msg void OnBnClickedLoadOsw2();
    afx_msg void OnBnClickedEnumerateOsw2();
    afx_msg void OnBnClickedConnectOsw2();
    afx_msg void OnBnClickedDisconnectOsw2();

    // 操作
    afx_msg void OnBnClickedZeroing();
    afx_msg void OnBnClickedMeasure();
    afx_msg void OnBnClickedContinuous();
    afx_msg void OnBnClickedStop();
    afx_msg void OnBnClickedClearLog();
    afx_msg void OnBnClickedOverride();

    afx_msg LRESULT OnLogMessage(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnWorkerDone(WPARAM wParam, LPARAM lParam);

private:
    void AppendLog(const CString& text);
    void UpdatePctStatus(const CString& status);
    void UpdateOswStatus(const CString& status);
    void UpdateOsw2Status(const CString& status);
    void EnableControls();
    void SetBusy(bool busy, const CString& statusText = _T(""));
    bool IsPctVisaMode();
    bool IsOswVisaMode();
    bool IsOsw2VisaMode();

    void PopulateResultsList(const std::vector<PCTMeasurementResult>& results);

    std::vector<double> GetSelectedWavelengths();
    std::vector<int> GetSelectedChannels();
    int GetOswDeviceNum();
    int GetOsw2DeviceNum();

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

    // PCT UI Controls
    CEdit       m_editPctDll;
    CComboBox   m_comboPctConnMode;
    CComboBox   m_comboPctAddr;
    CEdit       m_editPctPort;

    // OSW1 UI Controls
    CEdit       m_editOswDll;
    CComboBox   m_comboOswConnMode;
    CComboBox   m_comboOswAddr;
    CEdit       m_editOswPort;

    // OSW2 UI Controls
    CEdit       m_editOsw2Dll;
    CComboBox   m_comboOsw2ConnMode;
    CComboBox   m_comboOsw2Addr;
    CEdit       m_editOsw2Port;

    // Test Config
    CButton     m_check1310;
    CButton     m_check1550;
    CEdit       m_editChFrom;
    CEdit       m_editChTo;
    CEdit       m_editOswDeviceNum;
    CEdit       m_editOsw2DeviceNum;
    CButton     m_checkOverride;
    CEdit       m_editILValue;
    CEdit       m_editLengthValue;

    // Results / Log
    CListCtrl   m_listResults;
    CEdit       m_editLog;

    // Dynamic loaders
    CViaviPCTDllLoader m_pctLoader;
    CViaviOSWDllLoader m_oswLoader;
    CViaviOSWDllLoader m_osw2Loader;

    bool m_bBusy;
    std::atomic<bool> m_bStopRequested;
    bool m_bPctConnected;
    bool m_bOswConnected;
    bool m_bOsw2Connected;
};
