#include "stdafx.h"
#include "SantecSimServer.h"
#include <cstdarg>

CSantecSimServer::CSantecSimServer()
    : m_running(false)
    , m_port(5025)
    , m_listenSocket(INVALID_SOCKET)
    , m_serverThread(NULL)
    , m_model(SIM_RLM_100)
    , m_currentWavelength(1310.0)
    , m_currentChannel(1)
    , m_speedMode("FAST")
    , m_referenced(false)
    , m_measRunning(false)
    , m_measDelayMs(1500)
    , m_measStartTick(0)
    , m_lastIL(0.0)
    , m_lastRL(0.0)
    , m_errorMode(false)
    , m_verbose(false)
    , m_commandCount(0)
{
    srand(static_cast<unsigned>(time(NULL)));
}

CSantecSimServer::~CSantecSimServer()
{
    Stop();
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

bool CSantecSimServer::Start(int port, SimModel model)
{
    m_port = port;
    m_model = model;
    m_running = true;

    m_serverThread = CreateThread(NULL, 0, ServerThreadProc, this, 0, NULL);
    if (!m_serverThread)
    {
        m_running = false;
        return false;
    }
    return true;
}

void CSantecSimServer::Stop()
{
    m_running = false;

    if (m_listenSocket != INVALID_SOCKET)
    {
        closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
    }

    if (m_serverThread)
    {
        WaitForSingleObject(m_serverThread, 5000);
        CloseHandle(m_serverThread);
        m_serverThread = NULL;
    }
}

// ---------------------------------------------------------------------------
// Server thread
// ---------------------------------------------------------------------------

DWORD WINAPI CSantecSimServer::ServerThreadProc(LPVOID param)
{
    CSantecSimServer* self = static_cast<CSantecSimServer*>(param);
    self->RunServer();
    return 0;
}

DWORD WINAPI CSantecSimServer::MeasDelayThreadProc(LPVOID param)
{
    CSantecSimServer* self = static_cast<CSantecSimServer*>(param);
    DWORD delay = self->GetMeasDelayMs() > 0 ? (DWORD)self->GetMeasDelayMs() : 1500;
    Sleep(delay);
    {
        std::lock_guard<std::mutex> lock(self->m_mutex);
        self->m_measRunning = false;
    }
    self->Log("  [Meas] Measurement completed.");
    return 0;
}

DWORD WINAPI CSantecSimServer::RefDelayThreadProc(LPVOID param)
{
    CSantecSimServer* self = static_cast<CSantecSimServer*>(param);
    DWORD delay = self->GetMeasDelayMs() > 0 ? (DWORD)self->GetMeasDelayMs() : 2000;
    Sleep(delay);
    {
        std::lock_guard<std::mutex> lock(self->m_mutex);
        self->m_measRunning = false;
        self->m_referenced = true;
    }
    self->Log("  [Reference] Reference completed.");
    return 0;
}

void CSantecSimServer::RunServer()
{
    m_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_listenSocket == INVALID_SOCKET)
    {
        Log("[ERROR] Failed to create listen socket.");
        m_running = false;
        return;
    }

    BOOL reuseAddr = TRUE;
    setsockopt(m_listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuseAddr, sizeof(reuseAddr));

    sockaddr_in bindAddr = {};
    bindAddr.sin_family = AF_INET;
    bindAddr.sin_addr.s_addr = INADDR_ANY;
    bindAddr.sin_port = htons(static_cast<u_short>(m_port));

    if (bind(m_listenSocket, (sockaddr*)&bindAddr, sizeof(bindAddr)) == SOCKET_ERROR)
    {
        Log("[ERROR] Bind failed on port %d (WSA %d).", m_port, WSAGetLastError());
        closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
        m_running = false;
        return;
    }

    listen(m_listenSocket, SOMAXCONN);
    Log("[Santec Sim] Listening on port %d (%s mode)",
        m_port, m_model == SIM_RLM_100 ? "RLM-100" : "ILM-100");

    while (m_running)
    {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(m_listenSocket, &readfds);

        timeval tv = { 1, 0 };
        int sel = select(0, &readfds, NULL, NULL, &tv);
        if (sel <= 0) continue;

        sockaddr_in clientAddr = {};
        int addrLen = sizeof(clientAddr);
        SOCKET clientSocket = accept(m_listenSocket, (sockaddr*)&clientAddr, &addrLen);
        if (clientSocket == INVALID_SOCKET) continue;

        char addrBuf[64] = {};
        inet_ntop(AF_INET, &clientAddr.sin_addr, addrBuf, sizeof(addrBuf));
        Log("[Santec Sim] Client connected from %s:%d", addrBuf, ntohs(clientAddr.sin_port));

        HandleClient(clientSocket);

        Log("[Santec Sim] Client disconnected.");
    }
}

