#pragma once

#include "IOSXDriver.h"
#include <string>
#include <vector>
#include <functional>
#include <mutex>

namespace OSXSwitch {

// ---------------------------------------------------------------------------
// Log callback type
// ---------------------------------------------------------------------------
typedef std::function<void(LogLevel level, const std::string& source, const std::string& message)> LogCallback;

// ---------------------------------------------------------------------------
// COSXDriver -- concrete driver for the OSX Optical Switch
//
// Communication: TCP/IP on port 5025 (SCPI-RAW per LXI standard)
// Protocol:      SCPI, commands terminated with \n
// Async model:   All switch commands are asynchronous.  After issuing a
//                command, poll STAT:OPER:COND? until 0.
//
// Reference: OSX User Manual M-OSX_SX1-001.04, Tables 4 & 5
// ---------------------------------------------------------------------------
class OSX_API COSXDriver : public IOSXDriver
{
public:
    static const int DEFAULT_PORT           = 5025;
    static const int DEFAULT_TIMEOUT_MS     = 5000;
    static const int SWITCH_TIMEOUT_MS      = 5000;
    static const int MAX_POLL_INTERVAL_MS   = 10;
    static const int MAX_OPERATION_WAIT_MS  = 30000;

    COSXDriver(const std::string& ipAddress,
               int port = DEFAULT_PORT,
               double timeout = 5.0);
    virtual ~COSXDriver();

    // ---- IOSXDriver: Connection ----
    virtual bool Connect() override;
    virtual void Disconnect() override;
    virtual bool Reconnect() override;
    virtual bool IsConnected() const override;

    // ---- IOSXDriver: Device identification ----
    virtual DeviceInfo  GetDeviceInfo() override;
    virtual ErrorInfo   CheckError() override;
    virtual std::string GetSystemVersion() override;

    // ---- IOSXDriver: Module management ----
    virtual int  GetModuleCount() override;
    virtual std::vector<ModuleInfo> GetModuleCatalog() override;
    virtual ModuleInfo GetModuleInfo(int moduleIndex) override;
    virtual void SelectModule(int moduleIndex) override;
    virtual void SelectNextModule() override;
    virtual int  GetSelectedModule() override;

    // ---- IOSXDriver: Channel switching ----
    virtual void SwitchChannel(int channel) override;
    virtual void SwitchNext() override;
    virtual int  GetCurrentChannel() override;
    virtual int  GetChannelCount() override;

    // ---- IOSXDriver: Multi-module routing ----
    virtual void RouteChannel(int moduleIndex, int channel) override;
    virtual int  GetRouteChannel(int moduleIndex) override;
    virtual void RouteAllModules(int channel) override;
    virtual void SetCommonInput(int moduleIndex, int common) override;
    virtual int  GetCommonInput(int moduleIndex) override;
    virtual void HomeModule(int moduleIndex) override;

    // ---- IOSXDriver: Control ----
    virtual void SetLocalMode(bool local) override;
    virtual bool GetLocalMode() override;
    virtual void SendNotification(int icon, const std::string& message) override;
    virtual void Reset() override;

    // ---- IOSXDriver: Network configuration ----
    virtual std::string GetIPAddress() override;
    virtual std::string GetGateway() override;
    virtual std::string GetNetmask() override;
    virtual std::string GetHostname() override;
    virtual std::string GetMAC() override;

    // ---- IOSXDriver: Operation synchronization ----
    virtual bool WaitForOperation(int timeoutMs = 5000) override;

    // ---- IOSXDriver: Raw SCPI ----
    virtual std::string SendRawQuery(const std::string& command) override;
    virtual void        SendRawWrite(const std::string& command) override;

    // ---- Accessors ----
    ConnectionState GetState() const { return m_state; }
    const ConnectionConfig& GetConfig() const { return m_config; }

    // ---- Logging ----
    static void SetGlobalLogCallback(LogCallback callback);
    static void SetGlobalLogLevel(LogLevel level);

private:
    // Low-level TCP communication
    bool ConnectSocket(SOCKET sock, const sockaddr_in& addr, double timeoutSec);
    void EnableKeepAlive(SOCKET sock);
    bool ValidateConnection();
    void CleanupSocket();

    // SCPI command helpers
    std::string Query(const std::string& command);
    void        Write(const std::string& command);
    std::string ReceiveResponse();

    // Write a command then wait for operation complete
    void WriteAndWait(const std::string& command, int timeoutMs = SWITCH_TIMEOUT_MS);

    // Parsing
    void ParseIdentity(const std::string& idnResponse);
    static std::string Trim(const std::string& s);
    static SwitchConfigType ParseConfigType(const std::string& catalog);
    static int ParseChannelCount(const std::string& catalog);

    // Logging
    void Log(LogLevel level, const char* fmt, ...);

    // State
    SOCKET          m_socket;
    ConnectionConfig m_config;
    ConnectionState m_state;
    DeviceInfo      m_deviceInfo;

    // Logging
    static LogCallback s_globalCallback;
    static LogLevel    s_globalLevel;
    static std::mutex  s_logMutex;
};

} // namespace OSXSwitch
