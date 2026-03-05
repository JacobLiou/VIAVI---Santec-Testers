#include "stdafx.h"
#include "OSXDllLoader.h"

// ---------------------------------------------------------------------------
// OSXTestApp2 -- UDL.SantecOSX.dll 的动态加载调试工具
//
// 演示生产框架使用的确切模式：
//   1. LoadLibrary("UDL.SantecOSX.dll")
//   2. 对每个 OSX_* 函数调用 GetProcAddress
//   3. 通过函数指针进行调用
//   4. 退出时调用 FreeLibrary
//
// 不包含 UDL.SantecOSX 的任何 .h 文件。不链接任何 .lib 文件。
// ---------------------------------------------------------------------------

static COSXDllLoader g_loader;

// ---------------------------------------------------------------------------
// 日志回调（将从 DLL 内部调用）
// ---------------------------------------------------------------------------
static void WINAPI LogHandler(int level, const char* source, const char* message)
{
    const char* tag = "???";
    switch (level)
    {
    case 0: tag = "DBG"; break;
    case 1: tag = "INF"; break;
    case 2: tag = "WRN"; break;
    case 3: tag = "ERR"; break;
    }
    printf("  [%s][%s] %s\n", tag, source, message);
}

// ---------------------------------------------------------------------------
// 辅助函数
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// 菜单
// ---------------------------------------------------------------------------

static void PrintBanner()
{
    printf("\n");
    printf("====================================================\n");
    printf("  OSXTestApp2 -- Dynamic Loading Debug Tool v1.0\n");
    printf("  (LoadLibrary / GetProcAddress, no static import)\n");
    printf("====================================================\n\n");
}

static void PrintHelp()
{
    printf("\n--- DLL Management ---\n");
    printf("  0  - Load UDL.SantecOSX.dll\n");
    printf("  00 - Unload DLL\n");
    printf("\n--- Connection ---\n");
    printf("  1  - Create driver + Connect (TCP)\n");
    printf("  1v - Create driver + Connect (USB/VISA)\n");
    printf("  1e - Enumerate VISA resources\n");
    printf("  2  - Disconnect + Destroy driver\n");
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
    printf("  60 - Send raw SCPI command\n");
    printf("\n--- Other ---\n");
    printf("  h  - Help\n");
    printf("  q  - Quit\n\n");
}

// ---------------------------------------------------------------------------
// 守卫：检查 DLL 是否已加载 + 驱动是否已连接
// ---------------------------------------------------------------------------

static bool CheckDll()
{
    if (!g_loader.IsDllLoaded())
    {
        printf("DLL not loaded. Use '0' to load first.\n");
        return false;
    }
    return true;
}