void CSantecSimServer::HandleClient(SOCKET clientSocket)
{
    DWORD timeoutMs = 500;
    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeoutMs, sizeof(timeoutMs));

    std::string buffer;
    char chunk[4096];

    while (m_running)
    {
        int received = recv(clientSocket, chunk, sizeof(chunk) - 1, 0);
        if (received <= 0)
        {
            if (received == 0) break;
            int err = WSAGetLastError();
            if (err == WSAETIMEDOUT) continue;
            break;
        }

        chunk[received] = '\0';
        buffer.append(chunk, received);

        // Process all complete commands (delimited by \n)
        size_t pos;
        while ((pos = buffer.find('\n')) != std::string::npos)
        {
            std::string cmd = buffer.substr(0, pos);
            buffer.erase(0, pos + 1);

            // Trim CR
            while (!cmd.empty() && (cmd.back() == '\r' || cmd.back() == ' '))
                cmd.pop_back();
            if (cmd.empty()) continue;

            m_commandCount++;
            if (m_verbose)
                Log("  RX> %s", cmd.c_str());

            std::string response = ProcessCommand(cmd);
            if (!response.empty())
            {
                response += "\n";
                send(clientSocket, response.c_str(), static_cast<int>(response.size()), 0);
                if (m_verbose)
                    Log("  TX< %s", response.c_str());
            }
        }
    }

    shutdown(clientSocket, SD_BOTH);
    closesocket(clientSocket);
}

// ---------------------------------------------------------------------------
// SCPI Command Processing
// ---------------------------------------------------------------------------

