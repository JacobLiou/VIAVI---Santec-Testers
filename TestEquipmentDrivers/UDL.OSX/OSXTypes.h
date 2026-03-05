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
// 开关配置类型（根据 OSX-100 数据手册）
// ---------------------------------------------------------------------------
enum SwitchConfigType
{
    CONFIG_1A = 0,  // 单输入，可切换到任意输出
    CONFIG_2A,      // 两个可切换输入，可切换到任意输出
    CONFIG_2B,      // 两个输入切换到同步输出
    CONFIG_2C       // 两个输入，第二个跟随第一个
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
struct OSX_API ConnectionConfig
{
    std::string ipAddress;
    int    port;
    double timeout;             // 秒
    int    bufferSize;
    int    reconnectAttempts;
    double reconnectDelay;      // 秒

    ConnectionConfig()
        : port(5025), timeout(3.0), bufferSize(4096)
        , reconnectAttempts(3), reconnectDelay(2.0)
    {}
};

// ---------------------------------------------------------------------------
// 设备识别信息（从 *IDN? 解析）
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
// CheckError() 返回的错误信息
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
// 模块信息（从 MODule:CATalog? 和 MODule#:INFO? 解析）
// ---------------------------------------------------------------------------
struct OSX_API ModuleInfo
{
    int              index;          // 从 0 开始的模块索引
    std::string      catalogEntry;   // 例如 "SX 1Ax24"
    std::string      detailedInfo;   // 来自 MODule#:INFO?
    SwitchConfigType configType;
    int              channelCount;
    int              currentChannel;
    int              currentCommon;  // 用于 2A/2C 配置

    ModuleInfo()
        : index(0), configType(CONFIG_1A)
        , channelCount(0), currentChannel(0), currentCommon(0)
    {}
};

// ---------------------------------------------------------------------------
// 用于 DLL 导出接口的 C 兼容结构体
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
