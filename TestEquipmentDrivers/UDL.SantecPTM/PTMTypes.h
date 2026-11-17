#pragma once

#ifdef UDLSANTECPTM_EXPORTS
#define PTM_API __declspec(dllexport)
#else
#define PTM_API __declspec(dllimport)
#endif

#include <string>
#include <vector>
#include <cstring>

namespace SantecPTM {

enum ConnectionState
{
    STATE_DISCONNECTED = 0,
    STATE_CONNECTING,
    STATE_CONNECTED,
    STATE_ERROR
};

enum CommType
{
    COMM_TCP = 0,
    COMM_GPIB,
    COMM_USB,
    COMM_DLL
};

enum VflMode
{
    VFL_OFF = 0,
    VFL_ON,
    VFL_FLASH
};

// Single fiber mapping: output laser channel -> brightest/detected input
struct PTM_API PolarityChannelResult
{
    int    outputChannel;
    int    inputChannel;
    double rawData[4];
    int    rawDataCount;

    PolarityChannelResult()
        : outputChannel(0), inputChannel(0), rawDataCount(0)
    {
        memset(rawData, 0, sizeof(rawData));
    }
};

struct PTM_API ConnectionConfig
{
    std::string ipAddress;
    int    port;
    double timeout;
    int    bufferSize;
    int    reconnectAttempts;
    double reconnectDelay;

    ConnectionConfig()
        : port(5025), timeout(5.0), bufferSize(4096)
        , reconnectAttempts(3), reconnectDelay(2.0)
    {}
};

struct PTM_API DeviceInfo
{
    std::string manufacturer;
    std::string model;
    std::string serialNumber;
    std::string firmwareVersion;
    int slot;

    DeviceInfo() : slot(0) {}
};

struct PTM_API ErrorInfo
{
    int code;
    std::string message;

    ErrorInfo() : code(0) {}
    ErrorInfo(int c, const std::string& msg) : code(c), message(msg) {}
    bool IsOK() const { return code == 0; }
};

// C-compatible exports
struct PTM_API CPolarityChannelResult
{
    int    outputChannel;
    int    inputChannel;
    double rawData[4];
    int    rawDataCount;
};

struct PTM_API CDeviceInfo
{
    char manufacturer[64];
    char model[64];
    char serialNumber[64];
    char firmwareVersion[64];
    int  slot;
};

struct PTM_API CConnectionConfig
{
    char   ipAddress[64];
    int    port;
    double timeout;
    int    bufferSize;
    int    reconnectAttempts;
    double reconnectDelay;
};

} // namespace SantecPTM
