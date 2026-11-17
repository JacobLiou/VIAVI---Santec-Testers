#include "stdafx.h"
#include "PTMSimServer.h"

static CPTMSimServer g_server;

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

static void PrintHelp()
{
    printf("\nCommands:\n");
    printf("  e  - Toggle error injection mode\n");
    printf("  d  - Set measurement delay (ms)\n");
    printf("  c  - Set channel count\n");
    printf("  v  - Toggle verbose logging\n");
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
    int channels = 12;

    for (int i = 1; i < argc; ++i)
    {
        if (std::string(argv[i]) == "--port" && i + 1 < argc)
            port = atoi(argv[++i]);
        else if (std::string(argv[i]) == "--channels" && i + 1 < argc)
            channels = atoi(argv[++i]);
    }

    printf("\n====================================================\n");
    printf("     Santec PTM Simulator v1.0\n");
    printf("     (SCPI per M-PTM_PT1-001.04 Ch.8)\n");
    printf("====================================================\n");
    printf("  Port: %d\n", port);
    printf("  Channels: %d\n\n", channels);

    if (!g_server.Start(port, channels))
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

        switch (input[0])
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
        case 'c':
        case 'C':
        {
            printf("Enter channel count: ");
            char buf[64];
            if (fgets(buf, sizeof(buf), stdin))
            {
                int n = atoi(buf);
                if (n > 0 && n <= 288)
                {
                    g_server.SetChannels(n);
                    printf("Channels set to %d.\n", n);
                }
            }
            break;
        }
        case 'v':
        case 'V':
            g_server.SetVerbose(!g_server.GetVerbose());
            printf("Verbose: %s\n", g_server.GetVerbose() ? "ON" : "OFF");
            break;
        case 's':
        case 'S':
            printf("  Channels: %d\n", g_server.GetChannels());
            printf("  Delay:    %d ms\n", g_server.GetMeasDelayMs());
            printf("  Error:    %s\n", g_server.GetErrorMode() ? "ON" : "OFF");
            printf("  Verbose:  %s\n", g_server.GetVerbose() ? "ON" : "OFF");
            printf("  Commands: %d\n", g_server.GetCommandCount());
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
    printf("PTM simulator stopped.\n");
    return 0;
}
