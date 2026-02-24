#pragma once

#include "BaseEquipmentDriver.h"

namespace ViaviNSantecTester {

// ---------------------------------------------------------------------------
// Communication adapter interface for Santec
// Allows swapping between TCP, GPIB, USB, or DLL-based communication.
// ---------------------------------------------------------------------------
class DRIVER_API ISantecCommAdapter
{
public:
    virtual ~ISantecCommAdapter() {}
    virtual bool Open(const std::string& address, int port, double timeout) = 0;
    virtual void Close() = 0;
    virtual bool IsOpen() const = 0;
    virtual std::string SendQuery(const std::string& command) = 0;
    virtual void SendWrite(const std::string& command) = 0;
};

// Default TCP adapter (can be replaced with GPIB/USB/DLL adapters later)
class DRIVER_API CSantecTcpAdapter : public ISantecCommAdapter
{
public:
    CSantecTcpAdapter();
    virtual ~CSantecTcpAdapter();

    virtual bool Open(const std::string& address, int port, double timeout) override;
    virtual void Close() override;
    virtual bool IsOpen() const override;
    virtual std::string SendQuery(const std::string& command) override;
    virtual void SendWrite(const std::string& command) override;

private:
    SOCKET m_socket;
    int    m_bufferSize;
};


// ---------------------------------------------------------------------------
// Santec Driver for RLM-100 (Return Loss Meter) and ILM-100 (Insertion Loss Meter)
//
// Supports both devices through the same SCPI command set over TCP/IP.
// The device model is auto-detected from the *IDN? response.
//
// SCPI Command Reference:
//   Santec Programming Guide v1.23 & RLM-100 Sample Code
//   Download from: https://inst.santec.com/resources/programming
//
// NOTE: SCPI commands use standard IEEE 488.2 / SCPI 1999.0 patterns.
//       If your device firmware uses different mnemonics, update the
//       constants in the SCPI_CMD namespace at the top of SantecDriver.cpp.
// ---------------------------------------------------------------------------
class DRIVER_API CSantecDriver : public CBaseEquipmentDriver
{
public:
    // Default SCPI-RAW port per LXI standard; override if device differs
    static const int DEFAULT_PORT       = 5025;
    static const int POLL_INTERVAL_MS   = 100;
    static const int MAX_POLL_TIME_MS   = 300000;   // 5 minutes

    enum SantecModel
    {
        MODEL_UNKNOWN = 0,
        MODEL_RLM_100,      // Return Loss Meter (IL+RL)
        MODEL_ILM_100,      // Insertion Loss Meter (IL only)
        MODEL_BRM_100       // Backreflection Meter
    };

    enum MeasSpeed
    {
        SPEED_STANDARD = 0,
        SPEED_FAST     = 1
    };

    CSantecDriver(const std::string& ipAddress,
                  int port = DEFAULT_PORT,
                  double timeout = 5.0,
                  CommType commType = COMM_TCP);
    virtual ~CSantecDriver();

    // IEquipmentDriver overrides
    virtual bool Connect() override;
    virtual void Disconnect() override;
    virtual bool IsConnected() const override;
    virtual bool Initialize() override;
    virtual DeviceInfo GetDeviceInfo() override;
    virtual ErrorInfo CheckError() override;
    virtual void ConfigureWavelengths(const std::vector<double>& wavelengths) override;
    virtual void ConfigureChannels(const std::vector<int>& channels) override;
    virtual bool TakeReference(const ReferenceConfig& config) override;
    virtual bool TakeMeasurement() override;
    virtual std::vector<MeasurementResult> GetResults() override;

    // Set a custom communication adapter (for GPIB/USB/DLL support)
    void SetCommAdapter(ISantecCommAdapter* adapter, bool takeOwnership = true);

    // Santec-specific accessors
    SantecModel GetModel() const { return m_model; }
    void SetMeasurementSpeed(MeasSpeed speed) { m_speed = speed; }
    MeasSpeed GetMeasurementSpeed() const { return m_speed; }

protected:
    virtual bool ValidateConnection() override;

private:
    // Unified command interface: routes to adapter or base socket
    std::string Query(const std::string& command);
    void        Write(const std::string& command);

    // Helpers
    bool WaitForCompletion(const std::string& operationName);
    void ParseIdentity(const std::string& idnResponse);
    static std::vector<double> ParseDoubleList(const std::string& csv);
    static std::string Trim(const std::string& s);

    CommType                m_commType;
    ISantecCommAdapter*     m_adapter;
    bool                    m_ownsAdapter;
    SantecModel             m_model;
    MeasSpeed               m_speed;
    DeviceInfo              m_deviceInfo;
    std::vector<double>     m_wavelengths;
    std::vector<int>        m_channels;
    bool                    m_referenced;       // true after successful reference
};

} // namespace ViaviNSantecTester
