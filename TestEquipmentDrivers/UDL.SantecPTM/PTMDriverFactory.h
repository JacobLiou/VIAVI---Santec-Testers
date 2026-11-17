#pragma once

#include "PTMTypes.h"
#include "ISantecPTMDriver.h"
#include <string>
#include <vector>

namespace SantecPTM {

class PTM_API CPTMDriverFactory
{
public:
    // type: "ptm" / "santecptm"
    static ISantecPTMDriver* Create(const std::string& equipmentType,
                                    const std::string& ipAddress,
                                    int port = 0,
                                    int slot = 0);

    static ISantecPTMDriver* Create(const std::string& equipmentType,
                                    const std::string& address,
                                    int port,
                                    int slot,
                                    CommType commType);

    static void Destroy(ISantecPTMDriver* driver);
    static std::vector<std::string> SupportedTypes();
};

} // namespace SantecPTM
