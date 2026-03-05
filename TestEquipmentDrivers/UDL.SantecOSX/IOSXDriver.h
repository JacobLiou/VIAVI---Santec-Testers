#pragma once

#include "OSXTypes.h"
#include <vector>
#include <string>

namespace OSXSwitch {

// ---------------------------------------------------------------------------
// IOSXDriver -- OSX 光开关的抽象接口
//
// OSX 是一种多模块光开关，通过 TCP/IP（端口 5025）或 USB（USBTMC）
// 上的 SCPI 协议进行控制。与测量仪器不同，其主要操作是通道切换和模块管理。
//
// 重要提示：OSX 以异步方式运行 SCPI 命令。发出切换命令后，调用方必须
// 轮询 STAT:OPER:COND? 直到返回 0（或使用 WaitForOperation），
// 然后才能发送下一条命令。
// ---------------------------------------------------------------------------
class OSX_API IOSXDriver
{
public:
    virtual ~IOSXDriver() {}

    // ---- 连接生命周期 ----
    virtual bool Connect() = 0;
    virtual void Disconnect() = 0;
    virtual bool Reconnect() = 0;
    virtual bool IsConnected() const = 0;

    // ---- 设备识别 ----
    virtual DeviceInfo GetDeviceInfo() = 0;
    virtual ErrorInfo  CheckError() = 0;
    virtual std::string GetSystemVersion() = 0;

    // ---- 模块管理 ----
    virtual int  GetModuleCount() = 0;
    virtual std::vector<ModuleInfo> GetModuleCatalog() = 0;
    virtual ModuleInfo GetModuleInfo(int moduleIndex) = 0;
    virtual void SelectModule(int moduleIndex) = 0;
    virtual void SelectNextModule() = 0;
    virtual int  GetSelectedModule() = 0;

    // ---- 通道切换（在当前选中的模块上操作） ----
    virtual void SwitchChannel(int channel) = 0;
    virtual void SwitchNext() = 0;
    virtual int  GetCurrentChannel() = 0;
    virtual int  GetChannelCount() = 0;

    // ---- 多模块路由 ----
    virtual void RouteChannel(int moduleIndex, int channel) = 0;
    virtual int  GetRouteChannel(int moduleIndex) = 0;
    virtual void RouteAllModules(int channel) = 0;
    virtual void SetCommonInput(int moduleIndex, int common) = 0;
    virtual int  GetCommonInput(int moduleIndex) = 0;
    virtual void HomeModule(int moduleIndex) = 0;

    // ---- 控制 ----
    virtual void SetLocalMode(bool local) = 0;
    virtual bool GetLocalMode() = 0;
    virtual void SendNotification(int icon, const std::string& message) = 0;
    virtual void Reset() = 0;

    // ---- 网络配置（只读查询） ----
    virtual std::string GetIPAddress() = 0;
    virtual std::string GetGateway() = 0;
    virtual std::string GetNetmask() = 0;
    virtual std::string GetHostname() = 0;
    virtual std::string GetMAC() = 0;

    // ---- 操作同步 ----
    // 轮询 STAT:OPER:COND? 直到设备报告空闲（0）。
    // 如果在 timeoutMs 内变为空闲则返回 true，超时则返回 false。
    virtual bool WaitForOperation(int timeoutMs = 5000) = 0;

    // ---- 原始 SCPI 透传 ----
    virtual std::string SendRawQuery(const std::string& command) = 0;
    virtual void        SendRawWrite(const std::string& command) = 0;
};

} // namespace OSXSwitch
