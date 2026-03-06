#include "stdafx.h"
#include "ViaviOSWTcpAdapter.h"
#include <stdexcept>

namespace ViaviOSW {

CViaviOSWTcpAdapter::CViaviOSWTcpAdapter()
    : m_socket(INVALID_SOCKET)
    , m_bufferSize(4096)
{
}

CViaviOSWTcpAdapter::~CViaviOSWTcpAdapter()
{
    Close();
}

bool CViaviOSWTcpAdapter::Open(const std::string& address, int port, double timeout)
{
    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_socket == INVALID_SOCKET) return false;

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<u_short>(port));
    if (inet_pton(AF_INET, address.c_str(), &addr.sin_addr) != 1)
    {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        return false;
    }

    u_long nonBlocking = 1;
    ioctlsocket(m_socket, FIONBIO, &nonBlocking);

    int ret = connect(m_socket, (sockaddr*)&addr, sizeof(addr));
    if (ret == SOCKET_ERROR)
    {
        int err = WSAGetLastError();
        if (err != WSAEWOULDBLOCK)
        {
            closesocket(m_socket);
            m_socket = INVALID_SOCKET;
            return false;
        }

        fd_set writefds, exceptfds;
        FD_ZERO(&writefds);
        FD_ZERO(&exceptfds);
        FD_SET(m_socket, &writefds);
        FD_SET(m_socket, &exceptfds);

        timeval tv;
        tv.tv_sec = static_cast<long>(timeout);
        tv.tv_usec = static_cast<long>((timeout - static_cast<long>(timeout)) * 1000000);

        ret = select(0, NULL, &writefds, &exceptfds, &tv);
        if (ret <= 0 || FD_ISSET(m_socket, &exceptfds) || !FD_ISSET(m_socket, &writefds))
        {
            closesocket(m_socket);
            m_socket = INVALID_SOCKET;
            return false;
        }

        int optError = 0;
        int optLen = sizeof(optError);
        getsockopt(m_socket, SOL_SOCKET, SO_ERROR, (char*)&optError, &optLen);
        if (optError != 0)
        {
            closesocket(m_socket);
            m_socket = INVALID_SOCKET;
            return false;
        }
    }

    u_long blocking = 0;
    ioctlsocket(m_socket, FIONBIO, &blocking);

    DWORD timeoutMs = static_cast<DWORD>(timeout * 1000);
    setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeoutMs, sizeof(timeoutMs));
    setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeoutMs, sizeof(timeoutMs));

    BOOL keepAlive = TRUE;
    setsockopt(m_socket, SOL_SOCKET, SO_KEEPALIVE, (const char*)&keepAlive, sizeof(keepAlive));

    tcp_keepalive ka = {};
    ka.onoff = 1;
    ka.keepalivetime = 10000;
    ka.keepaliveinterval = 2000;
    DWORD bytesReturned = 0;
    WSAIoctl(m_socket, SIO_KEEPALIVE_VALS, &ka, sizeof(ka), NULL, 0, &bytesReturned, NULL, NULL);

    return true;
}

void CViaviOSWTcpAdapter::Close()
{
    if (m_socket != INVALID_SOCKET)
    {
        shutdown(m_socket, SD_BOTH);
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }
}

bool CViaviOSWTcpAdapter::IsOpen() const
{
    return m_socket != INVALID_SOCKET;
}

void CViaviOSWTcpAdapter::SetReadTimeout(DWORD timeoutMs)
{
    if (m_socket != INVALID_SOCKET)
    {
        setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO,
                   (const char*)&timeoutMs, sizeof(timeoutMs));
    }
}

std::string CViaviOSWTcpAdapter::SendQuery(const std::string& command)
{
    SendWrite(command);

    std::string data;
    char buf[4096];
    while (true)
    {
        int received = recv(m_socket, buf, sizeof(buf) - 1, 0);
        if (received <= 0) break;
        buf[received] = '\0';
        data.append(buf, received);
        if (data.find('\n') != std::string::npos) break;
    }
    while (!data.empty() && (data.back() == '\n' || data.back() == '\r'))
        data.pop_back();
    return data;
}

void CViaviOSWTcpAdapter::SendWrite(const std::string& command)
{
    if (m_socket == INVALID_SOCKET)
        throw std::runtime_error("Viavi OSW TCP adapter not connected.");

    std::string fullCmd = command + "\n";
    int sent = send(m_socket, fullCmd.c_str(), static_cast<int>(fullCmd.size()), 0);
    if (sent == SOCKET_ERROR)
        throw std::runtime_error(std::string("Viavi OSW TCP send failed: WSA ")
            + std::to_string(WSAGetLastError()));
}

} // namespace ViaviOSW
