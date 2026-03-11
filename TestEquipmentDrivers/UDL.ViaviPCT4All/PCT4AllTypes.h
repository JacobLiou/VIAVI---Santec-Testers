#pragma once

#ifdef UDLVIAVIPCT4ALL_EXPORTS
#define PCT4ALL_API __declspec(dllexport)
#else
#define PCT4ALL_API __declspec(dllimport)
#endif

#include <string>
#include <vector>

namespace ViaviPCT4All {

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
// 测量模式 (对应 :SENSe:FUNCtion)
// ---------------------------------------------------------------------------
enum MeasurementMode
{
    MODE_REFERENCE = 0,
    MODE_DUT = 1
};

// ---------------------------------------------------------------------------
// 测量状态 (对应 :MEASure:STATe? 响应)
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
// 测量范围 (对应 :SENSe:RANGe)
// ---------------------------------------------------------------------------
enum MeasRange
{
    RANGE_200M  = 0,
    RANGE_500M  = 1,
    RANGE_1KM   = 2,
    RANGE_2KM   = 3,
    RANGE_5KM   = 4,
    RANGE_10KM  = 5
};

// ---------------------------------------------------------------------------
// 温度灵敏度 (对应 :SENSe:TEMPerature)
// ---------------------------------------------------------------------------
enum TempSensitivity
{
    TEMP_VERY_LOW  = 0,
    TEMP_LOW       = 1,
    TEMP_MEDIUM    = 2,
    TEMP_HIGH      = 3,
    TEMP_VERY_HIGH = 4
};

// ---------------------------------------------------------------------------
// 通道组 (对应 :PATH:CHANnel 的 <group>)
// ---------------------------------------------------------------------------
enum ChannelGroup
{
    GROUP_MTJ1    = 1,
    GROUP_MTJ2    = 2,
    GROUP_RECEIVE = 3
};

// ---------------------------------------------------------------------------
// 连接模式 (对应 :PATH:CONNection)
// ---------------------------------------------------------------------------
enum ConnectionMode
{
    CONN_SINGLE_MTJ = 1,
    CONN_DUAL_MTJ   = 2
};

// ---------------------------------------------------------------------------
// Port Map 模式 (对应 :PMAP:SETup:MODE?)
// ---------------------------------------------------------------------------
enum PortMapMode
{
    PMAP_FIXED     = 0,
    PMAP_VAR_SW1   = 1,
    PMAP_VAR_SW2   = 2
};

// ---------------------------------------------------------------------------
// ORL 测量方法 (对应 :MEASure:ORL? <method>)
// ---------------------------------------------------------------------------
enum ORLMethod
{
    ORL_INTEGRATION = 1,
    ORL_DISCRETE    = 2
};

// ---------------------------------------------------------------------------
// ORL 标记原点 (对应 :MEASure:ORL? <origin>)
// ---------------------------------------------------------------------------
enum ORLOrigin
{
    ORL_ORIGIN_DUT_START   = 1,
    ORL_ORIGIN_DUT_END     = 2,
    ORL_ORIGIN_A_START_B_END = 3
};

// ---------------------------------------------------------------------------
// ORL 预设 (对应 :MEASure:ORL:SETup <zone>,<preset>)
// ---------------------------------------------------------------------------
enum ORLPreset
{
    ORL_PRESET_DISABLE   = 0,
    ORL_PRESET_INPUT     = 1,
    ORL_PRESET_OUTPUT    = 2,
    ORL_PRESET_INSIDE    = 3
};

// ---------------------------------------------------------------------------
// 超级应用状态 (对应 :SUPer:STATus?)
// ---------------------------------------------------------------------------
enum SuperAppStatus
{
    SUPER_NOT_RUNNING = 0,
    SUPER_RUNNING     = 1,
    SUPER_INVALID     = 2
};

// ---------------------------------------------------------------------------
// 设备标识信息 (从 *IDN? 解析)
// ---------------------------------------------------------------------------
struct PCT4ALL_API IdentificationInfo
{
    std::string manufacturer;
    std::string platform;
    std::string serialNumber;
    std::string firmwareVersion;

    IdentificationInfo() {}
};

// ---------------------------------------------------------------------------
// 模块信息 (从 :INFOrmation? 解析)
// ---------------------------------------------------------------------------
struct PCT4ALL_API CassetteInfo
{
    std::string serialNumber;
    std::string partNumber;
    std::string firmwareVersion;
    std::string moduleVersion1;
    std::string moduleVersion2;
    std::string hardwareVersion;
    std::string assemblyDate;
    std::string description;

