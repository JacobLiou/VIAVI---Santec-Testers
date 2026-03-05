#pragma once

#ifdef UDLSANTECRLM_EXPORTS
#define DRIVER_API __declspec(dllexport)
#else
#define DRIVER_API __declspec(dllimport)
#endif

#include <string>
#include <vector>

namespace SantecRLM {

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
// 测量模式 (0=参考, 1=测量)
// ---------------------------------------------------------------------------
enum MeasurementMode
{
    MODE_REFERENCE = 0,
    MODE_MEASUREMENT = 1
};

// ---------------------------------------------------------------------------
// 测量状态 (1=完成, 2=运行中, 3=错误)
// ---------------------------------------------------------------------------
enum MeasurementState
{
    MEAS_IDLE = 0,
    MEAS_RUNNING = 2,
    MEAS_COMPLETE = 1,
    MEAS_ERROR = 3
};

// ---------------------------------------------------------------------------
// 通信类型
// ---------------------------------------------------------------------------
enum CommType
{
    COMM_TCP = 0,
    COMM_GPIB,
    COMM_USB,
    COMM_DLL
};

// ---------------------------------------------------------------------------
// 单通道/单波长测量结果
// ---------------------------------------------------------------------------
struct DRIVER_API MeasurementResult
{
    int    channel;
    double wavelength;          // 纳米 (nm)
    double insertionLoss;       // 分贝 (dB) - 插入损耗 (IL)
    double returnLoss;          // 分贝 (dB) - 回波损耗 (RL)
    double returnLossA;         // 分贝 (dB) - 回波损耗A (RLA) - Santec RL1: 位置A连接器回波损耗
    double returnLossB;         // 分贝 (dB) - 回波损耗B (RLB) - Santec RL1: 位置B连接器回波损耗
    double returnLossTotal;     // 分贝 (dB) - 总回波损耗 (RLTOTAL) - Santec RL1: 总ORL
    double dutLength;           // 米 - Santec RL1: 从 READ:RL? 获取的被测器件长度
    double rawData[10];
    int    rawDataCount;

    MeasurementResult()
        : channel(0), wavelength(0.0)
        , insertionLoss(0.0), returnLoss(0.0)
        , returnLossA(0.0), returnLossB(0.0)
        , returnLossTotal(0.0), dutLength(0.0)
        , rawDataCount(0)
    {
        memset(rawData, 0, sizeof(rawData));
    }
};

// ---------------------------------------------------------------------------
// 连接配置
// ---------------------------------------------------------------------------
struct DRIVER_API ConnectionConfig
{
    std::string ipAddress;
    int    port;
    double timeout;             // 秒
    int    bufferSize;
    int    reconnectAttempts;
    double reconnectDelay;      // 秒

    ConnectionConfig()
        : port(0), timeout(3.0), bufferSize(1024)
        , reconnectAttempts(3), reconnectDelay(2.0)
    {}
};

// ---------------------------------------------------------------------------
// 设备标识信息
// ---------------------------------------------------------------------------
struct DRIVER_API DeviceInfo
{
    std::string manufacturer;
    std::string model;
    std::string serialNumber;
    std::string firmwareVersion;
    int slot;

    DeviceInfo() : slot(0) {}
};

// ---------------------------------------------------------------------------
// CheckError() 返回的错误信息
// ---------------------------------------------------------------------------
struct DRIVER_API ErrorInfo
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
struct DRIVER_API ReferenceConfig
{
    bool   useOverride;
    double ilValue;             // 分贝 (dB)
    double lengthValue;         // 米

    ReferenceConfig()
        : useOverride(false), ilValue(0.1), lengthValue(3.0)
    {}
};

// ---------------------------------------------------------------------------
// 用于 DLL 导出接口的 C 兼容结构体
// ---------------------------------------------------------------------------
struct DRIVER_API CMeasurementResult
{
    int    channel;
    double wavelength;
    double insertionLoss;
    double returnLoss;
    double returnLossA;
    double returnLossB;
    double returnLossTotal;
    double dutLength;
    double rawData[10];
    int    rawDataCount;
};

struct DRIVER_API CDeviceInfo
{
    char manufacturer[64];
    char model[64];
    char serialNumber[64];
    char firmwareVersion[64];
    int  slot;
};

struct DRIVER_API CConnectionConfig
{
    char   ipAddress[64];
    int    port;
    double timeout;
    int    bufferSize;
    int    reconnectAttempts;
    double reconnectDelay;
};

} // namespace SantecRLM
