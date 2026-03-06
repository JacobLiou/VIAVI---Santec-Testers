#pragma once

#include "IViaviOSWCommAdapter.h"
#include "OSWTypes.h"

namespace ViaviOSW {

// ---------------------------------------------------------------------------
// CViaviOSWTcpAdapter -- 通过 TCP 与 Viavi MAP OSW 模块通信
//
// 连接到 OSW 模块的专用 TCP 端口（默认 8203）。
// ---------------------------------------------------------------------------
class OSW_API CViaviOSWTcpAdapter : public IViaviOSWCommAdapter
{
public:
    CViaviOSWTcpAdapter();
    virtual ~CViaviOSWTcpAdapter();

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

} // namespace ViaviOSW
