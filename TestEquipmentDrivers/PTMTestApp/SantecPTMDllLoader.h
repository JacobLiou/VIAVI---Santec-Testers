#pragma once

#include <Windows.h>
#include <string>

struct PTMPolarityResult
{
    int    outputChannel;
    int    inputChannel;
    double rawData[4];
    int    rawDataCount;
};

struct PTMDeviceInfo
{
    char manufacturer[64];
    char model[64];
    char serialNumber[64];
    char firmwareVersion[64];
    int  slot;
};

typedef HANDLE (WINAPI *PFN_PTM_CreateDriver)(const char*, const char*, int, int);
typedef HANDLE (WINAPI *PFN_PTM_CreateDriverEx)(const char*, const char*, int, int, int);
typedef void   (WINAPI *PFN_PTM_DestroyDriver)(HANDLE);
typedef BOOL   (WINAPI *PFN_PTM_Connect)(HANDLE);
typedef void   (WINAPI *PFN_PTM_Disconnect)(HANDLE);
typedef BOOL   (WINAPI *PFN_PTM_Initialize)(HANDLE);
typedef BOOL   (WINAPI *PFN_PTM_IsConnected)(HANDLE);
typedef int    (WINAPI *PFN_PTM_GetChannelCount)(HANDLE);
typedef BOOL   (WINAPI *PFN_PTM_TakeMeasurement)(HANDLE, int);
typedef int    (WINAPI *PFN_PTM_GetResults)(HANDLE, PTMPolarityResult*, int);
typedef BOOL   (WINAPI *PFN_PTM_GetRawResult)(HANDLE, char*, int);
typedef int    (WINAPI *PFN_PTM_Detect)(HANDLE, int);
typedef BOOL   (WINAPI *PFN_PTM_SetLocalMode)(HANDLE, BOOL);
typedef BOOL   (WINAPI *PFN_PTM_GetLocalMode)(HANDLE);
typedef BOOL   (WINAPI *PFN_PTM_LaserOff)(HANDLE);
typedef BOOL   (WINAPI *PFN_PTM_SetVfl)(HANDLE, int, int);
typedef BOOL   (WINAPI *PFN_PTM_GetBarcode)(HANDLE, char*, int);
typedef BOOL   (WINAPI *PFN_PTM_Notify)(HANDLE, int, const char*);
typedef BOOL   (WINAPI *PFN_PTM_TestStart)(HANDLE);
typedef BOOL   (WINAPI *PFN_PTM_TestStatus)(HANDLE, char*, int);
typedef BOOL   (WINAPI *PFN_PTM_TestNext)(HANDLE);
typedef BOOL   (WINAPI *PFN_PTM_TestRetry)(HANDLE);
typedef BOOL   (WINAPI *PFN_PTM_GetDeviceInfo)(HANDLE, PTMDeviceInfo*);
typedef int    (WINAPI *PFN_PTM_CheckError)(HANDLE, char*, int);
typedef BOOL   (WINAPI *PFN_PTM_SendCommand)(HANDLE, const char*, char*, int);
typedef void   (WINAPI *PFN_PTM_LogCallback)(int, const char*, const char*);
typedef void   (WINAPI *PFN_PTM_SetLogCallback)(PFN_PTM_LogCallback);
typedef int    (WINAPI *PFN_PTM_EnumerateVisaResources)(char*, int);

class CSantecPTMDllLoader
{
public:
    CSantecPTMDllLoader()
        : m_hDll(NULL), m_hDriver(NULL)
        , pfnCreateDriver(NULL), pfnCreateDriverEx(NULL), pfnDestroyDriver(NULL)
        , pfnConnect(NULL), pfnDisconnect(NULL), pfnInitialize(NULL), pfnIsConnected(NULL)
        , pfnGetChannelCount(NULL), pfnTakeMeasurement(NULL), pfnGetResults(NULL), pfnGetRawResult(NULL)
        , pfnDetect(NULL), pfnSetLocalMode(NULL), pfnGetLocalMode(NULL), pfnLaserOff(NULL), pfnSetVfl(NULL)
        , pfnGetBarcode(NULL), pfnNotify(NULL)
        , pfnTestStart(NULL), pfnTestStatus(NULL), pfnTestNext(NULL), pfnTestRetry(NULL)
        , pfnGetDeviceInfo(NULL), pfnCheckError(NULL), pfnSendCommand(NULL)
        , pfnSetLogCallback(NULL), pfnEnumerateVisaResources(NULL)
    {}

    ~CSantecPTMDllLoader()
    {
        DestroyDriver();
        UnloadDll();
    }

    bool LoadDll(const char* dllPath)
    {
        if (m_hDll) UnloadDll();
        m_hDll = LoadLibraryA(dllPath);
        if (!m_hDll) return false;

#define LOAD(name) pfn##name = (PFN_PTM_##name)GetProcAddress(m_hDll, "PTM_" #name)
        LOAD(CreateDriver);
        pfnCreateDriverEx = (PFN_PTM_CreateDriverEx)GetProcAddress(m_hDll, "PTM_CreateDriverEx");
        LOAD(DestroyDriver);
        LOAD(Connect);
        LOAD(Disconnect);
        LOAD(Initialize);
        LOAD(IsConnected);
        LOAD(GetChannelCount);
        LOAD(TakeMeasurement);
        LOAD(GetResults);
        LOAD(GetRawResult);
        LOAD(Detect);
        LOAD(SetLocalMode);
        LOAD(GetLocalMode);
        LOAD(LaserOff);
        LOAD(SetVfl);
        LOAD(GetBarcode);
        LOAD(Notify);
        LOAD(TestStart);
        LOAD(TestStatus);
        LOAD(TestNext);
        LOAD(TestRetry);
        LOAD(GetDeviceInfo);
        LOAD(CheckError);
        LOAD(SendCommand);
        LOAD(SetLogCallback);
        pfnEnumerateVisaResources = (PFN_PTM_EnumerateVisaResources)GetProcAddress(m_hDll, "PTM_EnumerateVisaResources");
#undef LOAD

        if (!pfnCreateDriver || !pfnConnect || !pfnTakeMeasurement || !pfnGetResults)
        {
            UnloadDll();
            return false;
        }
        return true;
    }

