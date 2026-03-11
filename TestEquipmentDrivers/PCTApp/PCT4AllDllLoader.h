#pragma once

// ---------------------------------------------------------------------------
// CPCT4AllDllLoader -- UDL.ViaviPCT4All.dll 动态加载器
//
// 通过 LoadLibrary / GetProcAddress 动态加载驱动 DLL
// 无需引用 UDL.ViaviPCT4All 的头文件或 .lib 静态库
// ---------------------------------------------------------------------------

#include <Windows.h>
#include <string>
#include <cstdio>

namespace PCT4LoaderDetail {
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
// C 兼容结构体 (与 PCT4AllTypes.h 中导出结构体对齐)
// ---------------------------------------------------------------------------

struct PCT4IdentificationInfo
{
    char manufacturer[64];
    char platform[64];
    char serialNumber[64];
    char firmwareVersion[64];
};

struct PCT4CassetteInfo
{
    char serialNumber[64];
    char partNumber[64];
    char firmwareVersion[64];
    char hardwareVersion[64];
    char assemblyDate[32];
    char description[128];
};

// ---------------------------------------------------------------------------
// 函数指针类型定义
// ---------------------------------------------------------------------------

typedef HANDLE (WINAPI *PFN_PCT4_CreateDriver)(const char* type, const char* ip, int port, int slot);
typedef HANDLE (WINAPI *PFN_PCT4_CreateDriverEx)(const char* type, const char* address,
                                                  int port, int slot, int commType);
typedef void   (WINAPI *PFN_PCT4_DestroyDriver)(HANDLE hDriver);
typedef BOOL   (WINAPI *PFN_PCT4_Connect)(HANDLE hDriver);
typedef void   (WINAPI *PFN_PCT4_Disconnect)(HANDLE hDriver);
typedef BOOL   (WINAPI *PFN_PCT4_Initialize)(HANDLE hDriver);
typedef BOOL   (WINAPI *PFN_PCT4_IsConnected)(HANDLE hDriver);
typedef BOOL   (WINAPI *PFN_PCT4_GetIdentification)(HANDLE hDriver, PCT4IdentificationInfo* info);
typedef BOOL   (WINAPI *PFN_PCT4_GetCassetteInfo)(HANDLE hDriver, PCT4CassetteInfo* info);
typedef int    (WINAPI *PFN_PCT4_CheckError)(HANDLE hDriver, char* message, int messageSize);
typedef BOOL   (WINAPI *PFN_PCT4_SendCommand)(HANDLE hDriver, const char* command,
                                               char* response, int responseSize);
typedef BOOL   (WINAPI *PFN_PCT4_SendWrite)(HANDLE hDriver, const char* command);
typedef BOOL   (WINAPI *PFN_PCT4_StartMeasurement)(HANDLE hDriver);
typedef BOOL   (WINAPI *PFN_PCT4_StopMeasurement)(HANDLE hDriver);
typedef int    (WINAPI *PFN_PCT4_GetMeasurementState)(HANDLE hDriver);
typedef BOOL   (WINAPI *PFN_PCT4_WaitForIdle)(HANDLE hDriver, int timeoutMs);
typedef BOOL   (WINAPI *PFN_PCT4_MeasureReset)(HANDLE hDriver);
typedef BOOL   (WINAPI *PFN_PCT4_SetFunction)(HANDLE hDriver, int mode);
typedef int    (WINAPI *PFN_PCT4_GetFunction)(HANDLE hDriver);
typedef BOOL   (WINAPI *PFN_PCT4_SetWavelength)(HANDLE hDriver, int wavelength);
typedef BOOL   (WINAPI *PFN_PCT4_SetSourceList)(HANDLE hDriver, const char* wavelengths);
typedef BOOL   (WINAPI *PFN_PCT4_SetAveragingTime)(HANDLE hDriver, int seconds);
typedef BOOL   (WINAPI *PFN_PCT4_SetRange)(HANDLE hDriver, int range);
typedef BOOL   (WINAPI *PFN_PCT4_SetILOnly)(HANDLE hDriver, int state);
typedef BOOL   (WINAPI *PFN_PCT4_SetConnection)(HANDLE hDriver, int mode);
typedef BOOL   (WINAPI *PFN_PCT4_SetBiDir)(HANDLE hDriver, int state);
typedef BOOL   (WINAPI *PFN_PCT4_SetChannel)(HANDLE hDriver, int group, int channel);
typedef int    (WINAPI *PFN_PCT4_GetChannel)(HANDLE hDriver, int group);
typedef BOOL   (WINAPI *PFN_PCT4_SetPathList)(HANDLE hDriver, int sw, const char* channels);
typedef BOOL   (WINAPI *PFN_PCT4_SetLaunch)(HANDLE hDriver, int port);
typedef void   (WINAPI *PFN_PCT4LogCallback)(int level, const char* source, const char* message);
typedef void   (WINAPI *PFN_PCT4_SetLogCallback)(PFN_PCT4LogCallback callback);
typedef int    (WINAPI *PFN_PCT4_EnumerateVisaResources)(char* buffer, int bufferSize);

// ---------------------------------------------------------------------------
// CPCT4AllDllLoader 类
// ---------------------------------------------------------------------------

class CPCT4AllDllLoader
{
public:
    CPCT4AllDllLoader()
        : m_hDll(NULL), m_hDriver(NULL)
        , pfnCreateDriver(NULL), pfnCreateDriverEx(NULL), pfnDestroyDriver(NULL)
        , pfnConnect(NULL), pfnDisconnect(NULL), pfnInitialize(NULL), pfnIsConnected(NULL)
        , pfnGetIdentification(NULL), pfnGetCassetteInfo(NULL), pfnCheckError(NULL)
        , pfnSendCommand(NULL), pfnSendWrite(NULL)
        , pfnStartMeasurement(NULL), pfnStopMeasurement(NULL)
        , pfnGetMeasurementState(NULL), pfnWaitForIdle(NULL), pfnMeasureReset(NULL)
        , pfnSetFunction(NULL), pfnGetFunction(NULL)
        , pfnSetWavelength(NULL), pfnSetSourceList(NULL)
        , pfnSetAveragingTime(NULL), pfnSetRange(NULL)
        , pfnSetILOnly(NULL), pfnSetConnection(NULL), pfnSetBiDir(NULL)
        , pfnSetChannel(NULL), pfnGetChannel(NULL)
        , pfnSetPathList(NULL), pfnSetLaunch(NULL)
        , pfnSetLogCallback(NULL), pfnEnumerateVisaResources(NULL)
    {}

