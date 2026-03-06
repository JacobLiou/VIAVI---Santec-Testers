#pragma once

#ifdef UDLVIAVIPCT_EXPORTS
#define PCT_API __declspec(dllexport)
#else
#define PCT_API __declspec(dllimport)
#endif

#include <string>
#include <vector>

namespace ViaviPCT {

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
// 测量模式 (0=参考, 1=DUT测量)
// 对应 :SENSe:FUNCtion 命令
// ---------------------------------------------------------------------------
enum MeasurementMode
{
    MODE_REFERENCE = 0,
    MODE_DUT = 1
};

// ---------------------------------------------------------------------------
// 测量状态 (来自 :MEASure:STATe? 响应)
// 0=初始化中, 1=空闲, 2=忙碌, 3=故障, 4=系统功能
// ---------------------------------------------------------------------------
enum MeasurementState
{
    MEAS_INITIALIZING = 0,
    MEAS_IDLE = 1,
    MEAS_BUSY = 2,
    MEAS_FAULT = 3,
    MEAS_SYSTEM = 4
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
// 单通道/单波长测量结果
// ---------------------------------------------------------------------------
struct PCT_API MeasurementResult
{
    int    channel;
    double wavelength;          // 纳米 (nm)
    double insertionLoss;       // 分贝 (dB) - 插入损耗 (IL)
    double returnLoss;          // 分贝 (dB) - 回波损耗 (ORL)
    double returnLossZone1;     // 分贝 (dB) - 区域1 ORL
    double returnLossZone2;     // 分贝 (dB) - 区域2 ORL
    double dutLength;           // 米 - 被测器件长度
    double power;               // 分贝毫瓦 (dBm) - 光功率
    double rawData[10];
    int    rawDataCount;

    MeasurementResult()
        : channel(0), wavelength(0.0)
        , insertionLoss(0.0), returnLoss(0.0)
        , returnLossZone1(0.0), returnLossZone2(0.0)
        , dutLength(0.0), power(0.0)
        , rawDataCount(0)
    {
        memset(rawData, 0, sizeof(rawData));
    }
};

// ---------------------------------------------------------------------------
// 连接配置
// ---------------------------------------------------------------------------
struct PCT_API ConnectionConfig
{
    std::string ipAddress;
    int    port;
    double timeout;             // 秒
    int    bufferSize;
    int    reconnectAttempts;
    double reconnectDelay;      // 秒

    ConnectionConfig()
        : port(8301), timeout(5.0), bufferSize(4096)
        , reconnectAttempts(3), reconnectDelay(2.0)
    {}
};

// ---------------------------------------------------------------------------
// 设备标识信息 (从 :INFO? 解析)
// ---------------------------------------------------------------------------
struct PCT_API DeviceInfo
{
    std::string serialNumber;
    std::string partNumber;
    std::string firmwareVersion;
    std::string moduleVersion1;
    std::string moduleVersion2;
    std::string hardwareVersion;
    std::string assemblyDate;
    std::string description;
    int slot;

    DeviceInfo() : slot(0) {}
};

// ---------------------------------------------------------------------------
// CheckError() 返回的错误信息
// ---------------------------------------------------------------------------
struct PCT_API ErrorInfo
{
    int code;
    std::string message;

    ErrorInfo() : code(0) {}
    ErrorInfo(int c, const std::string& msg) : code(c), message(msg) {}
    bool IsOK() const { return code == 0; }
};

// ---------------------------------------------------------------------------
// 参考覆盖参数
// ---------------------------------------------------------------------------
struct PCT_API ReferenceConfig
{
    bool   useOverride;
    double ilValue;             // 分贝 (dB)
    double lengthValue;         // 米

    ReferenceConfig()
        : useOverride(false), ilValue(0.1), lengthValue(3.0)
    {}
};

// ---------------------------------------------------------------------------
// PCT 模块配置 (从 :CONFig? 解析)
// ---------------------------------------------------------------------------
struct PCT_API PCTConfig
{
    int         deviceNum;
    std::string deviceType;     // "PCT"
    std::string fiberType;      // "SM9", "MM50", "MM62.5"
    int         outputs;        // 1=单输出, 2=双向启用, 3=双向禁用
    int         sw1Channels;    // 开关1通道数 (1=无开关)
    int         sw2Channels;    // 开关2通道数 (1=无开关)
    std::vector<int> wavelengths; // 可用波长 (nm)

    PCTConfig()
        : deviceNum(1), outputs(1)
        , sw1Channels(1), sw2Channels(1)
    {}
};

// ---------------------------------------------------------------------------
// 用于 DLL 导出接口的 C 兼容结构体
// ---------------------------------------------------------------------------
struct PCT_API CMeasurementResult
{
    int    channel;
    double wavelength;
    double insertionLoss;
    double returnLoss;
    double returnLossZone1;
    double returnLossZone2;
    double dutLength;
    double power;
    double rawData[10];
    int    rawDataCount;
};

struct PCT_API CDeviceInfo
{
    char serialNumber[64];
    char partNumber[64];
    char firmwareVersion[64];
    char description[128];
    int  slot;
};

struct PCT_API CConnectionConfig
{
    char   ipAddress[64];
    int    port;
    double timeout;
    int    bufferSize;
    int    reconnectAttempts;
    double reconnectDelay;
};

} // namespace ViaviPCT
