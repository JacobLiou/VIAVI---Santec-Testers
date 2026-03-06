#pragma once

#include "PCTTypes.h"
#include <vector>
#include <string>

namespace ViaviPCT {

// ---------------------------------------------------------------------------
// IViaviPCTDriver -- Viavi MAP PCT (mORL) 的抽象驱动接口
//
// PCT 是 Viavi MAP 系列的被动元件测试超级应用，安装于 mORL-A1 模块。
// 通过 TCP/IP 连接到模块端口（默认 8301），
// 或通过 VISA/GPIB 连接。
//
// 通信协议：SCPI，命令以 \n 结尾
// 参考：MAP-PCT Programming Guide 22112369-346
// ---------------------------------------------------------------------------
class PCT_API IViaviPCTDriver
{
public:
    virtual ~IViaviPCTDriver() {}

    // ---- 连接生命周期 ----
    virtual bool Connect() = 0;
    virtual void Disconnect() = 0;
    virtual bool Reconnect() = 0;
    virtual bool IsConnected() const = 0;

    // ---- 连接后初始化 ----
    virtual bool Initialize() = 0;

    // ---- 设备信息 ----
    virtual DeviceInfo GetDeviceInfo() = 0;
    virtual ErrorInfo  CheckError() = 0;
    virtual PCTConfig  GetConfiguration() = 0;

    // ---- 配置 ----
    virtual void ConfigureWavelengths(const std::vector<double>& wavelengths) = 0;
    virtual void ConfigureChannels(const std::vector<int>& channels) = 0;
    virtual void SetMeasurementMode(MeasurementMode mode) = 0;
    virtual MeasurementMode GetMeasurementMode() = 0;

    // ---- PCT 特定配置 ----
    virtual void SetAveragingTime(int seconds) = 0;
    virtual int  GetAveragingTime() = 0;
    virtual void SetDUTRange(int rangeMeters) = 0;
    virtual void SetILOnly(bool ilOnly) = 0;
    virtual void SetConnectionMode(int mode) = 0; // 1=单跳线, 2=双跳线
    virtual void SetBidirectional(bool enabled) = 0;

    // ---- 测量操作 ----
    virtual bool TakeReference(const ReferenceConfig& config) = 0;
    virtual bool TakeMeasurement() = 0;
    virtual std::vector<MeasurementResult> GetResults() = 0;
    virtual MeasurementState GetMeasurementState() = 0;
    virtual bool WaitForMeasurement(int timeoutMs = 60000) = 0;

    // ---- 高级工作流 ----
    virtual std::vector<MeasurementResult> RunFullTest(
        const std::vector<double>& wavelengths,
        const std::vector<int>& channels,
        bool doReference,
        const ReferenceConfig& refConfig) = 0;

    // ---- 原始 SCPI 透传 ----
    virtual std::string SendRawQuery(const std::string& command) = 0;
    virtual void        SendRawWrite(const std::string& command) = 0;
};

} // namespace ViaviPCT
