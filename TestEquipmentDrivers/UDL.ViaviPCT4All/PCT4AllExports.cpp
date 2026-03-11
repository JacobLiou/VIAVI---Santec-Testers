#include "stdafx.h"
#include "PCT4AllExports.h"
#include "PCT4AllDriverFactory.h"
#include "CViaviPCT4AllDriver.h"
#include "Logger.h"
#include "../Common/VisaHelper.h"
#include <cstring>
#include <algorithm>

using namespace ViaviPCT4All;

static IViaviPCT4AllDriver* ToDriver(HANDLE h)
{
    return reinterpret_cast<IViaviPCT4AllDriver*>(h);
}

// ===========================================================================
// Driver lifecycle
// ===========================================================================

PCT4_C_API HANDLE WINAPI PCT4_CreateDriver(const char* type, const char* ip, int port, int slot)
{
    try
    {
        IViaviPCT4AllDriver* driver = CPCT4AllDriverFactory::Create(
            type ? type : "", ip ? ip : "", port, slot);
        return reinterpret_cast<HANDLE>(driver);
    }
    catch (...) { return NULL; }
}

PCT4_C_API HANDLE WINAPI PCT4_CreateDriverEx(const char* type, const char* address,
                                              int port, int slot, int commType)
{
    try
    {
        CommType ct = static_cast<CommType>(commType);
        IViaviPCT4AllDriver* driver = CPCT4AllDriverFactory::Create(
            type ? type : "", address ? address : "", port, slot, ct);
        return reinterpret_cast<HANDLE>(driver);
    }
    catch (...) { return NULL; }
}

PCT4_C_API void WINAPI PCT4_DestroyDriver(HANDLE hDriver)
{
    if (hDriver) CPCT4AllDriverFactory::Destroy(ToDriver(hDriver));
}

// ===========================================================================
// Connection
// ===========================================================================

PCT4_C_API BOOL WINAPI PCT4_Connect(HANDLE hDriver)
{
    if (!hDriver) return FALSE;
    try { return ToDriver(hDriver)->Connect() ? TRUE : FALSE; }
    catch (...) { return FALSE; }
}

PCT4_C_API void WINAPI PCT4_Disconnect(HANDLE hDriver)
{
    if (hDriver) { try { ToDriver(hDriver)->Disconnect(); } catch (...) {} }
}

PCT4_C_API BOOL WINAPI PCT4_Initialize(HANDLE hDriver)
{
    if (!hDriver) return FALSE;
    try { return ToDriver(hDriver)->Initialize() ? TRUE : FALSE; }
    catch (...) { return FALSE; }
}

PCT4_C_API BOOL WINAPI PCT4_IsConnected(HANDLE hDriver)
{
    if (!hDriver) return FALSE;
    try { return ToDriver(hDriver)->IsConnected() ? TRUE : FALSE; }
    catch (...) { return FALSE; }
}

// ===========================================================================
// Device info
// ===========================================================================

PCT4_C_API BOOL WINAPI PCT4_GetIdentification(HANDLE hDriver, CIdentificationInfo* info)
{
    if (!hDriver || !info) return FALSE;
    try
    {
        IdentificationInfo idn = ToDriver(hDriver)->GetIdentification();
        memset(info, 0, sizeof(CIdentificationInfo));
        strncpy_s(info->manufacturer, idn.manufacturer.c_str(), _TRUNCATE);
        strncpy_s(info->platform, idn.platform.c_str(), _TRUNCATE);
        strncpy_s(info->serialNumber, idn.serialNumber.c_str(), _TRUNCATE);
        strncpy_s(info->firmwareVersion, idn.firmwareVersion.c_str(), _TRUNCATE);
        return TRUE;
    }
    catch (...) { return FALSE; }
}

PCT4_C_API BOOL WINAPI PCT4_GetCassetteInfo(HANDLE hDriver, CCassetteInfo* info)
{
    if (!hDriver || !info) return FALSE;
    try
    {
        CassetteInfo ci = ToDriver(hDriver)->GetCassetteInfo();
        memset(info, 0, sizeof(CCassetteInfo));
        strncpy_s(info->serialNumber, ci.serialNumber.c_str(), _TRUNCATE);
        strncpy_s(info->partNumber, ci.partNumber.c_str(), _TRUNCATE);
        strncpy_s(info->firmwareVersion, ci.firmwareVersion.c_str(), _TRUNCATE);
        strncpy_s(info->hardwareVersion, ci.hardwareVersion.c_str(), _TRUNCATE);
        strncpy_s(info->assemblyDate, ci.assemblyDate.c_str(), _TRUNCATE);
        strncpy_s(info->description, ci.description.c_str(), _TRUNCATE);
        return TRUE;
    }
    catch (...) { return FALSE; }
}

PCT4_C_API int WINAPI PCT4_CheckError(HANDLE hDriver, char* message, int messageSize)
{
    if (!hDriver) return -1;
    try
    {
        ErrorInfo err = ToDriver(hDriver)->GetSystemError();
        if (message && messageSize > 0)
            strncpy_s(message, messageSize, err.message.c_str(), _TRUNCATE);
        return err.code;
    }
    catch (...) { return -1; }
}

// ===========================================================================
// Raw SCPI
// ===========================================================================

