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

    // IEquipmentDriver - connection lifecycle
    virtual bool Connect() override;
    virtual void Disconnect() override;
    virtual bool Reconnect() override;
    virtual bool IsConnected() const override;

    // Active health check: verifies the socket is still alive
    bool IsAlive() const;

    // IEquipmentDriver - high-level workflow (Template Method)
    virtual std::vector<MeasurementResult> RunFullTest(
        const std::vector<double>& wavelengths,
        const std::vector<int>& channels,
        bool doReference,
        const ReferenceConfig& refConfig) override;

    // Low-level command interface (auto-reconnects once on send failure)
    std::string SendCommand(const std::string& command,
                            const std::string& endChar = "\n",
                            bool expectResponse = false);

    // Error checking helper
    void AssertNoError();

    // Accessors
    ConnectionState GetState() const { return m_state; }
    const ConnectionConfig& GetConfig() const { return m_config; }
    CLogger& GetLogger() { return m_logger; }

protected:
    // Non-blocking TCP connect with configurable timeout via select().
    // Works on any socket; can be called from subclasses for secondary sockets.
    bool ConnectSocket(SOCKET sock, const sockaddr_in& addr, double timeoutSec);

    // Enable TCP keepalive with fast probing intervals for early dead-peer detection.
    void EnableKeepAlive(SOCKET sock);

    // Called after TCP handshake succeeds. Override in subclasses to send a
    // device-specific probe (e.g. *IDN?) and verify the response.
    // Default: checks socket for pending errors.
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
