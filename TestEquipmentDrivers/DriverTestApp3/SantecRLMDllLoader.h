#pragma once

// ---------------------------------------------------------------------------
// CSantecRLMDllLoader -- UDL.SantecRLM.dll 动态加载器
//
// 通过 LoadLibrary / GetProcAddress 动态加载驱动 DLL
// 无需引用 UDL.SantecRLM 的头文件或 .lib 静态库
// ---------------------------------------------------------------------------

#include <Windows.h>
#include <string>

// ---------------------------------------------------------------------------
// C??????????DLL??????????????
// ---------------------------------------------------------------------------

struct DriverMeasurementResult
{
    int    channel;          // ?????
    double wavelength;       // ???? (nm)
    double insertionLoss;    // ??????? (dB)
    double returnLoss;       // ?????? (dB)
    double returnLossA;      // A??????? (dB)
    double returnLossB;      // B??????? (dB)
    double returnLossTotal;  // ??????? (dB)
    double dutLength;        // ???????????? (m)
    double rawData[10];      // ??????
    int    rawDataCount;     // ??????????
};

struct DriverDeviceInfo
{
    char manufacturer[64];    // ??????
    char model[64];           // ???
    char serialNumber[64];    // ??????
    char firmwareVersion[64]; // ??????
    int  slot;                // ????
};

// ---------------------------------------------------------------------------
// UDL.SantecRLM C 导出函数指针类型定义
// ---------------------------------------------------------------------------

// ???????????
typedef HANDLE (WINAPI *PFN_CreateDriver)(const char* type, const char* ip, int port, int slot);
typedef void   (WINAPI *PFN_DestroyDriver)(HANDLE hDriver);

// ???????
typedef BOOL (WINAPI *PFN_DriverConnect)(HANDLE hDriver);
typedef void (WINAPI *PFN_DriverDisconnect)(HANDLE hDriver);
typedef BOOL (WINAPI *PFN_DriverInitialize)(HANDLE hDriver);
typedef BOOL (WINAPI *PFN_DriverIsConnected)(HANDLE hDriver);

// ????
typedef BOOL (WINAPI *PFN_DriverConfigureWavelengths)(HANDLE hDriver, double* wavelengths, int count);
typedef BOOL (WINAPI *PFN_DriverConfigureChannels)(HANDLE hDriver, int* channels, int count);
// ????
typedef BOOL (WINAPI *PFN_DriverTakeReference)(HANDLE hDriver, BOOL bOverride,
                                               double ilValue, double lengthValue);
typedef BOOL (WINAPI *PFN_DriverTakeMeasurement)(HANDLE hDriver);
typedef int  (WINAPI *PFN_DriverGetResults)(HANDLE hDriver, DriverMeasurementResult* results, int maxCount);

// ??? / ????
typedef BOOL (WINAPI *PFN_DriverGetDeviceInfo)(HANDLE hDriver, DriverDeviceInfo* info);
typedef int  (WINAPI *PFN_DriverCheckError)(HANDLE hDriver, char* message, int messageSize);

// ??SCPI????
typedef BOOL (WINAPI *PFN_DriverSendCommand)(HANDLE hDriver, const char* command,
                                             char* response, int responseSize);

// ???
typedef void (WINAPI *PFN_DriverLogCallback)(int level, const char* source, const char* message);
typedef void (WINAPI *PFN_DriverSetLogCallback)(PFN_DriverLogCallback callback);

// Santec???
typedef BOOL (WINAPI *PFN_DriverSantecSetRLSensitivity)(HANDLE hDriver, int sensitivity);
typedef BOOL (WINAPI *PFN_DriverSantecSetDUTLength)(HANDLE hDriver, int lengthBin);
typedef BOOL (WINAPI *PFN_DriverSantecSetRLGain)(HANDLE hDriver, int gain);
typedef BOOL (WINAPI *PFN_DriverSantecSetLocalMode)(HANDLE hDriver, BOOL enabled);

// 探测器选择
typedef BOOL (WINAPI *PFN_DriverSetDetector)(HANDLE hDriver, int detectorNum);
typedef int  (WINAPI *PFN_DriverGetDetectorCount)(HANDLE hDriver);
typedef BOOL (WINAPI *PFN_DriverGetDetectorInfo)(HANDLE hDriver, int detectorNum,
                                                 char* buffer, int bufferSize);

