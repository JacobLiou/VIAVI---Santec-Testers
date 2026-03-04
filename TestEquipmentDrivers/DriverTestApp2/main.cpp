#include "stdafx.h"
#include "ViaviSantecDllLoader.h"

// ---------------------------------------------------------------------------
// DriverTestApp2 -- Dynamic-loading debug tool for UDL.ViaviNSantecTester.dll
//
// Demonstrates the production framework pattern:
//   1. LoadLibrary("UDL.ViaviNSantecTester.dll")
//   2. GetProcAddress for each exported function
//   3. Call through function pointers
//   4. FreeLibrary on exit
//
// No .h from UDL.ViaviNSantecTester is included.  No .lib is linked.
// ---------------------------------------------------------------------------

static CViaviSantecDllLoader g_loader;

// ---------------------------------------------------------------------------
// Log callback (called from inside the DLL)
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
// Helpers
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
// Menu
// ---------------------------------------------------------------------------

static void PrintBanner()
{
    printf("\n");
    printf("====================================================\n");
    printf("  DriverTestApp2 -- Dynamic Loading Debug Tool v1.0\n");
    printf("  Viavi & Santec (LoadLibrary, no static import)\n");
    printf("====================================================\n\n");
}

static void PrintHelp()
{
    printf("\n--- DLL Management ---\n");
    printf("  0  - Load UDL.ViaviNSantecTester.dll\n");
    printf("  00 - Unload DLL\n");
    printf("\n--- Connection ---\n");
    printf("  1  - Create driver + Connect + Initialize\n");
    printf("  2  - Disconnect + Destroy driver\n");
    printf("  3  - Show connection status\n");
    printf("\n--- Device Info ---\n");
    printf("  10 - Get device info (*IDN?)\n");
    printf("  11 - Check error\n");
    printf("\n--- Configuration ---\n");
    printf("  20 - Configure wavelengths\n");
    printf("  21 - Configure channels\n");
    printf("  22 - Configure ORL (Viavi only)\n");
    printf("\n--- Measurement ---\n");
    printf("  30 - Take reference\n");
    printf("  31 - Take measurement\n");
    printf("  32 - Get results\n");
    printf("  33 - Full cycle (reference + measurement + results)\n");
    printf("\n--- Santec Specific ---\n");
    printf("  40 - Set RL sensitivity\n");
    printf("  41 - Set DUT length bin\n");
    printf("  42 - Set RL gain mode\n");
    printf("  43 - Set local mode\n");
    printf("\n--- Raw SCPI ---\n");
    printf("  60 - Send raw SCPI command\n");
    printf("\n--- Other ---\n");
    printf("  h  - Help\n");
    printf("  q  - Quit\n\n");
}

// ---------------------------------------------------------------------------
// Guards
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
// Commands
// ---------------------------------------------------------------------------

static void DoLoadDll()
{
    if (g_loader.IsDllLoaded())
    {
        printf("DLL already loaded.\n");
        return;
    }

    std::string path = ReadLine("DLL path [UDL.ViaviNSantecTester.dll]: ");
    if (path.empty()) path = "UDL.ViaviNSantecTester.dll";

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

    printf("Driver type:\n");
    printf("  1 - Viavi PCT\n");
    printf("  2 - Santec RL1\n");
    int typeChoice = ReadInt("Choice [1]: ", 1);
    const char* typeStr = (typeChoice == 2) ? "santec" : "viavi";

    std::string ip = ReadLine("IP address [10.14.132.194]: ");
    if (ip.empty()) ip = "10.14.132.194";

    int port = ReadInt("Port [0=default]: ", 0);
    int slot = 0;
    if (typeChoice != 2)
        slot = ReadInt("Slot [3]: ", 3);

    printf("Creating %s driver for %s:%d (slot=%d)...\n", typeStr, ip.c_str(), port, slot);
    if (!g_loader.CreateDriver(typeStr, ip.c_str(), port, slot))
    {
        printf("CreateDriver failed.\n");
        return;
    }

    printf("Connecting...\n");
    if (!g_loader.Connect())
    {
        printf("Connect failed.\n");
        g_loader.DestroyDriver();
        return;
    }
    printf("Connected.\n");

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
        printf("Initialize failed (connection still up).\n");
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

static void DoConfigureWavelengths()
{
    if (!CheckConnected()) return;

    printf("Enter wavelengths in nm, comma-separated (e.g. 1310,1550):\n");
    std::string s = ReadLine("Wavelengths [1310,1550]: ");
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
        printf("No valid wavelengths.\n");
        return;
    }

    printf("Configuring %d wavelength(s)...\n", (int)wl.size());
    if (g_loader.ConfigureWavelengths(wl.data(), (int)wl.size()))
        printf("Wavelengths configured.\n");
    else
        printf("ConfigureWavelengths failed.\n");
}

