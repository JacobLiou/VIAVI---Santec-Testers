#pragma once

#ifdef UDLVIAVINSANTECTESTER_EXPORTS
#define DRIVER_API __declspec(dllexport)
#else
#define DRIVER_API __declspec(dllimport)
#endif

#include <string>
#include <vector>

namespace ViaviNSantecTester {

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
// Measurement Mode (VIAVI: SENS:FUNC 0=reference, 1=measurement)
// ---------------------------------------------------------------------------
enum MeasurementMode
{
    MODE_REFERENCE = 0,
    MODE_MEASUREMENT = 1
};

// ---------------------------------------------------------------------------
// Measurement State (VIAVI: MEAS:STATE? 1=complete, 2=running, 3=error)
// ---------------------------------------------------------------------------
enum MeasurementState
{
    MEAS_IDLE = 0,
    MEAS_RUNNING = 2,
    MEAS_COMPLETE = 1,
    MEAS_ERROR = 3
};

// ---------------------------------------------------------------------------
// ORL Method (VIAVI: MEAS:ORL:SETUP method parameter)
// ---------------------------------------------------------------------------
enum ORLMethod
{
    ORL_INTEGRATION = 1,
    ORL_DISCRETE = 2
};

// ---------------------------------------------------------------------------
// ORL Origin (VIAVI: MEAS:ORL:SETUP origin parameter)
// ---------------------------------------------------------------------------
enum ORLOrigin
{
    ORL_ORIGIN_AB_START = 1,    // A+B anchored to DUT start
    ORL_ORIGIN_AB_END = 2,      // A+B anchored to DUT end
    ORL_ORIGIN_A_START_B_END = 3 // A from start, B from end
};

// ---------------------------------------------------------------------------
// Communication type for future Santec adaptation
// ---------------------------------------------------------------------------
enum CommType
{
    COMM_TCP = 0,
    COMM_GPIB,
    COMM_USB,
    COMM_DLL
};

// ---------------------------------------------------------------------------
// Single channel / single wavelength measurement result
// ---------------------------------------------------------------------------
struct DRIVER_API MeasurementResult
{
    int    channel;
    double wavelength;          // nm
    double insertionLoss;       // dB (IL)
    double returnLoss;          // dB (RL) - kept for backward compat / VIAVI
    double returnLossA;         // dB (RLA) - Santec RL1: connector RL at position A
    double returnLossB;         // dB (RLB) - Santec RL1: connector RL at position B
    double returnLossTotal;     // dB (RLTOTAL) - Santec RL1: total ORL
    double dutLength;           // meters - Santec RL1: DUT length from READ:RL?
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
// Connection configuration
// ---------------------------------------------------------------------------
struct DRIVER_API ConnectionConfig
{
    std::string ipAddress;
    int    port;
    double timeout;             // seconds
    int    bufferSize;
    int    reconnectAttempts;
    double reconnectDelay;      // seconds

    ConnectionConfig()
        : port(0), timeout(3.0), bufferSize(1024)
        , reconnectAttempts(3), reconnectDelay(2.0)
    {}
};

// ---------------------------------------------------------------------------
// Device identification info
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
// Error info returned by CheckError()
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
// ORL configuration parameters
// ---------------------------------------------------------------------------
struct DRIVER_API ORLConfig
{
    int channel;
    ORLMethod method;
    ORLOrigin origin;
    double aOffset;             // meters
    double bOffset;             // meters

    ORLConfig()
        : channel(1)
        , method(ORL_DISCRETE)
        , origin(ORL_ORIGIN_AB_START)
        , aOffset(-0.5)
        , bOffset(0.5)
    {}
};

// ---------------------------------------------------------------------------
// Reference override parameters
// ---------------------------------------------------------------------------
struct DRIVER_API ReferenceConfig
{
    bool   useOverride;
    double ilValue;             // dB
    double lengthValue;         // meters

    ReferenceConfig()
        : useOverride(false), ilValue(0.1), lengthValue(3.0)
    {}
};

// ---------------------------------------------------------------------------
// C-compatible structures for DLL export interface
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

} // namespace ViaviNSantecTester
