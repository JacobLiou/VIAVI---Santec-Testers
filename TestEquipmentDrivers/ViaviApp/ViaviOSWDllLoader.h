#pragma once

// ---------------------------------------------------------------------------
// CViaviOSWDllLoader -- UDL.ViaviOSW.dll 动态加载器
//
// 通过 LoadLibrary / GetProcAddress 动态加载驱动 DLL
// 无需引用 UDL.ViaviOSW 的头文件或 .lib 静态库
// ---------------------------------------------------------------------------

#include <Windows.h>
#include <string>
#include <cstdio>

namespace OSWLoaderDetail {
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
// C 兼容结构体（与 OSWTypes.h 中 DLL 导出结构体对齐）
// ---------------------------------------------------------------------------

struct OSWDeviceInfo
{
    char serialNumber[64];
    char partNumber[64];
    char firmwareVersion[64];
    char description[128];
    int  slot;
};

struct OSWSwitchInfo
{
    int  deviceNum;
    char description[128];
    int  switchType;
    int  channelCount;
    int  currentChannel;
};

// ---------------------------------------------------------------------------
// UDL.ViaviOSW C 导出函数指针类型定义
// ---------------------------------------------------------------------------

// 生命周期
typedef HANDLE (WINAPI *PFN_OSW_CreateDriver)(const char* ip, int port);
typedef HANDLE (WINAPI *PFN_OSW_CreateDriverEx)(const char* address, int port, int commType);
typedef void   (WINAPI *PFN_OSW_DestroyDriver)(HANDLE hDriver);

// 连接
typedef BOOL (WINAPI *PFN_OSW_Connect)(HANDLE hDriver);
typedef void (WINAPI *PFN_OSW_Disconnect)(HANDLE hDriver);
typedef BOOL (WINAPI *PFN_OSW_IsConnected)(HANDLE hDriver);

// 设备信息
typedef BOOL (WINAPI *PFN_OSW_GetDeviceInfo)(HANDLE hDriver, OSWDeviceInfo* info);
typedef int  (WINAPI *PFN_OSW_CheckError)(HANDLE hDriver, char* message, int messageSize);

// 开关信息
typedef int  (WINAPI *PFN_OSW_GetDeviceCount)(HANDLE hDriver);
typedef BOOL (WINAPI *PFN_OSW_GetSwitchInfo)(HANDLE hDriver, int deviceNum, OSWSwitchInfo* info);

// 通道切换
typedef BOOL (WINAPI *PFN_OSW_SwitchChannel)(HANDLE hDriver, int deviceNum, int channel);
typedef int  (WINAPI *PFN_OSW_GetCurrentChannel)(HANDLE hDriver, int deviceNum);
typedef int  (WINAPI *PFN_OSW_GetChannelCount)(HANDLE hDriver, int deviceNum);

// 控制
typedef BOOL (WINAPI *PFN_OSW_SetLocalMode)(HANDLE hDriver, BOOL local);
typedef BOOL (WINAPI *PFN_OSW_Reset)(HANDLE hDriver);

// 同步
typedef BOOL (WINAPI *PFN_OSW_WaitForIdle)(HANDLE hDriver, int timeoutMs);

// 原始 SCPI
typedef BOOL (WINAPI *PFN_OSW_SendCommand)(HANDLE hDriver, const char* command,
                                            char* response, int responseSize);

// 日志
typedef void (WINAPI *PFN_OSWLogCallback)(int level, const char* source, const char* message);
typedef void (WINAPI *PFN_OSW_SetLogCallback)(PFN_OSWLogCallback callback);
typedef void (WINAPI *PFN_OSW_SetLogCallbackEx)(HANDLE hDriver, PFN_OSWLogCallback callback);

// VISA 枚举
typedef int (WINAPI *PFN_OSW_EnumerateVisaResources)(char* buffer, int bufferSize);

// ---------------------------------------------------------------------------
// CViaviOSWDllLoader 类
// ---------------------------------------------------------------------------

class CViaviOSWDllLoader
{
public:
    CViaviOSWDllLoader()
        : m_hDll(NULL)
        , m_hDriver(NULL)
        , pfnCreateDriver(NULL)
        , pfnCreateDriverEx(NULL)
        , pfnDestroyDriver(NULL)
        , pfnConnect(NULL)
        , pfnDisconnect(NULL)
        , pfnIsConnected(NULL)
        , pfnGetDeviceInfo(NULL)
        , pfnCheckError(NULL)
        , pfnGetDeviceCount(NULL)
        , pfnGetSwitchInfo(NULL)
        , pfnSwitchChannel(NULL)
        , pfnGetCurrentChannel(NULL)
        , pfnGetChannelCount(NULL)
        , pfnSetLocalMode(NULL)
        , pfnReset(NULL)
        , pfnWaitForIdle(NULL)
        , pfnSendCommand(NULL)
        , pfnSetLogCallback(NULL)
        , pfnSetLogCallbackEx(NULL)
        , pfnEnumerateVisaResources(NULL)
    {
    }

