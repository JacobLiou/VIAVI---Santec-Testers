#include "stdafx.h"
#include "../UDL.OSX/COSXDriver.h"

using namespace OSXSwitch;

// ---------------------------------------------------------------------------
// OSX 光开关驱动的交互式控制台调试工具。
// 连接到真实的 OSX 设备或 localhost:5025 上的 OSXSimulator。
// ---------------------------------------------------------------------------

static COSXDriver* g_driver = nullptr;

static void WINAPI LogHandler(LogLevel level, const std::string& source, const std::string& message)
{
    const char* levelStr = "???";
    switch (level)
    {
    case LOG_DEBUG:   levelStr = "DBG"; break;
    case LOG_INFO:    levelStr = "INF"; break;
    case LOG_WARNING: levelStr = "WRN"; break;
    case LOG_ERROR:   levelStr = "ERR"; break;
    }
    printf("  [%s][%s] %s\n", levelStr, source.c_str(), message.c_str());
}

static std::string ReadLine(const char* prompt)
{
    printf("%s", prompt);
    char buf[256];
    if (!fgets(buf, sizeof(buf), stdin))
        return "";
    std::string s(buf);
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r'))
        s.pop_back();
    return s;
}

static int ReadInt(const char* prompt, int defaultVal)
{
    std::string s = ReadLine(prompt);
    if (s.empty()) return defaultVal;
    return atoi(s.c_str());
}

static void PrintBanner()
{
    printf("\n");
    printf("====================================================\n");
    printf("     OSX Optical Switch Debug Tool v1.0\n");
    printf("====================================================\n\n");
}

static void PrintHelp()
{
    printf("\n--- Connection ---\n");
    printf("  1  - Connect\n");
    printf("  2  - Disconnect\n");
    printf("  3  - Show connection status\n");
    printf("\n--- Device Info ---\n");
    printf("  10 - Get device info (*IDN?)\n");
    printf("  11 - Check error (SYST:ERR?)\n");
    printf("  12 - Get system version\n");
    printf("  13 - Get network info\n");
    printf("\n--- Modules ---\n");
    printf("  20 - Get module count\n");
    printf("  21 - Get module catalog\n");
    printf("  22 - Get module info (by index)\n");
    printf("  23 - Select module\n");
    printf("  24 - Get selected module\n");
    printf("\n--- Channel Switching ---\n");
    printf("  30 - Switch to channel #\n");
    printf("  31 - Switch to next channel\n");
    printf("  32 - Get current channel\n");
    printf("  33 - Get channel count\n");
    printf("\n--- Routing ---\n");
    printf("  40 - Route module to channel\n");
    printf("  41 - Get route channel\n");
    printf("  42 - Route all modules to channel\n");
    printf("  43 - Set common input\n");
    printf("  44 - Get common input\n");
    printf("  45 - Home module\n");
    printf("\n--- Control ---\n");
    printf("  50 - Set local/remote mode\n");
    printf("  51 - Get local mode\n");
    printf("  52 - Send notification\n");
    printf("  53 - Reset (*RST)\n");
    printf("\n--- Raw SCPI ---\n");
    printf("  60 - Send raw query\n");
    printf("  61 - Send raw write\n");
    printf("\n--- Other ---\n");
    printf("  h  - Help\n");
    printf("  q  - Quit\n\n");
}

static void DoConnect()
{
    if (g_driver)
    {
        printf("Already have a driver instance. Disconnect first.\n");
        return;
    }

    std::string ip = ReadLine("IP address [127.0.0.1]: ");
    if (ip.empty()) ip = "127.0.0.1";

    int port = ReadInt("Port [5025]: ", 5025);

    printf("Connecting to %s:%d...\n", ip.c_str(), port);
    g_driver = new COSXDriver(ip, port);

    if (g_driver->Connect())
    {
        printf("Connected successfully.\n");

        DeviceInfo di = g_driver->GetDeviceInfo();
        printf("  Manufacturer: %s\n", di.manufacturer.c_str());
        printf("  Model:        %s\n", di.model.c_str());
        printf("  Serial:       %s\n", di.serialNumber.c_str());
        printf("  Firmware:     %s\n", di.firmwareVersion.c_str());
    }
    else
    {
        printf("Connection failed.\n");
        delete g_driver;
        g_driver = nullptr;
    }
}

