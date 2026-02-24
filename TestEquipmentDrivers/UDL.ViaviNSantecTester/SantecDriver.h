#pragma once

#include "BaseEquipmentDriver.h"

namespace ViaviNSantecTester {

// ---------------------------------------------------------------------------
// Communication adapter interface for Santec
// Allows swapping between TCP, GPIB, USB, or DLL-based communication
// once the Santec protocol documentation is received.
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
// Santec Driver
//
// STATUS: SKELETON - Awaiting protocol documentation from supplier
//
// Known: 2 Santec IL/RL test units at Guad factory
// Unknown: Communication protocol, command set, data format
// ---------------------------------------------------------------------------
class DRIVER_API CSantecDriver : public CBaseEquipmentDriver
{
public:
    static const int DEFAULT_PORT       = 5000; // TODO: Confirm from documentation
    static const int POLL_INTERVAL_MS   = 100;
    static const int MAX_POLL_TIME_MS   = 300000;

    CSantecDriver(const std::string& ipAddress,
                  int port = DEFAULT_PORT,
                  double timeout = 3.0,
                  CommType commType = COMM_TCP);
    virtual ~CSantecDriver();

    // IEquipmentDriver overrides (all skeleton/placeholder)
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

protected:
    virtual bool ValidateConnection() override;

private:
    bool WaitForCompletion(const std::string& operationName);

    CommType                m_commType;
    ISantecCommAdapter*     m_adapter;
    bool                    m_ownsAdapter;
    std::vector<double>     m_wavelengths;
    std::vector<int>        m_channels;
};

} // namespace ViaviNSantecTester