    ~CViaviOSWDllLoader()
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
        if (!m_hDll)
            return false;

        int resolved = 0;
        int total = 0;

        #define LOAD_PROC(varName, exportName) \
            ++total; \
            varName = (decltype(varName))OSWLoaderDetail::ResolveProcAddress(m_hDll, exportName); \
            if (varName) ++resolved;

        LOAD_PROC(pfnCreateDriver,             "OSW_CreateDriver");
        LOAD_PROC(pfnCreateDriverEx,           "OSW_CreateDriverEx");
        LOAD_PROC(pfnDestroyDriver,            "OSW_DestroyDriver");
        LOAD_PROC(pfnConnect,                  "OSW_Connect");
        LOAD_PROC(pfnDisconnect,               "OSW_Disconnect");
        LOAD_PROC(pfnIsConnected,              "OSW_IsConnected");
        LOAD_PROC(pfnGetDeviceInfo,            "OSW_GetDeviceInfo");
        LOAD_PROC(pfnCheckError,               "OSW_CheckError");
        LOAD_PROC(pfnGetDeviceCount,           "OSW_GetDeviceCount");
        LOAD_PROC(pfnGetSwitchInfo,            "OSW_GetSwitchInfo");
        LOAD_PROC(pfnSwitchChannel,            "OSW_SwitchChannel");
        LOAD_PROC(pfnGetCurrentChannel,        "OSW_GetCurrentChannel");
        LOAD_PROC(pfnGetChannelCount,          "OSW_GetChannelCount");
        LOAD_PROC(pfnSetLocalMode,             "OSW_SetLocalMode");
        LOAD_PROC(pfnReset,                    "OSW_Reset");
        LOAD_PROC(pfnWaitForIdle,              "OSW_WaitForIdle");
        LOAD_PROC(pfnSendCommand,              "OSW_SendCommand");
        LOAD_PROC(pfnSetLogCallback,           "OSW_SetLogCallback");
        LOAD_PROC(pfnSetLogCallbackEx,         "OSW_SetLogCallbackEx");
        LOAD_PROC(pfnEnumerateVisaResources,   "OSW_EnumerateVisaResources");

        #undef LOAD_PROC

        return (resolved == total);
    }

    void UnloadDll()
    {
        if (m_hDll)
        {
            ::FreeLibrary(m_hDll);
            m_hDll = NULL;
            pfnCreateDriver = NULL;
            pfnCreateDriverEx = NULL;
            pfnDestroyDriver = NULL;
            pfnConnect = NULL;
            pfnDisconnect = NULL;
            pfnIsConnected = NULL;
            pfnGetDeviceInfo = NULL;
            pfnCheckError = NULL;
            pfnGetDeviceCount = NULL;
            pfnGetSwitchInfo = NULL;
            pfnSwitchChannel = NULL;
            pfnGetCurrentChannel = NULL;
            pfnGetChannelCount = NULL;
            pfnSetLocalMode = NULL;
            pfnReset = NULL;
            pfnWaitForIdle = NULL;
            pfnSendCommand = NULL;
            pfnSetLogCallback = NULL;
            pfnSetLogCallbackEx = NULL;
            pfnEnumerateVisaResources = NULL;
        }
    }

    bool IsDllLoaded() const { return m_hDll != NULL; }

    // -----------------------------------------------------------------------
    // 驱动实例管理
    // -----------------------------------------------------------------------

    bool CreateDriver(const char* ip, int port)
    {
        if (!pfnCreateDriver || m_hDriver) return false;
        m_hDriver = pfnCreateDriver(ip, port);
        return (m_hDriver != NULL);
    }

