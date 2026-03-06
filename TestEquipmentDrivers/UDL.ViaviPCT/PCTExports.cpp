#include "stdafx.h"
#include "PCTExports.h"
#include "PCTDriverFactory.h"
#include "CViaviPCTDriver.h"
#include "Logger.h"
#include "../Common/VisaHelper.h"
#include <cstring>
#include <algorithm>

using namespace ViaviPCT;

static IViaviPCTDriver* ToDriver(HANDLE h)
{
    return reinterpret_cast<IViaviPCTDriver*>(h);
}

static CViaviPCTDriver* ToPCTDriver(HANDLE h)
{
    return dynamic_cast<CViaviPCTDriver*>(ToDriver(h));
}

// ---------------------------------------------------------------------------
// 驱动生命周期
// ---------------------------------------------------------------------------

PCT_C_API HANDLE WINAPI PCT_CreateDriver(const char* type, const char* ip, int port, int slot)
{
    try
    {
        IViaviPCTDriver* driver = CPCTDriverFactory::Create(
            type ? type : "", ip ? ip : "", port, slot);
        return reinterpret_cast<HANDLE>(driver);
    }
    catch (...)
    {
        return NULL;
    }
}

PCT_C_API HANDLE WINAPI PCT_CreateDriverEx(const char* type, const char* address,
                                            int port, int slot, int commType)
{
    try
    {
        CommType ct = static_cast<CommType>(commType);
        IViaviPCTDriver* driver = CPCTDriverFactory::Create(
            type ? type : "", address ? address : "", port, slot, ct);
        return reinterpret_cast<HANDLE>(driver);
    }
    catch (...)
    {
        return NULL;
    }
}

PCT_C_API void WINAPI PCT_DestroyDriver(HANDLE hDriver)
{
    if (hDriver)
        CPCTDriverFactory::Destroy(ToDriver(hDriver));
}

// ---------------------------------------------------------------------------
// 连接
// ---------------------------------------------------------------------------

PCT_C_API BOOL WINAPI PCT_Connect(HANDLE hDriver)
{
    if (!hDriver) return FALSE;
    try { return ToDriver(hDriver)->Connect() ? TRUE : FALSE; }
    catch (...) { return FALSE; }
}

PCT_C_API void WINAPI PCT_Disconnect(HANDLE hDriver)
{
    if (hDriver)
    {
        try { ToDriver(hDriver)->Disconnect(); }
        catch (...) {}
    }
}

PCT_C_API BOOL WINAPI PCT_Initialize(HANDLE hDriver)
{
    if (!hDriver) return FALSE;
    try { return ToDriver(hDriver)->Initialize() ? TRUE : FALSE; }
    catch (...) { return FALSE; }
}

PCT_C_API BOOL WINAPI PCT_IsConnected(HANDLE hDriver)
{
    if (!hDriver) return FALSE;
    try { return ToDriver(hDriver)->IsConnected() ? TRUE : FALSE; }
    catch (...) { return FALSE; }
}

// ---------------------------------------------------------------------------
// 配置
// ---------------------------------------------------------------------------

PCT_C_API BOOL WINAPI PCT_ConfigureWavelengths(HANDLE hDriver, double* wavelengths, int count)
{
    if (!hDriver || !wavelengths || count <= 0) return FALSE;
    try
    {
        std::vector<double> wl(wavelengths, wavelengths + count);
        ToDriver(hDriver)->ConfigureWavelengths(wl);
        return TRUE;
    }
    catch (...) { return FALSE; }
}

PCT_C_API BOOL WINAPI PCT_ConfigureChannels(HANDLE hDriver, int* channels, int count)
{
    if (!hDriver || !channels || count <= 0) return FALSE;
    try
    {
        std::vector<int> ch(channels, channels + count);
        ToDriver(hDriver)->ConfigureChannels(ch);
        return TRUE;
    }
    catch (...) { return FALSE; }
}

PCT_C_API BOOL WINAPI PCT_SetMeasurementMode(HANDLE hDriver, int mode)
{
    if (!hDriver) return FALSE;
    try
    {
        ToDriver(hDriver)->SetMeasurementMode(static_cast<MeasurementMode>(mode));
        return TRUE;
    }
    catch (...) { return FALSE; }
}

PCT_C_API BOOL WINAPI PCT_SetAveragingTime(HANDLE hDriver, int seconds)
{
    if (!hDriver) return FALSE;
    try
    {
        ToDriver(hDriver)->SetAveragingTime(seconds);
        return TRUE;
    }
    catch (...) { return FALSE; }
}

