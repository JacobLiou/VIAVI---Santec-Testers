#include "stdafx.h"
#include "OSXExports.h"
#include "COSXDriver.h"
#include <cstring>

using namespace OSXSwitch;

static IOSXDriver* ToDriver(HANDLE h)
{
    return reinterpret_cast<IOSXDriver*>(h);
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

OSX_C_API HANDLE WINAPI OSX_CreateDriver(const char* ip, int port)
{
    try
    {
        int actualPort = (port > 0) ? port : COSXDriver::DEFAULT_PORT;
        COSXDriver* driver = new COSXDriver(ip ? ip : "", actualPort);
        return reinterpret_cast<HANDLE>(driver);
    }
    catch (...)
    {
        return NULL;
    }
}

OSX_C_API void WINAPI OSX_DestroyDriver(HANDLE hDriver)
{
    if (hDriver)
    {
        IOSXDriver* driver = ToDriver(hDriver);
        try { driver->Disconnect(); } catch (...) {}
        delete driver;
    }
}

// ---------------------------------------------------------------------------
// Connection
// ---------------------------------------------------------------------------

OSX_C_API BOOL WINAPI OSX_Connect(HANDLE hDriver)
{
    if (!hDriver) return FALSE;
    try { return ToDriver(hDriver)->Connect() ? TRUE : FALSE; }
    catch (...) { return FALSE; }
}

OSX_C_API void WINAPI OSX_Disconnect(HANDLE hDriver)
{
    if (hDriver)
    {
        try { ToDriver(hDriver)->Disconnect(); }
        catch (...) {}
    }
}

OSX_C_API BOOL WINAPI OSX_Reconnect(HANDLE hDriver)
{
    if (!hDriver) return FALSE;
    try { return ToDriver(hDriver)->Reconnect() ? TRUE : FALSE; }
    catch (...) { return FALSE; }
}

OSX_C_API BOOL WINAPI OSX_IsConnected(HANDLE hDriver)
{
    if (!hDriver) return FALSE;
    try { return ToDriver(hDriver)->IsConnected() ? TRUE : FALSE; }
    catch (...) { return FALSE; }
}

// ---------------------------------------------------------------------------
// Device identification
// ---------------------------------------------------------------------------

OSX_C_API BOOL WINAPI OSX_GetDeviceInfo(HANDLE hDriver, CDeviceInfo* info)
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
        return TRUE;
    }
    catch (...) { return FALSE; }
}

OSX_C_API int WINAPI OSX_CheckError(HANDLE hDriver, char* message, int messageSize)
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

OSX_C_API BOOL WINAPI OSX_GetSystemVersion(HANDLE hDriver, char* version, int versionSize)
{
    if (!hDriver || !version || versionSize <= 0) return FALSE;
    try
    {
        std::string ver = ToDriver(hDriver)->GetSystemVersion();
        strncpy_s(version, versionSize, ver.c_str(), _TRUNCATE);
        return TRUE;
    }
    catch (...) { return FALSE; }
}

// ---------------------------------------------------------------------------
// Module management
// ---------------------------------------------------------------------------

OSX_C_API int WINAPI OSX_GetModuleCount(HANDLE hDriver)
{
    if (!hDriver) return -1;
    try { return ToDriver(hDriver)->GetModuleCount(); }
    catch (...) { return -1; }
}

static void CopyModuleInfo(const ModuleInfo& src, CModuleInfo& dst)
{
    memset(&dst, 0, sizeof(CModuleInfo));
    dst.index = src.index;
    strncpy_s(dst.catalogEntry, src.catalogEntry.c_str(), _TRUNCATE);
    strncpy_s(dst.detailedInfo, src.detailedInfo.c_str(), _TRUNCATE);
    dst.configType = static_cast<int>(src.configType);
    dst.channelCount = src.channelCount;
    dst.currentChannel = src.currentChannel;
    dst.currentCommon = src.currentCommon;
}

OSX_C_API int WINAPI OSX_GetModuleCatalog(HANDLE hDriver, CModuleInfo* modules, int maxCount)
{
    if (!hDriver || !modules || maxCount <= 0) return 0;
    try
    {
        std::vector<ModuleInfo> mods = ToDriver(hDriver)->GetModuleCatalog();
        int count = (std::min)(static_cast<int>(mods.size()), maxCount);
        for (int i = 0; i < count; ++i)
            CopyModuleInfo(mods[i], modules[i]);
        return count;
    }
    catch (...) { return 0; }
}

