#pragma once

#include "ISantecPTMDriver.h"
#include "ISantecPTMCommAdapter.h"
#include "Logger.h"

namespace SantecPTM {

// ---------------------------------------------------------------------------
// Santec PTM Polarity Tester driver
// Protocol: M-PTM_PT1-001.04 Chapter 8
// TCP 5025 / USB USBTMC (VISA). Commands end with \n.
// Async ops: poll STAT:OPER:COND? until 0.
// ---------------------------------------------------------------------------
class PTM_API CSantecPTMDriver : public ISantecPTMDriver
{
public:
    static const int DEFAULT_PORT       = 5025;
    static const int DEFAULT_TIMEOUT_MS = 5000;
    static const int IDLE_TIMEOUT_MS    = 30000;
    static const int IDLE_POLL_MS       = 20;

    CSantecPTMDriver(const std::string& address,
                     int port = DEFAULT_PORT,
                     double timeout = 5.0,
                     CommType commType = COMM_TCP);
    virtual ~CSantecPTMDriver();

    virtual bool Connect() override;
    virtual void Disconnect() override;
    virtual bool Reconnect() override;
    virtual bool IsConnected() const override;

    virtual bool Initialize() override;
    virtual DeviceInfo GetDeviceInfo() override;
    virtual ErrorInfo CheckError() override;

    virtual int GetChannelCount() override;
    virtual bool TakeMeasurement(int nChannels = 0) override;
    virtual std::vector<PolarityChannelResult> GetResults() override;
    virtual std::string GetRawResult() override;

    virtual int Detect(int outputChannel) override;
    virtual void SetLocalMode(bool enabled) override;
    virtual bool GetLocalMode() override;
    virtual void LaserOff() override;
    virtual void SetVfl(int outputChannel, VflMode mode) override;
    virtual std::string GetBarcode() override;
    virtual void Notify(int icon, const std::string& message) override;

    virtual bool TestStart() override;
    virtual std::string TestStatus() override;
    virtual bool TestNext() override;
    virtual bool TestRetry() override;

    virtual std::string SendRawQuery(const std::string& command) override;
    virtual void SendRawWrite(const std::string& command) override;

    void SetCommAdapter(ISantecPTMCommAdapter* adapter, bool takeOwnership = true);

    // Provisional parsers for TEST:RESult? (adjust after real-device capture)
    static std::vector<PolarityChannelResult> ParsePolarityMap(const std::string& raw);

private:
    std::string Query(const std::string& command);
    void        Write(const std::string& command);
    bool        WaitUntilIdle(int timeoutMs = IDLE_TIMEOUT_MS);
    void        ParseIdentity(const std::string& idn);
    static std::string Trim(const std::string& s);

    CommType                m_commType;
    ConnectionConfig        m_config;
    ISantecPTMCommAdapter*  m_adapter;
    bool                    m_ownsAdapter;
    ConnectionState         m_state;
    DeviceInfo              m_deviceInfo;
    bool                    m_localMode;
    int                     m_channelCount;
    std::string             m_rawResult;
    std::vector<PolarityChannelResult> m_results;
    CLogger                 m_log;
};

} // namespace SantecPTM
