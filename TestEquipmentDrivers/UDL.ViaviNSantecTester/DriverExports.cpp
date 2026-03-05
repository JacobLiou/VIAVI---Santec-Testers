#include "stdafx.h"
#include "DriverExports.h"
#include "DriverFactory.h"
#include "ViaviPCTDriver.h"
#include "SantecDriver.h"
#include "Logger.h"
#include <cstring>
#include <algorithm>

using namespace ViaviNSantecTester;

// 验证句柄并转换为驱动指针
static IEquipmentDriver* ToDriver(HANDLE h)
{
    return reinterpret_cast<IEquipmentDriver*>(h);
}

static CBaseEquipmentDriver* ToBaseDriver(HANDLE h)
{
    return dynamic_cast<CBaseEquipmentDriver*>(ToDriver(h));
}

// ---------------------------------------------------------------------------
// 导出函数
// ---------------------------------------------------------------------------

DRIVER_C_API HANDLE WINAPI CreateDriver(const char* type, const char* ip, int port, int slot)
{
    try
    {
        IEquipmentDriver* driver = CDriverFactory::Create(
            type ? type : "", ip ? ip : "", port, slot);
        return reinterpret_cast<HANDLE>(driver);
    }
    catch (...)
    {
        return NULL;
    }
}

DRIVER_C_API void WINAPI DestroyDriver(HANDLE hDriver)
{
    if (hDriver)
        CDriverFactory::Destroy(ToDriver(hDriver));
}

DRIVER_C_API BOOL WINAPI DriverConnect(HANDLE hDriver)
{
    if (!hDriver) return FALSE;
    try { return ToDriver(hDriver)->Connect() ? TRUE : FALSE; }
    catch (...) { return FALSE; }
}

DRIVER_C_API void WINAPI DriverDisconnect(HANDLE hDriver)
{
    if (hDriver)
    {
        try { ToDriver(hDriver)->Disconnect(); }
        catch (...) {}
    }
}

DRIVER_C_API BOOL WINAPI DriverInitialize(HANDLE hDriver)
{
    if (!hDriver) return FALSE;
    try { return ToDriver(hDriver)->Initialize() ? TRUE : FALSE; }
    catch (...) { return FALSE; }
}

DRIVER_C_API BOOL WINAPI DriverIsConnected(HANDLE hDriver)
{
    if (!hDriver) return FALSE;
    try { return ToDriver(hDriver)->IsConnected() ? TRUE : FALSE; }
    catch (...) { return FALSE; }
}

DRIVER_C_API BOOL WINAPI DriverConfigureWavelengths(HANDLE hDriver, double* wavelengths, int count)
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

DRIVER_C_API BOOL WINAPI DriverConfigureChannels(HANDLE hDriver, int* channels, int count)
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

DRIVER_C_API BOOL WINAPI DriverConfigureORL(HANDLE hDriver, int channel, int method, int origin,
                                            double aOffset, double bOffset)
{
    if (!hDriver) return FALSE;
    try
    {
        CViaviPCTDriver* viavi = dynamic_cast<CViaviPCTDriver*>(ToDriver(hDriver));
        if (!viavi) return FALSE; // ORL 配置仅限 VIAVI

        ORLConfig cfg;
        cfg.channel = channel;
        cfg.method = static_cast<ORLMethod>(method);
        cfg.origin = static_cast<ORLOrigin>(origin);
        cfg.aOffset = aOffset;
        cfg.bOffset = bOffset;
        viavi->ConfigureORL(cfg);
        return TRUE;
    }
    catch (...) { return FALSE; }
}

DRIVER_C_API BOOL WINAPI DriverTakeReference(HANDLE hDriver, BOOL bOverride,
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

DRIVER_C_API BOOL WINAPI DriverTakeMeasurement(HANDLE hDriver)
{
    if (!hDriver) return FALSE;
    try { return ToDriver(hDriver)->TakeMeasurement() ? TRUE : FALSE; }
    catch (...) { return FALSE; }
}

DRIVER_C_API int WINAPI DriverGetResults(HANDLE hDriver,
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
            results[i].returnLossA = res[i].returnLossA;
            results[i].returnLossB = res[i].returnLossB;
            results[i].returnLossTotal = res[i].returnLossTotal;
            results[i].dutLength = res[i].dutLength;
            results[i].rawDataCount = res[i].rawDataCount;
            memcpy(results[i].rawData, res[i].rawData,
                   sizeof(double) * (std::min)(res[i].rawDataCount, 10));
        }
        return count;
    }
    catch (...) { return 0; }
}

DRIVER_C_API BOOL WINAPI DriverGetDeviceInfo(HANDLE hDriver, CDeviceInfo* info)
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

DRIVER_C_API int WINAPI DriverCheckError(HANDLE hDriver, char* message, int messageSize)
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

DRIVER_C_API BOOL WINAPI DriverSendCommand(HANDLE hDriver, const char* command,
                                           char* response, int responseSize)
{
    if (!hDriver || !command) return FALSE;
    try
    {
        CBaseEquipmentDriver* base = ToBaseDriver(hDriver);
        if (!base) return FALSE;

        std::string resp = base->SendCommand(command);
        if (response && responseSize > 0)
            strncpy_s(response, responseSize, resp.c_str(), _TRUNCATE);
        return TRUE;
    }
    catch (...) { return FALSE; }
}

DRIVER_C_API void WINAPI DriverSetLogCallback(DriverLogCallback callback)
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
// Santec RL1 特定配置导出
// ---------------------------------------------------------------------------

DRIVER_C_API BOOL WINAPI DriverSantecSetRLSensitivity(HANDLE hDriver, int sensitivity)
{
    if (!hDriver) return FALSE;
    try
    {
        CSantecDriver* santec = dynamic_cast<CSantecDriver*>(ToDriver(hDriver));
        if (!santec) return FALSE;
        santec->SetRLSensitivity(static_cast<CSantecDriver::RLSensitivity>(sensitivity));
        return TRUE;
    }
    catch (...) { return FALSE; }
}

DRIVER_C_API BOOL WINAPI DriverSantecSetDUTLength(HANDLE hDriver, int lengthBin)
{
    if (!hDriver) return FALSE;
    try
    {
        CSantecDriver* santec = dynamic_cast<CSantecDriver*>(ToDriver(hDriver));
        if (!santec) return FALSE;
        santec->SetDUTLength(static_cast<CSantecDriver::DUTLengthBin>(lengthBin));
        return TRUE;
    }
    catch (...) { return FALSE; }
}

DRIVER_C_API BOOL WINAPI DriverSantecSetRLGain(HANDLE hDriver, int gain)
{
    if (!hDriver) return FALSE;
    try
    {
        CSantecDriver* santec = dynamic_cast<CSantecDriver*>(ToDriver(hDriver));
        if (!santec) return FALSE;
        santec->SetRLGain(static_cast<CSantecDriver::RLGainMode>(gain));
        return TRUE;
    }
    catch (...) { return FALSE; }
}

DRIVER_C_API BOOL WINAPI DriverSantecSetLocalMode(HANDLE hDriver, BOOL enabled)
{
    if (!hDriver) return FALSE;
    try
    {
        CSantecDriver* santec = dynamic_cast<CSantecDriver*>(ToDriver(hDriver));
        if (!santec) return FALSE;
        santec->SetLocalMode(enabled != FALSE);
        return TRUE;
    }
    catch (...) { return FALSE; }
}
