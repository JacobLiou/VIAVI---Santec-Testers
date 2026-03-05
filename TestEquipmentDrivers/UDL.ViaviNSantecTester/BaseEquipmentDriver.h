#pragma once

#include "IEquipmentDriver.h"
#include "Logger.h"
#include <string>
#include <vector>

namespace ViaviNSantecTester {

class DRIVER_API CBaseEquipmentDriver : public IEquipmentDriver
{
public:
    CBaseEquipmentDriver(const ConnectionConfig& config, const std::string& deviceType);
    virtual ~CBaseEquipmentDriver();

    // IEquipmentDriver - 连接生命周期
    virtual bool Connect() override;
    virtual void Disconnect() override;
    virtual bool Reconnect() override;
    virtual bool IsConnected() const override;

    // 主动健康检查：验证套接字是否仍然活跃
    bool IsAlive() const;

    // IEquipmentDriver - 高级工作流（模板方法）
    virtual std::vector<MeasurementResult> RunFullTest(
        const std::vector<double>& wavelengths,
        const std::vector<int>& channels,
        bool doReference,
        const ReferenceConfig& refConfig) override;

    // 底层命令接口（发送失败时自动重连一次）
    std::string SendCommand(const std::string& command,
                            const std::string& endChar = "\n",
                            bool expectResponse = false);

    // 错误检查辅助函数
    void AssertNoError();

    // 访问器
    ConnectionState GetState() const { return m_state; }
    const ConnectionConfig& GetConfig() const { return m_config; }
    CLogger& GetLogger() { return m_logger; }

protected:
    // 通过 select() 实现可配置超时的非阻塞 TCP 连接。
    // 适用于任意套接字；子类可为辅助套接字调用此方法。
    bool ConnectSocket(SOCKET sock, const sockaddr_in& addr, double timeoutSec);

    // 启用 TCP 保活，使用快速探测间隔以尽早检测对端断开。
    void EnableKeepAlive(SOCKET sock);

    // TCP 握手成功后调用。在子类中覆盖以发送
    // 设备特定的探测命令（如 *IDN?）并验证响应。
    // 默认实现：检查套接字是否有待处理的错误。
    virtual bool ValidateConnection();

    std::string ReceiveResponse(int bufferSize);
    void CleanupSocket();

    SOCKET          m_socket;
    ConnectionConfig m_config;
    ConnectionState m_state;
    std::string     m_deviceType;
    CLogger         m_logger;
};

} // namespace ViaviNSantecTester