    void UnloadDll()
    {
        DestroyDriver();
        if (m_hDll) { FreeLibrary(m_hDll); m_hDll = NULL; }
        pfnCreateDriver = NULL;
        pfnCreateDriverEx = NULL;
    }

    bool IsDllLoaded() const { return m_hDll != NULL; }
    bool HasDriver() const { return m_hDriver != NULL; }

    bool CreateDriver(const char* type, const char* ip, int port, int slot)
    {
        DestroyDriver();
        if (!pfnCreateDriver) return false;
        m_hDriver = pfnCreateDriver(type, ip, port, slot);
        return m_hDriver != NULL;
    }

    bool CreateDriverEx(const char* type, const char* address, int port, int slot, int commType)
    {
        DestroyDriver();
        if (!pfnCreateDriverEx) return false;
        m_hDriver = pfnCreateDriverEx(type, address, port, slot, commType);
        return m_hDriver != NULL;
    }

    void DestroyDriver()
    {
        if (m_hDriver && pfnDestroyDriver)
        {
            pfnDestroyDriver(m_hDriver);
            m_hDriver = NULL;
        }
    }

    bool Connect() { return m_hDriver && pfnConnect && pfnConnect(m_hDriver); }
    void Disconnect() { if (m_hDriver && pfnDisconnect) pfnDisconnect(m_hDriver); }
    bool Initialize() { return m_hDriver && pfnInitialize && pfnInitialize(m_hDriver); }
    bool IsConnected() { return m_hDriver && pfnIsConnected && pfnIsConnected(m_hDriver); }
    int  GetChannelCount() { return (m_hDriver && pfnGetChannelCount) ? pfnGetChannelCount(m_hDriver) : 0; }
    bool TakeMeasurement(int n) { return m_hDriver && pfnTakeMeasurement && pfnTakeMeasurement(m_hDriver, n); }
    int  GetResults(PTMPolarityResult* r, int maxCount)
    {
        return (m_hDriver && pfnGetResults) ? pfnGetResults(m_hDriver, r, maxCount) : 0;
    }
    bool GetRawResult(char* buf, int size)
    {
        return m_hDriver && pfnGetRawResult && pfnGetRawResult(m_hDriver, buf, size);
    }
    int  Detect(int ch) { return (m_hDriver && pfnDetect) ? pfnDetect(m_hDriver, ch) : -1; }
    bool SetLocalMode(BOOL e) { return m_hDriver && pfnSetLocalMode && pfnSetLocalMode(m_hDriver, e); }
    bool LaserOff() { return m_hDriver && pfnLaserOff && pfnLaserOff(m_hDriver); }
    bool SetVfl(int ch, int mode) { return m_hDriver && pfnSetVfl && pfnSetVfl(m_hDriver, ch, mode); }
    bool GetDeviceInfo(PTMDeviceInfo* info)
    {
        return m_hDriver && pfnGetDeviceInfo && pfnGetDeviceInfo(m_hDriver, info);
    }
    bool SendCommand(const char* cmd, char* resp, int respSize)
    {
        return m_hDriver && pfnSendCommand && pfnSendCommand(m_hDriver, cmd, resp, respSize);
    }
    void SetLogCallback(PFN_PTM_LogCallback cb)
    {
        if (pfnSetLogCallback) pfnSetLogCallback(cb);
    }
    int EnumerateVisaResources(char* buffer, int bufferSize)
    {
        return pfnEnumerateVisaResources ? pfnEnumerateVisaResources(buffer, bufferSize) : 0;
    }

private:
    HMODULE m_hDll;
    HANDLE  m_hDriver;

    PFN_PTM_CreateDriver pfnCreateDriver;
    PFN_PTM_CreateDriverEx pfnCreateDriverEx;
    PFN_PTM_DestroyDriver pfnDestroyDriver;
    PFN_PTM_Connect pfnConnect;
    PFN_PTM_Disconnect pfnDisconnect;
    PFN_PTM_Initialize pfnInitialize;
    PFN_PTM_IsConnected pfnIsConnected;
    PFN_PTM_GetChannelCount pfnGetChannelCount;
    PFN_PTM_TakeMeasurement pfnTakeMeasurement;
    PFN_PTM_GetResults pfnGetResults;
    PFN_PTM_GetRawResult pfnGetRawResult;
    PFN_PTM_Detect pfnDetect;
    PFN_PTM_SetLocalMode pfnSetLocalMode;
    PFN_PTM_GetLocalMode pfnGetLocalMode;
    PFN_PTM_LaserOff pfnLaserOff;
    PFN_PTM_SetVfl pfnSetVfl;
    PFN_PTM_GetBarcode pfnGetBarcode;
    PFN_PTM_Notify pfnNotify;
    PFN_PTM_TestStart pfnTestStart;
    PFN_PTM_TestStatus pfnTestStatus;
    PFN_PTM_TestNext pfnTestNext;
    PFN_PTM_TestRetry pfnTestRetry;
    PFN_PTM_GetDeviceInfo pfnGetDeviceInfo;
    PFN_PTM_CheckError pfnCheckError;
    PFN_PTM_SendCommand pfnSendCommand;
    PFN_PTM_SetLogCallback pfnSetLogCallback;
    PFN_PTM_EnumerateVisaResources pfnEnumerateVisaResources;
};