static bool CheckConnected()
{
    if (!CheckDll()) return false;
    if (!g_loader.GetDriverHandle() || !g_loader.IsConnected())
    {
        printf("Not connected. Use '1' to connect first.\n");
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// 命令
// ---------------------------------------------------------------------------

static void DoLoadDll()
{
    if (g_loader.IsDllLoaded())
    {
        printf("DLL already loaded.\n");
        return;
    }

    std::string path = ReadLine("DLL path [UDL.SantecOSX.dll]: ");
    if (path.empty()) path = "UDL.SantecOSX.dll";

    if (g_loader.LoadDll(path.c_str()))
    {
        printf("DLL loaded successfully.\n");
        g_loader.SetLogCallback(LogHandler);
        printf("Log callback registered.\n");
    }
    else
    {
        printf("Failed to load DLL.\n");
    }
}

static void DoUnloadDll()
{
    if (!g_loader.IsDllLoaded())
    {
        printf("DLL not loaded.\n");
        return;
    }
    g_loader.DestroyDriver();
    g_loader.UnloadDll();
}

static void DoConnect()
{
    if (!CheckDll()) return;
    if (g_loader.GetDriverHandle())
    {
        printf("Driver already created. Destroy first.\n");
        return;
    }

    std::string ip = ReadLine("IP address [127.0.0.1]: ");
    if (ip.empty()) ip = "127.0.0.1";
    int port = ReadInt("Port [5025]: ", 5025);

    printf("Creating driver for %s:%d ...\n", ip.c_str(), port);
    if (!g_loader.CreateDriver(ip.c_str(), port))
    {
        printf("OSX_CreateDriver failed.\n");
        return;
    }

    printf("Connecting...\n");
    if (g_loader.Connect())
    {
        printf("Connected!\n");

        OSXDeviceInfo di;
        memset(&di, 0, sizeof(di));
        if (g_loader.GetDeviceInfo(&di))
        {
            printf("  Manufacturer: %s\n", di.manufacturer);
            printf("  Model:        %s\n", di.model);
            printf("  Serial:       %s\n", di.serialNumber);
            printf("  Firmware:     %s\n", di.firmwareVersion);
        }
    }
    else
    {
        printf("Connection failed.\n");
        g_loader.DestroyDriver();
    }
}

static void DoConnectVisa()
{
    if (!CheckDll()) return;
    if (g_loader.GetDriverHandle())
    {
        printf("Driver already created. Destroy first.\n");
        return;
    }
    if (!g_loader.HasVisaSupport())
    {
        printf("VISA support not available in this DLL version.\n");
        return;
    }

    printf("Enter VISA resource string\n");
    printf("  Example: USB0::0x1698::0x0337::SN12345::INSTR\n");
    std::string rsrc = ReadLine("VISA resource: ");
    if (rsrc.empty())
    {
        printf("No resource string entered.\n");
        return;
    }

    printf("Creating driver (VISA/USB) for %s ...\n", rsrc.c_str());
    if (!g_loader.CreateDriverEx(rsrc.c_str(), 0, 2))
    {
        printf("OSX_CreateDriverEx failed.\n");
        return;
    }

    printf("Connecting...\n");
    if (g_loader.Connect())
    {
        printf("Connected via VISA!\n");

        OSXDeviceInfo di;
        memset(&di, 0, sizeof(di));
        if (g_loader.GetDeviceInfo(&di))
        {
            printf("  Manufacturer: %s\n", di.manufacturer);
            printf("  Model:        %s\n", di.model);
            printf("  Serial:       %s\n", di.serialNumber);
            printf("  Firmware:     %s\n", di.firmwareVersion);
        }
    }
    else
    {
        printf("VISA connection failed.\n");
        g_loader.DestroyDriver();
    }
}

static void DoEnumerateVisa()
{
    if (!CheckDll()) return;
    if (!g_loader.HasVisaSupport())
    {
        printf("VISA support not available in this DLL version.\n");
        return;
    }

    char buffer[4096] = {0};
    int count = g_loader.EnumerateVisaResources(buffer, sizeof(buffer));
    if (count <= 0)
    {
        printf("No VISA resources found (or VISA not installed).\n");
        return;
    }

    printf("Found %d VISA resource(s):\n", count);
    std::string list(buffer);
    size_t pos = 0;
    int idx = 1;
    while (pos < list.size())
    {
        size_t semi = list.find(';', pos);
        if (semi == std::string::npos) semi = list.size();
        printf("  [%d] %s\n", idx++, list.substr(pos, semi - pos).c_str());
        pos = semi + 1;
    }
}

static void DoDisconnect()
{
    if (!g_loader.GetDriverHandle())
    {
        printf("No driver instance.\n");
        return;
    }
    g_loader.Disconnect();
    g_loader.DestroyDriver();
    printf("Disconnected and driver destroyed.\n");
}

// ---------------------------------------------------------------------------
// 主函数
// ---------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    PrintBanner();
    PrintHelp();

    bool running = true;
    while (running)
    {
        std::string input = ReadLine("> ");
        if (input.empty()) continue;

        // 特殊的字符串命令
        if (input == "00") { DoUnloadDll(); continue; }
        if (input == "1v" || input == "1V") { DoConnectVisa(); continue; }
        if (input == "1e" || input == "1E") { DoEnumerateVisa(); continue; }

        int cmdNum = -1;
        if (input[0] >= '0' && input[0] <= '9')
            cmdNum = atoi(input.c_str());

        switch (cmdNum)
        {
        // --- DLL 管理 ---
        case 0:
            DoLoadDll();
            break;

        // --- 连接 ---
        case 1:  DoConnect(); break;
        case 2:  DoDisconnect(); break;
        case 3:
            if (CheckDll())
            {
                printf("DLL loaded: YES\n");
                printf("Driver created: %s\n", g_loader.GetDriverHandle() ? "YES" : "NO");
                printf("Connected: %s\n", g_loader.IsConnected() ? "YES" : "NO");
            }
            break;

        // --- 设备信息 ---
        case 10:
            if (CheckConnected())
            {
                OSXDeviceInfo di;
                memset(&di, 0, sizeof(di));
                if (g_loader.GetDeviceInfo(&di))
                {
                    printf("  Manufacturer: %s\n", di.manufacturer);
                    printf("  Model:        %s\n", di.model);
                    printf("  Serial:       %s\n", di.serialNumber);
                    printf("  Firmware:     %s\n", di.firmwareVersion);
                }
                else
                    printf("  GetDeviceInfo failed.\n");
            }
            break;

        case 11:
            if (CheckConnected())
            {
                char msg[256] = {};
                int code = g_loader.CheckError(msg, sizeof(msg));
                printf("  Error code: %d\n", code);
                printf("  Message:    %s\n", msg);
            }
            break;

        case 12:
            if (CheckConnected())
            {
                char ver[128] = {};
                if (g_loader.GetSystemVersion(ver, sizeof(ver)))
                    printf("  Version: %s\n", ver);
                else
                    printf("  GetSystemVersion failed.\n");
            }
            break;

        case 13:
            if (CheckConnected())
            {
                char ip[64] = {}, gw[64] = {}, mask[64] = {}, host[64] = {}, mac[64] = {};
                if (g_loader.GetNetworkInfo(ip, gw, mask, host, mac, 64))
                {
                    printf("  IP:       %s\n", ip);
                    printf("  Gateway:  %s\n", gw);
                    printf("  Netmask:  %s\n", mask);
                    printf("  Hostname: %s\n", host);
                    printf("  MAC:      %s\n", mac);
                }
                else
                    printf("  GetNetworkInfo failed.\n");
            }
            break;

        // --- 模块 ---
        case 20:
            if (CheckConnected())
                printf("  Module count: %d\n", g_loader.GetModuleCount());
            break;

        case 21:
            if (CheckConnected())
            {
                OSXModuleInfo mods[16];
                memset(mods, 0, sizeof(mods));
                int count = g_loader.GetModuleCatalog(mods, 16);
                printf("  %d module(s):\n", count);
                for (int i = 0; i < count; ++i)
                {
                    printf("    [%d] %s  (config=%d, channels=%d)\n",
                           mods[i].index, mods[i].catalogEntry,
                           mods[i].configType, mods[i].channelCount);
                    if (mods[i].detailedInfo[0])
                        printf("         Detail: %s\n", mods[i].detailedInfo);
                }
            }
            break;

        case 22:
            if (CheckConnected())
            {
                int idx = ReadInt("Module index: ", 0);
                OSXModuleInfo mi;
                memset(&mi, 0, sizeof(mi));
                if (g_loader.GetModuleInfo(idx, &mi))
                    printf("  Module %d: %s\n", mi.index, mi.detailedInfo);
                else
                    printf("  GetModuleInfo failed.\n");
            }
            break;

        case 23:
            if (CheckConnected())
            {
                int idx = ReadInt("Module index: ", 0);
                if (g_loader.SelectModule(idx))
                    printf("  Selected module %d.\n", idx);
                else
                    printf("  SelectModule failed.\n");
            }
            break;

        case 24:
            if (CheckConnected())
                printf("  Selected module: %d\n", g_loader.GetSelectedModule());
            break;

        // --- 通道切换 ---
        case 30:
            if (CheckConnected())
            {
                int ch = ReadInt("Channel #: ", 1);
                DWORD t0 = GetTickCount();
                BOOL ok = g_loader.SwitchChannel(ch);
                DWORD dt = GetTickCount() - t0;
                if (ok)
                    printf("  Switched to channel %d (%lu ms).\n", ch, dt);
                else
                    printf("  SwitchChannel failed (%lu ms).\n", dt);
            }
            break;

        case 31:
            if (CheckConnected())
            {
                DWORD t0 = GetTickCount();
                BOOL ok = g_loader.SwitchNext();
                DWORD dt = GetTickCount() - t0;
                if (ok)
                    printf("  Switched to next channel (%lu ms).\n", dt);
                else
                    printf("  SwitchNext failed (%lu ms).\n", dt);
            }
            break;

        case 32:
            if (CheckConnected())
                printf("  Current channel: %d\n", g_loader.GetCurrentChannel());
            break;

        case 33:
            if (CheckConnected())
                printf("  Channel count: %d\n", g_loader.GetChannelCount());
            break;

        // --- 路由 ---
        case 40:
            if (CheckConnected())
            {
                int mod = ReadInt("Module index: ", 0);
                int ch  = ReadInt("Channel #: ", 1);
                DWORD t0 = GetTickCount();
                BOOL ok = g_loader.RouteChannel(mod, ch);
                DWORD dt = GetTickCount() - t0;
                if (ok)
                    printf("  Routed module %d -> channel %d (%lu ms).\n", mod, ch, dt);
                else
                    printf("  RouteChannel failed (%lu ms).\n", dt);
            }
            break;

        case 41:
            if (CheckConnected())
            {
                int mod = ReadInt("Module index: ", 0);
                printf("  Route channel: %d\n", g_loader.GetRouteChannel(mod));
            }
            break;

        case 42:
            if (CheckConnected())
            {
                int ch = ReadInt("Channel #: ", 1);
                DWORD t0 = GetTickCount();
                BOOL ok = g_loader.RouteAllModules(ch);
                DWORD dt = GetTickCount() - t0;
                if (ok)
                    printf("  Routed all modules -> channel %d (%lu ms).\n", ch, dt);
                else
                    printf("  RouteAllModules failed (%lu ms).\n", dt);
            }
            break;

        case 43:
            if (CheckConnected())
            {
                int mod = ReadInt("Module index: ", 0);
                int common = ReadInt("Common input #: ", 0);
                if (g_loader.SetCommonInput(mod, common))
                    printf("  Module %d common input set to %d.\n", mod, common);
                else
                    printf("  SetCommonInput failed.\n");
            }
            break;

        case 44:
            if (CheckConnected())
            {
                int mod = ReadInt("Module index: ", 0);
                printf("  Common input: %d\n", g_loader.GetCommonInput(mod));
            }
            break;

        case 45:
            if (CheckConnected())
            {
                int mod = ReadInt("Module index: ", 0);
                DWORD t0 = GetTickCount();
                BOOL ok = g_loader.HomeModule(mod);
                DWORD dt = GetTickCount() - t0;
                if (ok)
                    printf("  Module %d homed (%lu ms).\n", mod, dt);
                else
                    printf("  HomeModule failed (%lu ms).\n", dt);
            }
            break;

        // --- 控制 ---
        case 50:
            if (CheckConnected())
            {
                int mode = ReadInt("Mode (1=local, 0=remote): ", 0);
                if (g_loader.SetLocalMode(mode != 0))
                    printf("  Mode set to %s.\n", mode ? "local" : "remote");
                else
                    printf("  SetLocalMode failed.\n");
            }
            break;

        case 51:
            if (CheckConnected())
                printf("  Local mode: %s\n", g_loader.GetLocalMode() ? "YES" : "NO");
            break;

        case 52:
            if (CheckConnected())
            {
                int icon = ReadInt("Icon #: ", 0);
                std::string msg = ReadLine("Message: ");
                if (g_loader.SendNotification(icon, msg.c_str()))
                    printf("  Notification sent.\n");
                else
                    printf("  SendNotification failed.\n");
            }
            break;

        case 53:
            if (CheckConnected())
            {
                if (g_loader.Reset())
                    printf("  Device reset.\n");
                else
                    printf("  Reset failed.\n");
            }
            break;

        // --- 原始SCPI命令 ---
        case 60:
            if (CheckConnected())
            {
                std::string cmd = ReadLine("SCPI command: ");
                char resp[4096] = {};
                if (g_loader.SendCommand(cmd.c_str(), resp, sizeof(resp)))
                    printf("  Response: %s\n", resp);
                else
                    printf("  SendCommand failed.\n");
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

    g_loader.DestroyDriver();
    g_loader.UnloadDll();

    printf("OSXTestApp2 stopped.\n");
    return 0;
}