std::string CSantecSimServer::ProcessCommand(const std::string& cmd)
{
    // Uppercase for matching
    std::string upper = cmd;
    for (size_t i = 0; i < upper.size(); ++i)
        upper[i] = static_cast<char>(toupper(static_cast<unsigned char>(upper[i])));

    // ---- IEEE 488.2 Common Commands ----

    if (upper == "*IDN?")
    {
        if (m_model == SIM_RLM_100)
            return "Santec,RLM-100,SIM001,1.0.0";
        else
            return "Santec,ILM-100,SIM002,1.0.0";
    }

    if (upper == "*RST")
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_referenced = false;
        m_measRunning = false;
        m_currentWavelength = 1310.0;
        m_currentChannel = 1;
        m_speedMode = "FAST";
        m_errorQueue.clear();
        return "";
    }

    if (upper == "*CLS")
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_errorQueue.clear();
        return "";
    }

    if (upper == "*OPC?")
    {
        return "1";
    }

    // ---- System Error ----

    if (upper == "SYST:ERR?")
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_errorMode && (rand() % 5 == 0))
            return "-200,\"Execution error\"";
        if (!m_errorQueue.empty())
        {
            std::string err = m_errorQueue.front();
            m_errorQueue.erase(m_errorQueue.begin());
            return err;
        }
        return "0,\"No error\"";
    }

    // ---- Configuration ----

    if (upper.find("CONF:WAV") == 0)
    {
        if (upper.back() == '?')
        {
            char buf[32];
            sprintf_s(buf, "%d", static_cast<int>(m_currentWavelength));
            return buf;
        }

        size_t sp = upper.find(' ');
        if (sp != std::string::npos)
        {
            m_currentWavelength = atof(cmd.substr(sp + 1).c_str());
            Log("  Config: wavelength = %.0f nm", m_currentWavelength);
        }
        return "";
    }

    if (upper.find("CONF:CHAN") == 0)
    {
        size_t sp = upper.find(' ');
        if (sp != std::string::npos)
        {
            m_currentChannel = atoi(cmd.substr(sp + 1).c_str());
            Log("  Config: channel = %d", m_currentChannel);
        }
        return "";
    }

    if (upper.find("CONF:SPEED") == 0)
    {
        size_t sp = upper.find(' ');
        if (sp != std::string::npos)
        {
            m_speedMode = cmd.substr(sp + 1);
            Log("  Config: speed = %s", m_speedMode.c_str());
        }
        return "";
    }

    // ---- Reference ----

    if (upper == "REF:STAR")
    {
        Log("  [Reference] Starting reference calibration...");
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_measRunning = true;
            m_measStartTick = GetTickCount();
        }
        // Non-blocking: delay runs in a separate thread so the handler
        // can keep processing STAT:OPER? polls from the driver.
        CreateThread(NULL, 0, RefDelayThreadProc, this, 0, NULL);
        return "";
    }

    if (upper == "REF:STAT?")
    {
        return m_referenced ? "1" : "0";
    }

    if (upper == "REF:CLE")
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_referenced = false;
        }
        Log("  [Reference] Cleared.");
        return "";
    }

    // ---- Measurement ----

    if (upper == "INIT:IMM")
    {
        Log("  [Meas] Measurement initiated (Ch%d, %.0f nm)...",
            m_currentChannel, m_currentWavelength);
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_measRunning = true;
            m_measStartTick = GetTickCount();
            m_lastIL = GenerateIL(m_currentChannel, m_currentWavelength);
            m_lastRL = GenerateRL(m_currentChannel, m_currentWavelength);
        }
        // Simulate measurement delay in a separate thread to allow status polling
        CreateThread(NULL, 0, MeasDelayThreadProc, this, 0, NULL);
        return "";
    }

    if (upper == "ABOR")
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_measRunning = false;
        }
        Log("  [Meas] Aborted.");
        return "";
    }

    if (upper == "STAT:OPER?")
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_measRunning ? "16" : "0";  // bit 4 = measurement running
    }

    // ---- Fetch Results ----

    if (upper == "FETC:IL?" || upper == "FETC?")
    {
        char buf[64];
        sprintf_s(buf, "%.4f", m_lastIL);
        return buf;
    }

    if (upper == "FETC:RL?")
    {
        if (m_model == SIM_ILM_100)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_errorQueue.push_back("-200,\"RL measurement not supported on ILM-100\"");
            return "";
        }
        char buf[64];
        sprintf_s(buf, "%.4f", m_lastRL);
        return buf;
    }

    // ---- Unknown command ----
    Log("  [WARN] Unknown command: %s", cmd.c_str());
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_errorQueue.push_back("-100,\"Command error: " + cmd + "\"");
    }
    return "";
}

// ---------------------------------------------------------------------------
// Data Generation
// ---------------------------------------------------------------------------

double CSantecSimServer::GenerateIL(int channel, double wavelength)
{
    // Typical IL: 0.05 - 0.50 dB for good connectors
    double base = 0.10 + (channel - 1) * 0.02;
    if (wavelength > 1500) base += 0.03;  // slightly higher at 1550
    double noise = (rand() % 100 - 50) * 0.001;  // +/- 0.05 dB
    double result = base + noise;
    if (!m_referenced) result += 0.5;  // worse without reference
    if (m_errorMode) result += (rand() % 100) * 0.01;  // inject noise
    return result > 0 ? result : 0.01;
}

double CSantecSimServer::GenerateRL(int channel, double wavelength)
{
    // Typical RL: 45 - 65 dB for UPC, 55 - 75 dB for APC
    double base = 55.0 + (channel - 1) * 0.5;
    if (wavelength > 1500) base -= 1.0;
    double noise = (rand() % 100 - 50) * 0.1;  // +/- 5 dB
    double result = base + noise;
    if (!m_referenced) result -= 10.0;
    if (m_errorMode) result -= (rand() % 200) * 0.1;
    return result > 10 ? result : 10.0;
}

// ---------------------------------------------------------------------------
// Logging
// ---------------------------------------------------------------------------

void CSantecSimServer::Log(const char* fmt, ...)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    time_t now = time(NULL);
    struct tm local;
    localtime_s(&local, &now);

    char timeBuf[32];
    strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", &local);

    printf("[%s] ", timeBuf);

    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    printf("\n");
}