PCT_C_API BOOL WINAPI PCT_SetDUTRange(HANDLE hDriver, int rangeMeters)
{
    if (!hDriver) return FALSE;
    try
    {
        ToDriver(hDriver)->SetDUTRange(rangeMeters);
        return TRUE;
    }
    catch (...) { return FALSE; }
}

// ---------------------------------------------------------------------------
// 测量
// ---------------------------------------------------------------------------

PCT_C_API BOOL WINAPI PCT_TakeReference(HANDLE hDriver, BOOL bOverride,
                                         double ilValue, double lengthValue)
{
    if (!hDriver) return FALSE;
    try
    {
        ReferenceConfig cfg;
        cfg.useOverride = (bOverride != FALSE);
        cfg.ilValue = ilValue;
        cfg.lengthValue = lengthValue;
        return ToDriver(hDriver)->TakeReference(cfg) ? TRUE : FALSE;
    }
    catch (...) { return FALSE; }
}

PCT_C_API BOOL WINAPI PCT_TakeMeasurement(HANDLE hDriver)
{
    if (!hDriver) return FALSE;
    try { return ToDriver(hDriver)->TakeMeasurement() ? TRUE : FALSE; }
    catch (...) { return FALSE; }
}

PCT_C_API int WINAPI PCT_GetResults(HANDLE hDriver,
                                     CMeasurementResult* results,
                                     int maxCount)
{
    if (!hDriver || !results || maxCount <= 0) return 0;
    try
    {
        std::vector<MeasurementResult> res = ToDriver(hDriver)->GetResults();
        int count = (std::min)(static_cast<int>(res.size()), maxCount);
        for (int i = 0; i < count; ++i)
        {
            results[i].channel = res[i].channel;
            results[i].wavelength = res[i].wavelength;
            results[i].insertionLoss = res[i].insertionLoss;
            results[i].returnLoss = res[i].returnLoss;
            results[i].returnLossZone1 = res[i].returnLossZone1;
            results[i].returnLossZone2 = res[i].returnLossZone2;
            results[i].dutLength = res[i].dutLength;
            results[i].power = res[i].power;
            results[i].rawDataCount = res[i].rawDataCount;
            memcpy(results[i].rawData, res[i].rawData,
                   sizeof(double) * (std::min)(res[i].rawDataCount, 10));
        }
        return count;
    }
    catch (...) { return 0; }
}

// ---------------------------------------------------------------------------
// 设备信息
// ---------------------------------------------------------------------------

PCT_C_API BOOL WINAPI PCT_GetDeviceInfo(HANDLE hDriver, CDeviceInfo* info)
{
    if (!hDriver || !info) return FALSE;
    try
    {
        DeviceInfo di = ToDriver(hDriver)->GetDeviceInfo();
        memset(info, 0, sizeof(CDeviceInfo));
        strncpy_s(info->serialNumber, di.serialNumber.c_str(), _TRUNCATE);
        strncpy_s(info->partNumber, di.partNumber.c_str(), _TRUNCATE);
        strncpy_s(info->firmwareVersion, di.firmwareVersion.c_str(), _TRUNCATE);
        strncpy_s(info->description, di.description.c_str(), _TRUNCATE);
        info->slot = di.slot;
        return TRUE;
    }
    catch (...) { return FALSE; }
}

PCT_C_API int WINAPI PCT_CheckError(HANDLE hDriver, char* message, int messageSize)
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

// ---------------------------------------------------------------------------
// 原始命令
// ---------------------------------------------------------------------------

PCT_C_API BOOL WINAPI PCT_SendCommand(HANDLE hDriver, const char* command,
                                       char* response, int responseSize)
{
    if (!hDriver || !command) return FALSE;
    try
    {
        std::string resp = ToDriver(hDriver)->SendRawQuery(command);
        if (response && responseSize > 0)
            strncpy_s(response, responseSize, resp.c_str(), _TRUNCATE);
        return TRUE;
    }
    catch (...) { return FALSE; }
}

// ---------------------------------------------------------------------------
// 日志
// ---------------------------------------------------------------------------

PCT_C_API void WINAPI PCT_SetLogCallback(PCTLogCallback callback)
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

// ---------------------------------------------------------------------------
// VISA 枚举
// ---------------------------------------------------------------------------

PCT_C_API int WINAPI PCT_EnumerateVisaResources(char* buffer, int bufferSize)
{
    try
    {
        VisaHelper::CVisaLoader visa;
        if (!visa.LoadVisa())
        {
            if (buffer && bufferSize > 0) buffer[0] = '\0';
            return 0;
        }

        std::vector<std::string> resources = visa.FindResources("?*INSTR");
        if (resources.empty())
        {
            if (buffer && bufferSize > 0) buffer[0] = '\0';
            return 0;
        }

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
