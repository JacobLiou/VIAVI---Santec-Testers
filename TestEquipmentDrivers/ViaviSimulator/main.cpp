#include "stdafx.h"
#include "ViaviSimServer.h"

static CViaviSimServer g_server;

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
    printf("================================================\n");
    printf("     VIAVI MAP300 PCT Simulator v1.0\n");
    printf("================================================\n");
}

static void PrintStatus()
{
    printf("  Error mode:  %s\n", g_server.GetErrorMode() ? "ON  (injecting errors)" : "OFF");
    printf("  Meas delay:  %d ms\n", g_server.GetMeasurementDelay());
    printf("  Verbose:     %s\n", g_server.GetVerbose() ? "ON" : "OFF");
    printf("  Commands:    %d received\n", g_server.GetCommandCount());
    printf("------------------------------------------------\n");
}

static void PrintMenu()
{
    printf("------------------------------------------------\n");
    printf("  [E] Toggle error mode   [D] Set meas delay\n");
    printf("  [V] Toggle verbose      [S] Show status\n");
    printf("  [Q] Quit\n");
    printf("================================================\n\n");
}

int main(int argc, char* argv[])
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("WSAStartup failed.\n");
        return 1;
    }

    srand(static_cast<unsigned>(time(nullptr)));
    SetConsoleCtrlHandler(ConsoleHandler, TRUE);

    int chassisPort = 8100;
    int pctPort = 8301;

    // 允许通过命令行覆盖端口：
    //   ViaviSimulator.exe [chassisPort] [pctPort]
    if (argc >= 2) chassisPort = atoi(argv[1]);
    if (argc >= 3) pctPort = atoi(argv[2]);

    PrintBanner();
    printf("  Chassis port: %d\n", chassisPort);
    printf("  PCT port:     %d\n", pctPort);
    printf("------------------------------------------------\n");

    if (!g_server.Start(chassisPort, pctPort))
    {
        printf("Failed to start server. Check if ports are in use.\n");
        WSACleanup();
        return 1;
    }

    PrintStatus();
    PrintMenu();

    while (g_server.IsRunning())
    {
        if (_kbhit())
        {
            int ch = _getch();

            switch (toupper(ch))
            {
            case 'E':
            {
                bool newState = !g_server.GetErrorMode();
                g_server.SetErrorMode(newState);
                printf("\n>> Error mode: %s\n\n", newState ? "ON" : "OFF");
                break;
            }
            case 'D':
            {
                printf("\n>> Enter measurement delay (ms): ");
                char buf[32];
                if (fgets(buf, sizeof(buf), stdin))
                {
                    int delay = atoi(buf);
                    if (delay >= 0 && delay <= 60000)
                    {
                        g_server.SetMeasurementDelay(delay);
                        printf(">> Measurement delay set to %d ms\n\n", delay);
                    }
                    else
                    {
                        printf(">> Invalid value (0-60000)\n\n");
                    }
                }
                break;
            }
            case 'V':
            {
                bool newState = !g_server.GetVerbose();
                g_server.SetVerbose(newState);
                printf("\n>> Verbose mode: %s\n\n", newState ? "ON" : "OFF");
                break;
            }
            case 'S':
                printf("\n");
                PrintStatus();
                printf("\n");
                break;

            case 'Q':
                printf("\nShutting down...\n");
                g_server.Stop();
                break;
            }
        }
        else
        {
            Sleep(50);
        }
    }

    printf("Server stopped.\n");
    WSACleanup();
    return 0;
}
