#pragma once

#include "BaseEquipmentDriver.h"

namespace ViaviNSantecTester {

class DRIVER_API CViaviPCTDriver : public CBaseEquipmentDriver
{
public:
    static const int CHASSIS_PORT       = 8100;
    static const int PCT_BASE_PORT      = 8300;
    static const int SUPER_APP_WAIT_MS  = 10000;    // 10秒
    static const int POLL_INTERVAL_MS   = 100;
    static const int MAX_POLL_TIME_MS   = 300000;   // 5分钟

    CViaviPCTDriver(const std::string& ipAddress,
                    int slot = 1,
                    double timeout = 3.0,
                    int launchPort = 1);
    virtual ~CViaviPCTDriver();

    // IEquipmentDriver 覆盖
    virtual bool Connect() override;
    virtual void Disconnect() override;
    virtual bool Initialize() override;
    virtual DeviceInfo GetDeviceInfo() override;
    virtual ErrorInfo CheckError() override;
    virtual void ConfigureWavelengths(const std::vector<double>& wavelengths) override;
    virtual void ConfigureChannels(const std::vector<int>& channels) override;
    virtual bool TakeReference(const ReferenceConfig& config) override;
    virtual bool TakeMeasurement() override;
    virtual std::vector<MeasurementResult> GetResults() override;

    // VIAVI 特定配置
    void ConfigureORL(const ORLConfig& config);

protected:
    virtual bool ValidateConnection() override;

private:
    // 机箱通信
    std::string SendChassisCommand(const std::string& command);

    // 测量辅助函数
    bool WaitForCompletion(const std::string& operationName);
    bool OverrideReference(double ilValue, double lengthValue);
    bool AutoReference();

    // 通道字符串构建器: [1,2,...12] -> "1-12"
    static std::string BuildChannelString(const std::vector<int>& channels);

    // 解析逗号分隔的双精度浮点值
    static std::vector<double> ParseDoubleList(const std::string& csv);

    SOCKET              m_chassisSocket;
    std::string         m_ipAddress;
    int                 m_slot;
    int                 m_launchPort;
    std::vector<double> m_wavelengths;
    std::vector<int>    m_channels;
};

} // namespace ViaviNSantecTester
