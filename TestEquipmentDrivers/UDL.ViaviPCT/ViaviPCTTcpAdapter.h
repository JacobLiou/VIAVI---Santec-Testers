#pragma once

#include "IViaviPCTCommAdapter.h"
#include "PCTTypes.h"

namespace ViaviPCT {

// ---------------------------------------------------------------------------
// CViaviPCTTcpAdapter -- 通过 TCP 与 Viavi MAP PCT 模块通信
//
// 连接到 PCT 模块的专用 TCP 端口（默认 8301）。
// 每个 MAP 模块有独立的 TCP 端口，可通过系统控制器
// 端口 8100 的 :SYST:LAY:PORT? 命令查询。
// ---------------------------------------------------------------------------
class PCT_API CViaviPCTTcpAdapter : public IViaviPCTCommAdapter
{
public:
    CViaviPCTTcpAdapter();
    virtual ~CViaviPCTTcpAdapter();

    virtual bool Open(const std::string& address, int port, double timeout) override;
    virtual void Close() override;
    virtual bool IsOpen() const override;
    virtual std::string SendQuery(const std::string& command) override;
    virtual void SendWrite(const std::string& command) override;

    void SetReadTimeout(DWORD timeoutMs);

private:
    SOCKET m_socket;
    int    m_bufferSize;
};

} // namespace ViaviPCT
