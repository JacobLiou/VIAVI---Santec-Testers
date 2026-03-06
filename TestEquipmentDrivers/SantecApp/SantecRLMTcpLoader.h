#pragma once

// ---------------------------------------------------------------------------
// CSantecRLMTcpLoader -- UDL.SantecRLM.dll 动态加载器 (TCP 模式)
//
// 通过 LoadLibrary / GetProcAddress 动态加载驱动 DLL
// 无需引用 UDL.SantecRLM 的头文件或 .lib 静态库
// ---------------------------------------------------------------------------

#include <Windows.h>
#include <string>
#include <cstdio>

namespace RLMLoaderDetail {
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
// C 兼容结构体（与 DriverTypes.h 中 DLL 导出结构体对齐）
// ---------------------------------------------------------------------------

struct RLMMeasurementResult
{
    int    channel;
    double wavelength;          // nm
    double insertionLoss;       // dB (IL)
    double returnLoss;          // dB (RL)
    double returnLossA;         // dB (RLA)
    double returnLossB;         // dB (RLB)
    double returnLossTotal;     // dB (RLTotal)
    double dutLength;           // m
    double rawData[10];
    int    rawDataCount;
};

struct RLMDeviceInfo
{
    char manufacturer[64];
    char model[64];
    char serialNumber[64];
    char firmwareVersion[64];
    int  slot;
};

// ---------------------------------------------------------------------------
// UDL.SantecRLM C 导出函数指针类型定义
// ---------------------------------------------------------------------------

typedef HANDLE (WINAPI *PFN_RLM_CreateDriver)(const char* type, const char* ip, int port, int slot);
typedef void   (WINAPI *PFN_RLM_DestroyDriver)(HANDLE hDriver);

typedef BOOL (WINAPI *PFN_RLM_Connect)(HANDLE hDriver);
typedef void (WINAPI *PFN_RLM_Disconnect)(HANDLE hDriver);
typedef BOOL (WINAPI *PFN_RLM_Initialize)(HANDLE hDriver);
typedef BOOL (WINAPI *PFN_RLM_IsConnected)(HANDLE hDriver);

typedef BOOL (WINAPI *PFN_RLM_ConfigureWavelengths)(HANDLE hDriver, double* wavelengths, int count);
typedef BOOL (WINAPI *PFN_RLM_ConfigureChannels)(HANDLE hDriver, int* channels, int count);

typedef BOOL (WINAPI *PFN_RLM_TakeReference)(HANDLE hDriver, BOOL bOverride,
                                              double ilValue, double lengthValue);
typedef BOOL (WINAPI *PFN_RLM_TakeMeasurement)(HANDLE hDriver);
typedef int  (WINAPI *PFN_RLM_GetResults)(HANDLE hDriver, RLMMeasurementResult* results, int maxCount);

typedef BOOL (WINAPI *PFN_RLM_GetDeviceInfo)(HANDLE hDriver, RLMDeviceInfo* info);
typedef int  (WINAPI *PFN_RLM_CheckError)(HANDLE hDriver, char* message, int messageSize);

typedef BOOL (WINAPI *PFN_RLM_SendCommand)(HANDLE hDriver, const char* command,
                                            char* response, int responseSize);

typedef void (WINAPI *PFN_RLMLogCallback)(int level, const char* source, const char* message);
typedef void (WINAPI *PFN_RLM_SetLogCallback)(PFN_RLMLogCallback callback);

typedef BOOL (WINAPI *PFN_RLM_SantecSetRLSensitivity)(HANDLE hDriver, int sensitivity);
typedef BOOL (WINAPI *PFN_RLM_SantecSetDUTLength)(HANDLE hDriver, int lengthBin);
typedef BOOL (WINAPI *PFN_RLM_SantecSetRLGain)(HANDLE hDriver, int gain);
typedef BOOL (WINAPI *PFN_RLM_SantecSetLocalMode)(HANDLE hDriver, BOOL enabled);

typedef BOOL (WINAPI *PFN_RLM_SetDetector)(HANDLE hDriver, int detectorNum);
typedef int  (WINAPI *PFN_RLM_GetDetectorCount)(HANDLE hDriver);
typedef BOOL (WINAPI *PFN_RLM_GetDetectorInfo)(HANDLE hDriver, int detectorNum,
                                                char* buffer, int bufferSize);

// ---------------------------------------------------------------------------
// CSantecRLMTcpLoader 类
// ---------------------------------------------------------------------------

class CSantecRLMTcpLoader
{
public:
    CSantecRLMTcpLoader()
        : m_hDll(NULL)
        , m_hDriver(NULL)
        , pfnCreateDriver(NULL)
        , pfnDestroyDriver(NULL)
        , pfnConnect(NULL)
        , pfnDisconnect(NULL)
        , pfnInitialize(NULL)
        , pfnIsConnected(NULL)
        , pfnConfigureWavelengths(NULL)
        , pfnConfigureChannels(NULL)
        , pfnTakeReference(NULL)
        , pfnTakeMeasurement(NULL)
        , pfnGetResults(NULL)
        , pfnGetDeviceInfo(NULL)
        , pfnCheckError(NULL)
        , pfnSendCommand(NULL)
        , pfnSetLogCallback(NULL)
        , pfnSantecSetRLSensitivity(NULL)
        , pfnSantecSetDUTLength(NULL)
        , pfnSantecSetRLGain(NULL)
        , pfnSantecSetLocalMode(NULL)
        , pfnSetDetector(NULL)
        , pfnGetDetectorCount(NULL)
        , pfnGetDetectorInfo(NULL)
    {
    }

