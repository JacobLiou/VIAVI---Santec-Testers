#pragma once

#include "IOSXCommAdapter.h"
#include "OSXTypes.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mstcpip.h>

namespace OSXSwitch {

// ---------------------------------------------------------------------------
// COSXTcpAdapter -- 通过 TCP/IP 与 OSX 光开关通信
//
// 端口 5025（符合 LXI 标准的 SCPI-RAW），命令以 \n 结尾。
// ---------------------------------------------------------------------------
class OSX_API COSXTcpAdapter : public IOSXCommAdapter
{
public:
    COSXTcpAdapter();
    virtual ~COSXTcpAdapter();

    virtual bool Open(const std::string& address, int port, double timeout) override;
    virtual void Close() override;
    virtual bool IsOpen() const override;
    virtual std::string SendQuery(const std::string& command) override;
    virtual void SendWrite(const std::string& command) override;

private:
    bool ConnectSocket(SOCKET sock, const sockaddr_in& addr, double timeoutSec);
    void EnableKeepAlive(SOCKET sock);
    std::string ReceiveResponse();

    SOCKET m_socket;
    int    m_bufferSize;
};

} // namespace OSXSwitch