PCT4_C_API BOOL WINAPI PCT4_SendCommand(HANDLE hDriver, const char* command,
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

PCT4_C_API BOOL WINAPI PCT4_SendWrite(HANDLE hDriver, const char* command)
{
    if (!hDriver || !command) return FALSE;
    try
    {
        ToDriver(hDriver)->SendRawWrite(command);
        return TRUE;
    }
    catch (...) { return FALSE; }
}

// ===========================================================================
// Measurement control
// ===========================================================================

PCT4_C_API BOOL WINAPI PCT4_StartMeasurement(HANDLE hDriver)
{
    if (!hDriver) return FALSE;
    try { ToDriver(hDriver)->StartMeasurement(); return TRUE; }
    catch (...) { return FALSE; }
}

PCT4_C_API BOOL WINAPI PCT4_StopMeasurement(HANDLE hDriver)
{
    if (!hDriver) return FALSE;
    try { ToDriver(hDriver)->StopMeasurement(); return TRUE; }
    catch (...) { return FALSE; }
}

PCT4_C_API int WINAPI PCT4_GetMeasurementState(HANDLE hDriver)
{
    if (!hDriver) return -1;
    try { return ToDriver(hDriver)->GetMeasurementState(); }
    catch (...) { return -1; }
}

PCT4_C_API BOOL WINAPI PCT4_WaitForIdle(HANDLE hDriver, int timeoutMs)
{
    if (!hDriver) return FALSE;
    try { return ToDriver(hDriver)->WaitForIdle(timeoutMs) ? TRUE : FALSE; }
    catch (...) { return FALSE; }
}

PCT4_C_API BOOL WINAPI PCT4_MeasureReset(HANDLE hDriver)
{
    if (!hDriver) return FALSE;
    try { ToDriver(hDriver)->MeasureReset(); return TRUE; }
    catch (...) { return FALSE; }
}

// ===========================================================================
// Configuration
// ===========================================================================

PCT4_C_API BOOL WINAPI PCT4_SetFunction(HANDLE hDriver, int mode)
{
    if (!hDriver) return FALSE;
    try { ToDriver(hDriver)->SetFunction(mode); return TRUE; }
    catch (...) { return FALSE; }
}

PCT4_C_API int WINAPI PCT4_GetFunction(HANDLE hDriver)
{
    if (!hDriver) return -1;
    try { return ToDriver(hDriver)->GetFunction(); }
    catch (...) { return -1; }
}

PCT4_C_API BOOL WINAPI PCT4_SetWavelength(HANDLE hDriver, int wavelength)
{
    if (!hDriver) return FALSE;
    try { ToDriver(hDriver)->SetWavelength(wavelength); return TRUE; }
    catch (...) { return FALSE; }
}

PCT4_C_API BOOL WINAPI PCT4_SetSourceList(HANDLE hDriver, const char* wavelengths)
{
    if (!hDriver || !wavelengths) return FALSE;
    try { ToDriver(hDriver)->SetSourceList(wavelengths); return TRUE; }
    catch (...) { return FALSE; }
}

PCT4_C_API BOOL WINAPI PCT4_SetAveragingTime(HANDLE hDriver, int seconds)
{
    if (!hDriver) return FALSE;
    try { ToDriver(hDriver)->SetAveragingTime(seconds); return TRUE; }
    catch (...) { return FALSE; }
}

PCT4_C_API BOOL WINAPI PCT4_SetRange(HANDLE hDriver, int range)
{
    if (!hDriver) return FALSE;
    try { ToDriver(hDriver)->SetRange(range); return TRUE; }
    catch (...) { return FALSE; }
}

PCT4_C_API BOOL WINAPI PCT4_SetILOnly(HANDLE hDriver, int state)
{
    if (!hDriver) return FALSE;
    try { ToDriver(hDriver)->SetILOnly(state); return TRUE; }
    catch (...) { return FALSE; }
}

PCT4_C_API BOOL WINAPI PCT4_SetConnection(HANDLE hDriver, int mode)
{
    if (!hDriver) return FALSE;
    try { ToDriver(hDriver)->SetConnection(mode); return TRUE; }
    catch (...) { return FALSE; }
}

PCT4_C_API BOOL WINAPI PCT4_SetBiDir(HANDLE hDriver, int state)
{
    if (!hDriver) return FALSE;
    try { ToDriver(hDriver)->SetBiDir(state); return TRUE; }
    catch (...) { return FALSE; }
}

// ===========================================================================
// PATH / optical path control
// ===========================================================================

PCT4_C_API BOOL WINAPI PCT4_SetChannel(HANDLE hDriver, int group, int channel)
{
    if (!hDriver) return FALSE;
    try { ToDriver(hDriver)->SetChannel(group, channel); return TRUE; }
    catch (...) { return FALSE; }
}

PCT4_C_API int WINAPI PCT4_GetChannel(HANDLE hDriver, int group)
{
    if (!hDriver) return -1;
    try { return ToDriver(hDriver)->GetChannel(group); }
    catch (...) { return -1; }
}

PCT4_C_API BOOL WINAPI PCT4_SetPathList(HANDLE hDriver, int sw, const char* channels)
{
    if (!hDriver || !channels) return FALSE;
    try { ToDriver(hDriver)->SetPathList(sw, channels); return TRUE; }
    catch (...) { return FALSE; }
}

PCT4_C_API BOOL WINAPI PCT4_SetLaunch(HANDLE hDriver, int port)
{
    if (!hDriver) return FALSE;
    try { ToDriver(hDriver)->SetLaunch(port); return TRUE; }
    catch (...) { return FALSE; }
}

// ===========================================================================
// Logging
// ===========================================================================

PCT4_C_API void WINAPI PCT4_SetLogCallback(PCT4LogCallback callback)
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

// ===========================================================================
// VISA enumeration
// ===========================================================================

PCT4_C_API int WINAPI PCT4_EnumerateVisaResources(char* buffer, int bufferSize)
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
