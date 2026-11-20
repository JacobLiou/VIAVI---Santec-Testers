#pragma once

// Self-contained WinSock TCP client for line-oriented SCPI instruments.
// Visual Studio 2015 / v140 / Windows 8.1 compatible.

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <string>

class ScpiTcpClient
{
public:
    ScpiTcpClient();
    ~ScpiTcpClient();

    bool Connect(const std::string& host, unsigned short port, unsigned long timeoutMs, std::string& error);
    void Disconnect();
    bool IsConnected() const;

    bool Write(const std::string& command, std::string& error);
    bool Query(const std::string& command, std::string& response, std::string& error);
    void SetIoTimeout(unsigned long timeoutMs);

private:
    ScpiTcpClient(const ScpiTcpClient&);
    ScpiTcpClient& operator=(const ScpiTcpClient&);

    bool SendAll(const char* data, size_t size, std::string& error);
    bool ReadLine(std::string& line, std::string& error);
    static std::string SocketErrorText(const char* operation, int errorCode);

    SOCKET m_socket;
    bool m_wsaStarted;
    unsigned long m_ioTimeoutMs;
    std::string m_receiveBuffer;
};
