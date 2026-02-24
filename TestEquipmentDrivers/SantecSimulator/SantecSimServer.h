#pragma once

#include "stdafx.h"

// ---------------------------------------------------------------------------
// Santec RLM-100 / ILM-100 Simulator
//
// Single TCP server (default port 5025) that responds to SCPI commands
// matching the Santec IL/RL test instrument protocol.
//
// Simulates:
//   - Device identity (*IDN?)
//   - Wavelength and channel configuration
//   - Reference (calibration) procedure
//   - IL/RL measurement with realistic data generation
//   - Error query (SYST:ERR?)
//   - Operation status polling (STAT:OPER?)
//
// Configurable: model type (RLM/ILM), error injection, measurement delay.
// ---------------------------------------------------------------------------

class CSantecSimServer
{
public:
    enum SimModel { SIM_RLM_100, SIM_ILM_100 };

    CSantecSimServer();
    ~CSantecSimServer();

    bool Start(int port = 5025, SimModel model = SIM_RLM_100);
    void Stop();
    bool IsRunning() const { return m_running; }

    // Runtime controls
    void SetErrorMode(bool enabled)     { m_errorMode = enabled; }
    bool GetErrorMode() const           { return m_errorMode; }
    void SetMeasDelayMs(int ms)         { m_measDelayMs = ms; }
    int  GetMeasDelayMs() const         { return m_measDelayMs; }
    void SetVerbose(bool enabled)       { m_verbose = enabled; }
    bool GetVerbose() const             { return m_verbose; }
    void SetModel(SimModel m)           { m_model = m; }
    SimModel GetModel() const           { return m_model; }
    int  GetCommandCount() const        { return m_commandCount; }

private:
    static DWORD WINAPI ServerThreadProc(LPVOID param);
    static DWORD WINAPI MeasDelayThreadProc(LPVOID param);
    static DWORD WINAPI RefDelayThreadProc(LPVOID param);
    void RunServer();
    void HandleClient(SOCKET clientSocket);
    std::string ProcessCommand(const std::string& cmd);

    // Data generation
    double GenerateIL(int channel, double wavelength);
    double GenerateRL(int channel, double wavelength);

    // Logging
    void Log(const char* fmt, ...);

    // State
    bool            m_running;
    int             m_port;
    SOCKET          m_listenSocket;
    HANDLE          m_serverThread;
    SimModel        m_model;

    // Configuration state (mirrors what the driver sends)
    double          m_currentWavelength;
    int             m_currentChannel;
    std::string     m_speedMode;
    bool            m_referenced;
    bool            m_measRunning;
    int             m_measDelayMs;
    DWORD           m_measStartTick;

    // Generated results cache (populated after measurement completes)
    double          m_lastIL;
    double          m_lastRL;

    // Controls
    bool            m_errorMode;
    bool            m_verbose;
    int             m_commandCount;

    // Error queue
    std::vector<std::string> m_errorQueue;

    std::mutex      m_mutex;
};
