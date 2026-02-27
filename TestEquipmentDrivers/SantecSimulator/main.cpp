#include "stdafx.h"
#include "SantecSimServer.h"

static CSantecSimServer g_server;

static BOOL WINAPI ConsoleHandler(DWORD ctrlType)
{
    if (ctrlType == CTRL_C_EVENT || ctrlType == CTRL_CLOSE_EVENT)
    {
        printf("\nShutting down...\n");
        g_server.Stop();
        return TRUE;
    }
    return FALSE;
}

static void PrintBanner()
{
    printf("\n");
    printf("====================================================\n");
    printf("     Santec RL1 Simulator v2.0\n");
    printf("     (Official SCPI Protocol per M-RL1-001-07)\n");
    printf("====================================================\n");
}

static void PrintStatus()
{
    printf("  Model:       %s\n", g_server.GetModel() == CSantecSimServer::SIM_RL1 ? "RL1 (IL+RL)" : "ILM-100 (IL only)");
    printf("  Error mode:  %s\n", g_server.GetErrorMode() ? "ON  (injecting errors)" : "OFF");
    printf("  Meas delay:  %d ms\n", g_server.GetMeasDelayMs());
    printf("  Verbose:     %s\n", g_server.GetVerbose() ? "ON" : "OFF");
    printf("  Commands:    %d received\n", g_server.GetCommandCount());
}

static void PrintHelp()
{
    printf("\nCommands:\n");
    printf("  e  - Toggle error injection mode\n");
    printf("  d  - Set measurement delay (ms)\n");
    printf("  v  - Toggle verbose logging\n");
    printf("  m  - Switch model (RL1 / ILM-100)\n");
    printf("  s  - Show status\n");
    printf("  q  - Quit\n\n");
}

int main(int argc, char* argv[])
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("WSAStartup failed.\n");
        return 1;
    }

    SetConsoleCtrlHandler(ConsoleHandler, TRUE);

    int port = 5025;
    CSantecSimServer::SimModel model = CSantecSimServer::SIM_RL1;

    for (int i = 1; i < argc; ++i)
    {
        if (std::string(argv[i]) == "--port" && i + 1 < argc)
            port = atoi(argv[++i]);
        else if (std::string(argv[i]) == "--ilm")
            model = CSantecSimServer::SIM_ILM_100;
        else if (std::string(argv[i]) == "--rl1" || std::string(argv[i]) == "--rlm")
            model = CSantecSimServer::SIM_RL1;
    }

    PrintBanner();
    printf("  Port: %d\n", port);
    printf("  Model: %s\n\n", model == CSantecSimServer::SIM_RL1 ? "RL1" : "ILM-100");

    if (!g_server.Start(port, model))
    {
        printf("Failed to start server.\n");
        WSACleanup();
        return 1;
    }

    g_server.SetVerbose(true);
    PrintHelp();

    bool running = true;
    while (running && g_server.IsRunning())
    {
        printf("> ");
        char input[256];
        if (!fgets(input, sizeof(input), stdin))
            break;

        char ch = input[0];
        switch (ch)
        {
        case 'e':
        case 'E':
            g_server.SetErrorMode(!g_server.GetErrorMode());
            printf("Error mode: %s\n", g_server.GetErrorMode() ? "ON" : "OFF");
            break;
        case 'd':
        case 'D':
        {
            printf("Enter delay (ms): ");
            char delayBuf[64];
            if (fgets(delayBuf, sizeof(delayBuf), stdin))
            {
                int delay = atoi(delayBuf);
                if (delay >= 0 && delay <= 60000)
                {
                    g_server.SetMeasDelayMs(delay);
                    printf("Measurement delay set to %d ms.\n", delay);
                }
            }
            break;
        }
        case 'v':
        case 'V':
            g_server.SetVerbose(!g_server.GetVerbose());
            printf("Verbose: %s\n", g_server.GetVerbose() ? "ON" : "OFF");
            break;
        case 'm':
        case 'M':
        {
            CSantecSimServer::SimModel cur = g_server.GetModel();
            CSantecSimServer::SimModel next = (cur == CSantecSimServer::SIM_RL1)
                ? CSantecSimServer::SIM_ILM_100
                : CSantecSimServer::SIM_RL1;
            g_server.SetModel(next);
            printf("Model switched to: %s\n",
                next == CSantecSimServer::SIM_RL1 ? "RL1 (IL+RL)" : "ILM-100 (IL only)");
            break;
        }
        case 's':
        case 'S':
            PrintStatus();
            break;
        case 'q':
        case 'Q':
            running = false;
            break;
        case '?':
        case 'h':
        case 'H':
            PrintHelp();
            break;
        default:
            break;
        }
    }

    g_server.Stop();
    WSACleanup();

    printf("Santec simulator stopped.\n");
    return 0;
}
