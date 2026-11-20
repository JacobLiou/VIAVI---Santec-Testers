#include "ScpiTcpClient.h"

#include <sstream>

#pragma comment(lib, "Ws2_32.lib")

namespace
{
    const size_t kMaximumScpiLine = 64U * 1024U;
}

ScpiTcpClient::ScpiTcpClient()
    : m_socket(INVALID_SOCKET), m_wsaStarted(false), m_ioTimeoutMs(15000)
{
    WSADATA data;
    if (WSAStartup(MAKEWORD(2, 2), &data) == 0)
        m_wsaStarted = true;
}

ScpiTcpClient::~ScpiTcpClient()
{
    Disconnect();
    if (m_wsaStarted)
        WSACleanup();
}

bool ScpiTcpClient::Connect(const std::string& host, unsigned short port,
                            unsigned long timeoutMs, std::string& error)
{
    Disconnect();
    if (!m_wsaStarted)
    {
        error = "WSAStartup failed";
        return false;
    }
    if (host.empty() || port == 0)
    {
        error = "Host and port are required";
        return false;
    }

    std::ostringstream portText;
    portText << port;
    addrinfo hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    addrinfo* addresses = NULL;
    const int addressResult = getaddrinfo(host.c_str(), portText.str().c_str(), &hints, &addresses);
    if (addressResult != 0)
    {
        error = "Unable to resolve host: " + host;
        return false;
    }

    bool connected = false;
    for (addrinfo* current = addresses; current != NULL && !connected; current = current->ai_next)
    {
        SOCKET candidate = socket(current->ai_family, current->ai_socktype, current->ai_protocol);
        if (candidate == INVALID_SOCKET)
            continue;

        u_long nonBlocking = 1;
        ioctlsocket(candidate, FIONBIO, &nonBlocking);
        int result = connect(candidate, current->ai_addr, static_cast<int>(current->ai_addrlen));
        if (result == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK)
        {
            fd_set writeSet;
            FD_ZERO(&writeSet);
            FD_SET(candidate, &writeSet);
            timeval waitTime;
            waitTime.tv_sec = static_cast<long>(timeoutMs / 1000UL);
            waitTime.tv_usec = static_cast<long>((timeoutMs % 1000UL) * 1000UL);
            result = select(0, NULL, &writeSet, NULL, &waitTime);
            if (result > 0)
            {
                int socketError = 0;
                int length = sizeof(socketError);
                if (getsockopt(candidate, SOL_SOCKET, SO_ERROR,
                               reinterpret_cast<char*>(&socketError), &length) == 0 && socketError == 0)
                    connected = true;
                else
                    error = SocketErrorText("connect", socketError);
            }
            else if (result == 0)
                error = "Connection timed out";
            else
                error = SocketErrorText("select", WSAGetLastError());
        }
        else if (result == 0)
            connected = true;
        else
            error = SocketErrorText("connect", WSAGetLastError());

        if (connected)
        {
            nonBlocking = 0;
            ioctlsocket(candidate, FIONBIO, &nonBlocking);
            m_socket = candidate;
        }
        else
            closesocket(candidate);
    }
    freeaddrinfo(addresses);

    if (!connected)
    {
        if (error.empty())
            error = "Unable to connect to any resolved address";
        return false;
    }

    SetIoTimeout(m_ioTimeoutMs);
    m_receiveBuffer.clear();
    return true;
}

void ScpiTcpClient::Disconnect()
{
    if (m_socket != INVALID_SOCKET)
    {
        shutdown(m_socket, SD_BOTH);
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }
    m_receiveBuffer.clear();
}

bool ScpiTcpClient::IsConnected() const
{
    return m_socket != INVALID_SOCKET;
}

void ScpiTcpClient::SetIoTimeout(unsigned long timeoutMs)
{
    m_ioTimeoutMs = timeoutMs;
    if (m_socket != INVALID_SOCKET)
    {
        const DWORD value = static_cast<DWORD>(timeoutMs);
        setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO,
                   reinterpret_cast<const char*>(&value), sizeof(value));
        setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO,
                   reinterpret_cast<const char*>(&value), sizeof(value));
    }
}

bool ScpiTcpClient::Write(const std::string& command, std::string& error)
{
    if (!IsConnected())
    {
        error = "Not connected";
        return false;
    }
    std::string wireCommand = command;
    while (!wireCommand.empty() && (wireCommand[wireCommand.size() - 1] == '\r' ||
                                     wireCommand[wireCommand.size() - 1] == '\n'))
        wireCommand.erase(wireCommand.size() - 1);
    wireCommand.push_back('\n');
    return SendAll(wireCommand.data(), wireCommand.size(), error);
}

bool ScpiTcpClient::Query(const std::string& command, std::string& response, std::string& error)
{
    response.clear();
    if (!Write(command, error))
        return false;
    return ReadLine(response, error);
}

bool ScpiTcpClient::SendAll(const char* data, size_t size, std::string& error)
{
    size_t sent = 0;
    while (sent < size)
    {
        const int chunk = send(m_socket, data + sent, static_cast<int>(size - sent), 0);
        if (chunk == SOCKET_ERROR)
        {
            error = SocketErrorText("send", WSAGetLastError());
            Disconnect();
            return false;
        }
        if (chunk == 0)
        {
            error = "Connection closed while sending";
            Disconnect();
            return false;
        }
        sent += static_cast<size_t>(chunk);
    }
    return true;
}

bool ScpiTcpClient::ReadLine(std::string& line, std::string& error)
{
    for (;;)
    {
        const size_t newline = m_receiveBuffer.find('\n');
        if (newline != std::string::npos)
        {
            line.assign(m_receiveBuffer, 0, newline);
            m_receiveBuffer.erase(0, newline + 1);
            if (!line.empty() && line[line.size() - 1] == '\r')
                line.erase(line.size() - 1);
            return true;
        }
        if (m_receiveBuffer.size() >= kMaximumScpiLine)
        {
            error = "SCPI response exceeded 64 KiB";
            Disconnect();
            return false;
        }

        char buffer[2048];
        const int received = recv(m_socket, buffer, sizeof(buffer), 0);
        if (received == SOCKET_ERROR)
        {
            const int socketError = WSAGetLastError();
            error = (socketError == WSAETIMEDOUT)
                ? "Timed out waiting for SCPI response"
                : SocketErrorText("recv", socketError);
            Disconnect();
            return false;
        }
        if (received == 0)
        {
            error = "Instrument closed the connection";
            Disconnect();
            return false;
        }
        m_receiveBuffer.append(buffer, static_cast<size_t>(received));
    }
}

std::string ScpiTcpClient::SocketErrorText(const char* operation, int errorCode)
{
    std::ostringstream text;
    text << operation << " failed with WinSock error " << errorCode;
    return text.str();
}
