#include "stdafx.h"
#include "ViaviSantecDllLoader.h"

// ---------------------------------------------------------------------------
// DriverTestApp2 -- UDL.ViaviNSantecTester.dll ?????????????
//
// ??????????????
//   1. LoadLibrary("UDL.ViaviNSantecTester.dll")
//   2. GetProcAddress ??????????????
//   3. ?????????????
//   4. ???? FreeLibrary
//
// ?????? UDL.ViaviNSantecTester ?????? .h ??????????????? .lib ?????
// ---------------------------------------------------------------------------

static CViaviSantecDllLoader g_loader;

// ---------------------------------------------------------------------------
// ??????????DLL????????
// ---------------------------------------------------------------------------
static void WINAPI LogHandler(int level, const char* source, const char* message)
{
    const char* tag = "???";
    switch (level)
    {
    case 0: tag = "????"; break;
    case 1: tag = "???"; break;
    case 2: tag = "????"; break;
    case 3: tag = "????"; break;
    }
    printf("  [%s][%s] %s\n", tag, source, message);
}

// ---------------------------------------------------------------------------
// ????????
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
// ???
// ---------------------------------------------------------------------------

static void PrintBanner()
{
    printf("\n");
    printf("====================================================\n");
    printf("  DriverTestApp2 -- ????????????? v1.0\n");
    printf("  Viavi & Santec (LoadLibrary, ????????)\n");
    printf("====================================================\n\n");
}

static void PrintHelp()
{
    printf("\n--- DLL ???? ---\n");
    printf("  0  - ???? UDL.ViaviNSantecTester.dll\n");
    printf("  00 - ???? DLL\n");
    printf("\n--- ???? ---\n");
    printf("  1  - ???????? + ???? + ????? (TCP)\n");
    printf("  1v - ???????? + ???? (USB/VISA)\n");
    printf("  1e - ??? VISA ???\n");
    printf("  2  - ??????? + ????????\n");
    printf("  3  - ?????????\n");
    printf("\n--- ?????? ---\n");
    printf("  10 - ????????? (*IDN?)\n");
    printf("  11 - ??????\n");
    printf("\n--- ???? ---\n");
    printf("  20 - ????????\n");
    printf("  21 - ???????\n");
    printf("  22 - ???? ORL (??Viavi)\n");
    printf("\n--- ???? ---\n");
    printf("  30 - ?????\n");
    printf("  31 - ???????\n");
    printf("  32 - ??????\n");
    printf("  33 - ???????? (???? + ???? + ???)\n");
    printf("\n--- Santec ??? ---\n");
    printf("  40 - ???? RL ??????\n");
    printf("  41 - ???? DUT ???????\n");
    printf("  42 - ???? RL ??????\n");
    printf("  43 - ?????????\n");
    printf("\n--- ?? SCPI ---\n");
    printf("  60 - ?????? SCPI ????\n");
    printf("\n--- ???? ---\n");
    printf("  h  - ????\n");
    printf("  q  - ???\n\n");
}

// ---------------------------------------------------------------------------
// ???D??
// ---------------------------------------------------------------------------

static bool CheckDll()
{
    if (!g_loader.IsDllLoaded())
    {
        printf("DLL ?????????????? '0' ?????\n");
        return false;
    }
    return true;
}

