#pragma once

#include "PCT4AllTypes.h"
#include "IViaviPCT4AllDriver.h"
#include <string>
#include <vector>

namespace ViaviPCT4All {

class PCT4ALL_API CPCT4AllDriverFactory
{
public:
    static IViaviPCT4AllDriver* Create(const std::string& equipmentType,
                                        const std::string& ipAddress,
                                        int port = 0,
                                        int slot = 1);

    static IViaviPCT4AllDriver* Create(const std::string& equipmentType,
                                        const std::string& address,
                                        int port,
                                        int slot,
                                        CommType commType);

    static void Destroy(IViaviPCT4AllDriver* driver);

    static std::vector<std::string> SupportedTypes();
};

} // namespace ViaviPCT4All
