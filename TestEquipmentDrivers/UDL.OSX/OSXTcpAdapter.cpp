#include "stdafx.h"
#include "OSXTcpAdapter.h"
#include <stdexcept>

namespace OSXSwitch {

COSXTcpAdapter::COSXTcpAdapter()
    : m_socket(INVALID_SOCKET)
    , m_bufferSize(4096)
{
}

COSXTcpAdapter::~COSXTcpAdapter()
{
    Close();
}

bool COSXTcpAdapter::ConnectSocket(SOCKET sock, const sockaddr_in& addr, double timeoutSec)
{
    u_long nonBlocking = 1;
    ioctlsocket(sock, FIONBIO, &nonBlocking);

    int ret = connect(sock, (const sockaddr*)&addr, sizeof(addr));
    if (ret == 0)
    {
        u_long blocking = 0;
        ioctlsocket(sock, FIONBIO, &blocking);
        return true;
    }

    int err = WSAGetLastError();
    if (err != WSAEWOULDBLOCK)
        return false;

    fd_set writefds, exceptfds;
    FD_ZERO(&writefds);
    FD_ZERO(&exceptfds);
    FD_SET(sock, &writefds);
    FD_SET(sock, &exceptfds);

    timeval tv;
    tv.tv_sec = static_cast<long>(timeoutSec);
    tv.tv_usec = static_cast<long>((timeoutSec - tv.tv_sec) * 1000000);

    ret = select(0, NULL, &writefds, &exceptfds, &tv);
    if (ret <= 0 || FD_ISSET(sock, &exceptfds) || !FD_ISSET(sock, &writefds))
        return false;

    int optError = 0;
    int optLen = sizeof(optError);
    getsockopt(sock, SOL_SOCKET, SO_ERROR, (char*)&optError, &optLen);
    if (optError != 0)
        return false;

    u_long blocking = 0;
    ioctlsocket(sock, FIONBIO, &blocking);
    return true;
}

void COSXTcpAdapter::EnableKeepAlive(SOCKET sock)
{
    BOOL keepAlive = TRUE;
    setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (const char*)&keepAlive, sizeof(keepAlive));

    struct tcp_keepalive ka = {};
    ka.onoff = 1;
    ka.keepalivetime = 10000;
    ka.keepaliveinterval = 2000;
    DWORD bytesReturned = 0;
    WSAIoctl(sock, SIO_KEEPALIVE_VALS, &ka, sizeof(ka),
             NULL, 0, &bytesReturned, NULL, NULL);
}

bool COSXTcpAdapter::Open(const std::string& address, int port, double timeout)
{
    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_socket == INVALID_SOCKET)
        return false;

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<u_short>(port));

    if (inet_pton(AF_INET, address.c_str(), &addr.sin_addr) != 1)
    {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        return false;
    }

    if (!ConnectSocket(m_socket, addr, timeout))
    {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        return false;
    }

    DWORD timeoutMs = static_cast<DWORD>(timeout * 1000);
    setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeoutMs, sizeof(timeoutMs));
    setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeoutMs, sizeof(timeoutMs));
    EnableKeepAlive(m_socket);

    return true;
}

void COSXTcpAdapter::Close()
{
    if (m_socket != INVALID_SOCKET)
    {
        shutdown(m_socket, SD_BOTH);
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }
}

bool COSXTcpAdapter::IsOpen() const
{
    return m_socket != INVALID_SOCKET;
}

std::string COSXTcpAdapter::ReceiveResponse()
{
    std::string data;
    char buf[4096];

    while (true)
    {
        int received = recv(m_socket, buf, sizeof(buf) - 1, 0);
        if (received == SOCKET_ERROR)
        {
            int err = WSAGetLastError();
            if (err == WSAETIMEDOUT)
                throw std::runtime_error("OSX TCP 响应超时。");
            throw std::runtime_error(std::string("OSX TCP 接收错误: WSA ")
                + std::to_string(err));
        }
        if (received == 0)
            throw std::runtime_error("OSX TCP 连接被远程主机关闭。");

        buf[received] = '\0';
        data.append(buf, received);

        if (data.find('\n') != std::string::npos)
            break;
    }

    while (!data.empty() && (data.back() == '\n' || data.back() == '\r' || data.back() == ' '))
        data.pop_back();

    return data;
}

std::string COSXTcpAdapter::SendQuery(const std::string& command)
{
    SendWrite(command);
    return ReceiveResponse();
}

void COSXTcpAdapter::SendWrite(const std::string& command)
{
    if (m_socket == INVALID_SOCKET)
        throw std::runtime_error("OSX TCP adapter 未连接。");

    std::string fullCmd = command + "\n";
    int sent = send(m_socket, fullCmd.c_str(), static_cast<int>(fullCmd.size()), 0);
    if (sent == SOCKET_ERROR)
        throw std::runtime_error(std::string("OSX TCP 发送失败: WSA ")
            + std::to_string(WSAGetLastError()));
}

} // namespace OSXSwitch
