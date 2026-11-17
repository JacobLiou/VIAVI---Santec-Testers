#include "stdafx.h"
#include "PTMExports.h"
#include "PTMDriverFactory.h"
#include "SantecPTMDriver.h"
#include "Logger.h"
#include "../Common/VisaHelper.h"
#include <cstring>
#include <algorithm>

using namespace SantecPTM;

static ISantecPTMDriver* ToDriver(HANDLE h)
{
    return reinterpret_cast<ISantecPTMDriver*>(h);
}

PTM_C_API HANDLE WINAPI PTM_CreateDriver(const char* type, const char* ip, int port, int slot)
{
    try
    {
        return reinterpret_cast<HANDLE>(
            CPTMDriverFactory::Create(type ? type : "", ip ? ip : "", port, slot));
    }
    catch (...) { return NULL; }
}

PTM_C_API HANDLE WINAPI PTM_CreateDriverEx(const char* type, const char* address,
                                           int port, int slot, int commType)
{
    try
    {
        CommType ct = static_cast<CommType>(commType);
        return reinterpret_cast<HANDLE>(
            CPTMDriverFactory::Create(type ? type : "", address ? address : "",
                                      port, slot, ct));
    }
    catch (...) { return NULL; }
}

PTM_C_API void WINAPI PTM_DestroyDriver(HANDLE hDriver)
{
    if (hDriver)
        CPTMDriverFactory::Destroy(ToDriver(hDriver));
}

PTM_C_API BOOL WINAPI PTM_Connect(HANDLE hDriver)
{
    if (!hDriver) return FALSE;
    try { return ToDriver(hDriver)->Connect() ? TRUE : FALSE; }
    catch (...) { return FALSE; }
}

PTM_C_API void WINAPI PTM_Disconnect(HANDLE hDriver)
{
    if (!hDriver) return;
    try { ToDriver(hDriver)->Disconnect(); } catch (...) {}
}

PTM_C_API BOOL WINAPI PTM_Initialize(HANDLE hDriver)
{
    if (!hDriver) return FALSE;
    try { return ToDriver(hDriver)->Initialize() ? TRUE : FALSE; }
    catch (...) { return FALSE; }
}

PTM_C_API BOOL WINAPI PTM_IsConnected(HANDLE hDriver)
{
    if (!hDriver) return FALSE;
    try { return ToDriver(hDriver)->IsConnected() ? TRUE : FALSE; }
    catch (...) { return FALSE; }
}

PTM_C_API int WINAPI PTM_GetChannelCount(HANDLE hDriver)
{
    if (!hDriver) return 0;
    try { return ToDriver(hDriver)->GetChannelCount(); }
    catch (...) { return 0; }
}

PTM_C_API BOOL WINAPI PTM_TakeMeasurement(HANDLE hDriver, int nChannels)
{
    if (!hDriver) return FALSE;
    try { return ToDriver(hDriver)->TakeMeasurement(nChannels) ? TRUE : FALSE; }
    catch (...) { return FALSE; }
}

PTM_C_API int WINAPI PTM_GetResults(HANDLE hDriver,
                                    CPolarityChannelResult* results,
                                    int maxCount)
{
    if (!hDriver || !results || maxCount <= 0) return 0;
    try
    {
        auto res = ToDriver(hDriver)->GetResults();
        int count = (std::min)(static_cast<int>(res.size()), maxCount);
        for (int i = 0; i < count; ++i)
        {
            results[i].outputChannel = res[i].outputChannel;
            results[i].inputChannel = res[i].inputChannel;
            results[i].rawDataCount = res[i].rawDataCount;
            memcpy(results[i].rawData, res[i].rawData, sizeof(results[i].rawData));
        }
        return count;
    }
    catch (...) { return 0; }
}

PTM_C_API BOOL WINAPI PTM_GetRawResult(HANDLE hDriver, char* buffer, int bufferSize)
{
    if (!hDriver || !buffer || bufferSize <= 0) return FALSE;
    try
    {
        std::string raw = ToDriver(hDriver)->GetRawResult();
        strncpy_s(buffer, bufferSize, raw.c_str(), _TRUNCATE);
        return TRUE;
    }
    catch (...) { return FALSE; }
}

PTM_C_API int WINAPI PTM_Detect(HANDLE hDriver, int outputChannel)
{
    if (!hDriver) return -1;
    try { return ToDriver(hDriver)->Detect(outputChannel); }
    catch (...) { return -1; }
}

PTM_C_API BOOL WINAPI PTM_SetLocalMode(HANDLE hDriver, BOOL enabled)
{
    if (!hDriver) return FALSE;
    try
    {
        ToDriver(hDriver)->SetLocalMode(enabled != FALSE);
        return TRUE;
    }
    catch (...) { return FALSE; }
}

PTM_C_API BOOL WINAPI PTM_GetLocalMode(HANDLE hDriver)
{
    if (!hDriver) return FALSE;
    try { return ToDriver(hDriver)->GetLocalMode() ? TRUE : FALSE; }
    catch (...) { return FALSE; }
}

PTM_C_API BOOL WINAPI PTM_LaserOff(HANDLE hDriver)
{
    if (!hDriver) return FALSE;
    try { ToDriver(hDriver)->LaserOff(); return TRUE; }
    catch (...) { return FALSE; }
}

