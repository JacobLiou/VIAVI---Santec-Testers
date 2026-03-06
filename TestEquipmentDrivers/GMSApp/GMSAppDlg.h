#pragma once

#include "resource.h"
#include "SantecRLMDllLoader.h"
#include <vector>
#include <thread>
#include <functional>
#include <atomic>

#define WM_LOG_MESSAGE  (WM_USER + 100)
#define WM_WORKER_DONE  (WM_USER + 101)

class CGMSAppDlg : public CDialogEx
{
public:
    CGMSAppDlg(CWnd* pParent = NULL);
    virtual ~CGMSAppDlg();

    enum { IDD = IDD_GMSAPP_DIALOG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK() override;
    virtual void OnCancel() override;

    DECLARE_MESSAGE_MAP()

    afx_msg void OnClose();
    afx_msg void OnBnClickedLoadRlm();
    afx_msg void OnBnClickedEnumerate();
    afx_msg void OnBnClickedConnect();
    afx_msg void OnBnClickedDisconnect();
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
    void UpdateStatus(const CString& status);
    void EnableControls();
    void SetBusy(bool busy, const CString& statusText = _T(""));

    void PopulateResultsList(const std::vector<DriverMeasurementResult>& results);

    std::vector<double> GetSelectedWavelengths();
    std::vector<int> GetSelectedChannels();
    int GetSelectedSwitchNum();

    struct WorkerResult
    {
        bool success;
        bool hasResults;
        CString statusText;
        CString logMessage;
        std::vector<DriverMeasurementResult> results;
        WorkerResult() : success(false), hasResults(false) {}
    };

    void RunAsync(const CString& operationName, std::function<WorkerResult*()> task);

    // UI Controls
    CEdit       m_editRlmDll;
    CComboBox   m_comboRlmAddr;
    CButton     m_check1310;
    CButton     m_check1550;
    CEdit       m_editChFrom;
    CEdit       m_editChTo;
    CComboBox   m_comboSwitchNum;
    CButton     m_checkOverride;
    CEdit       m_editILValue;
    CEdit       m_editLengthValue;
    CListCtrl   m_listResults;
    CEdit       m_editLog;

    // Dynamic loader
    CSantecRLMDllLoader m_rlmLoader;

    bool m_bBusy;
    std::atomic<bool> m_bStopRequested;
    bool m_bRlmConnected;
};
