#pragma once

#include "resource.h"
#include "DriverFactory.h"
#include "ViaviPCTDriver.h"
#include "SantecDriver.h"
#include "Logger.h"
#include <memory>
#include <thread>
#include <functional>

#define WM_LOG_MESSAGE  (WM_USER + 100)
#define WM_WORKER_DONE  (WM_USER + 101)

class CDriverTestAppDlg : public CDialogEx
{
public:
    CDriverTestAppDlg(CWnd* pParent = NULL);
    virtual ~CDriverTestAppDlg();

    enum { IDD = IDD_DRIVERTESTAPP_DIALOG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK() override;
    virtual void OnCancel() override;

    DECLARE_MESSAGE_MAP()

    afx_msg void OnClose();
    afx_msg void OnBnClickedConnect();
    afx_msg void OnBnClickedDisconnect();
    afx_msg void OnBnClickedInitialize();
    afx_msg void OnBnClickedConfigureOrl();
    afx_msg void OnBnClickedTakeReference();
    afx_msg void OnBnClickedTakeMeasurement();
    afx_msg void OnBnClickedGetResults();
    afx_msg void OnBnClickedRunFullTest();
    afx_msg void OnBnClickedClearLog();
    afx_msg void OnCbnSelchangeDeviceType();
    afx_msg void OnBnClickedOverride();

    afx_msg LRESULT OnLogMessage(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnWorkerDone(WPARAM wParam, LPARAM lParam);

private:
    // UI helpers
    void AppendLog(const CString& text);
    void UpdateStatus(const CString& status);
    void EnableControls(bool connected);
    void SetBusy(bool busy, const CString& statusText = _T(""));
    void PopulateResultsList(const std::vector<EquipmentDriver::MeasurementResult>& results);

    std::vector<double> GetSelectedWavelengths();
    std::vector<int> GetSelectedChannels();

    // Async worker infrastructure
    struct WorkerResult
    {
        bool success;
        bool hasResults;
        CString statusText;
        CString logMessage;
        std::vector<EquipmentDriver::MeasurementResult> results;
        WorkerResult() : success(false), hasResults(false) {}
    };

    void RunAsync(const CString& operationName, std::function<WorkerResult*()> task);

    // Controls
    CComboBox   m_comboDeviceType;
    CEdit       m_editIP;
    CEdit       m_editPort;
    CEdit       m_editSlot;
    CButton     m_check1310;
    CButton     m_check1550;
    CEdit       m_editChFrom;
    CEdit       m_editChTo;
    CComboBox   m_comboOrlMethod;
    CComboBox   m_comboOrlOrigin;
    CEdit       m_editAOffset;
    CEdit       m_editBOffset;
    CButton     m_checkOverride;
    CEdit       m_editILValue;
    CEdit       m_editLengthValue;
    CListCtrl   m_listResults;
    CEdit       m_editLog;

    // State
    EquipmentDriver::IEquipmentDriver* m_pDriver;
    bool m_bBusy;
};
