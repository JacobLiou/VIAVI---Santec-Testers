#include "stdafx.h"
#include "CViaviOSWSimServer.h"

static CViaviOSWSimServer g_server;

static BOOL WINAPI ConsoleHandler(DWORD ctrlType)
{
    if (ctrlType == CTRL_C_EVENT || ctrlType == CTRL_CLOSE_EVENT)
    {
        printf("\n正在关闭...\n");
        g_server.Stop();
        return TRUE;
    }
    return FALSE;
}

static void PrintBanner()
{
    printf("\n");
    printf("====================================================\n");
    printf("     Viavi MAP mOSW-C1 Simulator v1.0\n");
    printf("     (SCPI over TCP - Standard Routing Commands)\n");
    printf("====================================================\n");
}

static void PrintStatus()
{
    printf("  错误注入:  %s\n", g_server.GetErrorMode() ? "开启（注入错误）" : "关闭");
    printf("  开关延迟:  %d ms\n", g_server.GetSwitchDelayMs());
    printf("  详细日志:  %s\n", g_server.GetVerbose() ? "开启" : "关闭");
    printf("  命令计数:  %d\n", g_server.GetCommandCount());
}

static void PrintHelp()
{
    printf("\n命令:\n");
    printf("  e  - 切换错误注入模式\n");
    printf("  d  - 设置开关延迟 (ms)\n");
    printf("  v  - 切换详细日志\n");
    printf("  s  - 显示状态\n");
    printf("  q  - 退出\n\n");
}

int main(int argc, char* argv[])
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("WSAStartup 失败。\n");
        return 1;
    }

    SetConsoleCtrlHandler(ConsoleHandler, TRUE);

    int port = 8203;

    for (int i = 1; i < argc; ++i)
    {
        if (std::string(argv[i]) == "--port" && i + 1 < argc)
            port = atoi(argv[++i]);
    }

    PrintBanner();
    printf("  端口: %d\n\n", port);

    if (!g_server.Start(port))
    {
        printf("启动服务器失败。\n");
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
            printf("错误注入: %s\n", g_server.GetErrorMode() ? "开启" : "关闭");
            break;
        case 'd':
        case 'D':
        {
            printf("输入延迟 (ms): ");
            char delayBuf[64];
            if (fgets(delayBuf, sizeof(delayBuf), stdin))
            {
                int delay = atoi(delayBuf);
                if (delay >= 0 && delay <= 60000)
                {
                    g_server.SetSwitchDelayMs(delay);
                    printf("开关延迟已设为 %d ms。\n", delay);
                }
            }
            break;
        }
        case 'v':
        case 'V':
            g_server.SetVerbose(!g_server.GetVerbose());
            printf("详细日志: %s\n", g_server.GetVerbose() ? "开启" : "关闭");
            break;
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

    printf("Viavi OSW 模拟器已停止。\n");
    return 0;
}
