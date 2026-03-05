#pragma once

#include "DriverTypes.h"
#include <vector>
#include <string>

namespace ViaviNSantecTester {

class DRIVER_API IEquipmentDriver
{
public:
    virtual ~IEquipmentDriver() {}

    // 连接生命周期
    virtual bool Connect() = 0;
    virtual void Disconnect() = 0;
    virtual bool Reconnect() = 0;
    virtual bool IsConnected() const = 0;

    // 连接后初始化
    virtual bool Initialize() = 0;

    // 设备信息
    virtual DeviceInfo GetDeviceInfo() = 0;

    // 错误查询
    virtual ErrorInfo CheckError() = 0;

    // 配置
    virtual void ConfigureWavelengths(const std::vector<double>& wavelengths) = 0;
    virtual void ConfigureChannels(const std::vector<int>& channels) = 0;

    // 测量操作
    virtual bool TakeReference(const ReferenceConfig& config) = 0;
    virtual bool TakeMeasurement() = 0;
    virtual std::vector<MeasurementResult> GetResults() = 0;

    // 高级工作流: 配置 -> 参考 -> 测量 -> 结果
    virtual std::vector<MeasurementResult> RunFullTest(
        const std::vector<double>& wavelengths,
        const std::vector<int>& channels,
        bool doReference,
        const ReferenceConfig& refConfig) = 0;
};

} // namespace ViaviNSantecTester
