#pragma once

// ---------------------------------------------------------------------------
// CViaviPCTDllLoader -- UDL.ViaviPCT.dll 动态加载器
//
// 通过 LoadLibrary / GetProcAddress 动态加载驱动 DLL
// 无需引用 UDL.ViaviPCT 的头文件或 .lib 静态库
// ---------------------------------------------------------------------------

#include <Windows.h>
#include <string>

// ---------------------------------------------------------------------------
// C 兼容结构体（与 PCTTypes.h 中 DLL 导出结构体对齐）
// ---------------------------------------------------------------------------

struct PCTMeasurementResult
{
    int    channel;
    double wavelength;          // nm
    double insertionLoss;       // dB (IL)
    double returnLoss;          // dB (ORL)
    double returnLossZone1;     // dB
    double returnLossZone2;     // dB
    double dutLength;           // m
    double power;               // dBm
    double rawData[10];
    int    rawDataCount;
};

struct PCTDeviceInfo
{
    char serialNumber[64];
    char partNumber[64];
    char firmwareVersion[64];
    char description[128];
    int  slot;
};

// ---------------------------------------------------------------------------
// UDL.ViaviPCT C 导出函数指针类型定义
// ---------------------------------------------------------------------------

// 生命周期
typedef HANDLE (WINAPI *PFN_PCT_CreateDriver)(const char* type, const char* ip, int port, int slot);
typedef HANDLE (WINAPI *PFN_PCT_CreateDriverEx)(const char* type, const char* address,
                                                int port, int slot, int commType);
typedef void   (WINAPI *PFN_PCT_DestroyDriver)(HANDLE hDriver);

// 连接
typedef BOOL (WINAPI *PFN_PCT_Connect)(HANDLE hDriver);
typedef void (WINAPI *PFN_PCT_Disconnect)(HANDLE hDriver);
typedef BOOL (WINAPI *PFN_PCT_Initialize)(HANDLE hDriver);
typedef BOOL (WINAPI *PFN_PCT_IsConnected)(HANDLE hDriver);

// 配置
typedef BOOL (WINAPI *PFN_PCT_ConfigureWavelengths)(HANDLE hDriver, double* wavelengths, int count);
typedef BOOL (WINAPI *PFN_PCT_ConfigureChannels)(HANDLE hDriver, int* channels, int count);
typedef BOOL (WINAPI *PFN_PCT_SetMeasurementMode)(HANDLE hDriver, int mode);
typedef BOOL (WINAPI *PFN_PCT_SetAveragingTime)(HANDLE hDriver, int seconds);
typedef BOOL (WINAPI *PFN_PCT_SetDUTRange)(HANDLE hDriver, int rangeMeters);

// 测量
typedef BOOL (WINAPI *PFN_PCT_TakeReference)(HANDLE hDriver, BOOL bOverride,
                                              double ilValue, double lengthValue);
typedef BOOL (WINAPI *PFN_PCT_TakeMeasurement)(HANDLE hDriver);
typedef int  (WINAPI *PFN_PCT_GetResults)(HANDLE hDriver, PCTMeasurementResult* results, int maxCount);

// 设备信息 / 错误
typedef BOOL (WINAPI *PFN_PCT_GetDeviceInfo)(HANDLE hDriver, PCTDeviceInfo* info);
typedef int  (WINAPI *PFN_PCT_CheckError)(HANDLE hDriver, char* message, int messageSize);

// 原始 SCPI
typedef BOOL (WINAPI *PFN_PCT_SendCommand)(HANDLE hDriver, const char* command,
                                            char* response, int responseSize);

// 日志
typedef void (WINAPI *PFN_PCTLogCallback)(int level, const char* source, const char* message);
typedef void (WINAPI *PFN_PCT_SetLogCallback)(PFN_PCTLogCallback callback);

// VISA 枚举
typedef int (WINAPI *PFN_PCT_EnumerateVisaResources)(char* buffer, int bufferSize);

