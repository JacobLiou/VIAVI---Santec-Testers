#include "stdafx.h"
#include "ViaviSantecDllLoader.h"

// ---------------------------------------------------------------------------
// DriverTestApp2 -- UDL.ViaviNSantecTester.dll 动态加载调试工具
//
// 演示生产框架模式：
//   1. LoadLibrary("UDL.ViaviNSantecTester.dll")
//   2. GetProcAddress 获取每个导出函数
//   3. 通过函数指针调用
//   4. 退出时 FreeLibrary
//
// 不包含 UDL.ViaviNSantecTester 的任何 .h 文件。不链接任何 .lib 文件。
// ---------------------------------------------------------------------------

static CViaviSantecDllLoader g_loader;

// ---------------------------------------------------------------------------
// 日志回调（从DLL内部调用）
// ---------------------------------------------------------------------------
static void WINAPI LogHandler(int level, const char* source, const char* message)
{
    const char* tag = "???";
    switch (level)
    {
    case 0: tag = "调试"; break;
    case 1: tag = "信息"; break;
    case 2: tag = "警告"; break;
    case 3: tag = "错误"; break;
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

static double ReadDouble(const char* prompt, double defaultVal)
{
    std::string s = ReadLine(prompt);
    if (s.empty()) return defaultVal;
    return atof(s.c_str());
}

// ---------------------------------------------------------------------------
// 菜单
// ---------------------------------------------------------------------------

static void PrintBanner()
{
    printf("\n");
    printf("====================================================\n");
    printf("  DriverTestApp2 -- 动态加载调试工具 v1.0\n");
    printf("  Viavi & Santec (LoadLibrary, 无静态导入)\n");
    printf("====================================================\n\n");
}

static void PrintHelp()
{
    printf("\n--- DLL 管理 ---\n");
    printf("  0  - 加载 UDL.ViaviNSantecTester.dll\n");
    printf("  00 - 卸载 DLL\n");
    printf("\n--- 连接 ---\n");
    printf("  1  - 创建驱动 + 连接 + 初始化\n");
    printf("  2  - 断开连接 + 销毁驱动\n");
    printf("  3  - 显示连接状态\n");
    printf("\n--- 设备信息 ---\n");
    printf("  10 - 获取设备信息 (*IDN?)\n");
    printf("  11 - 检查错误\n");
    printf("\n--- 配置 ---\n");
    printf("  20 - 配置波长\n");
    printf("  21 - 配置通道\n");
    printf("  22 - 配置 ORL (仅Viavi)\n");
    printf("\n--- 测量 ---\n");
    printf("  30 - 取参考\n");
    printf("  31 - 执行测量\n");
    printf("  32 - 获取结果\n");
    printf("  33 - 完整周期 (参考 + 测量 + 结果)\n");
    printf("\n--- Santec 专用 ---\n");
    printf("  40 - 设置 RL 灵敏度\n");
    printf("  41 - 设置 DUT 长度档位\n");
    printf("  42 - 设置 RL 增益模式\n");
    printf("  43 - 设置本地模式\n");
    printf("\n--- 原始 SCPI ---\n");
    printf("  60 - 发送原始 SCPI 命令\n");
    printf("\n--- 其他 ---\n");
    printf("  h  - 帮助\n");
    printf("  q  - 退出\n\n");
}

// ---------------------------------------------------------------------------
// 检查函数
// ---------------------------------------------------------------------------

static bool CheckDll()
{
    if (!g_loader.IsDllLoaded())
    {
        printf("DLL 未加载。请先使用 '0' 加载。\n");
        return false;
    }
    return true;
}

static bool CheckConnected()
{
    if (!CheckDll()) return false;
    if (!g_loader.GetDriverHandle() || !g_loader.IsConnected())
    {
        printf("未连接。请先使用 '1' 连接。\n");
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// 命令实现
// ---------------------------------------------------------------------------

static void DoLoadDll()
{
    if (g_loader.IsDllLoaded())
    {
        printf("DLL 已加载。\n");
        return;
    }

    std::string path = ReadLine("DLL 路径 [UDL.ViaviNSantecTester.dll]: ");
    if (path.empty()) path = "UDL.ViaviNSantecTester.dll";

    if (g_loader.LoadDll(path.c_str()))
    {
        printf("DLL 加载成功。\n");
        g_loader.SetLogCallback(LogHandler);
        printf("日志回调已注册。\n");
    }
    else
    {
        printf("DLL 加载失败。\n");
    }
}

static void DoUnloadDll()
{
    if (!g_loader.IsDllLoaded())
    {
        printf("DLL 未加载。\n");
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
        printf("驱动已创建。请先销毁。\n");
        return;
    }

    printf("驱动类型:\n");
    printf("  1 - Viavi PCT\n");
    printf("  2 - Santec RL1\n");
    int typeChoice = ReadInt("选择 [1]: ", 1);
    const char* typeStr = (typeChoice == 2) ? "santec" : "viavi";

    std::string ip = ReadLine("IP 地址 [10.14.132.194]: ");
    if (ip.empty()) ip = "10.14.132.194";

    int port = ReadInt("端口 [0=默认]: ", 0);
    int slot = 0;
    if (typeChoice != 2)
        slot = ReadInt("插槽 [3]: ", 3);

    printf("正在创建 %s 驱动，目标 %s:%d (插槽=%d)...\n", typeStr, ip.c_str(), port, slot);
    if (!g_loader.CreateDriver(typeStr, ip.c_str(), port, slot))
    {
        printf("CreateDriver 失败。\n");
        return;
    }

    printf("正在连接...\n");
    if (!g_loader.Connect())
    {
        printf("连接失败。\n");
        g_loader.DestroyDriver();
        return;
    }
    printf("已连接。\n");

    printf("正在初始化...\n");
    if (g_loader.Initialize())
    {
        printf("已初始化。\n");

        DriverDeviceInfo di;
        memset(&di, 0, sizeof(di));
        if (g_loader.GetDeviceInfo(&di))
        {
            printf("  制造商: %s\n", di.manufacturer);
            printf("  型号:   %s\n", di.model);
            printf("  序列号: %s\n", di.serialNumber);
            printf("  固件:   %s\n", di.firmwareVersion);
            printf("  插槽:   %d\n", di.slot);
        }
    }
    else
    {
        printf("初始化失败 (连接仍保持)。\n");
    }
}

static void DoDisconnect()
{
    if (!g_loader.GetDriverHandle())
    {
        printf("无驱动实例。\n");
        return;
    }
    g_loader.Disconnect();
    g_loader.DestroyDriver();
    printf("已断开连接并销毁驱动。\n");
}

static void DoConfigureWavelengths()
{
    if (!CheckConnected()) return;

    printf("输入波长(nm)，逗号分隔 (例如 1310,1550):\n");
    std::string s = ReadLine("波长 [1310,1550]: ");
    if (s.empty()) s = "1310,1550";

    std::vector<double> wl;
    const char* p = s.c_str();
    while (*p)
    {
        double val = atof(p);
        if (val > 0) wl.push_back(val);
        const char* comma = strchr(p, ',');
        if (!comma) break;
        p = comma + 1;
    }

    if (wl.empty())
    {
        printf("无有效波长。\n");
        return;
    }

    printf("正在配置 %d 个波长...\n", (int)wl.size());
    if (g_loader.ConfigureWavelengths(wl.data(), (int)wl.size()))
        printf("波长配置完成。\n");
    else
        printf("ConfigureWavelengths 失败。\n");
}

static void DoConfigureChannels()
{
    if (!CheckConnected()) return;

    printf("输入通道，逗号分隔 (例如 1,2,3,4):\n");
    std::string s = ReadLine("通道 [1]: ");
    if (s.empty()) s = "1";

    std::vector<int> ch;
    const char* p = s.c_str();
    while (*p)
    {
        int val = atoi(p);
        if (val > 0) ch.push_back(val);
        const char* comma = strchr(p, ',');
        if (!comma) break;
        p = comma + 1;
    }

    if (ch.empty())
    {
        printf("无有效通道。\n");
        return;
    }

    printf("正在配置 %d 个通道...\n", (int)ch.size());
    if (g_loader.ConfigureChannels(ch.data(), (int)ch.size()))
        printf("通道配置完成。\n");
    else
        printf("ConfigureChannels 失败。\n");
}

static void DoConfigureORL()
{
    if (!CheckConnected()) return;

    int channel = ReadInt("ORL 通道 [1]: ", 1);
    printf("ORL 方法: 0=连续, 1=脉冲\n");
    int method = ReadInt("方法 [0]: ", 0);
    printf("ORL 源: 0=内部, 1=外部\n");
    int origin = ReadInt("源 [0]: ", 0);
    double aOffset = ReadDouble("A偏移 [0.0]: ", 0.0);
    double bOffset = ReadDouble("B偏移 [0.0]: ", 0.0);

    if (g_loader.ConfigureORL(channel, method, origin, aOffset, bOffset))
        printf("ORL 配置完成。\n");
    else
        printf("ConfigureORL 失败 (仅Viavi支持)。\n");
}

static void DoTakeReference()
{
    if (!CheckConnected()) return;

    printf("是否覆盖参考值? (1=是, 0=自动)\n");
    int bOverride = ReadInt("覆盖 [0]: ", 0);
    double ilVal = 0.0, lenVal = 0.0;
    if (bOverride)
    {
        ilVal = ReadDouble("IL 覆盖值 [0.0]: ", 0.0);
        lenVal = ReadDouble("长度覆盖值 [0.0]: ", 0.0);
    }

    printf("正在取参考...\n");
    DWORD t0 = GetTickCount();
    BOOL ok = g_loader.TakeReference(bOverride, ilVal, lenVal);
    DWORD dt = GetTickCount() - t0;
    if (ok)
        printf("参考完成 (%lu ms)。\n", dt);
    else
        printf("TakeReference 失败 (%lu ms)。\n", dt);
}

static void DoTakeMeasurement()
{
    if (!CheckConnected()) return;

    printf("正在测量...\n");
    DWORD t0 = GetTickCount();
    BOOL ok = g_loader.TakeMeasurement();
    DWORD dt = GetTickCount() - t0;
    if (ok)
        printf("测量完成 (%lu ms)。\n", dt);
    else
        printf("TakeMeasurement 失败 (%lu ms)。\n", dt);
}

static void DoGetResults()
{
    if (!CheckConnected()) return;

    DriverMeasurementResult results[64];
    memset(results, 0, sizeof(results));
    int count = g_loader.GetResults(results, 64);
    printf("  %d 个结果:\n", count);
    for (int i = 0; i < count; ++i)
    {
        printf("  [%d] 通道=%d 波长=%.1fnm IL=%.3fdB RL=%.3fdB",
               i, results[i].channel, results[i].wavelength,
               results[i].insertionLoss, results[i].returnLoss);
        if (results[i].returnLossA != 0.0 || results[i].returnLossB != 0.0)
            printf(" RLA=%.3f RLB=%.3f", results[i].returnLossA, results[i].returnLossB);
        if (results[i].returnLossTotal != 0.0)
            printf(" RL总=%.3f", results[i].returnLossTotal);
        if (results[i].dutLength != 0.0)
            printf(" DUT=%.2fm", results[i].dutLength);
        printf("\n");
    }
}

static void DoFullCycle()
{
    if (!CheckConnected()) return;

    printf("=== 完整测量周期 ===\n");

    printf("正在取参考 (自动)...\n");
    DWORD t0 = GetTickCount();
    if (!g_loader.TakeReference(FALSE, 0.0, 0.0))
    {
        printf("TakeReference 失败。\n");
        return;
    }
    printf("参考完成 (%lu ms)。\n", GetTickCount() - t0);

    printf("正在测量...\n");
    t0 = GetTickCount();
    if (!g_loader.TakeMeasurement())
    {
        printf("TakeMeasurement 失败。\n");
        return;
    }
    printf("测量完成 (%lu ms)。\n", GetTickCount() - t0);

    DoGetResults();
}

static void PrintResultsSummary(DriverMeasurementResult* results, int count)
{
    for (int i = 0; i < count; ++i)
    {
        printf("  [%d] 通道=%d 波长=%.1fnm IL=%.3fdB RL=%.3fdB\n",
               i, results[i].channel, results[i].wavelength,
               results[i].insertionLoss, results[i].returnLoss);
    }
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

        if (input == "00") { DoUnloadDll(); continue; }

        int cmdNum = -1;
        if (input[0] >= '0' && input[0] <= '9')
            cmdNum = atoi(input.c_str());

        switch (cmdNum)
        {
        // --- DLL ---
        case 0:  DoLoadDll(); break;

        // --- 连接 ---
        case 1:  DoConnect(); break;
        case 2:  DoDisconnect(); break;
        case 3:
            if (CheckDll())
            {
                printf("DLL已加载: 是\n");
                printf("驱动已创建: %s\n", g_loader.GetDriverHandle() ? "是" : "否");
                printf("已连接: %s\n", g_loader.IsConnected() ? "是" : "否");
            }
            break;

        // --- 设备信息 ---
        case 10:
            if (CheckConnected())
            {
                DriverDeviceInfo di;
                memset(&di, 0, sizeof(di));
                if (g_loader.GetDeviceInfo(&di))
                {
                    printf("  制造商: %s\n", di.manufacturer);
                    printf("  型号:   %s\n", di.model);
                    printf("  序列号: %s\n", di.serialNumber);
                    printf("  固件:   %s\n", di.firmwareVersion);
                    printf("  插槽:   %d\n", di.slot);
                }
                else
                    printf("  GetDeviceInfo 失败。\n");
            }
            break;

        case 11:
            if (CheckConnected())
            {
                char msg[256] = {};
                int code = g_loader.CheckError(msg, sizeof(msg));
                printf("  错误码: %d\n", code);
                printf("  消息:   %s\n", msg);
            }
            break;

        // --- 配置 ---
        case 20: DoConfigureWavelengths(); break;
        case 21: DoConfigureChannels(); break;
        case 22: DoConfigureORL(); break;

        // --- 测量 ---
        case 30: DoTakeReference(); break;
        case 31: DoTakeMeasurement(); break;
        case 32: DoGetResults(); break;
        case 33: DoFullCycle(); break;

        // --- Santec 专用 ---
        case 40:
            if (CheckConnected())
            {
                printf("RL 灵敏度: 0=快速 (<1.5s, RL<=75dB), 1=标准 (<5s, RL<=85dB)\n");
                int sens = ReadInt("灵敏度 [0]: ", 0);
                if (g_loader.SantecSetRLSensitivity(sens))
                    printf("RL 灵敏度已设置为 %d。\n", sens);
                else
                    printf("失败 (仅Santec支持)。\n");
            }
            break;

        case 41:
            if (CheckConnected())
            {
                printf("DUT 长度档位: 100, 1500, 或 4000 (米)\n");
                int len = ReadInt("长度 [1500]: ", 1500);
                if (g_loader.SantecSetDUTLength(len))
                    printf("DUT 长度档位已设置为 %d。\n", len);
                else
                    printf("失败 (仅Santec支持)。\n");
            }
            break;

        case 42:
            if (CheckConnected())
            {
                printf("RL 增益: 0=正常 (40-85dB), 1=低 (30-40dB)\n");
                int gain = ReadInt("增益 [0]: ", 0);
                if (g_loader.SantecSetRLGain(gain))
                    printf("RL 增益模式已设置为 %d。\n", gain);
                else
                    printf("失败 (仅Santec支持)。\n");
            }
            break;

        case 43:
            if (CheckConnected())
            {
                int mode = ReadInt("本地模式 (1=启用, 0=禁用): ", 0);
                if (g_loader.SantecSetLocalMode(mode))
                    printf("本地模式已设置为 %s。\n", mode ? "启用" : "禁用");
                else
                    printf("失败 (仅Santec支持)。\n");
            }
            break;

        // --- 原始 SCPI ---
        case 60:
            if (CheckConnected())
            {
                std::string cmd = ReadLine("SCPI 命令: ");
                char resp[4096] = {};
                if (g_loader.SendCommand(cmd.c_str(), resp, sizeof(resp)))
                    printf("  响应: %s\n", resp);
                else
                    printf("  SendCommand 失败。\n");
            }
            break;

        default:
            if (input == "h" || input == "H" || input == "?")
                PrintHelp();
            else if (input == "q" || input == "Q")
                running = false;
            else
                printf("未知命令。输入 'h' 查看帮助。\n");
            break;
        }
    }

    g_loader.DestroyDriver();
    g_loader.UnloadDll();

    printf("DriverTestApp2 已停止。\n");
    return 0;
}
