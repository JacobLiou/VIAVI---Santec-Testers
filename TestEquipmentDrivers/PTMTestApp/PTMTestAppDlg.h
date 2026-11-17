#pragma once

#include "resource.h"
#include "SantecPTMDllLoader.h"

#define WM_LOG_MESSAGE (WM_USER + 100)

class CPTMTestAppDlg : public CDialogEx
{
public:
    CPTMTestAppDlg(CWnd* pParent = NULL);
    enum { IDD = IDD_PTMTESTAPP_DIALOG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK() override {}
    virtual void OnCancel() override { CDialogEx::OnCancel(); }

    DECLARE_MESSAGE_MAP()

    afx_msg void OnClose();
    afx_msg void OnBnClickedLoadDll();
    afx_msg void OnBnClickedUnloadDll();
    afx_msg void OnBnClickedConnect();
    afx_msg void OnBnClickedDisconnect();
    afx_msg void OnBnClickedInitialize();
    afx_msg void OnBnClickedEnumerate();
    afx_msg void OnBnClickedMeasure();
    afx_msg void OnBnClickedGetResults();
    afx_msg void OnBnClickedDetect();
    afx_msg void OnBnClickedVfl();
    afx_msg void OnBnClickedLaserOff();
    afx_msg void OnBnClickedLocal();
    afx_msg void OnBnClickedSendScpi();
    afx_msg void OnBnClickedClearLog();
    afx_msg void OnCbnSelchangeConnMode();
    afx_msg LRESULT OnLogMessage(WPARAM wParam, LPARAM lParam);

private:
    void AppendLog(const CString& text);
    void UpdateStatus(const CString& status);
    void EnableConnectionControls(bool dllLoaded);
    void EnableDeviceControls(bool connected);
    bool IsVisaMode();
    void PopulateResults(const PTMPolarityResult* results, int count);

    static void WINAPI LogThunk(int level, const char* source, const char* message);
    static CPTMTestAppDlg* s_instance;

    CEdit m_editDllPath;
    CComboBox m_comboConnMode;
    CComboBox m_comboAddress;
    CEdit m_editPort;
    CStatic m_staticStatus;
    CEdit m_editChannels;
    CEdit m_editDetectCh;
    CButton m_checkLocal;
    CEdit m_editScpi;
    CListCtrl m_listResults;
    CEdit m_editRaw;
    CEdit m_editLog;

    CSantecPTMDllLoader m_loader;
};
