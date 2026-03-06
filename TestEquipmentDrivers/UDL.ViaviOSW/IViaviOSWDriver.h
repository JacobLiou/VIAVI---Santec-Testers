#pragma once

#include "OSWTypes.h"
#include <vector>
#include <string>

namespace ViaviOSW {

// ---------------------------------------------------------------------------
// IViaviOSWDriver -- Viavi MAP mOSW-C1 光开关的抽象驱动接口
//
// mOSW-C1 是 Viavi MAP 系列的光开关模块。
// 通过 TCP/IP 连接到模块端口（默认 8203），
// 或通过 VISA/GPIB 连接。
//
// 通信协议：SCPI，命令以 \n 结尾
// 开关命令后需轮询 :BUSY? 直到返回 0
// ---------------------------------------------------------------------------
class OSW_API IViaviOSWDriver
{
public:
    virtual ~IViaviOSWDriver() {}

    // ---- 连接生命周期 ----
    virtual bool Connect() = 0;
    virtual void Disconnect() = 0;
    virtual bool Reconnect() = 0;
    virtual bool IsConnected() const = 0;

    // ---- 设备识别 ----
    virtual DeviceInfo GetDeviceInfo() = 0;
    virtual ErrorInfo  CheckError() = 0;

    // ---- 开关信息 ----
    virtual int GetDeviceCount() = 0;
    virtual SwitchInfo GetSwitchInfo(int deviceNum = 1) = 0;

    // ---- 通道切换 ----
    virtual void SwitchChannel(int deviceNum, int channel) = 0;
    virtual int  GetCurrentChannel(int deviceNum = 1) = 0;
    virtual int  GetChannelCount(int deviceNum = 1) = 0;

    // ---- 控制 ----
    virtual void SetLocalMode(bool local) = 0;
    virtual void Reset() = 0;

    // ---- 操作同步 ----
    virtual bool WaitForIdle(int timeoutMs = 5000) = 0;

    // ---- 原始 SCPI 透传 ----
    virtual std::string SendRawQuery(const std::string& command) = 0;
    virtual void        SendRawWrite(const std::string& command) = 0;
};

} // namespace ViaviOSW
