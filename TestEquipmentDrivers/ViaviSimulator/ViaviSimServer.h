#pragma once

#include "stdafx.h"

// ---------------------------------------------------------------------------
// VIAVI MAP300 PCT Simulator
//
// Emulates two TCP servers:
//   Chassis (port 8100) - Accepts SUPER:LAUNCH commands
//   PCT Module (port 8301) - Handles SCPI measurement commands
//
// Tracks full protocol state: wavelengths, channels, measurement lifecycle.
// Generates realistic IL/RL measurement data for driver testing.
// ---------------------------------------------------------------------------

class CViaviSimServer
{
public:
    CViaviSimServer();
    ~CViaviSimServer();

    bool Start(int chassisPort = 8100, int pctPort = 8301);
    void Stop();
    bool IsRunning() const { return m_running; }

    // Runtime controls
    void SetErrorMode(bool enabled)    { m_errorMode = enabled; }
    bool GetErrorMode() const           { return m_errorMode; }
    void SetMeasurementDelay(int ms)    { m_measDelayMs = ms; }
    int  GetMeasurementDelay() const    { return m_measDelayMs; }
    void SetVerbose(bool enabled)       { m_verbose = enabled; }
    bool GetVerbose() const             { return m_verbose; }

    // Statistics
    int  GetCommandCount() const        { return m_commandCount; }

private:
    // Server thread entry points
    static DWORD WINAPI ChassisThreadProc(LPVOID param);
    static DWORD WINAPI PctThreadProc(LPVOID param);

    void RunChassisServer();
    void RunPctServer();
    void HandleClient(SOCKET clientSocket, const char* serverTag);

    // Protocol handling
    std::string ProcessChassisCommand(const std::string& cmd);
    std::string ProcessPctCommand(const std::string& cmd);
    std::string GenerateMeasResults(int channel, int launchPort);

    // Logging
    void Log(const char* tag, const char* fmt, ...);
    void LogVerbose(const char* tag, const char* fmt, ...);

    // Sockets
    SOCKET m_chassisListen;
    SOCKET m_pctListen;
    int    m_chassisPort;
    int    m_pctPort;

    // Threads
    HANDLE m_hChassisThread;
    HANDLE m_hPctThread;

    // Runtime flags
    std::atomic<bool> m_running;
    std::atomic<bool> m_errorMode;
    std::atomic<bool> m_verbose;
    std::atomic<int>  m_measDelayMs;
    std::atomic<int>  m_commandCount;

    // PCT protocol state (protected by m_stateMutex)
    std::mutex          m_stateMutex;
    std::vector<double> m_wavelengths;
    int                 m_launchPort;
    int                 m_sensFunc;         // 0 = reference, 1 = measurement
    bool                m_measRunning;
    DWORD               m_measStartTick;

    // Console output lock
    std::mutex m_logMutex;
};
