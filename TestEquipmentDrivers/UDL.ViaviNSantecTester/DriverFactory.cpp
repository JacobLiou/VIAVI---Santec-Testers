#include "stdafx.h"
#include "DriverFactory.h"
#include "ViaviPCTDriver.h"
#include "SantecDriver.h"
#include <algorithm>
#include <stdexcept>

namespace ViaviNSantecTester {

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

    // 去除空白字符
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

IEquipmentDriver* CDriverFactory::Create(const std::string& equipmentType,
                                          const std::string& address,
                                          int port,
                                          int slot,
                                          CommType commType)
{
    std::string key = ToLower(equipmentType);
    while (!key.empty() && key.front() == ' ') key.erase(key.begin());
    while (!key.empty() && key.back() == ' ') key.pop_back();

    if (key == "viavi" || key == "viavi_pct" || key == "map300")
    {
        return new CViaviPCTDriver(address, slot);
    }
    else if (key == "santec")
    {
        int santecPort = (port > 0) ? port : CSantecDriver::DEFAULT_PORT;
        return new CSantecDriver(address, santecPort, 5.0, commType);
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

} // namespace ViaviNSantecTester
