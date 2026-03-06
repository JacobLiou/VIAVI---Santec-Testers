#pragma once

// ---------------------------------------------------------------------------
// CSantecOSXTcpLoader -- UDL.SantecOSX.dll 动态加载器 (TCP 模式)
//
// 通过 LoadLibrary / GetProcAddress 动态加载驱动 DLL
// 无需引用 UDL.SantecOSX 的头文件或 .lib 静态库
// ---------------------------------------------------------------------------

#include <Windows.h>
#include <string>
#include <cstdio>

namespace OSXLoaderDetail {
    static FARPROC ResolveProcAddress(HMODULE hDll, const char* name)
    {
        FARPROC p = ::GetProcAddress(hDll, name);
        if (p) return p;
#ifndef _WIN64
        char decorated[256];
        for (int n = 0; n <= 64; n += 4) {
            sprintf_s(decorated, "_%s@%d", name, n);
            p = ::GetProcAddress(hDll, decorated);
            if (p) return p;
        }
#endif
        return nullptr;
    }
}

// ---------------------------------------------------------------------------
// C 兼容结构体（与 OSXTypes.h 中 DLL 导出结构体对齐）
// ---------------------------------------------------------------------------

struct OSXDeviceInfo
{
    char manufacturer[64];
    char model[64];
    char serialNumber[64];
    char firmwareVersion[64];
};

struct OSXModuleInfo
{
    int  index;
    char catalogEntry[128];
    char detailedInfo[256];
    int  configType;
    int  channelCount;
    int  currentChannel;
    int  currentCommon;
};

// ---------------------------------------------------------------------------
// UDL.SantecOSX C 导出函数指针类型定义
// ---------------------------------------------------------------------------

// 生命周期
typedef HANDLE (WINAPI *PFN_OSX_CreateDriver)(const char* ip, int port);
typedef void   (WINAPI *PFN_OSX_DestroyDriver)(HANDLE hDriver);

// 连接
typedef BOOL (WINAPI *PFN_OSX_Connect)(HANDLE hDriver);
typedef void (WINAPI *PFN_OSX_Disconnect)(HANDLE hDriver);
typedef BOOL (WINAPI *PFN_OSX_Reconnect)(HANDLE hDriver);
typedef BOOL (WINAPI *PFN_OSX_IsConnected)(HANDLE hDriver);

// 设备识别
typedef BOOL (WINAPI *PFN_OSX_GetDeviceInfo)(HANDLE hDriver, OSXDeviceInfo* info);
typedef int  (WINAPI *PFN_OSX_CheckError)(HANDLE hDriver, char* message, int messageSize);
typedef BOOL (WINAPI *PFN_OSX_GetSystemVersion)(HANDLE hDriver, char* version, int versionSize);

// 模块管理
typedef int  (WINAPI *PFN_OSX_GetModuleCount)(HANDLE hDriver);
typedef int  (WINAPI *PFN_OSX_GetModuleCatalog)(HANDLE hDriver, OSXModuleInfo* modules, int maxCount);
typedef BOOL (WINAPI *PFN_OSX_GetModuleInfo)(HANDLE hDriver, int moduleIndex, OSXModuleInfo* info);
typedef BOOL (WINAPI *PFN_OSX_SelectModule)(HANDLE hDriver, int moduleIndex);
typedef BOOL (WINAPI *PFN_OSX_SelectNextModule)(HANDLE hDriver);
typedef int  (WINAPI *PFN_OSX_GetSelectedModule)(HANDLE hDriver);

// 通道切换
typedef BOOL (WINAPI *PFN_OSX_SwitchChannel)(HANDLE hDriver, int channel);
typedef BOOL (WINAPI *PFN_OSX_SwitchNext)(HANDLE hDriver);
typedef int  (WINAPI *PFN_OSX_GetCurrentChannel)(HANDLE hDriver);
typedef int  (WINAPI *PFN_OSX_GetChannelCount)(HANDLE hDriver);