static bool CheckConnected()
{
    if (!CheckDll()) return false;
    if (!g_loader.GetDriverHandle() || !g_loader.IsConnected())
    {
        printf("?????????????? '1' ?????\n");
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// ???????
// ---------------------------------------------------------------------------

static void DoLoadDll()
{
    if (g_loader.IsDllLoaded())
    {
        printf("DLL ??????\n");
        return;
    }

    std::string path = ReadLine("DLL ???? [UDL.ViaviNSantecTester.dll]: ");
    if (path.empty()) path = "UDL.ViaviNSantecTester.dll";

    if (g_loader.LoadDll(path.c_str()))
    {
        printf("DLL ????????\n");
        g_loader.SetLogCallback(LogHandler);
        printf("???????????\n");
    }
    else
    {
        printf("DLL ????????\n");
    }
}

static void DoUnloadDll()
{
    if (!g_loader.IsDllLoaded())
    {
        printf("DLL ???????\n");
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
        printf("????????????????????\n");
        return;
    }

    printf("????????:\n");
    printf("  1 - Viavi PCT\n");
    printf("  2 - Santec RL1\n");
    int typeChoice = ReadInt("??? [1]: ", 1);
    const char* typeStr = (typeChoice == 2) ? "santec" : "viavi";

    std::string ip = ReadLine("IP ??? [10.14.132.194]: ");
    if (ip.empty()) ip = "10.14.132.194";

    int port = ReadInt("??? [0=???]: ", 0);
    int slot = 0;
    if (typeChoice != 2)
        slot = ReadInt("??? [3]: ", 3);

    printf("??????? %s ????????? %s:%d (???=%d)...\n", typeStr, ip.c_str(), port, slot);
    if (!g_loader.CreateDriver(typeStr, ip.c_str(), port, slot))
    {
        printf("CreateDriver ????\n");
        return;
    }

    printf("????????...\n");
    if (!g_loader.Connect())
    {
        printf("????????\n");
        g_loader.DestroyDriver();
        return;
    }
    printf("???????\n");

    printf("????????...\n");
    if (g_loader.Initialize())
    {
        printf("????????\n");

        DriverDeviceInfo di;
        memset(&di, 0, sizeof(di));
        if (g_loader.GetDeviceInfo(&di))
        {
            printf("  ??????: %s\n", di.manufacturer);
            printf("  ???:   %s\n", di.model);
            printf("  ??????: %s\n", di.serialNumber);
            printf("  ???:   %s\n", di.firmwareVersion);
            printf("  ???:   %d\n", di.slot);
        }
    }
    else
    {
        printf("???????? (?????????)??\n");
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

    printf("Driver type:\n");
    printf("  1 - Viavi PCT\n");
    printf("  2 - Santec RL1\n");
    int typeChoice = ReadInt("Choose [2]: ", 2);
    const char* typeStr = (typeChoice == 2) ? "santec" : "viavi";

    printf("Enter VISA resource string\n");
    printf("  Example: USB0::0x1698::0x0368::SN12345::INSTR\n");
    std::string rsrc = ReadLine("VISA resource: ");
    if (rsrc.empty())
    {
        printf("No resource string entered.\n");
        return;
    }

    int slot = 0;
    if (typeChoice != 2)
        slot = ReadInt("Slot [3]: ", 3);

    printf("Creating %s driver (VISA/USB) for %s ...\n", typeStr, rsrc.c_str());
    if (!g_loader.CreateDriverEx(typeStr, rsrc.c_str(), 0, slot, 2))
    {
        printf("CreateDriverEx failed.\n");
        return;
    }

    printf("Connecting...\n");
    if (!g_loader.Connect())
    {
        printf("VISA connection failed.\n");
        g_loader.DestroyDriver();
        return;
    }
    printf("Connected via VISA.\n");

    printf("Initializing...\n");
    if (g_loader.Initialize())
    {
        printf("Initialized.\n");

        DriverDeviceInfo di;
        memset(&di, 0, sizeof(di));
        if (g_loader.GetDeviceInfo(&di))
        {
            printf("  Manufacturer: %s\n", di.manufacturer);
            printf("  Model:        %s\n", di.model);
            printf("  Serial:       %s\n", di.serialNumber);
            printf("  Firmware:     %s\n", di.firmwareVersion);
            printf("  Slot:         %d\n", di.slot);
        }
    }
    else
    {
        printf("Initialize failed (connection kept).\n");
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
        printf("???????????\n");
        return;
    }
    g_loader.Disconnect();
    g_loader.DestroyDriver();
    printf("???????????????????\n");
}

static void DoConfigureWavelengths()
{
    if (!CheckConnected()) return;

    printf("??????(nm)???????? (???? 1310,1550):\n");
    std::string s = ReadLine("???? [1310,1550]: ");
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
        printf("????????????\n");
        return;
    }

    printf("???????? %d ??????...\n", (int)wl.size());
    if (g_loader.ConfigureWavelengths(wl.data(), (int)wl.size()))
        printf("?????????????\n");
    else
        printf("ConfigureWavelengths ????\n");
}

static void DoConfigureChannels()
{
    if (!CheckConnected()) return;

    printf("??????????????? (???? 1,2,3,4):\n");
    std::string s = ReadLine("??? [1]: ");
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
        printf("???????????\n");
        return;
    }

    printf("???????? %d ?????...\n", (int)ch.size());
    if (g_loader.ConfigureChannels(ch.data(), (int)ch.size()))
        printf("????????????\n");
    else
        printf("ConfigureChannels ????\n");
}

static void DoConfigureORL()
{
    if (!CheckConnected()) return;

    int channel = ReadInt("ORL ??? [1]: ", 1);
    printf("ORL ????: 0=????, 1=????\n");
    int method = ReadInt("???? [0]: ", 0);
    printf("ORL ?: 0=???, 1=??\n");
    int origin = ReadInt("? [0]: ", 0);
    double aOffset = ReadDouble("A??? [0.0]: ", 0.0);
    double bOffset = ReadDouble("B??? [0.0]: ", 0.0);

    if (g_loader.ConfigureORL(channel, method, origin, aOffset, bOffset))
        printf("ORL ?????????\n");
    else
        printf("ConfigureORL ??? (??Viavi???)??\n");
}

static void DoTakeReference()
{
    if (!CheckConnected()) return;

    printf("????????? (1=??, 0=???)\n");
    int bOverride = ReadInt("???? [0]: ", 0);
    double ilVal = 0.0, lenVal = 0.0;
    if (bOverride)
    {
        ilVal = ReadDouble("IL ????? [0.0]: ", 0.0);
        lenVal = ReadDouble("???????? [0.0]: ", 0.0);
    }

    printf("?????????...\n");
    DWORD t0 = GetTickCount();
    BOOL ok = g_loader.TakeReference(bOverride, ilVal, lenVal);
    DWORD dt = GetTickCount() - t0;
    if (ok)
        printf("??????? (%lu ms)??\n", dt);
    else
        printf("TakeReference ??? (%lu ms)??\n", dt);
}

static void DoTakeMeasurement()
{
    if (!CheckConnected()) return;

    printf("???????...\n");
    DWORD t0 = GetTickCount();
    BOOL ok = g_loader.TakeMeasurement();
    DWORD dt = GetTickCount() - t0;
    if (ok)
        printf("??????? (%lu ms)??\n", dt);
    else
        printf("TakeMeasurement ??? (%lu ms)??\n", dt);
}

static void DoGetResults()
{
    if (!CheckConnected()) return;

    DriverMeasurementResult results[64];
    memset(results, 0, sizeof(results));
    int count = g_loader.GetResults(results, 64);
    printf("  %d ?????:\n", count);
    for (int i = 0; i < count; ++i)
    {
        printf("  [%d] ???=%d ????=%.1fnm IL=%.3fdB RL=%.3fdB",
               i, results[i].channel, results[i].wavelength,
               results[i].insertionLoss, results[i].returnLoss);
        if (results[i].returnLossA != 0.0 || results[i].returnLossB != 0.0)
            printf(" RLA=%.3f RLB=%.3f", results[i].returnLossA, results[i].returnLossB);
        if (results[i].returnLossTotal != 0.0)
            printf(" RL??=%.3f", results[i].returnLossTotal);
        if (results[i].dutLength != 0.0)
            printf(" DUT=%.2fm", results[i].dutLength);
        printf("\n");
    }
}

static void DoFullCycle()
{
    if (!CheckConnected()) return;

    printf("=== ???????????? ===\n");

    printf("????????? (???)...\n");
    DWORD t0 = GetTickCount();
    if (!g_loader.TakeReference(FALSE, 0.0, 0.0))
    {
        printf("TakeReference ????\n");
        return;
    }
    printf("??????? (%lu ms)??\n", GetTickCount() - t0);

    printf("???????...\n");
    t0 = GetTickCount();
    if (!g_loader.TakeMeasurement())
    {
        printf("TakeMeasurement ????\n");
        return;
    }
    printf("??????? (%lu ms)??\n", GetTickCount() - t0);

    DoGetResults();
}

static void PrintResultsSummary(DriverMeasurementResult* results, int count)
{
    for (int i = 0; i < count; ++i)
    {
        printf("  [%d] ???=%d ????=%.1fnm IL=%.3fdB RL=%.3fdB\n",
               i, results[i].channel, results[i].wavelength,
               results[i].insertionLoss, results[i].returnLoss);
    }
}

// ---------------------------------------------------------------------------
// ??????
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
        if (input == "1v" || input == "1V") { DoConnectVisa(); continue; }
        if (input == "1e" || input == "1E") { DoEnumerateVisa(); continue; }

        int cmdNum = -1;
        if (input[0] >= '0' && input[0] <= '9')
            cmdNum = atoi(input.c_str());

        switch (cmdNum)
        {
        // --- DLL ---
        case 0:  DoLoadDll(); break;

        // --- ???? ---
        case 1:  DoConnect(); break;
        case 2:  DoDisconnect(); break;
        case 3:
            if (CheckDll())
            {
                printf("DLL?????: ??\n");
                printf("?????????: %s\n", g_loader.GetDriverHandle() ? "??" : "??");
                printf("??????: %s\n", g_loader.IsConnected() ? "??" : "??");
            }
            break;

        // --- ?????? ---
        case 10:
            if (CheckConnected())
            {
                DriverDeviceInfo di;
                memset(&di, 0, sizeof(di));
                if (g_loader.GetDeviceInfo(&di))
                {
                    printf("  ??????: %s\n", di.manufacturer);
                    printf("  ???:   %s\n", di.model);
                    printf("  ??????: %s\n", di.serialNumber);
                    printf("  ???:   %s\n", di.firmwareVersion);
                    printf("  ???:   %d\n", di.slot);
                }
                else
                    printf("  GetDeviceInfo ????\n");
            }
            break;

        case 11:
            if (CheckConnected())
            {
                char msg[256] = {};
                int code = g_loader.CheckError(msg, sizeof(msg));
                printf("  ??????: %d\n", code);
                printf("  ???:   %s\n", msg);
            }
            break;

        // --- ???? ---
        case 20: DoConfigureWavelengths(); break;
        case 21: DoConfigureChannels(); break;
        case 22: DoConfigureORL(); break;

        // --- ???? ---
        case 30: DoTakeReference(); break;
        case 31: DoTakeMeasurement(); break;
        case 32: DoGetResults(); break;
        case 33: DoFullCycle(); break;

        // --- Santec ??? ---
        case 40:
            if (CheckConnected())
            {
                printf("RL ??????: 0=???? (<1.5s, RL<=75dB), 1=??? (<5s, RL<=85dB)\n");
                int sens = ReadInt("?????? [0]: ", 0);
                if (g_loader.SantecSetRLSensitivity(sens))
                    printf("RL ????????????? %d??\n", sens);
                else
                    printf("??? (??Santec???)??\n");
            }
            break;

        case 41:
            if (CheckConnected())
            {
                printf("DUT ???????: 100, 1500, ?? 4000 (??)\n");
                int len = ReadInt("???? [1500]: ", 1500);
                if (g_loader.SantecSetDUTLength(len))
                    printf("DUT ?????????????? %d??\n", len);
                else
                    printf("??? (??Santec???)??\n");
            }
            break;

        case 42:
            if (CheckConnected())
            {
                printf("RL ????: 0=???? (40-85dB), 1=?? (30-40dB)\n");
                int gain = ReadInt("???? [0]: ", 0);
                if (g_loader.SantecSetRLGain(gain))
                    printf("RL ????????????? %d??\n", gain);
                else
                    printf("??? (??Santec???)??\n");
            }
            break;

        case 43:
            if (CheckConnected())
            {
                int mode = ReadInt("?????? (1=????, 0=????): ", 0);
                if (g_loader.SantecSetLocalMode(mode))
                    printf("????????????? %s??\n", mode ? "????" : "????");
                else
                    printf("??? (??Santec???)??\n");
            }
            break;

        // --- ?? SCPI ---
        case 60:
            if (CheckConnected())
            {
                std::string cmd = ReadLine("SCPI ????: ");
                char resp[4096] = {};
                if (g_loader.SendCommand(cmd.c_str(), resp, sizeof(resp)))
                    printf("  ???: %s\n", resp);
                else
                    printf("  SendCommand ????\n");
            }
            break;

        default:
            if (input == "h" || input == "H" || input == "?")
                PrintHelp();
            else if (input == "q" || input == "Q")
                running = false;
            else
                printf("??????????? 'h' ????????\n");
            break;
        }
    }

    g_loader.DestroyDriver();
    g_loader.UnloadDll();

    printf("DriverTestApp2 ??????\n");
    return 0;
}
