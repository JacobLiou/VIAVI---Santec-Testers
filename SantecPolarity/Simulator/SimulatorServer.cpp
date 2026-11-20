#include "SimulatorServer.h"
#include "ScpiSession.h"

#include <iostream>
#include <sstream>

#pragma comment(lib, "Ws2_32.lib")

SimulatorServer::SimulatorServer()
    : m_listenSocket(INVALID_SOCKET), m_wsaStarted(false), m_stopping(0)
{
    WSADATA data;
    if (WSAStartup(MAKEWORD(2, 2), &data) == 0)
        m_wsaStarted = true;
}

SimulatorServer::~SimulatorServer()
{
    Stop();
    if (m_wsaStarted)
        WSACleanup();
}

bool SimulatorServer::Start(unsigned short port, std::string& error)
{
    if (!m_wsaStarted)
    {
        error = "WSAStartup failed";
        return false;
    }
    m_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_listenSocket == INVALID_SOCKET)
    {
        error = "socket failed";
        return false;
    }

    BOOL reuse = TRUE;
    setsockopt(m_listenSocket, SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<const char*>(&reuse), sizeof(reuse));

    sockaddr_in address;
    ZeroMemory(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);
    if (bind(m_listenSocket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR)
    {
        std::ostringstream text;
        text << "bind failed with WinSock error " << WSAGetLastError();
        error = text.str();
        Stop();
        return false;
    }
    if (listen(m_listenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        error = "listen failed";
        Stop();
        return false;
    }
    InterlockedExchange(&m_stopping, 0);
    return true;
}

void SimulatorServer::Run()
{
    while (InterlockedCompareExchange(&m_stopping, 0, 0) == 0)
    {
        SOCKET client = accept(m_listenSocket, NULL, NULL);
        if (client == INVALID_SOCKET)
        {
            if (InterlockedCompareExchange(&m_stopping, 0, 0) == 0)
                std::cerr << "accept failed with WinSock error " << WSAGetLastError() << std::endl;
            break;
        }
        SOCKET* context = new SOCKET(client);
        HANDLE thread = CreateThread(NULL, 0, ClientThread, context, 0, NULL);
        if (thread != NULL)
            CloseHandle(thread);
        else
        {
            closesocket(client);
            delete context;
        }
    }
}

void SimulatorServer::Stop()
{
    InterlockedExchange(&m_stopping, 1);
    if (m_listenSocket != INVALID_SOCKET)
    {
        shutdown(m_listenSocket, SD_BOTH);
        closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
    }
}

DWORD WINAPI SimulatorServer::ClientThread(void* parameter)
{
    SOCKET* context = static_cast<SOCKET*>(parameter);
    const SOCKET socket = *context;
    delete context;
    ScpiSession session(socket);
    session.Run();
    return 0;
}
