#include "stdafx.h"
#include "PTMDriverFactory.h"
#include "SantecPTMDriver.h"
#include <algorithm>
#include <stdexcept>

namespace SantecPTM {

static std::string ToLower(const std::string& s)
{
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

static std::string TrimKey(std::string key)
{
    while (!key.empty() && key.front() == ' ') key.erase(key.begin());
    while (!key.empty() && key.back() == ' ') key.pop_back();
    return key;
}

ISantecPTMDriver* CPTMDriverFactory::Create(const std::string& equipmentType,
                                            const std::string& ipAddress,
                                            int port,
                                            int slot)
{
    (void)slot;
    return Create(equipmentType, ipAddress, port, slot, COMM_TCP);
}

ISantecPTMDriver* CPTMDriverFactory::Create(const std::string& equipmentType,
                                            const std::string& address,
                                            int port,
                                            int slot,
                                            CommType commType)
{
    (void)slot;
    std::string key = TrimKey(ToLower(equipmentType));

    if (key == "ptm" || key == "santecptm")
    {
        int ptmPort = (port > 0) ? port : CSantecPTMDriver::DEFAULT_PORT;
        return new CSantecPTMDriver(address, ptmPort, 5.0, commType);
    }

    throw std::invalid_argument(
        std::string("Unsupported equipment type: '") + equipmentType
        + "'. Supported: ptm, santecptm");
}

void CPTMDriverFactory::Destroy(ISantecPTMDriver* driver)
{
    if (driver)
    {
        driver->Disconnect();
        delete driver;
    }
}

std::vector<std::string> CPTMDriverFactory::SupportedTypes()
{
    std::vector<std::string> types;
    types.push_back("ptm");
    types.push_back("santecptm");
    return types;
}

} // namespace SantecPTM