static void DoConfigureChannels()
{
    if (!CheckConnected()) return;

    printf("Enter channels, comma-separated (e.g. 1,2,3,4):\n");
    std::string s = ReadLine("Channels [1]: ");
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
        printf("No valid channels.\n");
        return;
    }

    printf("Configuring %d channel(s)...\n", (int)ch.size());
    if (g_loader.ConfigureChannels(ch.data(), (int)ch.size()))
        printf("Channels configured.\n");
    else
        printf("ConfigureChannels failed.\n");
}

static void DoConfigureORL()
{
    if (!CheckConnected()) return;

    int channel = ReadInt("ORL channel [1]: ", 1);
    printf("ORL method: 0=CONTINUOUS, 1=PULSE\n");
    int method = ReadInt("Method [0]: ", 0);
    printf("ORL origin: 0=INTERNAL, 1=EXTERNAL\n");
    int origin = ReadInt("Origin [0]: ", 0);
    double aOffset = ReadDouble("A-offset [0.0]: ", 0.0);
    double bOffset = ReadDouble("B-offset [0.0]: ", 0.0);

    if (g_loader.ConfigureORL(channel, method, origin, aOffset, bOffset))
        printf("ORL configured.\n");
    else
        printf("ConfigureORL failed (Viavi only).\n");
}

static void DoTakeReference()
{
    if (!CheckConnected()) return;

    printf("Override reference values? (1=yes, 0=auto)\n");
    int bOverride = ReadInt("Override [0]: ", 0);
    double ilVal = 0.0, lenVal = 0.0;
    if (bOverride)
    {
        ilVal = ReadDouble("IL override value [0.0]: ", 0.0);
        lenVal = ReadDouble("Length override value [0.0]: ", 0.0);
    }

    printf("Taking reference...\n");
    DWORD t0 = GetTickCount();
    BOOL ok = g_loader.TakeReference(bOverride, ilVal, lenVal);
    DWORD dt = GetTickCount() - t0;
    if (ok)
        printf("Reference taken (%lu ms).\n", dt);
    else
        printf("TakeReference failed (%lu ms).\n", dt);
}

static void DoTakeMeasurement()
{
    if (!CheckConnected()) return;

    printf("Taking measurement...\n");
    DWORD t0 = GetTickCount();
    BOOL ok = g_loader.TakeMeasurement();
    DWORD dt = GetTickCount() - t0;
    if (ok)
        printf("Measurement complete (%lu ms).\n", dt);
    else
        printf("TakeMeasurement failed (%lu ms).\n", dt);
}