    ~CPCT4AllDllLoader()
    {
        DestroyDriver();
        UnloadDll();
    }

    // -----------------------------------------------------------------------
    // DLL 加载 / 卸载
    // -----------------------------------------------------------------------

    bool LoadDll(const char* dllPath)
    {
        if (m_hDll) return false;
        m_hDll = ::LoadLibraryA(dllPath);
        if (!m_hDll) return false;

        int resolved = 0, total = 0;

        #define LOAD_PROC(var, name) \
            ++total; \
            var = (decltype(var))PCT4LoaderDetail::ResolveProcAddress(m_hDll, name); \
            if (var) ++resolved;

        LOAD_PROC(pfnCreateDriver,         "PCT4_CreateDriver");
        LOAD_PROC(pfnCreateDriverEx,       "PCT4_CreateDriverEx");
        LOAD_PROC(pfnDestroyDriver,        "PCT4_DestroyDriver");
        LOAD_PROC(pfnConnect,              "PCT4_Connect");
        LOAD_PROC(pfnDisconnect,           "PCT4_Disconnect");
        LOAD_PROC(pfnInitialize,           "PCT4_Initialize");
        LOAD_PROC(pfnIsConnected,          "PCT4_IsConnected");
        LOAD_PROC(pfnGetIdentification,    "PCT4_GetIdentification");
        LOAD_PROC(pfnGetCassetteInfo,      "PCT4_GetCassetteInfo");
        LOAD_PROC(pfnCheckError,           "PCT4_CheckError");
        LOAD_PROC(pfnSendCommand,          "PCT4_SendCommand");
        LOAD_PROC(pfnSendWrite,            "PCT4_SendWrite");
        LOAD_PROC(pfnStartMeasurement,     "PCT4_StartMeasurement");
        LOAD_PROC(pfnStopMeasurement,      "PCT4_StopMeasurement");
        LOAD_PROC(pfnGetMeasurementState,  "PCT4_GetMeasurementState");
        LOAD_PROC(pfnWaitForIdle,          "PCT4_WaitForIdle");
        LOAD_PROC(pfnMeasureReset,         "PCT4_MeasureReset");
        LOAD_PROC(pfnSetFunction,          "PCT4_SetFunction");
        LOAD_PROC(pfnGetFunction,          "PCT4_GetFunction");
        LOAD_PROC(pfnSetWavelength,        "PCT4_SetWavelength");
        LOAD_PROC(pfnSetSourceList,        "PCT4_SetSourceList");
        LOAD_PROC(pfnSetAveragingTime,     "PCT4_SetAveragingTime");
        LOAD_PROC(pfnSetRange,             "PCT4_SetRange");
        LOAD_PROC(pfnSetILOnly,            "PCT4_SetILOnly");
        LOAD_PROC(pfnSetConnection,        "PCT4_SetConnection");
        LOAD_PROC(pfnSetBiDir,             "PCT4_SetBiDir");
        LOAD_PROC(pfnSetChannel,           "PCT4_SetChannel");
        LOAD_PROC(pfnGetChannel,           "PCT4_GetChannel");
        LOAD_PROC(pfnSetPathList,          "PCT4_SetPathList");
        LOAD_PROC(pfnSetLaunch,            "PCT4_SetLaunch");
        LOAD_PROC(pfnSetLogCallback,       "PCT4_SetLogCallback");
        LOAD_PROC(pfnEnumerateVisaResources, "PCT4_EnumerateVisaResources");

        #undef LOAD_PROC

        return (resolved == total);
    }

