#pragma once

#include "PCTTypes.h"
#include "IViaviPCTDriver.h"
#include <string>
#include <vector>

namespace ViaviPCT {

class PCT_API CPCTDriverFactory
{
public:
    // 根据设备类型字符串创建驱动
    // 支持的类型: "viavi", "pct", "morl"
    static IViaviPCTDriver* Create(const std::string& equipmentType,
                                    const std::string& ipAddress,
                                    int port = 0,
                                    int slot = 1);

    // 扩展版本：支持指定通信类型
    // address: TCP 模式为 IP 地址，VISA 模式为 VISA 资源字符串
    static IViaviPCTDriver* Create(const std::string& equipmentType,
                                    const std::string& address,
                                    int port,
                                    int slot,
                                    CommType commType);

    // 销毁由 Create() 创建的驱动
    static void Destroy(IViaviPCTDriver* driver);

    // 列出支持的类型名称
    static std::vector<std::string> SupportedTypes();
};

} // namespace ViaviPCT