static void DoGetResults()
{
    if (!CheckConnected()) return;

    DriverMeasurementResult results[64];
    memset(results, 0, sizeof(results));
    int count = g_loader.GetResults(results, 64);
    printf("  %d result(s):\n", count);
    for (int i = 0; i < count; ++i)
    {
        printf("  [%d] ch=%d wl=%.1fnm IL=%.3fdB RL=%.3fdB",
               i, results[i].channel, results[i].wavelength,
               results[i].insertionLoss, results[i].returnLoss);
        if (results[i].returnLossA != 0.0 || results[i].returnLossB != 0.0)
            printf(" RLA=%.3f RLB=%.3f", results[i].returnLossA, results[i].returnLossB);
        if (results[i].returnLossTotal != 0.0)
            printf(" RLTotal=%.3f", results[i].returnLossTotal);
        if (results[i].dutLength != 0.0)
            printf(" DUT=%.2fm", results[i].dutLength);
        printf("\n");
    }
}

static void DoFullCycle()
{
    if (!CheckConnected()) return;

    printf("=== Full measurement cycle ===\n");

    printf("Taking reference (auto)...\n");
    DWORD t0 = GetTickCount();
    if (!g_loader.TakeReference(FALSE, 0.0, 0.0))
    {
        printf("TakeReference failed.\n");
        return;
    }
    printf("Reference OK (%lu ms).\n", GetTickCount() - t0);

    printf("Taking measurement...\n");
    t0 = GetTickCount();
    if (!g_loader.TakeMeasurement())
    {
        printf("TakeMeasurement failed.\n");
        return;
    }
    printf("Measurement OK (%lu ms).\n", GetTickCount() - t0);

    DoGetResults();
}

static void PrintResultsSummary(DriverMeasurementResult* results, int count)
{
    for (int i = 0; i < count; ++i)
    {
        printf("  [%d] ch=%d wl=%.1fnm IL=%.3fdB RL=%.3fdB\n",
               i, results[i].channel, results[i].wavelength,
               results[i].insertionLoss, results[i].returnLoss);
    }
}

// ---------------------------------------------------------------------------
// main
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

        // --- Connection ---
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

        // --- Device Info ---
        case 10:
            if (CheckConnected())
            {
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

        // --- Configuration ---
        case 20: DoConfigureWavelengths(); break;
        case 21: DoConfigureChannels(); break;
        case 22: DoConfigureORL(); break;

        // --- Measurement ---
        case 30: DoTakeReference(); break;
        case 31: DoTakeMeasurement(); break;
        case 32: DoGetResults(); break;
        case 33: DoFullCycle(); break;

        // --- Santec Specific ---
        case 40:
            if (CheckConnected())
            {
                printf("RL Sensitivity: 0=FAST (<1.5s, RL<=75dB), 1=STANDARD (<5s, RL<=85dB)\n");
                int sens = ReadInt("Sensitivity [0]: ", 0);
                if (g_loader.SantecSetRLSensitivity(sens))
                    printf("RL sensitivity set to %d.\n", sens);
                else
                    printf("Failed (Santec only).\n");
            }
            break;

        case 41:
            if (CheckConnected())
            {
                printf("DUT Length bin: 100, 1500, or 4000 (meters)\n");
                int len = ReadInt("Length [1500]: ", 1500);
                if (g_loader.SantecSetDUTLength(len))
                    printf("DUT length bin set to %d.\n", len);
                else
                    printf("Failed (Santec only).\n");
            }
            break;

        case 42:
            if (CheckConnected())
            {
                printf("RL Gain: 0=NORMAL (40-85dB), 1=LOW (30-40dB)\n");
                int gain = ReadInt("Gain [0]: ", 0);
                if (g_loader.SantecSetRLGain(gain))
                    printf("RL gain mode set to %d.\n", gain);
                else
                    printf("Failed (Santec only).\n");
            }
            break;

        case 43:
            if (CheckConnected())
            {
                int mode = ReadInt("Local mode (1=enabled, 0=disabled): ", 0);
                if (g_loader.SantecSetLocalMode(mode))
                    printf("Local mode set to %s.\n", mode ? "enabled" : "disabled");
                else
                    printf("Failed (Santec only).\n");
            }
            break;

        // --- Raw SCPI ---
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

    printf("DriverTestApp2 stopped.\n");
    return 0;
}