// 多模块路由
typedef BOOL (WINAPI *PFN_OSX_RouteChannel)(HANDLE hDriver, int moduleIndex, int channel);

// 控制
typedef BOOL (WINAPI *PFN_OSX_WaitForOperation)(HANDLE hDriver, int timeoutMs);
typedef BOOL (WINAPI *PFN_OSX_SetLocalMode)(HANDLE hDriver, BOOL local);
typedef BOOL (WINAPI *PFN_OSX_Reset)(HANDLE hDriver);

// 原始 SCPI
typedef BOOL (WINAPI *PFN_OSX_SendCommand)(HANDLE hDriver, const char* command,
                                           char* response, int responseSize);

// 日志
typedef void (WINAPI *PFN_OSXLogCallback)(int level, const char* source, const char* message);
typedef void (WINAPI *PFN_OSX_SetLogCallback)(PFN_OSXLogCallback callback);

// ---------------------------------------------------------------------------
// CSantecOSXTcpLoader 类
// ---------------------------------------------------------------------------

class CSantecOSXTcpLoader
{
public:
    CSantecOSXTcpLoader()
        : m_hDll(NULL)
        , m_hDriver(NULL)
        , pfnCreateDriver(NULL)
        , pfnDestroyDriver(NULL)
        , pfnConnect(NULL)
        , pfnDisconnect(NULL)
        , pfnReconnect(NULL)
        , pfnIsConnected(NULL)
        , pfnGetDeviceInfo(NULL)
        , pfnCheckError(NULL)
        , pfnGetSystemVersion(NULL)
        , pfnGetModuleCount(NULL)
        , pfnGetModuleCatalog(NULL)
        , pfnGetModuleInfo(NULL)
        , pfnSelectModule(NULL)
        , pfnSelectNextModule(NULL)
        , pfnGetSelectedModule(NULL)
        , pfnSwitchChannel(NULL)
        , pfnSwitchNext(NULL)
        , pfnGetCurrentChannel(NULL)
        , pfnGetChannelCount(NULL)
        , pfnRouteChannel(NULL)
        , pfnWaitForOperation(NULL)
        , pfnSetLocalMode(NULL)
        , pfnReset(NULL)
        , pfnSendCommand(NULL)
        , pfnSetLogCallback(NULL)
    {
    }

    ~CSantecOSXTcpLoader()
    {
        DestroyDriver();
        UnloadDll();
    }

    // -----------------------------------------------------------------------
    // DLL 加载 / 卸载
    // -----------------------------------------------------------------------

