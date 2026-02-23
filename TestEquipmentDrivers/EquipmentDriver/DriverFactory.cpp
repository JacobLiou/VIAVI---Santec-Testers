#include "stdafx.h"
#include "DriverFactory.h"
#include "ViaviPCTDriver.h"
#include "SantecDriver.h"
#include <algorithm>
#include <stdexcept>

namespace EquipmentDriver {

static std::string ToLower(const std::string& s)
{
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

IEquipmentDriver* CDriverFactory::Create(const std::string& equipmentType,
                                          const std::string& ipAddress,
                                          int port,
                                          int slot)
{
    std::string key = ToLower(equipmentType);

    // Trim whitespace
    while (!key.empty() && key.front() == ' ') key.erase(key.begin());
    while (!key.empty() && key.back() == ' ') key.pop_back();

    if (key == "viavi" || key == "viavi_pct" || key == "map300")
    {
        return new CViaviPCTDriver(ipAddress, slot);
    }
    else if (key == "santec")
    {
        int santecPort = (port > 0) ? port : CSantecDriver::DEFAULT_PORT;
        return new CSantecDriver(ipAddress, santecPort);
    }
    else
    {
        throw std::invalid_argument(
            std::string("Unsupported equipment type: '") + equipmentType
            + "'. Supported: viavi, viavi_pct, map300, santec");
    }
}

void CDriverFactory::Destroy(IEquipmentDriver* driver)
{
    if (driver)
    {
        driver->Disconnect();
        delete driver;
    }
}

std::vector<std::string> CDriverFactory::SupportedTypes()
{
    std::vector<std::string> types;
    types.push_back("viavi");
    types.push_back("viavi_pct");
    types.push_back("map300");
    types.push_back("santec");
    return types;
}

} // namespace EquipmentDriver