OSX_C_API BOOL WINAPI OSX_GetModuleInfo(HANDLE hDriver, int moduleIndex, CModuleInfo* info)
{
    if (!hDriver || !info) return FALSE;
    try
    {
        ModuleInfo mi = ToDriver(hDriver)->GetModuleInfo(moduleIndex);
        CopyModuleInfo(mi, *info);
        return TRUE;
    }
    catch (...) { return FALSE; }
}

OSX_C_API BOOL WINAPI OSX_SelectModule(HANDLE hDriver, int moduleIndex)
{
    if (!hDriver) return FALSE;
    try { ToDriver(hDriver)->SelectModule(moduleIndex); return TRUE; }
    catch (...) { return FALSE; }
}

OSX_C_API BOOL WINAPI OSX_SelectNextModule(HANDLE hDriver)
{
    if (!hDriver) return FALSE;
    try { ToDriver(hDriver)->SelectNextModule(); return TRUE; }
    catch (...) { return FALSE; }
}

OSX_C_API int WINAPI OSX_GetSelectedModule(HANDLE hDriver)
{
    if (!hDriver) return -1;
    try { return ToDriver(hDriver)->GetSelectedModule(); }
    catch (...) { return -1; }
}

// ---------------------------------------------------------------------------
// Channel switching
// ---------------------------------------------------------------------------

OSX_C_API BOOL WINAPI OSX_SwitchChannel(HANDLE hDriver, int channel)
{
    if (!hDriver) return FALSE;
    try { ToDriver(hDriver)->SwitchChannel(channel); return TRUE; }
    catch (...) { return FALSE; }
}

OSX_C_API BOOL WINAPI OSX_SwitchNext(HANDLE hDriver)
{
    if (!hDriver) return FALSE;
    try { ToDriver(hDriver)->SwitchNext(); return TRUE; }
    catch (...) { return FALSE; }
}

OSX_C_API int WINAPI OSX_GetCurrentChannel(HANDLE hDriver)
{
    if (!hDriver) return -1;
    try { return ToDriver(hDriver)->GetCurrentChannel(); }
    catch (...) { return -1; }
}

OSX_C_API int WINAPI OSX_GetChannelCount(HANDLE hDriver)
{
    if (!hDriver) return -1;
    try { return ToDriver(hDriver)->GetChannelCount(); }
    catch (...) { return -1; }
}

// ---------------------------------------------------------------------------
// Multi-module routing
// ---------------------------------------------------------------------------

OSX_C_API BOOL WINAPI OSX_RouteChannel(HANDLE hDriver, int moduleIndex, int channel)
{
    if (!hDriver) return FALSE;
    try { ToDriver(hDriver)->RouteChannel(moduleIndex, channel); return TRUE; }
    catch (...) { return FALSE; }
}

OSX_C_API int WINAPI OSX_GetRouteChannel(HANDLE hDriver, int moduleIndex)
{
    if (!hDriver) return -1;
    try { return ToDriver(hDriver)->GetRouteChannel(moduleIndex); }
    catch (...) { return -1; }
}

OSX_C_API BOOL WINAPI OSX_RouteAllModules(HANDLE hDriver, int channel)
{
    if (!hDriver) return FALSE;
    try { ToDriver(hDriver)->RouteAllModules(channel); return TRUE; }
    catch (...) { return FALSE; }
}

OSX_C_API BOOL WINAPI OSX_SetCommonInput(HANDLE hDriver, int moduleIndex, int common)
{
    if (!hDriver) return FALSE;
    try { ToDriver(hDriver)->SetCommonInput(moduleIndex, common); return TRUE; }
    catch (...) { return FALSE; }
}

OSX_C_API int WINAPI OSX_GetCommonInput(HANDLE hDriver, int moduleIndex)
{
    if (!hDriver) return -1;
    try { return ToDriver(hDriver)->GetCommonInput(moduleIndex); }
    catch (...) { return -1; }
}

OSX_C_API BOOL WINAPI OSX_HomeModule(HANDLE hDriver, int moduleIndex)
{
    if (!hDriver) return FALSE;
    try { ToDriver(hDriver)->HomeModule(moduleIndex); return TRUE; }
    catch (...) { return FALSE; }
}

