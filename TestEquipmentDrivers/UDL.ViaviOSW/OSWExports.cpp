#include "stdafx.h"
#include "OSWExports.h"
#include "CViaviOSWDriver.h"
#include "../Common/VisaHelper.h"
#include <cstring>
#include <algorithm>

using namespace ViaviOSW;

static IViaviOSWDriver* ToDriver(HANDLE h)
{
    return reinterpret_cast<IViaviOSWDriver*>(h);
}

static CViaviOSWDriver* ToOSWDriver(HANDLE h)
{
    return dynamic_cast<CViaviOSWDriver*>(ToDriver(h));
}

// ---------------------------------------------------------------------------
// 驱动生命周期
// ---------------------------------------------------------------------------

OSW_C_API HANDLE WINAPI OSW_CreateDriver(const char* ip, int port)
{
    try
    {
        int oswPort = (port > 0) ? port : CViaviOSWDriver::DEFAULT_PORT;
        CViaviOSWDriver* driver = new CViaviOSWDriver(ip ? ip : "", oswPort);
        return reinterpret_cast<HANDLE>(driver);
    }
    catch (...)
    {
        return NULL;
    }
}

OSW_C_API HANDLE WINAPI OSW_CreateDriverEx(const char* address, int port, int commType)
{
    try
    {
        int oswPort = (port > 0) ? port : CViaviOSWDriver::DEFAULT_PORT;
        CommType ct = static_cast<CommType>(commType);
        CViaviOSWDriver* driver = new CViaviOSWDriver(
            address ? address : "", oswPort, 5.0, ct);
        return reinterpret_cast<HANDLE>(driver);
    }
    catch (...)
    {
        return NULL;
    }
}

OSW_C_API void WINAPI OSW_DestroyDriver(HANDLE hDriver)
{
    if (hDriver)
    {
        IViaviOSWDriver* driver = ToDriver(hDriver);
        driver->Disconnect();
        delete driver;
    }
}

// ---------------------------------------------------------------------------
// 连接
// ---------------------------------------------------------------------------

OSW_C_API BOOL WINAPI OSW_Connect(HANDLE hDriver)
{
    if (!hDriver) return FALSE;
    try { return ToDriver(hDriver)->Connect() ? TRUE : FALSE; }
    catch (...) { return FALSE; }
}

OSW_C_API void WINAPI OSW_Disconnect(HANDLE hDriver)
{
    if (hDriver)
    {
        try { ToDriver(hDriver)->Disconnect(); }
        catch (...) {}
    }
}

OSW_C_API BOOL WINAPI OSW_IsConnected(HANDLE hDriver)
{
    if (!hDriver) return FALSE;
    try { return ToDriver(hDriver)->IsConnected() ? TRUE : FALSE; }
    catch (...) { return FALSE; }
}

// ---------------------------------------------------------------------------
// 设备识别
// ---------------------------------------------------------------------------

OSW_C_API BOOL WINAPI OSW_GetDeviceInfo(HANDLE hDriver, CDeviceInfo* info)
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

OSW_C_API int WINAPI OSW_CheckError(HANDLE hDriver, char* message, int messageSize)
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
// 开关信息
// ---------------------------------------------------------------------------

OSW_C_API int WINAPI OSW_GetDeviceCount(HANDLE hDriver)
{
    if (!hDriver) return 0;
    try { return ToDriver(hDriver)->GetDeviceCount(); }
    catch (...) { return 0; }
}

OSW_C_API BOOL WINAPI OSW_GetSwitchInfo(HANDLE hDriver, int deviceNum, CSwitchInfo* info)
{
    if (!hDriver || !info) return FALSE;
    try
    {
        SwitchInfo si = ToDriver(hDriver)->GetSwitchInfo(deviceNum);
        memset(info, 0, sizeof(CSwitchInfo));
        info->deviceNum = si.deviceNum;
        strncpy_s(info->description, si.description.c_str(), _TRUNCATE);
        info->switchType = static_cast<int>(si.switchType);
        info->channelCount = si.channelCount;
        info->currentChannel = si.currentChannel;
        return TRUE;
    }
    catch (...) { return FALSE; }
}

// ---------------------------------------------------------------------------
// 通道切换
// ---------------------------------------------------------------------------

OSW_C_API BOOL WINAPI OSW_SwitchChannel(HANDLE hDriver, int deviceNum, int channel)
{
    if (!hDriver) return FALSE;
    try
    {
        ToDriver(hDriver)->SwitchChannel(deviceNum, channel);
        return TRUE;
    }
    catch (...) { return FALSE; }
}

OSW_C_API int WINAPI OSW_GetCurrentChannel(HANDLE hDriver, int deviceNum)
{
    if (!hDriver) return -1;
    try { return ToDriver(hDriver)->GetCurrentChannel(deviceNum); }
    catch (...) { return -1; }
}

OSW_C_API int WINAPI OSW_GetChannelCount(HANDLE hDriver, int deviceNum)
{
    if (!hDriver) return 0;
    try { return ToDriver(hDriver)->GetChannelCount(deviceNum); }
    catch (...) { return 0; }
}

// ---------------------------------------------------------------------------
// 控制
// ---------------------------------------------------------------------------

OSW_C_API BOOL WINAPI OSW_SetLocalMode(HANDLE hDriver, BOOL local)
{
    if (!hDriver) return FALSE;
    try
    {
        ToDriver(hDriver)->SetLocalMode(local != FALSE);
        return TRUE;
    }
    catch (...) { return FALSE; }
}

OSW_C_API BOOL WINAPI OSW_Reset(HANDLE hDriver)
{
    if (!hDriver) return FALSE;
    try
    {
        ToDriver(hDriver)->Reset();
        return TRUE;
    }
    catch (...) { return FALSE; }
}

// ---------------------------------------------------------------------------
// 操作同步
// ---------------------------------------------------------------------------

OSW_C_API BOOL WINAPI OSW_WaitForIdle(HANDLE hDriver, int timeoutMs)
{
    if (!hDriver) return FALSE;
    try { return ToDriver(hDriver)->WaitForIdle(timeoutMs) ? TRUE : FALSE; }
    catch (...) { return FALSE; }
}

// ---------------------------------------------------------------------------
// 原始 SCPI
// ---------------------------------------------------------------------------

OSW_C_API BOOL WINAPI OSW_SendCommand(HANDLE hDriver, const char* command,
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

OSW_C_API void WINAPI OSW_SetLogCallback(OSWLogCallback callback)
{
    if (callback)
    {
        CViaviOSWDriver::SetGlobalLogCallback(
            [callback](LogLevel level, const std::string& source, const std::string& message)
            {
                callback(static_cast<int>(level), source.c_str(), message.c_str());
            });
    }
    else
    {
        CViaviOSWDriver::SetGlobalLogCallback(nullptr);
    }
}

OSW_C_API void WINAPI OSW_SetLogCallbackEx(HANDLE hDriver, OSWLogCallback callback)
{
    CViaviOSWDriver* driver = static_cast<CViaviOSWDriver*>(hDriver);
    if (!driver) return;

    if (callback)
    {
        driver->SetLogCallback(
            [callback](LogLevel level, const std::string& source, const std::string& message)
            {
                callback(static_cast<int>(level), source.c_str(), message.c_str());
            });
    }
    else
    {
        driver->SetLogCallback(nullptr);
    }
}

// ---------------------------------------------------------------------------
// VISA 枚举
// ---------------------------------------------------------------------------

OSW_C_API int WINAPI OSW_EnumerateVisaResources(char* buffer, int bufferSize)
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
