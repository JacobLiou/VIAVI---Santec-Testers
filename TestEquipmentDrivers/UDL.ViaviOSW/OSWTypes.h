#pragma once

#ifdef UDLVIAVIOSW_EXPORTS
#define OSW_API __declspec(dllexport)
#else
#define OSW_API __declspec(dllimport)
#endif

#include <string>
#include <vector>
#include <cstring>

namespace ViaviOSW {

// ---------------------------------------------------------------------------
// 连接状态
// ---------------------------------------------------------------------------
enum ConnectionState
{
    STATE_DISCONNECTED = 0,
    STATE_CONNECTING,
    STATE_CONNECTED,
    STATE_ERROR
};

// ---------------------------------------------------------------------------
// 开关类型（根据 mOSW-C1 数据手册的配置）
// ---------------------------------------------------------------------------
enum SwitchType
{
    SWITCH_1C = 0,  // 1xN 通道切换
    SWITCH_2D,      // 双 1xN 切换
    SWITCH_2E,      // 1xN + 直通
    SWITCH_2X       // 双重 1xN
};

// ---------------------------------------------------------------------------
// 通信类型
// ---------------------------------------------------------------------------
enum CommType
{
    COMM_TCP = 0,
    COMM_GPIB,
    COMM_VISA
};

// ---------------------------------------------------------------------------
// 日志级别
// ---------------------------------------------------------------------------
enum LogLevel
{
    LOG_DEBUG = 0,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
};

// ---------------------------------------------------------------------------
// 连接配置
// ---------------------------------------------------------------------------
struct OSW_API ConnectionConfig
{
    std::string ipAddress;
    int    port;
    double timeout;             // 秒
    int    bufferSize;
    int    reconnectAttempts;
    double reconnectDelay;      // 秒

    ConnectionConfig()
        : port(8203), timeout(5.0), bufferSize(4096)
        , reconnectAttempts(3), reconnectDelay(2.0)
    {}
};

// ---------------------------------------------------------------------------
// 设备标识信息（从 :INFO? 解析）
// ---------------------------------------------------------------------------
struct OSW_API DeviceInfo
{
    std::string serialNumber;
    std::string partNumber;
    std::string firmwareVersion;
    std::string hardwareVersion;
    std::string description;
    int slot;

    DeviceInfo() : slot(0) {}
};

// ---------------------------------------------------------------------------
// CheckError() 返回的错误信息
// ---------------------------------------------------------------------------
struct OSW_API ErrorInfo
{
    int code;
    std::string message;

    ErrorInfo() : code(0) {}
    ErrorInfo(int c, const std::string& msg) : code(c), message(msg) {}
    bool IsOK() const { return code == 0; }
};

// ---------------------------------------------------------------------------
// 开关信息（从 :CONF? 解析）
// ---------------------------------------------------------------------------
struct OSW_API SwitchInfo
{
    int         deviceNum;
    std::string description;
    SwitchType  switchType;
    int         channelCount;
    int         currentChannel;

    SwitchInfo()
        : deviceNum(1), switchType(SWITCH_1C)
        , channelCount(0), currentChannel(0)
    {}
};

// ---------------------------------------------------------------------------
// 用于 DLL 导出接口的 C 兼容结构体
// ---------------------------------------------------------------------------
struct OSW_API CDeviceInfo
{
    char serialNumber[64];
    char partNumber[64];
    char firmwareVersion[64];
    char description[128];
    int  slot;
};

struct OSW_API CSwitchInfo
{
    int  deviceNum;
    char description[128];
    int  switchType;
    int  channelCount;
    int  currentChannel;
};

struct OSW_API CConnectionConfig
{
    char   ipAddress[64];
    int    port;
    double timeout;
    int    bufferSize;
    int    reconnectAttempts;
    double reconnectDelay;
};

} // namespace ViaviOSW