    bool CreateDriverEx(const char* address, int port, int commType)
    {
        if (!pfnCreateDriverEx || m_hDriver) return false;
        m_hDriver = pfnCreateDriverEx(address, port, commType);
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
    bool HasVisaSupport() const { return pfnCreateDriverEx != NULL; }

    int EnumerateVisaResources(char* buffer, int bufferSize)
    {
        if (!pfnEnumerateVisaResources) return 0;
        return pfnEnumerateVisaResources(buffer, bufferSize);
    }

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

    BOOL IsConnected()
    {
        if (!m_hDriver || !pfnIsConnected) return FALSE;
        return pfnIsConnected(m_hDriver);
    }

    // -----------------------------------------------------------------------
    // 设备信息
    // -----------------------------------------------------------------------

    BOOL GetDeviceInfo(OSWDeviceInfo* info)
    {
        if (!m_hDriver || !pfnGetDeviceInfo || !info) return FALSE;
        return pfnGetDeviceInfo(m_hDriver, info);
    }

    int CheckError(char* message, int messageSize)
    {
        if (!m_hDriver || !pfnCheckError) return -1;
        return pfnCheckError(m_hDriver, message, messageSize);
    }

    // -----------------------------------------------------------------------
    // 开关信息
    // -----------------------------------------------------------------------

    int GetDeviceCount()
    {
        if (!m_hDriver || !pfnGetDeviceCount) return 0;
        return pfnGetDeviceCount(m_hDriver);
    }

    BOOL GetSwitchInfo(int deviceNum, OSWSwitchInfo* info)
    {
        if (!m_hDriver || !pfnGetSwitchInfo || !info) return FALSE;
        return pfnGetSwitchInfo(m_hDriver, deviceNum, info);
    }

    // -----------------------------------------------------------------------
    // 通道切换
    // -----------------------------------------------------------------------

    BOOL SwitchChannel(int deviceNum, int channel)
    {
        if (!m_hDriver || !pfnSwitchChannel) return FALSE;
        return pfnSwitchChannel(m_hDriver, deviceNum, channel);
    }

    int GetCurrentChannel(int deviceNum)
    {
        if (!m_hDriver || !pfnGetCurrentChannel) return -1;
        return pfnGetCurrentChannel(m_hDriver, deviceNum);
    }

    int GetChannelCount(int deviceNum)
    {
        if (!m_hDriver || !pfnGetChannelCount) return 0;
        return pfnGetChannelCount(m_hDriver, deviceNum);
    }

    // -----------------------------------------------------------------------
    // 控制
    // -----------------------------------------------------------------------

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

    BOOL WaitForIdle(int timeoutMs)
    {
        if (!m_hDriver || !pfnWaitForIdle) return FALSE;
        return pfnWaitForIdle(m_hDriver, timeoutMs);
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

    void SetLogCallback(PFN_OSWLogCallback callback)
    {
        if (m_hDriver && pfnSetLogCallbackEx)
            pfnSetLogCallbackEx(m_hDriver, callback);
        else if (pfnSetLogCallback)
            pfnSetLogCallback(callback);
    }

private:
    HMODULE m_hDll;
    HANDLE  m_hDriver;

    PFN_OSW_CreateDriver             pfnCreateDriver;
    PFN_OSW_CreateDriverEx           pfnCreateDriverEx;
    PFN_OSW_DestroyDriver            pfnDestroyDriver;
    PFN_OSW_Connect                  pfnConnect;
    PFN_OSW_Disconnect               pfnDisconnect;
    PFN_OSW_IsConnected              pfnIsConnected;
    PFN_OSW_GetDeviceInfo            pfnGetDeviceInfo;
    PFN_OSW_CheckError               pfnCheckError;
    PFN_OSW_GetDeviceCount           pfnGetDeviceCount;
    PFN_OSW_GetSwitchInfo            pfnGetSwitchInfo;
    PFN_OSW_SwitchChannel            pfnSwitchChannel;
    PFN_OSW_GetCurrentChannel        pfnGetCurrentChannel;
    PFN_OSW_GetChannelCount          pfnGetChannelCount;
    PFN_OSW_SetLocalMode             pfnSetLocalMode;
    PFN_OSW_Reset                    pfnReset;
    PFN_OSW_WaitForIdle              pfnWaitForIdle;
    PFN_OSW_SendCommand              pfnSendCommand;
    PFN_OSW_SetLogCallback           pfnSetLogCallback;
    PFN_OSW_SetLogCallbackEx         pfnSetLogCallbackEx;
    PFN_OSW_EnumerateVisaResources   pfnEnumerateVisaResources;
};
