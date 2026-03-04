#pragma once

#ifndef __AFXWIN_H__
#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"

class CDriverTestApp3App : public CWinApp
{
public:
    CDriverTestApp3App();

    virtual BOOL InitInstance();

    DECLARE_MESSAGE_MAP()
};

extern CDriverTestApp3App theApp;
