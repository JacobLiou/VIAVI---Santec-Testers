#pragma once

#include "stdafx.h"

// ---------------------------------------------------------------------------
// OSX Optical Switch Simulator
//
// TCP server (default port 5025) that responds to official SCPI commands
// matching the OSX protocol per OSX User Manual M-OSX_SX1-001.04.
//
// Simulates:
//   - Standard SCPI (Table 4): *IDN?, *RST, *CLS, *OPC?, status, system
//   - OSX switch commands (Table 5): CLOSe, module, route, LCL, notify
//   - Asynchronous operation model: STAT:OPER:COND? returns busy during switch
//   - Multi-module support with configurable configurations
// ---------------------------------------------------------------------------

class COSXSimServer
{
public:
    struct SimModule
    {
        std::string name;           // e.g. "SX 1Ax24"
        int  configType;            // 0=1A, 1=2A, 2=2B, 3=2C
        int  channelCount;
        int  currentChannel;
        int  currentCommon;         // for 2A/2C configs

        SimModule()
            : configType(0), channelCount(24)
            , currentChannel(1), currentCommon(0)
        {}
    };

    COSXSimServer();
    ~COSXSimServer();

    bool Start(int port = 5025);
    void Stop();
    bool IsRunning() const { return m_running; }

    // Configuration
    void SetSwitchDelayMs(int ms)    { m_switchDelayMs = ms; }
    int  GetSwitchDelayMs() const    { return m_switchDelayMs; }
    void SetVerbose(bool enabled)    { m_verbose = enabled; }
    bool GetVerbose() const          { return m_verbose; }
    void SetErrorMode(bool enabled)  { m_errorMode = enabled; }
    bool GetErrorMode() const        { return m_errorMode; }
    int  GetCommandCount() const     { return m_commandCount; }
    int  GetModuleCount() const      { return static_cast<int>(m_modules.size()); }

    // Module management
    void AddModule(const std::string& name, int configType, int channelCount);
    void ClearModules();
    const std::vector<SimModule>& GetModules() const { return m_modules; }

private:
    static DWORD WINAPI ServerThreadProc(LPVOID param);
    void RunServer();
    void HandleClient(SOCKET clientSocket);
    std::string ProcessCommand(const std::string& cmd);

    void Log(const char* fmt, ...);

    // Async operation tracking
    void BeginOperation();
    bool IsOperationBusy() const;

    // Server state
    bool            m_running;
    int             m_port;
    SOCKET          m_listenSocket;
    HANDLE          m_serverThread;

    // Device state
    std::vector<SimModule> m_modules;
    int             m_selectedModule;
    bool            m_localMode;
    int             m_switchDelayMs;

    // Simulated network settings
    std::string     m_ipAddress;
    std::string     m_gateway;
    std::string     m_netmask;
    std::string     m_hostname;
    std::string     m_macAddress;

    // Async operation timing
    DWORD           m_operationStartTick;
    int             m_operationDurationMs;

    // Controls
    bool            m_errorMode;
    bool            m_verbose;
    int             m_commandCount;

    // Error queue
    std::vector<std::string> m_errorQueue;

    // Status registers
    int             m_eseRegister;
    int             m_sreRegister;

    std::mutex      m_mutex;
};