// ---------------------------------------------------------------------------
// Control
// ---------------------------------------------------------------------------

OSX_C_API BOOL WINAPI OSX_SetLocalMode(HANDLE hDriver, BOOL local)
{
    if (!hDriver) return FALSE;
    try { ToDriver(hDriver)->SetLocalMode(local != FALSE); return TRUE; }
    catch (...) { return FALSE; }
}

OSX_C_API BOOL WINAPI OSX_GetLocalMode(HANDLE hDriver)
{
    if (!hDriver) return FALSE;
    try { return ToDriver(hDriver)->GetLocalMode() ? TRUE : FALSE; }
    catch (...) { return FALSE; }
}

OSX_C_API BOOL WINAPI OSX_SendNotification(HANDLE hDriver, int icon, const char* message)
{
    if (!hDriver || !message) return FALSE;
    try { ToDriver(hDriver)->SendNotification(icon, message); return TRUE; }
    catch (...) { return FALSE; }
}

OSX_C_API BOOL WINAPI OSX_Reset(HANDLE hDriver)
{
    if (!hDriver) return FALSE;
    try { ToDriver(hDriver)->Reset(); return TRUE; }
    catch (...) { return FALSE; }
}

// ---------------------------------------------------------------------------
// Network configuration
// ---------------------------------------------------------------------------

OSX_C_API BOOL WINAPI OSX_GetNetworkInfo(HANDLE hDriver, char* ip, char* gateway,
                                         char* netmask, char* hostname, char* mac,
                                         int bufSize)
{
    if (!hDriver || bufSize <= 0) return FALSE;
    try
    {
        IOSXDriver* drv = ToDriver(hDriver);
        if (ip)       strncpy_s(ip, bufSize, drv->GetIPAddress().c_str(), _TRUNCATE);
        if (gateway)  strncpy_s(gateway, bufSize, drv->GetGateway().c_str(), _TRUNCATE);
        if (netmask)  strncpy_s(netmask, bufSize, drv->GetNetmask().c_str(), _TRUNCATE);
        if (hostname) strncpy_s(hostname, bufSize, drv->GetHostname().c_str(), _TRUNCATE);
        if (mac)      strncpy_s(mac, bufSize, drv->GetMAC().c_str(), _TRUNCATE);
        return TRUE;
    }
    catch (...) { return FALSE; }
}

// ---------------------------------------------------------------------------
// Operation synchronization
// ---------------------------------------------------------------------------

OSX_C_API BOOL WINAPI OSX_WaitForOperation(HANDLE hDriver, int timeoutMs)
{
    if (!hDriver) return FALSE;
    try { return ToDriver(hDriver)->WaitForOperation(timeoutMs) ? TRUE : FALSE; }
    catch (...) { return FALSE; }
}

// ---------------------------------------------------------------------------
// Raw SCPI
// ---------------------------------------------------------------------------

OSX_C_API BOOL WINAPI OSX_SendCommand(HANDLE hDriver, const char* command,
                                      char* response, int responseSize)
{
    if (!hDriver || !command) return FALSE;
    try
    {
        IOSXDriver* drv = ToDriver(hDriver);
        bool isQuery = (std::string(command).find('?') != std::string::npos);

        if (isQuery)
        {
            std::string resp = drv->SendRawQuery(command);
            if (response && responseSize > 0)
                strncpy_s(response, responseSize, resp.c_str(), _TRUNCATE);
        }
        else
        {
            drv->SendRawWrite(command);
            if (response && responseSize > 0)
                response[0] = '\0';
        }
        return TRUE;
    }
    catch (...) { return FALSE; }
}

// ---------------------------------------------------------------------------
// Logging
// ---------------------------------------------------------------------------

OSX_C_API void WINAPI OSX_SetLogCallback(OSXLogCallback callback)
{
    if (callback)
    {
        COSXDriver::SetGlobalLogCallback(
            [callback](LogLevel level, const std::string& source, const std::string& message)
            {
                callback(static_cast<int>(level), source.c_str(), message.c_str());
            });
    }
    else
    {
        COSXDriver::SetGlobalLogCallback(nullptr);
    }
}
