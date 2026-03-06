#pragma once

#include "resource.h"
#include "SantecRLMTcpLoader.h"
#include "SantecOSXTcpLoader.h"
#include <vector>
#include <thread>
#include <functional>
#include <atomic>

#define WM_LOG_MESSAGE  (WM_USER + 100)
#define WM_WORKER_DONE  (WM_USER + 101)

class CSantecAppDlg : public CDialogEx
{
public:
    CSantecAppDlg(CWnd* pParent = NULL);
    virtual ~CSantecAppDlg();

    enum { IDD = IDD_SANTECAPP_DIALOG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK() override;
    virtual void OnCancel() override;

    DECLARE_MESSAGE_MAP()

    afx_msg void OnClose();

    // RLM
    afx_msg void OnBnClickedLoadRlm();
    afx_msg void OnBnClickedConnectRlm();
    afx_msg void OnBnClickedDisconnectRlm();

    // OSX
    afx_msg void OnBnClickedLoadOsx();
    afx_msg void OnBnClickedConnectOsx();
    afx_msg void OnBnClickedDisconnectOsx();

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
    void UpdateRlmStatus(const CString& status);
    void UpdateOsxStatus(const CString& status);
    void EnableControls();
    void SetBusy(bool busy, const CString& statusText = _T(""));

    void PopulateResultsList(const std::vector<RLMMeasurementResult>& results);

    std::vector<double> GetSelectedWavelengths();
    std::vector<int> GetSelectedChannels();
    int GetModuleIndex();

    struct WorkerResult
    {
        bool success;
        bool hasResults;
        CString statusText;
        CString logMessage;
        std::vector<RLMMeasurementResult> results;
        WorkerResult() : success(false), hasResults(false) {}
    };

    void RunAsync(const CString& operationName, std::function<WorkerResult*()> task);

    // RLM UI Controls
    CEdit       m_editRlmDll;
    CEdit       m_editRlmIp;
    CEdit       m_editRlmPort;

    // OSX UI Controls
    CEdit       m_editOsxDll;
    CEdit       m_editOsxIp;
    CEdit       m_editOsxPort;

    // Test Config
    CButton     m_check1310;
    CButton     m_check1550;
    CEdit       m_editChFrom;
    CEdit       m_editChTo;
    CEdit       m_editModuleIndex;
    CButton     m_checkOverride;
    CEdit       m_editILValue;
    CEdit       m_editLengthValue;

    // Results / Log
    CListCtrl   m_listResults;
    CEdit       m_editLog;

    // Dynamic loaders
    CSantecRLMTcpLoader m_rlmLoader;
    CSantecOSXTcpLoader m_osxLoader;

    bool m_bBusy;
    std::atomic<bool> m_bStopRequested;
    bool m_bRlmConnected;
    bool m_bOsxConnected;
};
