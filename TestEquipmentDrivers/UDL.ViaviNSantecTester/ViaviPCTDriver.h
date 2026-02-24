#pragma once

#include "BaseEquipmentDriver.h"

namespace ViaviNSantecTester {

class DRIVER_API CViaviPCTDriver : public CBaseEquipmentDriver
{
public:
    static const int CHASSIS_PORT       = 8100;
    static const int PCT_BASE_PORT      = 8300;
    static const int SUPER_APP_WAIT_MS  = 10000;    // 10 seconds
    static const int POLL_INTERVAL_MS   = 100;
    static const int MAX_POLL_TIME_MS   = 300000;   // 5 minutes

    CViaviPCTDriver(const std::string& ipAddress,
                    int slot = 1,
                    double timeout = 3.0,
                    int launchPort = 1);
    virtual ~CViaviPCTDriver();

    // IEquipmentDriver overrides
    virtual bool Connect() override;
    virtual void Disconnect() override;
    virtual bool Initialize() override;
    virtual DeviceInfo GetDeviceInfo() override;
    virtual ErrorInfo CheckError() override;
    virtual void ConfigureWavelengths(const std::vector<double>& wavelengths) override;
    virtual void ConfigureChannels(const std::vector<int>& channels) override;
    virtual bool TakeReference(const ReferenceConfig& config) override;
    virtual bool TakeMeasurement() override;
    virtual std::vector<MeasurementResult> GetResults() override;

    // VIAVI-specific configuration
    void ConfigureORL(const ORLConfig& config);

protected:
    virtual bool ValidateConnection() override;

private:
    // Chassis communication
    std::string SendChassisCommand(const std::string& command);

    // Measurement helpers
    bool WaitForCompletion(const std::string& operationName);
    bool OverrideReference(double ilValue, double lengthValue);
    bool AutoReference();

    // Channel string builder: [1,2,...12] -> "1-12"
    static std::string BuildChannelString(const std::vector<int>& channels);

    // Parse comma-separated double values
    static std::vector<double> ParseDoubleList(const std::string& csv);

    SOCKET              m_chassisSocket;
    std::string         m_ipAddress;
    int                 m_slot;
    int                 m_launchPort;
    std::vector<double> m_wavelengths;
    std::vector<int>    m_channels;
};

} // namespace ViaviNSantecTester