    void UnloadDll()
    {
        if (m_hDll) { ::FreeLibrary(m_hDll); m_hDll = NULL; }
        pfnCreateDriver = NULL; pfnCreateDriverEx = NULL; pfnDestroyDriver = NULL;
        pfnConnect = NULL; pfnDisconnect = NULL; pfnInitialize = NULL; pfnIsConnected = NULL;
        pfnGetIdentification = NULL; pfnGetCassetteInfo = NULL; pfnCheckError = NULL;
        pfnSendCommand = NULL; pfnSendWrite = NULL;
        pfnStartMeasurement = NULL; pfnStopMeasurement = NULL;
        pfnGetMeasurementState = NULL; pfnWaitForIdle = NULL; pfnMeasureReset = NULL;
        pfnSetFunction = NULL; pfnGetFunction = NULL;
        pfnSetWavelength = NULL; pfnSetSourceList = NULL;
        pfnSetAveragingTime = NULL; pfnSetRange = NULL;
        pfnSetILOnly = NULL; pfnSetConnection = NULL; pfnSetBiDir = NULL;
        pfnSetChannel = NULL; pfnGetChannel = NULL;
        pfnSetPathList = NULL; pfnSetLaunch = NULL;
        pfnSetLogCallback = NULL; pfnEnumerateVisaResources = NULL;
    }

    bool IsDllLoaded() const { return m_hDll != NULL; }

    // -----------------------------------------------------------------------
    // 驱动实例管理
    // -----------------------------------------------------------------------

    bool CreateDriver(const char* type, const char* ip, int port, int slot)
    {
        if (!pfnCreateDriver || m_hDriver) return false;
        m_hDriver = pfnCreateDriver(type, ip, port, slot);
        return (m_hDriver != NULL);
    }

    bool CreateDriverEx(const char* type, const char* address, int port, int slot, int commType)
    {
        if (!pfnCreateDriverEx || m_hDriver) return false;
        m_hDriver = pfnCreateDriverEx(type, address, port, slot, commType);
        return (m_hDriver != NULL);
    }

    void DestroyDriver()
    {
        if (m_hDriver && pfnDestroyDriver) { pfnDestroyDriver(m_hDriver); m_hDriver = NULL; }
    }

