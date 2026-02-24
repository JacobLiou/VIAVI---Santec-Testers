#pragma once

#include "DriverTypes.h"
#include "IEquipmentDriver.h"
#include <string>
#include <map>

namespace ViaviNSantecTester {

class DRIVER_API CDriverFactory
{
public:
    // Create a driver by equipment type string
    // Supported types: "viavi", "viavi_pct", "map300", "santec"
    static IEquipmentDriver* Create(const std::string& equipmentType,
                                    const std::string& ipAddress,
                                    int port = 0,
                                    int slot = 1);

    // Destroy a driver created by Create()
    static void Destroy(IEquipmentDriver* driver);

    // List supported type names
    static std::vector<std::string> SupportedTypes();
};

} // namespace ViaviNSantecTester