    bool LoadDll(const char* dllPath)
    {
        if (m_hDll)
            return false;

        m_hDll = ::LoadLibraryA(dllPath);
        // #region agent log
        { char _modPath[512]={0}; if(m_hDll) ::GetModuleFileNameA(m_hDll, _modPath, 511);
          FILE* _f = fopen("c:\\Users\\menghl2\\WorkSpace\\Projects\\VIAVI---Santec-Testers\\debug-021a55.log", "a");
          if (_f) { fprintf(_f, "{\"sessionId\":\"021a55\",\"hypothesisId\":\"H1\",\"location\":\"SantecOSXTcpLoader.h:LoadDll\",\"message\":\"LoadLibraryA result\",\"data\":{\"dllPath\":\"%s\",\"hDll\":\"%p\",\"lastError\":%lu,\"fullPath\":\"%s\",\"ptrSize\":%d},\"timestamp\":%llu}\n", dllPath, m_hDll, m_hDll ? 0UL : ::GetLastError(), _modPath, (int)sizeof(void*), (unsigned long long)::GetTickCount64()); fclose(_f); } }
        // #endregion
        if (!m_hDll)
            return false;

        int resolved = 0;
        int total = 0;
        // #region agent log
        std::string _failedExports;
        // #endregion

        #define LOAD_PROC(varName, exportName) \
            ++total; \
            varName = (decltype(varName))OSXLoaderDetail::ResolveProcAddress(m_hDll, exportName); \
            if (varName) ++resolved; else { if(!_failedExports.empty()) _failedExports += ","; _failedExports += exportName; }

        LOAD_PROC(pfnCreateDriver,     "OSX_CreateDriver");
        LOAD_PROC(pfnDestroyDriver,    "OSX_DestroyDriver");
        LOAD_PROC(pfnConnect,          "OSX_Connect");
        LOAD_PROC(pfnDisconnect,       "OSX_Disconnect");
        LOAD_PROC(pfnReconnect,        "OSX_Reconnect");
        LOAD_PROC(pfnIsConnected,      "OSX_IsConnected");
        LOAD_PROC(pfnGetDeviceInfo,    "OSX_GetDeviceInfo");
        LOAD_PROC(pfnCheckError,       "OSX_CheckError");
        LOAD_PROC(pfnGetSystemVersion, "OSX_GetSystemVersion");
        LOAD_PROC(pfnGetModuleCount,   "OSX_GetModuleCount");
        LOAD_PROC(pfnGetModuleCatalog, "OSX_GetModuleCatalog");
        LOAD_PROC(pfnGetModuleInfo,    "OSX_GetModuleInfo");
        LOAD_PROC(pfnSelectModule,     "OSX_SelectModule");
        LOAD_PROC(pfnSelectNextModule, "OSX_SelectNextModule");
        LOAD_PROC(pfnGetSelectedModule,"OSX_GetSelectedModule");
        LOAD_PROC(pfnSwitchChannel,    "OSX_SwitchChannel");
        LOAD_PROC(pfnSwitchNext,       "OSX_SwitchNext");
        LOAD_PROC(pfnGetCurrentChannel,"OSX_GetCurrentChannel");
        LOAD_PROC(pfnGetChannelCount,  "OSX_GetChannelCount");
        LOAD_PROC(pfnRouteChannel,     "OSX_RouteChannel");
        LOAD_PROC(pfnWaitForOperation, "OSX_WaitForOperation");
        LOAD_PROC(pfnSetLocalMode,     "OSX_SetLocalMode");
        LOAD_PROC(pfnReset,            "OSX_Reset");
        LOAD_PROC(pfnSendCommand,      "OSX_SendCommand");
        LOAD_PROC(pfnSetLogCallback,   "OSX_SetLogCallback");

        #undef LOAD_PROC

        // #region agent log
        { FILE* _f = fopen("c:\\Users\\menghl2\\WorkSpace\\Projects\\VIAVI---Santec-Testers\\debug-021a55.log", "a");
          if (_f) { fprintf(_f, "{\"sessionId\":\"021a55\",\"hypothesisId\":\"H3\",\"location\":\"SantecOSXTcpLoader.h:LoadDll\",\"message\":\"OSX resolve summary\",\"data\":{\"resolved\":%d,\"total\":%d,\"failed\":\"%s\"},\"timestamp\":%llu}\n", resolved, total, _failedExports.c_str(), (unsigned long long)::GetTickCount64()); fclose(_f); } }
        // #endregion

        return (resolved == total);
    }

    void UnloadDll()
    {
        if (m_hDll)
        {
            ::FreeLibrary(m_hDll);
            m_hDll = NULL;
            pfnCreateDriver = NULL;
            pfnDestroyDriver = NULL;
            pfnConnect = NULL;
            pfnDisconnect = NULL;
            pfnReconnect = NULL;
            pfnIsConnected = NULL;
            pfnGetDeviceInfo = NULL;
            pfnCheckError = NULL;
            pfnGetSystemVersion = NULL;
            pfnGetModuleCount = NULL;
            pfnGetModuleCatalog = NULL;
            pfnGetModuleInfo = NULL;
            pfnSelectModule = NULL;
            pfnSelectNextModule = NULL;
            pfnGetSelectedModule = NULL;
            pfnSwitchChannel = NULL;
            pfnSwitchNext = NULL;
            pfnGetCurrentChannel = NULL;
            pfnGetChannelCount = NULL;
            pfnRouteChannel = NULL;
            pfnWaitForOperation = NULL;
            pfnSetLocalMode = NULL;
            pfnReset = NULL;
            pfnSendCommand = NULL;
            pfnSetLogCallback = NULL;
        }
    }

