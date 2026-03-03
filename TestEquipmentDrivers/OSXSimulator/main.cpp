#include "stdafx.h"
#include "COSXSimServer.h"

static COSXSimServer g_server;

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
    printf("     OSX Optical Switch Simulator v1.0\n");
    printf("     (SCPI Protocol per M-OSX_SX1-001.04)\n");
    printf("====================================================\n");
}

static void PrintStatus()
{
    printf("  Modules:      %d\n", g_server.GetModuleCount());
    const std::vector<COSXSimServer::SimModule>& mods = g_server.GetModules();
    for (size_t i = 0; i < mods.size(); ++i)
    {
        printf("    [%d] %s  ch=%d/%d  common=%d\n",
            (int)i, mods[i].name.c_str(),
            mods[i].currentChannel, mods[i].channelCount,
            mods[i].currentCommon);
    }
    printf("  Switch delay: %d ms\n", g_server.GetSwitchDelayMs());
    printf("  Error mode:   %s\n", g_server.GetErrorMode() ? "ON" : "OFF");
    printf("  Verbose:      %s\n", g_server.GetVerbose() ? "ON" : "OFF");
    printf("  Commands:     %d received\n", g_server.GetCommandCount());
}

static void PrintHelp()
{
    printf("\nCommands:\n");
    printf("  s  - Show status\n");
    printf("  v  - Toggle verbose logging\n");
    printf("  e  - Toggle error injection mode\n");
    printf("  d  - Set switch delay (ms)\n");
    printf("  a  - Add a module\n");
    printf("  c  - Clear all modules and add defaults\n");
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
    for (int i = 1; i < argc; ++i)
    {
        if (std::string(argv[i]) == "--port" && i + 1 < argc)
            port = atoi(argv[++i]);
    }

    PrintBanner();
    printf("  Port: %d\n", port);
    printf("  Default modules:\n");
    const std::vector<COSXSimServer::SimModule>& mods = g_server.GetModules();
    for (size_t i = 0; i < mods.size(); ++i)
        printf("    [%d] %s (%d channels)\n", (int)i, mods[i].name.c_str(), mods[i].channelCount);
    printf("\n");

    if (!g_server.Start(port))
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
        case 's':
        case 'S':
            PrintStatus();
            break;
        case 'v':
        case 'V':
            g_server.SetVerbose(!g_server.GetVerbose());
            printf("Verbose: %s\n", g_server.GetVerbose() ? "ON" : "OFF");
            break;
        case 'e':
        case 'E':
            g_server.SetErrorMode(!g_server.GetErrorMode());
            printf("Error mode: %s\n", g_server.GetErrorMode() ? "ON" : "OFF");
            break;
        case 'd':
        case 'D':
        {
            printf("Enter switch delay (ms): ");
            char delayBuf[64];
            if (fgets(delayBuf, sizeof(delayBuf), stdin))
            {
                int delay = atoi(delayBuf);
                if (delay >= 0 && delay <= 60000)
                {
                    g_server.SetSwitchDelayMs(delay);
                    printf("Switch delay set to %d ms.\n", delay);
                }
            }
            break;
        }
        case 'a':
        case 'A':
        {
            printf("Module name (e.g. SX 2Bx12): ");
            char nameBuf[128];
            if (!fgets(nameBuf, sizeof(nameBuf), stdin)) break;
            std::string name(nameBuf);
            while (!name.empty() && (name.back() == '\n' || name.back() == '\r'))
                name.pop_back();

            printf("Config type (0=1A, 1=2A, 2=2B, 3=2C): ");
            char typeBuf[16];
            if (!fgets(typeBuf, sizeof(typeBuf), stdin)) break;
            int cfgType = atoi(typeBuf);

            printf("Channel count: ");
            char chBuf[16];
            if (!fgets(chBuf, sizeof(chBuf), stdin)) break;
            int chCount = atoi(chBuf);

            if (!name.empty() && chCount > 0)
            {
                g_server.AddModule(name, cfgType, chCount);
                printf("Added module: %s (%d channels)\n", name.c_str(), chCount);
            }
            break;
        }
        case 'c':
        case 'C':
        {
            g_server.ClearModules();
            g_server.AddModule("SX 1Ax24", 0, 24);
            printf("Modules reset to default (1x SX 1Ax24).\n");
            break;
        }
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

    printf("OSX simulator stopped.\n");
    return 0;
}
