#pragma once

// ---------------------------------------------------------------------------
// CViaviSantecDllLoader -- Pure dynamic binding to UDL.ViaviNSantecTester.dll
//
// All interaction with the driver goes through LoadLibrary / GetProcAddress.
// No .h from UDL.ViaviNSantecTester is included; no .lib is linked.
// This is the pattern the production framework will use.
// ---------------------------------------------------------------------------

#include <Windows.h>
#include <string>

// ---------------------------------------------------------------------------
// C-compatible structures (binary-compatible with DLL exports)
// ---------------------------------------------------------------------------

struct DriverMeasurementResult
{
    int    channel;
    double wavelength;
    double insertionLoss;
    double returnLoss;
    double returnLossA;
    double returnLossB;
    double returnLossTotal;
    double dutLength;
    double rawData[10];
    int    rawDataCount;
};

struct DriverDeviceInfo
{
    char manufacturer[64];
    char model[64];
    char serialNumber[64];
    char firmwareVersion[64];
    int  slot;
};

// ---------------------------------------------------------------------------
// Function pointer typedefs matching UDL.ViaviNSantecTester C exports
// ---------------------------------------------------------------------------

// Lifecycle
typedef HANDLE (WINAPI *PFN_CreateDriver)(const char* type, const char* ip, int port, int slot);
typedef void   (WINAPI *PFN_DestroyDriver)(HANDLE hDriver);

// Connection
typedef BOOL (WINAPI *PFN_DriverConnect)(HANDLE hDriver);
typedef void (WINAPI *PFN_DriverDisconnect)(HANDLE hDriver);
typedef BOOL (WINAPI *PFN_DriverInitialize)(HANDLE hDriver);
typedef BOOL (WINAPI *PFN_DriverIsConnected)(HANDLE hDriver);

// Configuration
typedef BOOL (WINAPI *PFN_DriverConfigureWavelengths)(HANDLE hDriver, double* wavelengths, int count);
typedef BOOL (WINAPI *PFN_DriverConfigureChannels)(HANDLE hDriver, int* channels, int count);
typedef BOOL (WINAPI *PFN_DriverConfigureORL)(HANDLE hDriver, int channel, int method, int origin,
                                              double aOffset, double bOffset);

// Measurement
typedef BOOL (WINAPI *PFN_DriverTakeReference)(HANDLE hDriver, BOOL bOverride,
                                               double ilValue, double lengthValue);
typedef BOOL (WINAPI *PFN_DriverTakeMeasurement)(HANDLE hDriver);
typedef int  (WINAPI *PFN_DriverGetResults)(HANDLE hDriver, DriverMeasurementResult* results, int maxCount);

// Info / Error
typedef BOOL (WINAPI *PFN_DriverGetDeviceInfo)(HANDLE hDriver, DriverDeviceInfo* info);
typedef int  (WINAPI *PFN_DriverCheckError)(HANDLE hDriver, char* message, int messageSize);

// Raw SCPI
typedef BOOL (WINAPI *PFN_DriverSendCommand)(HANDLE hDriver, const char* command,
                                             char* response, int responseSize);

// Logging
typedef void (WINAPI *PFN_DriverLogCallback)(int level, const char* source, const char* message);
typedef void (WINAPI *PFN_DriverSetLogCallback)(PFN_DriverLogCallback callback);

// Santec-specific
typedef BOOL (WINAPI *PFN_DriverSantecSetRLSensitivity)(HANDLE hDriver, int sensitivity);
typedef BOOL (WINAPI *PFN_DriverSantecSetDUTLength)(HANDLE hDriver, int lengthBin);
typedef BOOL (WINAPI *PFN_DriverSantecSetRLGain)(HANDLE hDriver, int gain);
typedef BOOL (WINAPI *PFN_DriverSantecSetLocalMode)(HANDLE hDriver, BOOL enabled);

// ---------------------------------------------------------------------------
// CViaviSantecDllLoader
// ---------------------------------------------------------------------------

class CViaviSantecDllLoader
{
public:
    CViaviSantecDllLoader()
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
        , pfnConfigureORL(NULL)
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
    {
    }

    ~CViaviSantecDllLoader()
    {
        DestroyDriver();
        UnloadDll();
    }