    bool IsDllLoaded() const { return m_hDll != NULL; }

    // -----------------------------------------------------------------------
    // 驱动实例管理 (TCP only)
    // -----------------------------------------------------------------------

    bool CreateDriver(const char* ip, int port)
    {
        if (!pfnCreateDriver || m_hDriver) return false;
        m_hDriver = pfnCreateDriver(ip, port);
        return (m_hDriver != NULL);
    }

    void DestroyDriver()
    {
        if (m_hDriver && pfnDestroyDriver)
        {
            pfnDestroyDriver(m_hDriver);
            m_hDriver = NULL;
        }
    }

    HANDLE GetDriverHandle() const { return m_hDriver; }

    // -----------------------------------------------------------------------
    // 连接操作
    // -----------------------------------------------------------------------

    BOOL Connect()
    {
        if (!m_hDriver || !pfnConnect) return FALSE;
        return pfnConnect(m_hDriver);
    }

    void Disconnect()
    {
        if (m_hDriver && pfnDisconnect) pfnDisconnect(m_hDriver);
    }

    BOOL Reconnect()
    {
        if (!m_hDriver || !pfnReconnect) return FALSE;
        return pfnReconnect(m_hDriver);
    }

    BOOL IsConnected()
    {
        if (!m_hDriver || !pfnIsConnected) return FALSE;
        return pfnIsConnected(m_hDriver);
    }

    // -----------------------------------------------------------------------
    // 设备识别
    // -----------------------------------------------------------------------

    BOOL GetDeviceInfo(OSXDeviceInfo* info)
    {
        if (!m_hDriver || !pfnGetDeviceInfo || !info) return FALSE;
        return pfnGetDeviceInfo(m_hDriver, info);
    }

    int CheckError(char* message, int messageSize)
    {
        if (!m_hDriver || !pfnCheckError) return -1;
        return pfnCheckError(m_hDriver, message, messageSize);
    }

    BOOL GetSystemVersion(char* version, int versionSize)
    {
        if (!m_hDriver || !pfnGetSystemVersion || !version) return FALSE;
        return pfnGetSystemVersion(m_hDriver, version, versionSize);
    }

    // -----------------------------------------------------------------------
    // 模块管理
    // -----------------------------------------------------------------------

    int GetModuleCount()
    {
        if (!m_hDriver || !pfnGetModuleCount) return 0;
        return pfnGetModuleCount(m_hDriver);
    }

    int GetModuleCatalog(OSXModuleInfo* modules, int maxCount)
    {
        if (!m_hDriver || !pfnGetModuleCatalog) return 0;
        return pfnGetModuleCatalog(m_hDriver, modules, maxCount);
    }

    BOOL GetModuleInfo(int moduleIndex, OSXModuleInfo* info)
    {
        if (!m_hDriver || !pfnGetModuleInfo || !info) return FALSE;
        return pfnGetModuleInfo(m_hDriver, moduleIndex, info);
    }

    BOOL SelectModule(int moduleIndex)
    {
        if (!m_hDriver || !pfnSelectModule) return FALSE;
        return pfnSelectModule(m_hDriver, moduleIndex);
    }

    BOOL SelectNextModule()
    {
        if (!m_hDriver || !pfnSelectNextModule) return FALSE;
        return pfnSelectNextModule(m_hDriver);
    }

    int GetSelectedModule()
    {
        if (!m_hDriver || !pfnGetSelectedModule) return -1;
        return pfnGetSelectedModule(m_hDriver);
    }

    // -----------------------------------------------------------------------
    // 通道切换
    // -----------------------------------------------------------------------

    BOOL SwitchChannel(int channel)
    {
        if (!m_hDriver || !pfnSwitchChannel) return FALSE;
        return pfnSwitchChannel(m_hDriver, channel);
    }

