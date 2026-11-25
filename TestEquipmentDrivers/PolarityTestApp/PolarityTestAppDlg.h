#pragma once

#include "resource.h"
#include "SantecPolarityDllLoader.h"

class CPolarityTestAppDlg : public CDialogEx
{
public:
    CPolarityTestAppDlg(CWnd* pParent = NULL);
    enum { IDD = IDD_POLARITYTESTAPP_DIALOG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK() override {}
    virtual void OnCancel() override { CDialogEx::OnCancel(); }

    DECLARE_MESSAGE_MAP()

    afx_msg void OnClose();
    afx_msg void OnBnClickedLoadDll();
    afx_msg void OnBnClickedUnloadDll();
    afx_msg void OnBnClickedCreate();
    afx_msg void OnBnClickedDestroy();
    afx_msg void OnBnClickedOpen();
    afx_msg void OnBnClickedClose();
    afx_msg void OnBnClickedSnapshot();
    afx_msg void OnBnClickedRunMapping();
    afx_msg void OnBnClickedCancel();
    afx_msg void OnBnClickedGetResults();
    afx_msg void OnBnClickedClearLog();

private:
    void AppendLog(const CString& text);
    void UpdateStatus(const CString& status);
    void EnableAfterLoad(bool loaded);
    void EnableAfterCreate(bool created);
    void EnableAfterOpen(bool opened);
    CString StatusToString(SP_STATUS st);
    CString FiberStatusToString(SP_FIBER_STATUS st);
    CStringA GetUtf8(CEdit& edit);
    void PopulateResults(const SP_FIBER_RESULT* results, uint32_t count);

    CEdit m_editDllPath;
    CEdit m_editStation;
    CEdit m_editRlmSerial;
    CEdit m_editRdspSerial;
    CEdit m_editProfile;
    CStatic m_staticStatus;
    CListCtrl m_listResults;
    CEdit m_editRaw;
    CEdit m_editLog;

    CSantecPolarityDllLoader m_loader;
    bool m_opened;
};
