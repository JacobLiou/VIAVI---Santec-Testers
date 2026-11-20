#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <windows.h>
#include <string>

class SimulatorServer
{
public:
    SimulatorServer();
    ~SimulatorServer();

    bool Start(unsigned short port, std::string& error);
    void Run();
    void Stop();

private:
    SimulatorServer(const SimulatorServer&);
    SimulatorServer& operator=(const SimulatorServer&);

    static DWORD WINAPI ClientThread(void* parameter);

    SOCKET m_listenSocket;
    bool m_wsaStarted;
    volatile LONG m_stopping;
};
