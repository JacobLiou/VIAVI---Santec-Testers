#pragma once

#include "IViaviOSWDriver.h"
#include "IViaviOSWCommAdapter.h"
#include <string>
#include <vector>
#include <functional>
#include <mutex>

namespace ViaviOSW {

typedef std::function<void(LogLevel level, const std::string& source, const std::string& message)> LogCallback;

// ---------------------------------------------------------------------------
// CViaviOSWDriver -- Viavi MAP mOSW-C1 光开关驱动实现
//
// SCPI 命令参考：
//   MAP-PCT Programming Guide 22112369-346, Chapter 2 (通用模块命令)
//   标准 SCPI 路由命令
//
// 通信方式：
//   - TCP/IP 端口 8203（OSW 模块默认端口）
//   - GPIB 或 VXI-11 通过 VISA
//   - 命令以 \n（换行符）结尾
//   - 开关命令后轮询 :BUSY? 直到返回 0
// ---------------------------------------------------------------------------
class OSW_API CViaviOSWDriver : public IViaviOSWDriver
{
public:
    static const int DEFAULT_PORT           = 8203;
    static const int DEFAULT_TIMEOUT_MS     = 5000;
    static const int SWITCH_TIMEOUT_MS      = 5000;
    static const int POLL_INTERVAL_MS       = 50;

    CViaviOSWDriver(const std::string& address,
                     int port = DEFAULT_PORT,
                     double timeout = 5.0,
                     CommType commType = COMM_TCP);
    virtual ~CViaviOSWDriver();

    // IViaviOSWDriver 实现 - 连接
    virtual bool Connect() override;
    virtual void Disconnect() override;
    virtual bool Reconnect() override;
    virtual bool IsConnected() const override;

    // 设备识别
    virtual DeviceInfo GetDeviceInfo() override;
    virtual ErrorInfo  CheckError() override;

    // 开关信息
    virtual int GetDeviceCount() override;
    virtual SwitchInfo GetSwitchInfo(int deviceNum = 1) override;

    // 通道切换
    virtual void SwitchChannel(int deviceNum, int channel) override;
    virtual int  GetCurrentChannel(int deviceNum = 1) override;
    virtual int  GetChannelCount(int deviceNum = 1) override;

    // 控制
    virtual void SetLocalMode(bool local) override;
    virtual void Reset() override;

    // 操作同步
    virtual bool WaitForIdle(int timeoutMs = 5000) override;

    // 原始 SCPI
    virtual std::string SendRawQuery(const std::string& command) override;
    virtual void        SendRawWrite(const std::string& command) override;

    // 访问器
    ConnectionState GetState() const { return m_state; }
    const ConnectionConfig& GetConfig() const { return m_config; }

    // 适配器注入
    void SetCommAdapter(IViaviOSWCommAdapter* adapter, bool takeOwnership = true);

    // 日志 (per-instance preferred over global)
    void SetLogCallback(LogCallback callback);
    static void SetGlobalLogCallback(LogCallback callback);
    static void SetGlobalLogLevel(LogLevel level);

private:
    // 连接验证
    bool ValidateConnection();

    // SCPI 命令
    std::string Query(const std::string& command);
    void        Write(const std::string& command);

    // 发送命令后等待操作完成
    void WriteAndWait(const std::string& command, int timeoutMs = SWITCH_TIMEOUT_MS);

    // 解析
    void ParseDeviceInfo(const std::string& response);
    SwitchInfo ParseSwitchConfig(const std::string& response);

    // 辅助函数
    static std::string Trim(const std::string& s);

    // 日志
    void Log(LogLevel level, const char* fmt, ...);

    // 通信
    CommType              m_commType;
    IViaviOSWCommAdapter* m_adapter;
    bool                  m_ownsAdapter;

    // 状态
    ConnectionConfig  m_config;
    ConnectionState   m_state;
    DeviceInfo        m_deviceInfo;
    SwitchInfo        m_switchInfo;
    int               m_deviceCount;

    // 日志
    LogCallback           m_instanceCallback;
    static LogCallback    s_globalCallback;
    static LogLevel       s_globalLevel;
    static std::mutex     s_logMutex;
};

} // namespace ViaviOSW
