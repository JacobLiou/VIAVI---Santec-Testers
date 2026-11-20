#include "SimulatorServer.h"

#include <windows.h>
#include <cstdlib>
#include <iostream>
#include <string>

namespace
{
    SimulatorServer* g_server = NULL;

    BOOL WINAPI ConsoleControlHandler(DWORD eventType)
    {
        if (eventType == CTRL_C_EVENT || eventType == CTRL_BREAK_EVENT || eventType == CTRL_CLOSE_EVENT)
        {
            if (g_server != NULL)
                g_server->Stop();
            return TRUE;
        }
        return FALSE;
    }
}

int main(int argc, char* argv[])
{
    unsigned short port = 5025;
    if (argc > 1)
    {
        const long parsed = std::strtol(argv[1], NULL, 10);
        if (parsed < 1 || parsed > 65535)
        {
            std::cerr << "Usage: UDL.SantecPolarity.Simulator.exe [port]" << std::endl;
            return 2;
        }
        port = static_cast<unsigned short>(parsed);
    }

    SimulatorServer server;
    g_server = &server;
    SetConsoleCtrlHandler(ConsoleControlHandler, TRUE);

    std::string error;
    if (!server.Start(port, error))
    {
        std::cerr << "Simulator start failed: " << error << std::endl;
        return 1;
    }

    std::cout << "Santec integrated polarity simulator listening on TCP port " << port << std::endl;
    std::cout << "Profiles: PASS, CROSS, OPEN, SHORT, REVERSE" << std::endl;
    std::cout << "Press Ctrl+C to stop." << std::endl;
    server.Run();
    g_server = NULL;
    return 0;
}
