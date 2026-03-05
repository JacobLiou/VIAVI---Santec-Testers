#pragma once

#include "DriverTypes.h"
#include "IEquipmentDriver.h"
#include <string>
#include <map>

namespace ViaviNSantecTester {

class DRIVER_API CDriverFactory
{
public:
    // 根据设备类型字符串创建驱动
    // 支持的类型: "viavi", "viavi_pct", "map300", "santec"
    static IEquipmentDriver* Create(const std::string& equipmentType,
                                    const std::string& ipAddress,
                                    int port = 0,
                                    int slot = 1);

    // 销毁由 Create() 创建的驱动
    static void Destroy(IEquipmentDriver* driver);

    // 列出支持的类型名称
    static std::vector<std::string> SupportedTypes();
};

} // namespace ViaviNSantecTester