static void DoDisconnect()
{
    if (!g_driver)
    {
        printf("Not connected.\n");
        return;
    }
    g_driver->Disconnect();
    delete g_driver;
    g_driver = nullptr;
    printf("Disconnected.\n");
}

static bool CheckConnected()
{
    if (!g_driver || !g_driver->IsConnected())
    {
        printf("Not connected. Use '1' to connect first.\n");
        return false;
    }
    return true;
}

int main(int argc, char* argv[])
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("WSAStartup failed.\n");
        return 1;
    }

    COSXDriver::SetGlobalLogCallback(LogHandler);
    COSXDriver::SetGlobalLogLevel(LOG_INFO);

    PrintBanner();
    PrintHelp();

    bool running = true;
    while (running)
    {
        std::string input = ReadLine("> ");
        if (input.empty()) continue;

        try
        {
            // 解析数字命令
            int cmdNum = -1;
            if (input[0] >= '0' && input[0] <= '9')
                cmdNum = atoi(input.c_str());

            switch (cmdNum)
            {
            case 1: DoConnect(); break;
            case 2: DoDisconnect(); break;
            case 3:
                printf("Connected: %s\n",
                    (g_driver && g_driver->IsConnected()) ? "YES" : "NO");
                break;

            case 10:
                if (CheckConnected())
                {
                    DeviceInfo di = g_driver->GetDeviceInfo();
                    printf("  Manufacturer: %s\n", di.manufacturer.c_str());
                    printf("  Model:        %s\n", di.model.c_str());
                    printf("  Serial:       %s\n", di.serialNumber.c_str());
                    printf("  Firmware:     %s\n", di.firmwareVersion.c_str());
                }
                break;

            case 11:
                if (CheckConnected())
                {
                    ErrorInfo err = g_driver->CheckError();
                    printf("  Error code: %d\n", err.code);
                    printf("  Message:    %s\n", err.message.c_str());
                }
                break;

            case 12:
                if (CheckConnected())
                    printf("  Version: %s\n", g_driver->GetSystemVersion().c_str());
                break;

            case 13:
                if (CheckConnected())
                {
                    printf("  IP:       %s\n", g_driver->GetIPAddress().c_str());
                    printf("  Gateway:  %s\n", g_driver->GetGateway().c_str());
                    printf("  Netmask:  %s\n", g_driver->GetNetmask().c_str());
                    printf("  Hostname: %s\n", g_driver->GetHostname().c_str());
                    printf("  MAC:      %s\n", g_driver->GetMAC().c_str());
                }
                break;

            case 20:
                if (CheckConnected())
                    printf("  Module count: %d\n", g_driver->GetModuleCount());
                break;

            case 21:
                if (CheckConnected())
                {
                    std::vector<ModuleInfo> mods = g_driver->GetModuleCatalog();
                    printf("  %d module(s):\n", (int)mods.size());
                    for (size_t i = 0; i < mods.size(); ++i)
                    {
                        printf("    [%d] %s  (config=%d, channels=%d)\n",
                            mods[i].index, mods[i].catalogEntry.c_str(),
                            (int)mods[i].configType, mods[i].channelCount);
                        if (!mods[i].detailedInfo.empty())
                            printf("         Detail: %s\n", mods[i].detailedInfo.c_str());
                    }
                }
                break;

            case 22:
                if (CheckConnected())
                {
                    int idx = ReadInt("Module index: ", 0);
                    ModuleInfo mi = g_driver->GetModuleInfo(idx);
                    printf("  Module %d: %s\n", mi.index, mi.detailedInfo.c_str());
                }
                break;

            case 23:
                if (CheckConnected())
                {
                    int idx = ReadInt("Module index: ", 0);
                    g_driver->SelectModule(idx);
                    printf("  Selected module %d.\n", idx);
                }
                break;

            case 24:
                if (CheckConnected())
                    printf("  Selected module: %d\n", g_driver->GetSelectedModule());
                break;

            case 30:
                if (CheckConnected())
                {
                    int ch = ReadInt("Channel #: ", 1);
                    DWORD start = GetTickCount();
                    g_driver->SwitchChannel(ch);
                    DWORD elapsed = GetTickCount() - start;
                    printf("  Switched to channel %d (%d ms).\n", ch, elapsed);
                }
                break;

            case 31:
                if (CheckConnected())
                {
                    DWORD start = GetTickCount();
                    g_driver->SwitchNext();
                    DWORD elapsed = GetTickCount() - start;
                    printf("  Switched to next channel (%d ms).\n", elapsed);
                }
                break;

            case 32:
                if (CheckConnected())
                    printf("  Current channel: %d\n", g_driver->GetCurrentChannel());
                break;

            case 33:
                if (CheckConnected())
                    printf("  Channel count: %d\n", g_driver->GetChannelCount());
                break;

            case 40:
                if (CheckConnected())
                {
                    int mod = ReadInt("Module index: ", 0);
                    int ch = ReadInt("Channel #: ", 1);
                    DWORD start = GetTickCount();
                    g_driver->RouteChannel(mod, ch);
                    DWORD elapsed = GetTickCount() - start;
                    printf("  Routed module %d -> channel %d (%d ms).\n", mod, ch, elapsed);
                }
                break;

            case 41:
                if (CheckConnected())
                {
                    int mod = ReadInt("Module index: ", 0);
                    printf("  Route channel: %d\n", g_driver->GetRouteChannel(mod));
                }
                break;

            case 42:
                if (CheckConnected())
                {
                    int ch = ReadInt("Channel #: ", 1);
                    DWORD start = GetTickCount();
                    g_driver->RouteAllModules(ch);
                    DWORD elapsed = GetTickCount() - start;
                    printf("  Routed all modules -> channel %d (%d ms).\n", ch, elapsed);
                }
                break;

            case 43:
                if (CheckConnected())
                {
                    int mod = ReadInt("Module index: ", 0);
                    int common = ReadInt("Common input #: ", 0);
                    g_driver->SetCommonInput(mod, common);
                    printf("  Module %d common input set to %d.\n", mod, common);
                }
                break;

            case 44:
                if (CheckConnected())
                {
                    int mod = ReadInt("Module index: ", 0);
                    printf("  Common input: %d\n", g_driver->GetCommonInput(mod));
                }
                break;

            case 45:
                if (CheckConnected())
                {
                    int mod = ReadInt("Module index: ", 0);
                    DWORD start = GetTickCount();
                    g_driver->HomeModule(mod);
                    DWORD elapsed = GetTickCount() - start;
                    printf("  Module %d homed (%d ms).\n", mod, elapsed);
                }
                break;

            case 50:
                if (CheckConnected())
                {
                    int mode = ReadInt("Mode (1=local, 0=remote): ", 0);
                    g_driver->SetLocalMode(mode != 0);
                    printf("  Mode set to %s.\n", mode ? "local" : "remote");
                }
                break;

            case 51:
                if (CheckConnected())
                    printf("  Local mode: %s\n", g_driver->GetLocalMode() ? "YES" : "NO");
                break;

            case 52:
                if (CheckConnected())
                {
                    int icon = ReadInt("Icon #: ", 0);
                    std::string msg = ReadLine("Message: ");
                    g_driver->SendNotification(icon, msg);
                    printf("  Notification sent.\n");
                }
                break;

            case 53:
                if (CheckConnected())
                {
                    g_driver->Reset();
                    printf("  Device reset.\n");
                }
                break;

            case 60:
                if (CheckConnected())
                {
                    std::string cmd = ReadLine("SCPI query: ");
                    std::string resp = g_driver->SendRawQuery(cmd);
                    printf("  Response: %s\n", resp.c_str());
                }
                break;

            case 61:
                if (CheckConnected())
                {
                    std::string cmd = ReadLine("SCPI write: ");
                    g_driver->SendRawWrite(cmd);
                    printf("  Sent.\n");
                }
                break;

            default:
                if (input == "h" || input == "H" || input == "?")
                    PrintHelp();
                else if (input == "q" || input == "Q")
                    running = false;
                else
                    printf("Unknown command. Type 'h' for help.\n");
                break;
            }
        }
        catch (const std::exception& e)
        {
            printf("  ERROR: %s\n", e.what());
        }
    }

    if (g_driver)
    {
        g_driver->Disconnect();
        delete g_driver;
        g_driver = nullptr;
    }

    WSACleanup();
    printf("OSX debug tool stopped.\n");
    return 0;
}
