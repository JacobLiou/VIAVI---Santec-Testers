#pragma once

#include "IOSXDriver.h"
#include <string>
#include <vector>
#include <functional>
#include <mutex>

namespace OSXSwitch {

// ---------------------------------------------------------------------------
// 日志回调类型
// ---------------------------------------------------------------------------
typedef std::function<void(LogLevel level, const std::string& source, const std::string& message)> LogCallback;

// ---------------------------------------------------------------------------
// COSXDriver -- OSX 光开关的具体驱动实现
//
// 通信方式：TCP/IP 端口 5025（符合 LXI 标准的 SCPI-RAW）
// 协议：    SCPI，命令以 \n 结尾
// 异步模型：所有开关命令均为异步执行。发出命令后，
//           需轮询 STAT:OPER:COND? 直到返回 0。
//
// 参考：OSX 用户手册 M-OSX_SX1-001.04，表 4 和表 5
// ---------------------------------------------------------------------------
class OSX_API COSXDriver : public IOSXDriver
{
public:
    static const int DEFAULT_PORT           = 5025;
    static const int DEFAULT_TIMEOUT_MS     = 5000;
    static const int SWITCH_TIMEOUT_MS      = 5000;
    static const int MAX_POLL_INTERVAL_MS   = 10;
    static const int MAX_OPERATION_WAIT_MS  = 30000;

    COSXDriver(const std::string& ipAddress,
               int port = DEFAULT_PORT,
               double timeout = 5.0);
    virtual ~COSXDriver();

    // ---- IOSXDriver: 连接 ----
    virtual bool Connect() override;
    virtual void Disconnect() override;
    virtual bool Reconnect() override;
    virtual bool IsConnected() const override;

    // ---- IOSXDriver: 设备识别 ----
    virtual DeviceInfo  GetDeviceInfo() override;
    virtual ErrorInfo   CheckError() override;
    virtual std::string GetSystemVersion() override;

    // ---- IOSXDriver: 模块管理 ----
    virtual int  GetModuleCount() override;
    virtual std::vector<ModuleInfo> GetModuleCatalog() override;
    virtual ModuleInfo GetModuleInfo(int moduleIndex) override;
    virtual void SelectModule(int moduleIndex) override;
    virtual void SelectNextModule() override;
    virtual int  GetSelectedModule() override;

    // ---- IOSXDriver: 通道切换 ----
    virtual void SwitchChannel(int channel) override;
    virtual void SwitchNext() override;
    virtual int  GetCurrentChannel() override;
    virtual int  GetChannelCount() override;

    // ---- IOSXDriver: 多模块路由 ----
    virtual void RouteChannel(int moduleIndex, int channel) override;
    virtual int  GetRouteChannel(int moduleIndex) override;
    virtual void RouteAllModules(int channel) override;
    virtual void SetCommonInput(int moduleIndex, int common) override;
    virtual int  GetCommonInput(int moduleIndex) override;
    virtual void HomeModule(int moduleIndex) override;

    // ---- IOSXDriver: 控制 ----
    virtual void SetLocalMode(bool local) override;
    virtual bool GetLocalMode() override;
    virtual void SendNotification(int icon, const std::string& message) override;
    virtual void Reset() override;

    // ---- IOSXDriver: 网络配置 ----
    virtual std::string GetIPAddress() override;
    virtual std::string GetGateway() override;
    virtual std::string GetNetmask() override;
    virtual std::string GetHostname() override;
    virtual std::string GetMAC() override;

    // ---- IOSXDriver: 操作同步 ----
    virtual bool WaitForOperation(int timeoutMs = 5000) override;

    // ---- IOSXDriver: 原始 SCPI ----
    virtual std::string SendRawQuery(const std::string& command) override;
    virtual void        SendRawWrite(const std::string& command) override;

    // ---- 访问器 ----
    ConnectionState GetState() const { return m_state; }
    const ConnectionConfig& GetConfig() const { return m_config; }

    // ---- 日志 ----
    static void SetGlobalLogCallback(LogCallback callback);
    static void SetGlobalLogLevel(LogLevel level);

private:
    // 底层 TCP 通信
    bool ConnectSocket(SOCKET sock, const sockaddr_in& addr, double timeoutSec);
    void EnableKeepAlive(SOCKET sock);
    bool ValidateConnection();
    void CleanupSocket();

    // SCPI 命令辅助方法
    std::string Query(const std::string& command);
    void        Write(const std::string& command);
    std::string ReceiveResponse();

    // 发送命令后等待操作完成
    void WriteAndWait(const std::string& command, int timeoutMs = SWITCH_TIMEOUT_MS);

    // 解析
    void ParseIdentity(const std::string& idnResponse);
    static std::string Trim(const std::string& s);
    static SwitchConfigType ParseConfigType(const std::string& catalog);
    static int ParseChannelCount(const std::string& catalog);

    // 日志
    void Log(LogLevel level, const char* fmt, ...);

    // 状态
    SOCKET          m_socket;
    ConnectionConfig m_config;
    ConnectionState m_state;
    DeviceInfo      m_deviceInfo;

    // 日志
    static LogCallback s_globalCallback;
    static LogLevel    s_globalLevel;
    static std::mutex  s_logMutex;
};

} // namespace OSXSwitch