    CassetteInfo() {}
};

// ---------------------------------------------------------------------------
// 错误信息 (从 :SYSTem:ERRor? 解析)
// ---------------------------------------------------------------------------
struct PCT4ALL_API ErrorInfo
{
    int code;
    std::string message;

    ErrorInfo() : code(0) {}
    ErrorInfo(int c, const std::string& msg) : code(c), message(msg) {}
    bool IsOK() const { return code == 0; }
};

// ---------------------------------------------------------------------------
// PCT 模块配置 (从 :CONFig? 解析)
// ---------------------------------------------------------------------------
struct PCT4ALL_API PCTConfig
{
    int         deviceNum;
    std::string deviceType;
    std::string fiberType;
    int         outputs;
    int         sw1Channels;
    int         sw2Channels;
    std::vector<int> wavelengths;
    int         coreType;
    int         opmCount;

    PCTConfig()
        : deviceNum(1), outputs(1)
        , sw1Channels(1), sw2Channels(1)
        , coreType(0), opmCount(0)
    {}
};

// ---------------------------------------------------------------------------
// 连接配置
// ---------------------------------------------------------------------------
struct PCT4ALL_API ConnectionConfig
{
    std::string address;
    int    port;
    double timeout;
    int    bufferSize;
    int    reconnectAttempts;
    double reconnectDelay;

    ConnectionConfig()
        : port(8301), timeout(5.0), bufferSize(4096)
        , reconnectAttempts(3), reconnectDelay(2.0)
    {}
};

// ---------------------------------------------------------------------------
// 单通道/单波长测量结果
// ---------------------------------------------------------------------------
struct PCT4ALL_API MeasurementResult
{
    int    channel;
    double wavelength;
    double insertionLoss;
    double returnLoss;
    double returnLossZone1;
    double returnLossZone2;
    double dutLength;
    double power;
    double ilOffset;
    double rawData[10];
    int    rawDataCount;

    MeasurementResult()
        : channel(0), wavelength(0.0)
        , insertionLoss(0.0), returnLoss(0.0)
        , returnLossZone1(0.0), returnLossZone2(0.0)
        , dutLength(0.0), power(0.0), ilOffset(0.0)
        , rawDataCount(0)
    {
        memset(rawData, 0, sizeof(rawData));
    }
};

// ---------------------------------------------------------------------------
// ORL Zone 设置
// ---------------------------------------------------------------------------
struct PCT4ALL_API ORLZoneSetup
{
    int    zone;
    int    preset;
    int    method;
    int    origin;
    double aOffset;
    double bOffset;
    bool   isPresetMode;

    ORLZoneSetup()
        : zone(1), preset(0), method(1), origin(1)
        , aOffset(0.0), bOffset(0.0), isPresetMode(true)
    {}
};

// ---------------------------------------------------------------------------
// 系统信息 (从 :SYSTem:INFO? 解析)
// ---------------------------------------------------------------------------
struct PCT4ALL_API SystemInfo
{
    std::string mainframePart;
    std::string mainframeSerial;
    std::string mainframeHW;
    std::string mainframeDate;
    std::string controllerPart;
    std::string controllerSerial;
    std::string controllerHW;
    std::string controllerDate;
    std::string raw;

    SystemInfo() {}
};

// ---------------------------------------------------------------------------
// C 兼容导出结构体
// ---------------------------------------------------------------------------
struct PCT4ALL_API CIdentificationInfo
{
    char manufacturer[64];
    char platform[64];
    char serialNumber[64];
    char firmwareVersion[64];
};

struct PCT4ALL_API CCassetteInfo
{
    char serialNumber[64];
    char partNumber[64];
    char firmwareVersion[64];
    char hardwareVersion[64];
    char assemblyDate[32];
    char description[128];
};

struct PCT4ALL_API CMeasurementResult
{
    int    channel;
    double wavelength;
    double insertionLoss;
    double returnLoss;
    double returnLossZone1;
    double returnLossZone2;
    double dutLength;
    double power;
    double ilOffset;
    double rawData[10];
    int    rawDataCount;
};

struct PCT4ALL_API CConnectionConfig
{
    char   address[64];
    int    port;
    double timeout;
    int    bufferSize;
    int    reconnectAttempts;
    double reconnectDelay;
};

} // namespace ViaviPCT4All