    ~CSantecRLMTcpLoader()
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
          if (_f) { fprintf(_f, "{\"sessionId\":\"021a55\",\"hypothesisId\":\"H1\",\"location\":\"SantecRLMTcpLoader.h:LoadDll\",\"message\":\"LoadLibraryA result\",\"data\":{\"dllPath\":\"%s\",\"hDll\":\"%p\",\"lastError\":%lu,\"fullPath\":\"%s\",\"ptrSize\":%d},\"timestamp\":%llu}\n", dllPath, m_hDll, m_hDll ? 0UL : ::GetLastError(), _modPath, (int)sizeof(void*), (unsigned long long)::GetTickCount64()); fclose(_f); } }
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
            varName = (decltype(varName))RLMLoaderDetail::ResolveProcAddress(m_hDll, exportName); \
            if (varName) ++resolved; else { if(!_failedExports.empty()) _failedExports += ","; _failedExports += exportName; }

        LOAD_PROC(pfnCreateDriver,             "CreateDriver");
        LOAD_PROC(pfnDestroyDriver,            "DestroyDriver");
        LOAD_PROC(pfnConnect,                  "DriverConnect");
        LOAD_PROC(pfnDisconnect,               "DriverDisconnect");
        LOAD_PROC(pfnInitialize,               "DriverInitialize");
        LOAD_PROC(pfnIsConnected,              "DriverIsConnected");
        LOAD_PROC(pfnConfigureWavelengths,     "DriverConfigureWavelengths");
        LOAD_PROC(pfnConfigureChannels,        "DriverConfigureChannels");
        LOAD_PROC(pfnTakeReference,            "DriverTakeReference");
        LOAD_PROC(pfnTakeMeasurement,          "DriverTakeMeasurement");
        LOAD_PROC(pfnGetResults,               "DriverGetResults");
        LOAD_PROC(pfnGetDeviceInfo,            "DriverGetDeviceInfo");
        LOAD_PROC(pfnCheckError,               "DriverCheckError");
        LOAD_PROC(pfnSendCommand,              "DriverSendCommand");
        LOAD_PROC(pfnSetLogCallback,           "DriverSetLogCallback");
        LOAD_PROC(pfnSantecSetRLSensitivity,   "DriverSantecSetRLSensitivity");
        LOAD_PROC(pfnSantecSetDUTLength,       "DriverSantecSetDUTLength");
        LOAD_PROC(pfnSantecSetRLGain,          "DriverSantecSetRLGain");
        LOAD_PROC(pfnSantecSetLocalMode,       "DriverSantecSetLocalMode");
        LOAD_PROC(pfnSetDetector,              "DriverSetDetector");
        LOAD_PROC(pfnGetDetectorCount,         "DriverGetDetectorCount");
        LOAD_PROC(pfnGetDetectorInfo,          "DriverGetDetectorInfo");

        #undef LOAD_PROC

