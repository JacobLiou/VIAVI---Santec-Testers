#pragma once

#include "resource.h"
#include "PCT4AllDllLoader.h"
#include <vector>
#include <thread>
#include <functional>
#include <atomic>

#define WM_LOG_MESSAGE  (WM_USER + 100)
#define WM_WORKER_DONE  (WM_USER + 101)

class CPCTAppDlg : public CDialogEx
{
public:
    CPCTAppDlg(CWnd* pParent = NULL);
    virtual ~CPCTAppDlg();

    enum { IDD = IDD_PCTAPP_DIALOG };

    struct MeasResult
    {
        int    channel;
        double wavelength;
        double insertionLoss;
        double returnLoss;
        double orlZone1;
        double orlZone2;
        double dutLength;
        double power;
    };

    static std::vector<MeasResult> ParseMeasureAllResponse(
        const std::string& response, int channel, int connMode = 0);

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK() override;
    virtual void OnCancel() override;

    DECLARE_MESSAGE_MAP()

    afx_msg void OnClose();

    afx_msg void OnBnClickedLoad();
    afx_msg void OnBnClickedConnect();
    afx_msg void OnBnClickedDisconnect();
    afx_msg void OnBnClickedZeroing();
    afx_msg void OnBnClickedMeasure();
    afx_msg void OnBnClickedStop();
    afx_msg void OnBnClickedClearLog();

    afx_msg LRESULT OnLogMessage(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnWorkerDone(WPARAM wParam, LPARAM lParam);
    afx_msg void OnTabSelChange(NMHDR* pNMHDR, LRESULT* pResult);

private:
    void AppendLog(const CString& text);
    void EnableControls();
    void SetBusy(bool busy, const CString& statusText = _T(""));
    void ShowTabControls(int tabIndex);
    int  GetActiveTab();

    std::string BuildSourceList();
    std::string BuildPathListChannels();
    std::string BuildDualSW1Channels();
    std::string BuildDualSW2Channels();

    void PopulateResultsList(const std::vector<MeasResult>& results);

    struct WorkerResult
    {
        bool success;
        bool hasResults;
        CString logMessage;
        CString statusText;
        std::vector<MeasResult> results;
        WorkerResult() : success(false), hasResults(false) {}
    };

    void RunAsync(const CString& operationName, std::function<WorkerResult*()> task);

    // Connection controls
    CEdit       m_editDllPath;
    CComboBox   m_comboAddr;
    CEdit       m_editPort;

    // Configuration controls - Tab
    CTabCtrl    m_tabConfig;

    // Shared controls
    CButton     m_check1310;
    CButton     m_check1450;
    CButton     m_check1550;
    CButton     m_check1625;
    CEdit       m_editAvgTime;

    // Single MTJ controls (Tab 0)
    CButton     m_radioJ1;
    CButton     m_radioJ2;
    CEdit       m_editChFrom;
    CEdit       m_editChTo;

    // Dual MTJ controls (Tab 1)
    CButton     m_radioDualJ1;
    CButton     m_radioDualJ2;
    CButton     m_checkBiDir;
    CEdit       m_editSW1From;
    CEdit       m_editSW1To;
    CEdit       m_editSW2From;
    CEdit       m_editSW2To;

    // Results / Log
    CListCtrl   m_listResults;
    CEdit       m_editLog;

    // State
    CPCT4AllDllLoader m_loader;
    bool m_bBusy;
    std::atomic<bool> m_bStopRequested;
    bool m_bConnected;
};
