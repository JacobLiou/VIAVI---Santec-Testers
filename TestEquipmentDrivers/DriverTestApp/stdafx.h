#pragma once

#include "targetver.h"

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif

#include <afxwin.h>
#include <afxext.h>
#include <afxdisp.h>

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>
#endif

#include <afxdialogex.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <mstcpip.h>
#pragma comment(lib, "ws2_32.lib")

#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <functional>
