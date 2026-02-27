#pragma once

#include "stdafx.h"

// ---------------------------------------------------------------------------
// Santec RL1 Simulator
//
// TCP server (default port 5025) that responds to official SCPI commands
// matching the Santec RL1 protocol per RLM User Manual M-RL1-001-07.
//
// Simulates:
//   - Device identity (*IDN?)
//   - Laser control (LAS:ENAB, LAS:DISAB, LAS:INFO?)
//   - Fiber info (FIBER:INFO?)
//   - Synchronous IL/RL measurement (READ:IL:det#?, READ:RL?)
//   - Reference (REF:RL, REF:IL:det#)
//   - Configuration (DUT:LENGTH, RL:SENS, RL:GAIN, RL:POSB, OUT:CLOS, SW#:CLOS)
//   - Control (LCL, AUTO:ENAB)
//   - Error query (SYST:ERR?)
//   - Power meter (POW:NUM?, READ:POW:det#?, READ:POW:MON?)
// ---------------------------------------------------------------------------

class CSantecSimServer
{
public:
    enum SimModel { SIM_RL1, SIM_ILM_100 };

    CSantecSimServer();
    ~CSantecSimServer();

    bool Start(int port = 5025, SimModel model = SIM_RL1);
    void Stop();
    bool IsRunning() const { return m_running; }

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
    void RunServer();
    void HandleClient(SOCKET clientSocket);
    std::string ProcessCommand(const std::string& cmd);

    // Data generation
    double GenerateIL(int channel, double wavelength);
    double GenerateRL(int channel, double wavelength);
    double GenerateRLTotal(int channel, double wavelength);
    double GenerateLength();

    void Log(const char* fmt, ...);

    // Server state
    bool            m_running;
    int             m_port;
    SOCKET          m_listenSocket;
    HANDLE          m_serverThread;
    SimModel        m_model;

    // Device state (mirrors what the driver sends)
    int             m_enabledLaser;         // 0=disabled, or wavelength nm
    std::string     m_fiberType;            // "SM" or "MM"
    std::vector<int> m_supportedWavelengths;
    int             m_dutLengthBin;         // 100, 1500, or 4000
    std::string     m_rlSensitivity;        // "fast" or "standard"
    std::string     m_rlGain;               // "normal" or "low"
    std::string     m_rlPosB;               // "eof" or "zero"
    int             m_outputChannel;
    int             m_sw1Channel;
    int             m_sw2Channel;
    bool            m_localMode;
    bool            m_autoStart;
    double          m_dutIL;
    int             m_measDelayMs;

    // Reference state
    bool            m_rlReferenced;
    double          m_refMTJ1Length;
    double          m_refMTJ2Length;
    struct ILRef { int wavelength; double value; };
    std::vector<ILRef> m_ilRefs;

    // Controls
    bool            m_errorMode;
    bool            m_verbose;
    int             m_commandCount;

    // Error queue
    std::vector<std::string> m_errorQueue;

    std::mutex      m_mutex;
};