        // #region agent log
        { FILE* _f = fopen("c:\\Users\\menghl2\\WorkSpace\\Projects\\VIAVI---Santec-Testers\\debug-021a55.log", "a");
          if (_f) { fprintf(_f, "{\"sessionId\":\"021a55\",\"hypothesisId\":\"H2\",\"location\":\"SantecRLMTcpLoader.h:LoadDll\",\"message\":\"RLM resolve summary\",\"data\":{\"resolved\":%d,\"total\":%d,\"failed\":\"%s\"},\"timestamp\":%llu}\n", resolved, total, _failedExports.c_str(), (unsigned long long)::GetTickCount64()); fclose(_f); } }
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
            pfnInitialize = NULL;
            pfnIsConnected = NULL;
            pfnConfigureWavelengths = NULL;
            pfnConfigureChannels = NULL;
            pfnTakeReference = NULL;
            pfnTakeMeasurement = NULL;
            pfnGetResults = NULL;
            pfnGetDeviceInfo = NULL;
            pfnCheckError = NULL;
            pfnSendCommand = NULL;
            pfnSetLogCallback = NULL;
            pfnSantecSetRLSensitivity = NULL;
            pfnSantecSetDUTLength = NULL;
            pfnSantecSetRLGain = NULL;
            pfnSantecSetLocalMode = NULL;
            pfnSetDetector = NULL;
            pfnGetDetectorCount = NULL;
            pfnGetDetectorInfo = NULL;
        }
    }

    bool IsDllLoaded() const { return m_hDll != NULL; }

    // -----------------------------------------------------------------------
    // 驱动实例管理 (TCP only)
    // -----------------------------------------------------------------------

    bool CreateDriver(const char* type, const char* ip, int port, int slot)
    {
        if (!pfnCreateDriver || m_hDriver) return false;
        m_hDriver = pfnCreateDriver(type, ip, port, slot);
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

    int GetResults(RLMMeasurementResult* results, int maxCount)
    {
        if (!m_hDriver || !pfnGetResults) return 0;
        return pfnGetResults(m_hDriver, results, maxCount);
    }

    // -----------------------------------------------------------------------
    // 设备信息 / 错误
    // -----------------------------------------------------------------------

    BOOL GetDeviceInfo(RLMDeviceInfo* info)
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

    void SetLogCallback(PFN_RLMLogCallback callback)
    {
        if (pfnSetLogCallback) pfnSetLogCallback(callback);
    }

    // -----------------------------------------------------------------------
    // Santec 特定设置
    // -----------------------------------------------------------------------

    BOOL SantecSetRLSensitivity(int sensitivity)
    {
        if (!m_hDriver || !pfnSantecSetRLSensitivity) return FALSE;
        return pfnSantecSetRLSensitivity(m_hDriver, sensitivity);
    }

    BOOL SantecSetDUTLength(int lengthBin)
    {
        if (!m_hDriver || !pfnSantecSetDUTLength) return FALSE;
        return pfnSantecSetDUTLength(m_hDriver, lengthBin);
    }

    BOOL SantecSetRLGain(int gain)
    {
        if (!m_hDriver || !pfnSantecSetRLGain) return FALSE;
        return pfnSantecSetRLGain(m_hDriver, gain);
    }

    BOOL SantecSetLocalMode(BOOL enabled)
    {
        if (!m_hDriver || !pfnSantecSetLocalMode) return FALSE;
        return pfnSantecSetLocalMode(m_hDriver, enabled);
    }

    // -----------------------------------------------------------------------
    // 探测器选择
    // -----------------------------------------------------------------------

    BOOL SetDetector(int detectorNum)
    {
        if (!m_hDriver || !pfnSetDetector) return FALSE;
        return pfnSetDetector(m_hDriver, detectorNum);
    }

    int GetDetectorCount()
    {
        if (!m_hDriver || !pfnGetDetectorCount) return 0;
        return pfnGetDetectorCount(m_hDriver);
    }

    BOOL GetDetectorInfo(int detectorNum, char* buffer, int bufferSize)
    {
        if (!m_hDriver || !pfnGetDetectorInfo || !buffer) return FALSE;
        return pfnGetDetectorInfo(m_hDriver, detectorNum, buffer, bufferSize);
    }

private:
    HMODULE m_hDll;
    HANDLE  m_hDriver;

    PFN_RLM_CreateDriver             pfnCreateDriver;
    PFN_RLM_DestroyDriver            pfnDestroyDriver;
    PFN_RLM_Connect                  pfnConnect;
    PFN_RLM_Disconnect               pfnDisconnect;
    PFN_RLM_Initialize               pfnInitialize;
    PFN_RLM_IsConnected              pfnIsConnected;
    PFN_RLM_ConfigureWavelengths     pfnConfigureWavelengths;
    PFN_RLM_ConfigureChannels        pfnConfigureChannels;
    PFN_RLM_TakeReference            pfnTakeReference;
    PFN_RLM_TakeMeasurement          pfnTakeMeasurement;
    PFN_RLM_GetResults               pfnGetResults;
    PFN_RLM_GetDeviceInfo            pfnGetDeviceInfo;
    PFN_RLM_CheckError               pfnCheckError;
    PFN_RLM_SendCommand              pfnSendCommand;
    PFN_RLM_SetLogCallback           pfnSetLogCallback;
    PFN_RLM_SantecSetRLSensitivity   pfnSantecSetRLSensitivity;
    PFN_RLM_SantecSetDUTLength       pfnSantecSetDUTLength;
    PFN_RLM_SantecSetRLGain          pfnSantecSetRLGain;
    PFN_RLM_SantecSetLocalMode       pfnSantecSetLocalMode;
    PFN_RLM_SetDetector              pfnSetDetector;
    PFN_RLM_GetDetectorCount         pfnGetDetectorCount;
    PFN_RLM_GetDetectorInfo          pfnGetDetectorInfo;
};
