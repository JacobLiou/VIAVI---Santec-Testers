#include "stdafx.h"
#include "PCT4AllDriverFactory.h"
#include "CViaviPCT4AllDriver.h"
#include <algorithm>
#include <stdexcept>

namespace ViaviPCT4All {

static std::string ToLower(const std::string& s)
{
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

IViaviPCT4AllDriver* CPCT4AllDriverFactory::Create(const std::string& equipmentType,
                                                    const std::string& ipAddress,
                                                    int port,
                                                    int slot)
{
    std::string key = ToLower(equipmentType);
    while (!key.empty() && key.front() == ' ') key.erase(key.begin());
    while (!key.empty() && key.back() == ' ') key.pop_back();

    if (key == "viavi" || key == "pct" || key == "morl" || key == "pct4all" || key == "map")
    {
        int pctPort = (port > 0) ? port : CViaviPCT4AllDriver::DEFAULT_PORT;
        return new CViaviPCT4AllDriver(ipAddress, pctPort);
    }
    else
    {
        throw std::invalid_argument(
            std::string("Unsupported equipment type: '") + equipmentType
            + "'. Supported: viavi, pct, morl, pct4all, map");
    }
}

IViaviPCT4AllDriver* CPCT4AllDriverFactory::Create(const std::string& equipmentType,
                                                    const std::string& address,
                                                    int port,
                                                    int slot,
                                                    CommType commType)
{
    std::string key = ToLower(equipmentType);
    while (!key.empty() && key.front() == ' ') key.erase(key.begin());
    while (!key.empty() && key.back() == ' ') key.pop_back();

    if (key == "viavi" || key == "pct" || key == "morl" || key == "pct4all" || key == "map")
    {
        int pctPort = (port > 0) ? port : CViaviPCT4AllDriver::DEFAULT_PORT;
        return new CViaviPCT4AllDriver(address, pctPort, 5.0, commType);
    }
    else
    {
        throw std::invalid_argument(
            std::string("Unsupported equipment type: '") + equipmentType
            + "'. Supported: viavi, pct, morl, pct4all, map");
    }
}

void CPCT4AllDriverFactory::Destroy(IViaviPCT4AllDriver* driver)
{
    if (driver)
    {
        driver->Disconnect();
        delete driver;
    }
}

std::vector<std::string> CPCT4AllDriverFactory::SupportedTypes()
{
    std::vector<std::string> types;
    types.push_back("viavi");
    types.push_back("pct");
    types.push_back("morl");
    types.push_back("pct4all");
    types.push_back("map");
    return types;
}

} // namespace ViaviPCT4All
