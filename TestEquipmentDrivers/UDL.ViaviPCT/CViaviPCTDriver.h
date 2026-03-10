#pragma once

#include "IViaviPCTDriver.h"
#include "IViaviPCTCommAdapter.h"
#include "Logger.h"
#include <string>
#include <vector>

namespace ViaviPCT {

// ---------------------------------------------------------------------------
// CViaviPCTDriver -- Viavi MAP PCT (mORL) 驱动实现
//
// SCPI 命令参考：
//   MAP-PCT Programming Guide 22112369-346
//
// 通信方式：
//   - TCP/IP 端口 8301（PCT 模块默认端口）
//   - GPIB 或 VXI-11 通过 VISA
//   - 命令以 \n（换行符）结尾
//   - 使用 :MEAS:STAT? 轮询测量完成状态
//   - 连接后发送 *REM 激活远程控制
// ---------------------------------------------------------------------------
class PCT_API CViaviPCTDriver : public IViaviPCTDriver
{
public:
    static const int DEFAULT_PORT           = 8301;
    static const int MEAS_TIMEOUT_MS        = 180000;
    static const int DEFAULT_TIMEOUT_MS     = 5000;
    static const int POLL_INTERVAL_MS       = 500;

    CViaviPCTDriver(const std::string& address,
                    int port = DEFAULT_PORT,
                    double timeout = 5.0,
                    CommType commType = COMM_TCP);
    virtual ~CViaviPCTDriver();

    // IViaviPCTDriver 实现 - 连接
    virtual bool Connect() override;
    virtual void Disconnect() override;
    virtual bool Reconnect() override;
    virtual bool IsConnected() const override;

    // 初始化
    virtual bool Initialize() override;

    // 设备信息
    virtual DeviceInfo GetDeviceInfo() override;
    virtual ErrorInfo  CheckError() override;
    virtual PCTConfig  GetConfiguration() override;

    // 配置
    virtual void ConfigureWavelengths(const std::vector<double>& wavelengths) override;
    virtual void ConfigureChannels(const std::vector<int>& channels) override;
    virtual void SetMeasurementMode(MeasurementMode mode) override;
    virtual MeasurementMode GetMeasurementMode() override;

    // PCT 特定配置
    virtual void SetAveragingTime(int seconds) override;
    virtual int  GetAveragingTime() override;
    virtual void SetDUTRange(int rangeMeters) override;
    virtual void SetILOnly(bool ilOnly) override;
    virtual void SetConnectionMode(int mode) override;
    virtual void SetBidirectional(bool enabled) override;

    // 测量操作
    virtual bool TakeReference(const ReferenceConfig& config) override;
    virtual bool TakeMeasurement() override;
    virtual std::vector<MeasurementResult> GetResults() override;
    virtual MeasurementState GetMeasurementState() override;
    virtual bool WaitForMeasurement(int timeoutMs = 60000) override;

    // 高级工作流
    virtual std::vector<MeasurementResult> RunFullTest(
        const std::vector<double>& wavelengths,
        const std::vector<int>& channels,
        bool doReference,
        const ReferenceConfig& refConfig) override;

    // 原始 SCPI 透传
    virtual std::string SendRawQuery(const std::string& command) override;
    virtual void        SendRawWrite(const std::string& command) override;

    // 设置自定义通信适配器
    void SetCommAdapter(IViaviPCTCommAdapter* adapter, bool takeOwnership = true);

    // 访问器
    ConnectionState GetState() const { return m_state; }
    CLogger& GetLogger() { return m_logger; }

private:
    // 统一命令接口
    std::string Query(const std::string& command);
    std::string QueryLong(const std::string& command);
    void        Write(const std::string& command);

    // 连接验证
    bool ValidateConnection();

    // 解析 :INFO? 响应
    void ParseDeviceInfo(const std::string& response);

    // 解析 :CONF? 响应
    PCTConfig ParseConfiguration(const std::string& response);

    // 解析 :MEAS:ALL? 响应
    MeasurementResult ParseMeasurementAll(const std::string& response, int channel);

    // 辅助函数
    static std::vector<double> ParseDoubleList(const std::string& csv);
    static std::vector<int> ParseIntList(const std::string& csv);
    static std::string Trim(const std::string& s);

    CommType                m_commType;
    IViaviPCTCommAdapter*   m_adapter;
    bool                    m_ownsAdapter;
    ConnectionConfig        m_config;
    ConnectionState         m_state;
    DeviceInfo              m_deviceInfo;
    PCTConfig               m_pctConfig;
    CLogger                 m_logger;

    // 配置状态
    MeasurementMode         m_measMode;
    int                     m_averagingTime;
    int                     m_dutRange;
    bool                    m_ilOnly;
    int                     m_connectionMode;
    bool                    m_bidirectional;
    bool                    m_referenced;

    std::vector<double>     m_wavelengths;
    std::vector<int>        m_channels;

    // TakeMeasurement() 的缓存结果
    std::vector<MeasurementResult> m_lastResults;
};

} // namespace ViaviPCT