// ---------------------------------------------------------------------------
// CViaviPCTDllLoader 类
// ---------------------------------------------------------------------------

class CViaviPCTDllLoader
{
public:
    CViaviPCTDllLoader()
        : m_hDll(NULL)
        , m_hDriver(NULL)
        , pfnCreateDriver(NULL)
        , pfnCreateDriverEx(NULL)
        , pfnDestroyDriver(NULL)
        , pfnConnect(NULL)
        , pfnDisconnect(NULL)
        , pfnInitialize(NULL)
        , pfnIsConnected(NULL)
        , pfnConfigureWavelengths(NULL)
        , pfnConfigureChannels(NULL)
        , pfnSetMeasurementMode(NULL)
        , pfnSetAveragingTime(NULL)
        , pfnSetDUTRange(NULL)
        , pfnTakeReference(NULL)
        , pfnTakeMeasurement(NULL)
        , pfnGetResults(NULL)
        , pfnGetDeviceInfo(NULL)
        , pfnCheckError(NULL)
        , pfnSendCommand(NULL)
        , pfnSetLogCallback(NULL)
        , pfnEnumerateVisaResources(NULL)
    {
    }

    ~CViaviPCTDllLoader()
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
            varName = (decltype(varName))::GetProcAddress(m_hDll, exportName); \
            if (varName) ++resolved;

        LOAD_PROC(pfnCreateDriver,             "PCT_CreateDriver");
        LOAD_PROC(pfnCreateDriverEx,           "PCT_CreateDriverEx");
        LOAD_PROC(pfnDestroyDriver,            "PCT_DestroyDriver");
        LOAD_PROC(pfnConnect,                  "PCT_Connect");
        LOAD_PROC(pfnDisconnect,               "PCT_Disconnect");
        LOAD_PROC(pfnInitialize,               "PCT_Initialize");
        LOAD_PROC(pfnIsConnected,              "PCT_IsConnected");
        LOAD_PROC(pfnConfigureWavelengths,     "PCT_ConfigureWavelengths");
        LOAD_PROC(pfnConfigureChannels,        "PCT_ConfigureChannels");
        LOAD_PROC(pfnSetMeasurementMode,       "PCT_SetMeasurementMode");
        LOAD_PROC(pfnSetAveragingTime,         "PCT_SetAveragingTime");
        LOAD_PROC(pfnSetDUTRange,              "PCT_SetDUTRange");
        LOAD_PROC(pfnTakeReference,            "PCT_TakeReference");
        LOAD_PROC(pfnTakeMeasurement,          "PCT_TakeMeasurement");
        LOAD_PROC(pfnGetResults,               "PCT_GetResults");
        LOAD_PROC(pfnGetDeviceInfo,            "PCT_GetDeviceInfo");
        LOAD_PROC(pfnCheckError,               "PCT_CheckError");
        LOAD_PROC(pfnSendCommand,              "PCT_SendCommand");
        LOAD_PROC(pfnSetLogCallback,           "PCT_SetLogCallback");
        LOAD_PROC(pfnEnumerateVisaResources,   "PCT_EnumerateVisaResources");

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
            pfnInitialize = NULL;
            pfnIsConnected = NULL;
            pfnConfigureWavelengths = NULL;
            pfnConfigureChannels = NULL;
            pfnSetMeasurementMode = NULL;
            pfnSetAveragingTime = NULL;
            pfnSetDUTRange = NULL;
            pfnTakeReference = NULL;
            pfnTakeMeasurement = NULL;
            pfnGetResults = NULL;
            pfnGetDeviceInfo = NULL;
            pfnCheckError = NULL;
            pfnSendCommand = NULL;
            pfnSetLogCallback = NULL;
            pfnEnumerateVisaResources = NULL;
        }
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

    BOOL Initialize()
    {
        if (!m_hDriver || !pfnInitialize) return FALSE;
        return pfnInitialize(m_hDriver);
    }

    BOOL IsConnected()
    {
        if (!m_hDriver || !pfnIsConnected) return FALSE;
        return pfnIsConnected(m_hDriver);
    }

    // -----------------------------------------------------------------------
    // 配置
    // -----------------------------------------------------------------------

    BOOL ConfigureWavelengths(double* wavelengths, int count)
    {
        if (!m_hDriver || !pfnConfigureWavelengths) return FALSE;
        return pfnConfigureWavelengths(m_hDriver, wavelengths, count);
    }

    BOOL ConfigureChannels(int* channels, int count)
    {
        if (!m_hDriver || !pfnConfigureChannels) return FALSE;
        return pfnConfigureChannels(m_hDriver, channels, count);
    }

    BOOL SetMeasurementMode(int mode)
    {
        if (!m_hDriver || !pfnSetMeasurementMode) return FALSE;
        return pfnSetMeasurementMode(m_hDriver, mode);
    }

    BOOL SetAveragingTime(int seconds)
    {
        if (!m_hDriver || !pfnSetAveragingTime) return FALSE;
        return pfnSetAveragingTime(m_hDriver, seconds);
    }

    BOOL SetDUTRange(int rangeMeters)
    {
        if (!m_hDriver || !pfnSetDUTRange) return FALSE;
        return pfnSetDUTRange(m_hDriver, rangeMeters);
    }

    // -----------------------------------------------------------------------
    // 测量
    // -----------------------------------------------------------------------

    BOOL TakeReference(BOOL bOverride, double ilValue, double lengthValue)
    {
        if (!m_hDriver || !pfnTakeReference) return FALSE;
        return pfnTakeReference(m_hDriver, bOverride, ilValue, lengthValue);
    }

    BOOL TakeMeasurement()
    {
        if (!m_hDriver || !pfnTakeMeasurement) return FALSE;
        return pfnTakeMeasurement(m_hDriver);
    }

    int GetResults(PCTMeasurementResult* results, int maxCount)
    {
        if (!m_hDriver || !pfnGetResults) return 0;
        return pfnGetResults(m_hDriver, results, maxCount);
    }

    // -----------------------------------------------------------------------
    // 设备信息 / 错误
    // -----------------------------------------------------------------------

    BOOL GetDeviceInfo(PCTDeviceInfo* info)
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

    void SetLogCallback(PFN_PCTLogCallback callback)
    {
        if (pfnSetLogCallback) pfnSetLogCallback(callback);
    }

