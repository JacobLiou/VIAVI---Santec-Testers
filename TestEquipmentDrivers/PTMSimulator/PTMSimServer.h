#pragma once

#include "stdafx.h"

// ---------------------------------------------------------------------------
// Santec PTM Polarity Tester simulator
// TCP SCPI server (default 5025) per M-PTM_PT1-001.04 Chapter 8
// ---------------------------------------------------------------------------

class CPTMSimServer
{
public:
    CPTMSimServer();
    ~CPTMSimServer();

    bool Start(int port = 5025, int channels = 12);
    void Stop();
    bool IsRunning() const { return m_running; }

    void SetChannels(int n) { m_channels = (n > 0) ? n : 12; }
    int  GetChannels() const { return m_channels; }
    void SetMeasDelayMs(int ms) { m_measDelayMs = ms; }
    int  GetMeasDelayMs() const { return m_measDelayMs; }
    void SetVerbose(bool enabled) { m_verbose = enabled; }
    bool GetVerbose() const { return m_verbose; }
    void SetErrorMode(bool enabled) { m_errorMode = enabled; }
    bool GetErrorMode() const { return m_errorMode; }
    int  GetCommandCount() const { return m_commandCount; }

private:
    static DWORD WINAPI ServerThreadProc(LPVOID param);
    void RunServer();
    void HandleClient(SOCKET clientSocket);
    std::string ProcessCommand(const std::string& cmd);
    void BeginBusy(int delayMs);
    bool IsBusy() const;
    std::string BuildTypeAResult(int n) const;
    void Log(const char* fmt, ...);

    bool            m_running;
    int             m_port;
    SOCKET          m_listenSocket;
    HANDLE          m_serverThread;

    int             m_channels;
    int             m_localMode;
    int             m_measDelayMs;
    DWORD           m_busyUntilTick;
    int             m_operCond;          // STAT:OPER:COND?
    std::string     m_lastResult;
    std::string     m_lastStatus;        // PASS/FAIL
    std::string     m_barcode;
    bool            m_errorMode;
    bool            m_verbose;
    int             m_commandCount;
    std::vector<std::string> m_errorQueue;
    std::mutex      m_mutex;
};