    BOOL SwitchNext()
    {
        if (!m_hDriver || !pfnSwitchNext) return FALSE;
        return pfnSwitchNext(m_hDriver);
    }

    int GetCurrentChannel()
    {
        if (!m_hDriver || !pfnGetCurrentChannel) return -1;
        return pfnGetCurrentChannel(m_hDriver);
    }

    int GetChannelCount()
    {
        if (!m_hDriver || !pfnGetChannelCount) return 0;
        return pfnGetChannelCount(m_hDriver);
    }

    // -----------------------------------------------------------------------
    // 多模块路由
    // -----------------------------------------------------------------------

    BOOL RouteChannel(int moduleIndex, int channel)
    {
        if (!m_hDriver || !pfnRouteChannel) return FALSE;
        return pfnRouteChannel(m_hDriver, moduleIndex, channel);
    }

    // -----------------------------------------------------------------------
    // 控制 / 同步
    // -----------------------------------------------------------------------

    BOOL WaitForOperation(int timeoutMs)
    {
        if (!m_hDriver || !pfnWaitForOperation) return FALSE;
        return pfnWaitForOperation(m_hDriver, timeoutMs);
    }

    BOOL SetLocalMode(BOOL local)
    {
        if (!m_hDriver || !pfnSetLocalMode) return FALSE;
        return pfnSetLocalMode(m_hDriver, local);
    }

    BOOL Reset()
    {
        if (!m_hDriver || !pfnReset) return FALSE;
        return pfnReset(m_hDriver);
    }

    // -----------------------------------------------------------------------
    // 原始 SCPI 命令
    // -----------------------------------------------------------------------

    BOOL SendCommand(const char* command, char* response, int responseSize)
    {
        if (!m_hDriver || !pfnSendCommand) return FALSE;
        return pfnSendCommand(m_hDriver, command, response, responseSize);
    }

    // -----------------------------------------------------------------------
    // 日志
    // -----------------------------------------------------------------------

    void SetLogCallback(PFN_OSXLogCallback callback)
    {
        if (pfnSetLogCallback) pfnSetLogCallback(callback);
    }

private:
    HMODULE m_hDll;
    HANDLE  m_hDriver;

    PFN_OSX_CreateDriver       pfnCreateDriver;
    PFN_OSX_DestroyDriver      pfnDestroyDriver;
    PFN_OSX_Connect            pfnConnect;
    PFN_OSX_Disconnect         pfnDisconnect;
    PFN_OSX_Reconnect          pfnReconnect;
    PFN_OSX_IsConnected        pfnIsConnected;
    PFN_OSX_GetDeviceInfo      pfnGetDeviceInfo;
    PFN_OSX_CheckError         pfnCheckError;
    PFN_OSX_GetSystemVersion   pfnGetSystemVersion;
    PFN_OSX_GetModuleCount     pfnGetModuleCount;
    PFN_OSX_GetModuleCatalog   pfnGetModuleCatalog;
    PFN_OSX_GetModuleInfo      pfnGetModuleInfo;
    PFN_OSX_SelectModule       pfnSelectModule;
    PFN_OSX_SelectNextModule   pfnSelectNextModule;
    PFN_OSX_GetSelectedModule  pfnGetSelectedModule;
    PFN_OSX_SwitchChannel      pfnSwitchChannel;
    PFN_OSX_SwitchNext         pfnSwitchNext;
    PFN_OSX_GetCurrentChannel  pfnGetCurrentChannel;
    PFN_OSX_GetChannelCount    pfnGetChannelCount;
    PFN_OSX_RouteChannel       pfnRouteChannel;
    PFN_OSX_WaitForOperation   pfnWaitForOperation;
    PFN_OSX_SetLocalMode       pfnSetLocalMode;
    PFN_OSX_Reset              pfnReset;
    PFN_OSX_SendCommand        pfnSendCommand;
    PFN_OSX_SetLogCallback     pfnSetLogCallback;
};
