#pragma once

#include <string>

namespace ViaviPCT4All {

// ---------------------------------------------------------------------------
// IViaviPCT4AllCommAdapter -- 通信适配器接口
//
// 支持 TCP、GPIB、VISA 等通信方式之间的切换。
// 所有 SCPI 命令以 \n 结尾，查询读取直到 \n。
// ---------------------------------------------------------------------------
class IViaviPCT4AllCommAdapter
{
public:
    virtual ~IViaviPCT4AllCommAdapter() {}

    virtual bool Open(const std::string& address, int port, double timeout) = 0;
    virtual void Close() = 0;
    virtual bool IsOpen() const = 0;
    virtual std::string SendQuery(const std::string& command) = 0;
    virtual void SendWrite(const std::string& command) = 0;
};

} // namespace ViaviPCT4All