private:
    HMODULE m_hDll;
    HANDLE  m_hDriver;

    PFN_PCT_CreateDriver             pfnCreateDriver;
    PFN_PCT_CreateDriverEx           pfnCreateDriverEx;
    PFN_PCT_DestroyDriver            pfnDestroyDriver;
    PFN_PCT_Connect                  pfnConnect;
    PFN_PCT_Disconnect               pfnDisconnect;
    PFN_PCT_Initialize               pfnInitialize;
    PFN_PCT_IsConnected              pfnIsConnected;
    PFN_PCT_ConfigureWavelengths     pfnConfigureWavelengths;
    PFN_PCT_ConfigureChannels        pfnConfigureChannels;
    PFN_PCT_SetMeasurementMode       pfnSetMeasurementMode;
    PFN_PCT_SetAveragingTime         pfnSetAveragingTime;
    PFN_PCT_SetDUTRange              pfnSetDUTRange;
    PFN_PCT_TakeReference            pfnTakeReference;
    PFN_PCT_TakeMeasurement          pfnTakeMeasurement;
    PFN_PCT_GetResults               pfnGetResults;
    PFN_PCT_GetDeviceInfo            pfnGetDeviceInfo;
    PFN_PCT_CheckError               pfnCheckError;
    PFN_PCT_SendCommand              pfnSendCommand;
    PFN_PCT_SetLogCallback           pfnSetLogCallback;
    PFN_PCT_EnumerateVisaResources   pfnEnumerateVisaResources;
};
