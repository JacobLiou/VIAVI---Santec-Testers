#include "stdafx.h"
#include "PCTDriverFactory.h"
#include "CViaviPCTDriver.h"
#include <algorithm>
#include <stdexcept>

namespace ViaviPCT {

static std::string ToLower(const std::string& s)
{
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

IViaviPCTDriver* CPCTDriverFactory::Create(const std::string& equipmentType,
                                            const std::string& ipAddress,
                                            int port,
                                            int slot)
{
    std::string key = ToLower(equipmentType);
    while (!key.empty() && key.front() == ' ') key.erase(key.begin());
    while (!key.empty() && key.back() == ' ') key.pop_back();

    if (key == "viavi" || key == "pct" || key == "morl")
    {
        int pctPort = (port > 0) ? port : CViaviPCTDriver::DEFAULT_PORT;
        return new CViaviPCTDriver(ipAddress, pctPort);
    }
    else
    {
        throw std::invalid_argument(
            std::string("不支持的设备类型: '") + equipmentType
            + "'。支持: viavi, pct, morl");
    }
}

IViaviPCTDriver* CPCTDriverFactory::Create(const std::string& equipmentType,
                                            const std::string& address,
                                            int port,
                                            int slot,
                                            CommType commType)
{
    std::string key = ToLower(equipmentType);
    while (!key.empty() && key.front() == ' ') key.erase(key.begin());
    while (!key.empty() && key.back() == ' ') key.pop_back();

    if (key == "viavi" || key == "pct" || key == "morl")
    {
        int pctPort = (port > 0) ? port : CViaviPCTDriver::DEFAULT_PORT;
        return new CViaviPCTDriver(address, pctPort, 5.0, commType);
    }
    else
    {
        throw std::invalid_argument(
            std::string("不支持的设备类型: '") + equipmentType
            + "'。支持: viavi, pct, morl");
    }
}

void CPCTDriverFactory::Destroy(IViaviPCTDriver* driver)
{
    if (driver)
    {
        driver->Disconnect();
        delete driver;
    }
}

std::vector<std::string> CPCTDriverFactory::SupportedTypes()
{
    std::vector<std::string> types;
    types.push_back("viavi");
    types.push_back("pct");
    types.push_back("morl");
    return types;
}

} // namespace ViaviPCT