    HANDLE GetDriverHandle() const { return m_hDriver; }
    bool HasVisaSupport() const { return pfnCreateDriverEx != NULL; }

    // -----------------------------------------------------------------------
    // 连接
    // -----------------------------------------------------------------------

    BOOL Connect()          { if (!m_hDriver || !pfnConnect) return FALSE; return pfnConnect(m_hDriver); }
    void Disconnect()       { if (m_hDriver && pfnDisconnect) pfnDisconnect(m_hDriver); }
    BOOL Initialize()       { if (!m_hDriver || !pfnInitialize) return FALSE; return pfnInitialize(m_hDriver); }
    BOOL IsConnected()      { if (!m_hDriver || !pfnIsConnected) return FALSE; return pfnIsConnected(m_hDriver); }

    // -----------------------------------------------------------------------
    // 设备信息
    // -----------------------------------------------------------------------

    BOOL GetIdentification(PCT4IdentificationInfo* info)
    {
        if (!m_hDriver || !pfnGetIdentification || !info) return FALSE;
        return pfnGetIdentification(m_hDriver, info);
    }

    BOOL GetCassetteInfo(PCT4CassetteInfo* info)
    {
        if (!m_hDriver || !pfnGetCassetteInfo || !info) return FALSE;
        return pfnGetCassetteInfo(m_hDriver, info);
    }

    int CheckError(char* message, int messageSize)
    {
        if (!m_hDriver || !pfnCheckError) return -1;
        return pfnCheckError(m_hDriver, message, messageSize);
    }

    // -----------------------------------------------------------------------
    // 原始 SCPI
    // -----------------------------------------------------------------------

    BOOL SendCommand(const char* command, char* response, int responseSize)
    {
        if (!m_hDriver || !pfnSendCommand) return FALSE;
        return pfnSendCommand(m_hDriver, command, response, responseSize);
    }

    BOOL SendWrite(const char* command)
    {
        if (!m_hDriver || !pfnSendWrite) return FALSE;
        return pfnSendWrite(m_hDriver, command);
    }

    // -----------------------------------------------------------------------
    // 测量控制
    // -----------------------------------------------------------------------

    BOOL StartMeasurement()
    {
        if (!m_hDriver || !pfnStartMeasurement) return FALSE;
        return pfnStartMeasurement(m_hDriver);
    }

    BOOL StopMeasurement()
    {
        if (!m_hDriver || !pfnStopMeasurement) return FALSE;
        return pfnStopMeasurement(m_hDriver);
    }

    int GetMeasurementState()
    {
        if (!m_hDriver || !pfnGetMeasurementState) return -1;
        return pfnGetMeasurementState(m_hDriver);
    }

    BOOL WaitForIdle(int timeoutMs)
    {
        if (!m_hDriver || !pfnWaitForIdle) return FALSE;
        return pfnWaitForIdle(m_hDriver, timeoutMs);
    }

    BOOL MeasureReset()
    {
        if (!m_hDriver || !pfnMeasureReset) return FALSE;
        return pfnMeasureReset(m_hDriver);
    }

    // -----------------------------------------------------------------------
    // 配置
    // -----------------------------------------------------------------------

    BOOL SetFunction(int mode)          { if (!m_hDriver || !pfnSetFunction) return FALSE; return pfnSetFunction(m_hDriver, mode); }
    int  GetFunction()                  { if (!m_hDriver || !pfnGetFunction) return -1; return pfnGetFunction(m_hDriver); }
    BOOL SetWavelength(int wavelength)  { if (!m_hDriver || !pfnSetWavelength) return FALSE; return pfnSetWavelength(m_hDriver, wavelength); }
    BOOL SetSourceList(const char* wl)  { if (!m_hDriver || !pfnSetSourceList) return FALSE; return pfnSetSourceList(m_hDriver, wl); }
    BOOL SetAveragingTime(int seconds)  { if (!m_hDriver || !pfnSetAveragingTime) return FALSE; return pfnSetAveragingTime(m_hDriver, seconds); }
    BOOL SetRange(int range)            { if (!m_hDriver || !pfnSetRange) return FALSE; return pfnSetRange(m_hDriver, range); }
    BOOL SetILOnly(int state)           { if (!m_hDriver || !pfnSetILOnly) return FALSE; return pfnSetILOnly(m_hDriver, state); }
    BOOL SetConnection(int mode)        { if (!m_hDriver || !pfnSetConnection) return FALSE; return pfnSetConnection(m_hDriver, mode); }
    BOOL SetBiDir(int state)            { if (!m_hDriver || !pfnSetBiDir) return FALSE; return pfnSetBiDir(m_hDriver, state); }