// 外部开关控制
typedef BOOL (WINAPI *PFN_DriverSetSwitchChannel)(HANDLE hDriver, int switchNum, int channel);
typedef int  (WINAPI *PFN_DriverGetSwitchChannel)(HANDLE hDriver, int switchNum);
typedef BOOL (WINAPI *PFN_DriverGetSwitchInfo)(HANDLE hDriver, int switchNum,
                                               char* buffer, int bufferSize);

// VISA / USB ???
typedef HANDLE (WINAPI *PFN_CreateDriverEx)(const char* type, const char* address,
                                            int port, int slot, int commType);
typedef int    (WINAPI *PFN_EnumerateVisaResources)(char* buffer, int bufferSize);

// ---------------------------------------------------------------------------
// CSantecRLMDllLoader 类
// ---------------------------------------------------------------------------

class CSantecRLMDllLoader
{
public:
    CSantecRLMDllLoader()
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
        , pfnSetSwitchChannel(NULL)
        , pfnGetSwitchChannel(NULL)
        , pfnGetSwitchInfo(NULL)
        , pfnCreateDriverEx(NULL)
        , pfnEnumerateVisaResources(NULL)
    {
    }

    ~CSantecRLMDllLoader()
    {
        DestroyDriver();
        UnloadDll();
    }

    // -----------------------------------------------------------------------
    // DLL ???????????
    // -----------------------------------------------------------------------

    bool LoadDll(const char* dllPath)
    {
        if (m_hDll)
        {
            printf("[??????] DLL???????????????\n");
            return false;
        }

        m_hDll = ::LoadLibraryA(dllPath);
        if (!m_hDll)
        {
            printf("[??????] LoadLibrary(\"%s\") ??????????=%lu\n", dllPath, GetLastError());
            return false;
        }

        int resolved = 0;
        int total = 0;

        #define LOAD_PROC(varName, exportName) \
            ++total; \
            varName = (decltype(varName))::GetProcAddress(m_hDll, exportName); \
            if (varName) ++resolved; \
            else printf("[??????] ????: %s ?????\n", exportName);

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

        LOAD_PROC(pfnSetSwitchChannel,         "DriverSetSwitchChannel");
        LOAD_PROC(pfnGetSwitchChannel,         "DriverGetSwitchChannel");
        LOAD_PROC(pfnGetSwitchInfo,            "DriverGetSwitchInfo");

        // VISA ????????????????????????
        pfnCreateDriverEx = (PFN_CreateDriverEx)::GetProcAddress(m_hDll, "CreateDriverEx");
        pfnEnumerateVisaResources = (PFN_EnumerateVisaResources)::GetProcAddress(m_hDll, "EnumerateVisaResources");

        #undef LOAD_PROC

        printf("[??????] DLL?????: %d/%d ?????????????\n", resolved, total);
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
            pfnSetSwitchChannel = NULL;
            pfnGetSwitchChannel = NULL;
            pfnGetSwitchInfo = NULL;
            pfnCreateDriverEx = NULL;
            pfnEnumerateVisaResources = NULL;
            printf("[??????] DLL???????\n");
        }
    }

    bool IsDllLoaded() const { return m_hDll != NULL; }

    // -----------------------------------------------------------------------
    // ??????????????????
    // -----------------------------------------------------------------------

    bool CreateDriver(const char* type, const char* ip, int port, int slot)
    {
        if (!pfnCreateDriver) { printf("[??????] DLL???????\n"); return false; }
        if (m_hDriver) { printf("[??????] ????????????????????\n"); return false; }

        m_hDriver = pfnCreateDriver(type, ip, port, slot);
        if (!m_hDriver)
        {
            printf("[??????] CreateDriver ????\n");
            return false;
        }
        return true;
    }

    // ????????????????????? (0=TCP, 1=GPIB, 2=USB/VISA)
    bool CreateDriverEx(const char* type, const char* address, int port, int slot, int commType)
    {
        if (!pfnCreateDriverEx) { printf("[Loader] CreateDriverEx not available.\n"); return false; }
        if (m_hDriver) { printf("[Loader] Driver already created. Destroy first.\n"); return false; }

        m_hDriver = pfnCreateDriverEx(type, address, port, slot, commType);
        if (!m_hDriver)
        {
            printf("[Loader] CreateDriverEx failed.\n");
            return false;
        }
        return true;
    }

    // ??? VISA ???
    int EnumerateVisaResources(char* buffer, int bufferSize)
    {
        if (!pfnEnumerateVisaResources) { printf("[Loader] EnumerateVisaResources not available.\n"); return 0; }
        return pfnEnumerateVisaResources(buffer, bufferSize);
    }

    bool HasVisaSupport() const { return pfnCreateDriverEx != NULL; }

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
    // ???????
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
    // ????
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
    // ????
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

    int GetResults(DriverMeasurementResult* results, int maxCount)
    {
        if (!m_hDriver || !pfnGetResults) return 0;
        return pfnGetResults(m_hDriver, results, maxCount);
    }

    // -----------------------------------------------------------------------
    // ??? / ????
    // -----------------------------------------------------------------------

    BOOL GetDeviceInfo(DriverDeviceInfo* info)
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
    // ??SCPI????
    // -----------------------------------------------------------------------

    BOOL SendCommand(const char* command, char* response, int responseSize)
    {
        if (!m_hDriver || !pfnSendCommand) return FALSE;
        return pfnSendCommand(m_hDriver, command, response, responseSize);
    }

    // -----------------------------------------------------------------------
    // ???
    // -----------------------------------------------------------------------

    void SetLogCallback(PFN_DriverLogCallback callback)
    {
        if (pfnSetLogCallback) pfnSetLogCallback(callback);
    }

    // -----------------------------------------------------------------------
    // Santec???????
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

    // -----------------------------------------------------------------------
    // 外部开关控制（集成模式）
    // -----------------------------------------------------------------------

    BOOL SetSwitchChannel(int switchNum, int channel)
    {
        if (!m_hDriver || !pfnSetSwitchChannel) return FALSE;
        return pfnSetSwitchChannel(m_hDriver, switchNum, channel);
    }

    int GetSwitchChannel(int switchNum)
    {
        if (!m_hDriver || !pfnGetSwitchChannel) return -1;
        return pfnGetSwitchChannel(m_hDriver, switchNum);
    }

    BOOL GetSwitchInfo(int switchNum, char* buffer, int bufferSize)
    {
        if (!m_hDriver || !pfnGetSwitchInfo || !buffer) return FALSE;
        return pfnGetSwitchInfo(m_hDriver, switchNum, buffer, bufferSize);
    }