PTM_C_API BOOL WINAPI PTM_SetVfl(HANDLE hDriver, int outputChannel, int mode)
{
    if (!hDriver) return FALSE;
    try
    {
        ToDriver(hDriver)->SetVfl(outputChannel, static_cast<VflMode>(mode));
        return TRUE;
    }
    catch (...) { return FALSE; }
}

PTM_C_API BOOL WINAPI PTM_GetBarcode(HANDLE hDriver, char* buffer, int bufferSize)
{
    if (!hDriver || !buffer || bufferSize <= 0) return FALSE;
    try
    {
        std::string bc = ToDriver(hDriver)->GetBarcode();
        strncpy_s(buffer, bufferSize, bc.c_str(), _TRUNCATE);
        return TRUE;
    }
    catch (...) { return FALSE; }
}

PTM_C_API BOOL WINAPI PTM_Notify(HANDLE hDriver, int icon, const char* message)
{
    if (!hDriver) return FALSE;
    try
    {
        ToDriver(hDriver)->Notify(icon, message ? message : "");
        return TRUE;
    }
    catch (...) { return FALSE; }
}

PTM_C_API BOOL WINAPI PTM_TestStart(HANDLE hDriver)
{
    if (!hDriver) return FALSE;
    try { return ToDriver(hDriver)->TestStart() ? TRUE : FALSE; }
    catch (...) { return FALSE; }
}

PTM_C_API BOOL WINAPI PTM_TestStatus(HANDLE hDriver, char* buffer, int bufferSize)
{
    if (!hDriver || !buffer || bufferSize <= 0) return FALSE;
    try
    {
        std::string s = ToDriver(hDriver)->TestStatus();
        strncpy_s(buffer, bufferSize, s.c_str(), _TRUNCATE);
        return TRUE;
    }
    catch (...) { return FALSE; }
}

PTM_C_API BOOL WINAPI PTM_TestNext(HANDLE hDriver)
{
    if (!hDriver) return FALSE;
    try { return ToDriver(hDriver)->TestNext() ? TRUE : FALSE; }
    catch (...) { return FALSE; }
}

PTM_C_API BOOL WINAPI PTM_TestRetry(HANDLE hDriver)
{
    if (!hDriver) return FALSE;
    try { return ToDriver(hDriver)->TestRetry() ? TRUE : FALSE; }
    catch (...) { return FALSE; }
}

PTM_C_API BOOL WINAPI PTM_GetDeviceInfo(HANDLE hDriver, CDeviceInfo* info)
{
    if (!hDriver || !info) return FALSE;
    try
    {
        DeviceInfo di = ToDriver(hDriver)->GetDeviceInfo();
        memset(info, 0, sizeof(CDeviceInfo));
        strncpy_s(info->manufacturer, di.manufacturer.c_str(), _TRUNCATE);
        strncpy_s(info->model, di.model.c_str(), _TRUNCATE);
        strncpy_s(info->serialNumber, di.serialNumber.c_str(), _TRUNCATE);
        strncpy_s(info->firmwareVersion, di.firmwareVersion.c_str(), _TRUNCATE);
        info->slot = di.slot;
        return TRUE;
    }
    catch (...) { return FALSE; }
}

PTM_C_API int WINAPI PTM_CheckError(HANDLE hDriver, char* message, int messageSize)
{
    if (!hDriver) return -1;
    try
    {
        ErrorInfo err = ToDriver(hDriver)->CheckError();
        if (message && messageSize > 0)
            strncpy_s(message, messageSize, err.message.c_str(), _TRUNCATE);
        return err.code;
    }
    catch (...) { return -1; }
}

PTM_C_API BOOL WINAPI PTM_SendCommand(HANDLE hDriver, const char* command,
                                      char* response, int responseSize)
{
    if (!hDriver || !command) return FALSE;
    try
    {
        std::string cmd(command);
        bool isQuery = (!cmd.empty() && cmd.back() == '?');
        if (isQuery)
        {
            std::string resp = ToDriver(hDriver)->SendRawQuery(cmd);
            if (response && responseSize > 0)
                strncpy_s(response, responseSize, resp.c_str(), _TRUNCATE);
        }
        else
        {
            ToDriver(hDriver)->SendRawWrite(cmd);
            if (response && responseSize > 0)
                response[0] = '\0';
        }
        return TRUE;
    }
    catch (...) { return FALSE; }
}

PTM_C_API void WINAPI PTM_SetLogCallback(PTM_LogCallback callback)
{
    if (callback)
    {
        CLogger::SetGlobalCallback(
            [callback](LogLevel level, const std::string& source, const std::string& message)
            {
                callback(static_cast<int>(level), source.c_str(), message.c_str());
            });
    }
    else
    {
        CLogger::SetGlobalCallback(nullptr);
    }
}

PTM_C_API int WINAPI PTM_EnumerateVisaResources(char* buffer, int bufferSize)
{
    try
    {
        VisaHelper::CVisaLoader visa;
        if (!visa.LoadVisa())
        {
            if (buffer && bufferSize > 0) buffer[0] = '\0';
            return 0;
        }

        std::vector<std::string> resources = visa.FindResources("USB?*INSTR");
        std::string joined;
        for (size_t i = 0; i < resources.size(); ++i)
        {
            if (i > 0) joined += ";";
            joined += resources[i];
        }
        if (buffer && bufferSize > 0)
            strncpy_s(buffer, bufferSize, joined.c_str(), _TRUNCATE);
        return static_cast<int>(resources.size());
    }
    catch (...)
    {
        if (buffer && bufferSize > 0) buffer[0] = '\0';
        return 0;
    }
}