    // -----------------------------------------------------------------------
    // DLL lifecycle
    // -----------------------------------------------------------------------

    bool LoadDll(const char* dllPath)
    {
        if (m_hDll)
        {
            printf("[Loader] DLL already loaded. Unload first.\n");
            return false;
        }

        m_hDll = ::LoadLibraryA(dllPath);
        if (!m_hDll)
        {
            printf("[Loader] LoadLibrary(\"%s\") failed, error=%lu\n", dllPath, GetLastError());
            return false;
        }

        int resolved = 0;
        int total = 0;

        #define LOAD_PROC(varName, exportName) \
            ++total; \
            varName = (decltype(varName))::GetProcAddress(m_hDll, exportName); \
            if (varName) ++resolved; \
            else printf("[Loader] WARNING: %s not found\n", exportName);

        LOAD_PROC(pfnCreateDriver,             "CreateDriver");
        LOAD_PROC(pfnDestroyDriver,            "DestroyDriver");
        LOAD_PROC(pfnConnect,                  "DriverConnect");
        LOAD_PROC(pfnDisconnect,               "DriverDisconnect");
        LOAD_PROC(pfnInitialize,               "DriverInitialize");
        LOAD_PROC(pfnIsConnected,              "DriverIsConnected");
        LOAD_PROC(pfnConfigureWavelengths,     "DriverConfigureWavelengths");
        LOAD_PROC(pfnConfigureChannels,        "DriverConfigureChannels");
        LOAD_PROC(pfnConfigureORL,             "DriverConfigureORL");
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

        #undef LOAD_PROC

        printf("[Loader] DLL loaded: %d/%d functions resolved.\n", resolved, total);
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
            pfnConfigureORL = NULL;
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
            printf("[Loader] DLL unloaded.\n");
        }
    }

    bool IsDllLoaded() const { return m_hDll != NULL; }

    // -----------------------------------------------------------------------
    // Driver instance lifecycle
    // -----------------------------------------------------------------------

    bool CreateDriver(const char* type, const char* ip, int port, int slot)
    {
        if (!pfnCreateDriver) { printf("[Loader] DLL not loaded.\n"); return false; }
        if (m_hDriver) { printf("[Loader] Driver already created. Destroy first.\n"); return false; }

        m_hDriver = pfnCreateDriver(type, ip, port, slot);
        if (!m_hDriver)
        {
            printf("[Loader] CreateDriver failed.\n");
            return false;
        }
        return true;
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
    // Connection
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
    // Configuration
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

    BOOL ConfigureORL(int channel, int method, int origin, double aOffset, double bOffset)
    {
        if (!m_hDriver || !pfnConfigureORL) return FALSE;
        return pfnConfigureORL(m_hDriver, channel, method, origin, aOffset, bOffset);
    }

    // -----------------------------------------------------------------------
    // Measurement
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
    // Info / Error
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
    // Raw SCPI
    // -----------------------------------------------------------------------

    BOOL SendCommand(const char* command, char* response, int responseSize)
    {
        if (!m_hDriver || !pfnSendCommand) return FALSE;
        return pfnSendCommand(m_hDriver, command, response, responseSize);
    }

    // -----------------------------------------------------------------------
    // Logging
    // -----------------------------------------------------------------------

    void SetLogCallback(PFN_DriverLogCallback callback)
    {
        if (pfnSetLogCallback) pfnSetLogCallback(callback);
    }

    // -----------------------------------------------------------------------
    // Santec-specific
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

private:
    HMODULE m_hDll;
    HANDLE  m_hDriver;

    // Function pointers -- resolved at LoadDll time
    PFN_CreateDriver                pfnCreateDriver;
    PFN_DestroyDriver               pfnDestroyDriver;
    PFN_DriverConnect               pfnConnect;
    PFN_DriverDisconnect            pfnDisconnect;
    PFN_DriverInitialize            pfnInitialize;
    PFN_DriverIsConnected           pfnIsConnected;
    PFN_DriverConfigureWavelengths  pfnConfigureWavelengths;
    PFN_DriverConfigureChannels     pfnConfigureChannels;
    PFN_DriverConfigureORL          pfnConfigureORL;
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
};