private:
    HMODULE m_hDll;      // DLL?????
    HANDLE  m_hDriver;   // ??????????

    // ??????? -- ?? LoadDll ?????
    PFN_CreateDriver                pfnCreateDriver;
    PFN_DestroyDriver               pfnDestroyDriver;
    PFN_DriverConnect               pfnConnect;
    PFN_DriverDisconnect            pfnDisconnect;
    PFN_DriverInitialize            pfnInitialize;
    PFN_DriverIsConnected           pfnIsConnected;
    PFN_DriverConfigureWavelengths  pfnConfigureWavelengths;
    PFN_DriverConfigureChannels     pfnConfigureChannels;
    PFN_DriverTakeReference         pfnTakeReference;
    PFN_DriverTakeMeasurement       pfnTakeMeasurement;
    PFN_DriverGetResults            pfnGetResults;
    PFN_DriverGetDeviceInfo         pfnGetDeviceInfo;
    PFN_DriverCheckError            pfnCheckError;
    PFN_DriverSendCommand           pfnSendCommand;
    PFN_DriverSetLogCallback        pfnSetLogCallback;
    PFN_DriverSantecSetRLSensitivity pfnSantecSetRLSensitivity;
    PFN_DriverSantecSetDUTLength    pfnSantecSetDUTLength;
    PFN_DriverSantecSetRLGain       pfnSantecSetRLGain;
    PFN_DriverSantecSetLocalMode    pfnSantecSetLocalMode;

    // 探测器选择
    PFN_DriverSetDetector           pfnSetDetector;
    PFN_DriverGetDetectorCount      pfnGetDetectorCount;
    PFN_DriverGetDetectorInfo       pfnGetDetectorInfo;

    // 外部开关控制
    PFN_DriverSetSwitchChannel      pfnSetSwitchChannel;
    PFN_DriverGetSwitchChannel      pfnGetSwitchChannel;
    PFN_DriverGetSwitchInfo         pfnGetSwitchInfo;

    // VISA ???
    PFN_CreateDriverEx              pfnCreateDriverEx;
    PFN_EnumerateVisaResources      pfnEnumerateVisaResources;
};
