#pragma once

#ifdef UDLOSX_EXPORTS
#define OSX_API __declspec(dllexport)
#else
#define OSX_API __declspec(dllimport)
#endif

#include <string>
#include <vector>
#include <cstring>

namespace OSXSwitch {

// ---------------------------------------------------------------------------
// Connection State
// ---------------------------------------------------------------------------
enum ConnectionState
{
    STATE_DISCONNECTED = 0,
    STATE_CONNECTING,
    STATE_CONNECTED,
    STATE_ERROR
};

// ---------------------------------------------------------------------------
// Switch configuration type (per OSX-100 datasheet)
// ---------------------------------------------------------------------------
enum SwitchConfigType
{
    CONFIG_1A = 0,  // Single input, switched to any output
    CONFIG_2A,      // Two switchable inputs, switched to any output
    CONFIG_2B,      // Two inputs switched to synchronized outputs
    CONFIG_2C       // Two inputs, second trailing first
};

// ---------------------------------------------------------------------------
// Log level
// ---------------------------------------------------------------------------
enum LogLevel
{
    LOG_DEBUG = 0,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
};

// ---------------------------------------------------------------------------
// Connection configuration
// ---------------------------------------------------------------------------
struct OSX_API ConnectionConfig
{
    std::string ipAddress;
    int    port;
    double timeout;             // seconds
    int    bufferSize;
    int    reconnectAttempts;
    double reconnectDelay;      // seconds

    ConnectionConfig()
        : port(5025), timeout(3.0), bufferSize(4096)
        , reconnectAttempts(3), reconnectDelay(2.0)
    {}
};

// ---------------------------------------------------------------------------
// Device identification info (parsed from *IDN?)
// ---------------------------------------------------------------------------
struct OSX_API DeviceInfo
{
    std::string manufacturer;
    std::string model;
    std::string serialNumber;
    std::string firmwareVersion;

    DeviceInfo() {}
};

// ---------------------------------------------------------------------------
// Error info returned by CheckError()
// ---------------------------------------------------------------------------
struct OSX_API ErrorInfo
{
    int code;
    std::string message;

    ErrorInfo() : code(0) {}
    ErrorInfo(int c, const std::string& msg) : code(c), message(msg) {}
    bool IsOK() const { return code == 0; }
};

// ---------------------------------------------------------------------------
// Module info (parsed from MODule:CATalog? and MODule#:INFO?)
// ---------------------------------------------------------------------------
struct OSX_API ModuleInfo
{
    int              index;          // 0-based module index
    std::string      catalogEntry;   // e.g. "SX 1Ax24"
    std::string      detailedInfo;   // from MODule#:INFO?
    SwitchConfigType configType;
    int              channelCount;
    int              currentChannel;
    int              currentCommon;  // for 2A/2C configurations

    ModuleInfo()
        : index(0), configType(CONFIG_1A)
        , channelCount(0), currentChannel(0), currentCommon(0)
    {}
};

// ---------------------------------------------------------------------------
// C-compatible structures for DLL export interface
// ---------------------------------------------------------------------------
struct OSX_API CDeviceInfo
{
    char manufacturer[64];
    char model[64];
    char serialNumber[64];
    char firmwareVersion[64];
};

struct OSX_API CModuleInfo
{
    int  index;
    char catalogEntry[128];
    char detailedInfo[256];
    int  configType;
    int  channelCount;
    int  currentChannel;
    int  currentCommon;
};

struct OSX_API CConnectionConfig
{
    char   ipAddress[64];
    int    port;
    double timeout;
    int    bufferSize;
    int    reconnectAttempts;
    double reconnectDelay;
};

} // namespace OSXSwitch
