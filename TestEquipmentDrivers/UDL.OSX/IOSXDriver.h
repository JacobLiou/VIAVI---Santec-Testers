#pragma once

#include "OSXTypes.h"
#include <vector>
#include <string>

namespace OSXSwitch {

// ---------------------------------------------------------------------------
// IOSXDriver -- abstract interface for the OSX Optical Switch
//
// The OSX is a multi-module optical switch controlled via SCPI over TCP/IP
// (port 5025) or USB (USBTMC).  Unlike measurement instruments, the primary
// operations are channel switching and module management.
//
// IMPORTANT: OSX runs SCPI commands asynchronously.  After issuing a switch
// command the caller must poll STAT:OPER:COND? until 0 (or use
// WaitForOperation) before sending the next command.
// ---------------------------------------------------------------------------
class OSX_API IOSXDriver
{
public:
    virtual ~IOSXDriver() {}

    // ---- Connection lifecycle ----
    virtual bool Connect() = 0;
    virtual void Disconnect() = 0;
    virtual bool Reconnect() = 0;
    virtual bool IsConnected() const = 0;

    // ---- Device identification ----
    virtual DeviceInfo GetDeviceInfo() = 0;
    virtual ErrorInfo  CheckError() = 0;
    virtual std::string GetSystemVersion() = 0;

    // ---- Module management ----
    virtual int  GetModuleCount() = 0;
    virtual std::vector<ModuleInfo> GetModuleCatalog() = 0;
    virtual ModuleInfo GetModuleInfo(int moduleIndex) = 0;
    virtual void SelectModule(int moduleIndex) = 0;
    virtual void SelectNextModule() = 0;
    virtual int  GetSelectedModule() = 0;

    // ---- Channel switching (operates on the currently selected module) ----
    virtual void SwitchChannel(int channel) = 0;
    virtual void SwitchNext() = 0;
    virtual int  GetCurrentChannel() = 0;
    virtual int  GetChannelCount() = 0;

    // ---- Multi-module routing ----
    virtual void RouteChannel(int moduleIndex, int channel) = 0;
    virtual int  GetRouteChannel(int moduleIndex) = 0;
    virtual void RouteAllModules(int channel) = 0;
    virtual void SetCommonInput(int moduleIndex, int common) = 0;
    virtual int  GetCommonInput(int moduleIndex) = 0;
    virtual void HomeModule(int moduleIndex) = 0;

    // ---- Control ----
    virtual void SetLocalMode(bool local) = 0;
    virtual bool GetLocalMode() = 0;
    virtual void SendNotification(int icon, const std::string& message) = 0;
    virtual void Reset() = 0;

    // ---- Network configuration (read-only queries) ----
    virtual std::string GetIPAddress() = 0;
    virtual std::string GetGateway() = 0;
    virtual std::string GetNetmask() = 0;
    virtual std::string GetHostname() = 0;
    virtual std::string GetMAC() = 0;

    // ---- Operation synchronization ----
    // Polls STAT:OPER:COND? until the device reports idle (0).
    // Returns true if idle within timeoutMs, false on timeout.
    virtual bool WaitForOperation(int timeoutMs = 5000) = 0;

    // ---- Raw SCPI passthrough ----
    virtual std::string SendRawQuery(const std::string& command) = 0;
    virtual void        SendRawWrite(const std::string& command) = 0;
};

} // namespace OSXSwitch
