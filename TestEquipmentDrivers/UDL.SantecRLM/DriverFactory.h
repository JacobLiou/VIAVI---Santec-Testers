#pragma once

#include "DriverTypes.h"
#include "IEquipmentDriver.h"
#include <string>
#include <map>

namespace SantecRLM {

class DRIVER_API CDriverFactory
{
public:
    // 根据设备类型字符串创建驱动
    // 支持的类型: "santec", "rlm"
    static IEquipmentDriver* Create(const std::string& equipmentType,
                                    const std::string& ipAddress,
                                    int port = 0,
                                    int slot = 1);

    // 扩展版本：支持指定通信类型
    // address: TCP 模式为 IP 地址，USB 模式为 VISA 资源字符串
    static IEquipmentDriver* Create(const std::string& equipmentType,
                                    const std::string& address,
                                    int port,
                                    int slot,
                                    CommType commType);

    // 销毁由 Create() 创建的驱动
    static void Destroy(IEquipmentDriver* driver);

    // 列出支持的类型名称
    static std::vector<std::string> SupportedTypes();
};

} // namespace SantecRLM
