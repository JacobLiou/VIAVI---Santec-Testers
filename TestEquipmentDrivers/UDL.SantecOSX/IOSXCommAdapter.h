#pragma once

#include <string>

namespace OSXSwitch {

// ---------------------------------------------------------------------------
// IOSXCommAdapter -- OSX 光开关通信适配器接口
//
// 允许在 TCP、GPIB、USB(VISA) 或基于 DLL 的通信之间切换。
// ---------------------------------------------------------------------------
class IOSXCommAdapter
{
public:
    virtual ~IOSXCommAdapter() {}

    // 建立连接
    // address: TCP 模式为 IP 地址，USB 模式为 VISA 资源字符串
    // port:    TCP 端口号（USB 模式忽略）
    // timeout: 超时时间（秒）
    virtual bool Open(const std::string& address, int port, double timeout) = 0;

    // 关闭连接
    virtual void Close() = 0;

    // 检查连接是否打开
    virtual bool IsOpen() const = 0;

    // 发送 SCPI 查询并接收响应
    virtual std::string SendQuery(const std::string& command) = 0;

    // 发送 SCPI 写命令（无响应）
    virtual void SendWrite(const std::string& command) = 0;
};

} // namespace OSXSwitch