    // -----------------------------------------------------------------------
    // PATH / 光路控制
    // -----------------------------------------------------------------------

    BOOL SetChannel(int group, int ch)           { if (!m_hDriver || !pfnSetChannel) return FALSE; return pfnSetChannel(m_hDriver, group, ch); }
    int  GetChannel(int group)                   { if (!m_hDriver || !pfnGetChannel) return -1; return pfnGetChannel(m_hDriver, group); }
    BOOL SetPathList(int sw, const char* channels) { if (!m_hDriver || !pfnSetPathList) return FALSE; return pfnSetPathList(m_hDriver, sw, channels); }
    BOOL SetLaunch(int port)                     { if (!m_hDriver || !pfnSetLaunch) return FALSE; return pfnSetLaunch(m_hDriver, port); }

    // -----------------------------------------------------------------------
    // 日志 / VISA
    // -----------------------------------------------------------------------

    void SetLogCallback(PFN_PCT4LogCallback callback) { if (pfnSetLogCallback) pfnSetLogCallback(callback); }

    int EnumerateVisaResources(char* buffer, int bufferSize)
    {
        if (!pfnEnumerateVisaResources) return 0;
        return pfnEnumerateVisaResources(buffer, bufferSize);
    }

private:
    HMODULE m_hDll;
    HANDLE  m_hDriver;

    PFN_PCT4_CreateDriver           pfnCreateDriver;
    PFN_PCT4_CreateDriverEx         pfnCreateDriverEx;
    PFN_PCT4_DestroyDriver          pfnDestroyDriver;
    PFN_PCT4_Connect                pfnConnect;
    PFN_PCT4_Disconnect             pfnDisconnect;
    PFN_PCT4_Initialize             pfnInitialize;
    PFN_PCT4_IsConnected            pfnIsConnected;
    PFN_PCT4_GetIdentification      pfnGetIdentification;
    PFN_PCT4_GetCassetteInfo        pfnGetCassetteInfo;
    PFN_PCT4_CheckError             pfnCheckError;
    PFN_PCT4_SendCommand            pfnSendCommand;
    PFN_PCT4_SendWrite              pfnSendWrite;
    PFN_PCT4_StartMeasurement       pfnStartMeasurement;
    PFN_PCT4_StopMeasurement        pfnStopMeasurement;
    PFN_PCT4_GetMeasurementState    pfnGetMeasurementState;
    PFN_PCT4_WaitForIdle            pfnWaitForIdle;
    PFN_PCT4_MeasureReset           pfnMeasureReset;
    PFN_PCT4_SetFunction            pfnSetFunction;
    PFN_PCT4_GetFunction            pfnGetFunction;
    PFN_PCT4_SetWavelength          pfnSetWavelength;
    PFN_PCT4_SetSourceList          pfnSetSourceList;
    PFN_PCT4_SetAveragingTime       pfnSetAveragingTime;
    PFN_PCT4_SetRange               pfnSetRange;
    PFN_PCT4_SetILOnly              pfnSetILOnly;
    PFN_PCT4_SetConnection          pfnSetConnection;
    PFN_PCT4_SetBiDir               pfnSetBiDir;
    PFN_PCT4_SetChannel             pfnSetChannel;
    PFN_PCT4_GetChannel             pfnGetChannel;
    PFN_PCT4_SetPathList            pfnSetPathList;
    PFN_PCT4_SetLaunch              pfnSetLaunch;
    PFN_PCT4_SetLogCallback         pfnSetLogCallback;
    PFN_PCT4_EnumerateVisaResources pfnEnumerateVisaResources;
};
