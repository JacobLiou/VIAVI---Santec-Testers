#pragma once

#include "DriverTypes.h"
#include <vector>
#include <string>

namespace EquipmentDriver {

class DRIVER_API IEquipmentDriver
{
public:
    virtual ~IEquipmentDriver() {}

    // Connection lifecycle
    virtual bool Connect() = 0;
    virtual void Disconnect() = 0;
    virtual bool Reconnect() = 0;
    virtual bool IsConnected() const = 0;

    // Initialization after connection
    virtual bool Initialize() = 0;

    // Device information
    virtual DeviceInfo GetDeviceInfo() = 0;

    // Error query
    virtual ErrorInfo CheckError() = 0;

    // Configuration
    virtual void ConfigureWavelengths(const std::vector<double>& wavelengths) = 0;
    virtual void ConfigureChannels(const std::vector<int>& channels) = 0;

    // Measurement operations
    virtual bool TakeReference(const ReferenceConfig& config) = 0;
    virtual bool TakeMeasurement() = 0;
    virtual std::vector<MeasurementResult> GetResults() = 0;

    // High-level workflow: configure -> reference -> measure -> results
    virtual std::vector<MeasurementResult> RunFullTest(
        const std::vector<double>& wavelengths,
        const std::vector<int>& channels,
        bool doReference,
        const ReferenceConfig& refConfig) = 0;
};

} // namespace EquipmentDriver
