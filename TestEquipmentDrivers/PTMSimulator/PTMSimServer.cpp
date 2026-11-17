#include "stdafx.h"
#include "PTMSimServer.h"

static std::string ToUpper(std::string s)
{
    for (size_t i = 0; i < s.size(); ++i)
        s[i] = static_cast<char>(::toupper(static_cast<unsigned char>(s[i])));
    return s;
}

static std::string Trim(const std::string& s)
{
    size_t b = 0;
    while (b < s.size() && isspace(static_cast<unsigned char>(s[b]))) ++b;
    size_t e = s.size();
    while (e > b && isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return s.substr(b, e - b);
}

CPTMSimServer::CPTMSimServer()
    : m_running(false)
    , m_port(5025)
    , m_listenSocket(INVALID_SOCKET)
    , m_serverThread(NULL)
    , m_channels(12)
    , m_localMode(1)
    , m_measDelayMs(200)
    , m_busyUntilTick(0)
    , m_operCond(0)
    , m_lastStatus("PASS")
    , m_barcode("")
    , m_errorMode(false)
    , m_verbose(false)
    , m_commandCount(0)
{
}

CPTMSimServer::~CPTMSimServer()
{
    Stop();
}

bool CPTMSimServer::Start(int port, int channels)
{
    m_port = port;
    SetChannels(channels);
    m_running = true;
    m_serverThread = CreateThread(NULL, 0, ServerThreadProc, this, 0, NULL);
    if (!m_serverThread)
    {
        m_running = false;
        return false;
    }
    return true;
}

void CPTMSimServer::Stop()
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

DWORD WINAPI CPTMSimServer::ServerThreadProc(LPVOID param)
{
    static_cast<CPTMSimServer*>(param)->RunServer();
    return 0;
}

void CPTMSimServer::RunServer()
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
    Log("[PTM Sim] Listening on port %d (%d channels)", m_port, m_channels);

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
        Log("[PTM Sim] Client connected from %s:%d", addrBuf, ntohs(clientAddr.sin_port));
        HandleClient(clientSocket);
        Log("[PTM Sim] Client disconnected.");
    }
}

void CPTMSimServer::HandleClient(SOCKET clientSocket)
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
            if (WSAGetLastError() == WSAETIMEDOUT) continue;
            break;
        }

        chunk[received] = '\0';
        buffer.append(chunk, received);

        size_t pos;
        while ((pos = buffer.find('\n')) != std::string::npos)
        {
            std::string cmd = buffer.substr(0, pos);
            buffer.erase(0, pos + 1);
            while (!cmd.empty() && (cmd.back() == '\r' || cmd.back() == ' '))
                cmd.pop_back();
            if (cmd.empty()) continue;

            std::string response = ProcessCommand(cmd);
            if (!response.empty())
            {
                response += "\n";
                send(clientSocket, response.c_str(), static_cast<int>(response.size()), 0);
            }
        }
    }

    closesocket(clientSocket);
}

void CPTMSimServer::BeginBusy(int delayMs)
{
    m_operCond = 1;
    m_busyUntilTick = GetTickCount() + static_cast<DWORD>((std::max)(0, delayMs));
}

bool CPTMSimServer::IsBusy() const
{
    return m_operCond != 0 && GetTickCount() < m_busyUntilTick;
}

std::string CPTMSimServer::BuildTypeAResult(int n) const
{
    // Provisional CSV: out,in,out,in,... Type-A straight-through
    std::ostringstream oss;
    for (int i = 1; i <= n; ++i)
    {
        if (i > 1) oss << ",";
        oss << i << "," << i;
    }
    return oss.str();
}

std::string CPTMSimServer::ProcessCommand(const std::string& cmdRaw)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    ++m_commandCount;

    std::string cmd = Trim(cmdRaw);
    std::string upper = ToUpper(cmd);

    if (m_verbose)
        Log("RX> %s", cmd.c_str());

    // Update busy flag
    if (m_operCond != 0 && GetTickCount() >= m_busyUntilTick)
        m_operCond = 0;

    // ---- IEEE / system ----
    if (upper == "*IDN?")
        return "SANTEC,PTM,SIM-0001,1.0.0";
    if (upper == "*CLS")
    {
        m_errorQueue.clear();
        return "";
    }
    if (upper == "*RST")
    {
        m_localMode = 1;
        m_operCond = 0;
        m_lastResult.clear();
        return "";
    }
    if (upper == "*OPC?")
        return "1";
    if (upper.find("SYST:ERR") == 0 || upper.find("SYSTEM:ERROR") == 0)
    {
        if (m_errorMode)
            return "100,\"Simulated error\"";
        if (!m_errorQueue.empty())
        {
            std::string e = m_errorQueue.front();
            m_errorQueue.erase(m_errorQueue.begin());
            return e;
        }
        return "0,\"No error\"";
    }
    if (upper.find("SYST:VER") == 0)
        return "1999.0";

    if (upper.find("STAT:OPER:COND") == 0)
    {
        if (m_operCond != 0 && GetTickCount() >= m_busyUntilTick)
            m_operCond = 0;
        return std::to_string(m_operCond);
    }

    // ---- Table 8 ----
    if (upper.find("READ:BARC") == 0)
        return m_barcode.empty() ? "NOBARCODE" : m_barcode;

    if (upper.find("TEST:NOTIFY") == 0)
    {
        Log("[NOTIFY] %s", cmd.c_str());
        return "";
    }

    if (upper == "LCL?")
        return std::to_string(m_localMode);
    if (upper.find("LCL") == 0)
    {
        size_t sp = cmd.find_first_of(" \t");
        if (sp != std::string::npos)
            m_localMode = atoi(cmd.substr(sp + 1).c_str()) ? 1 : 0;
        return "";
    }

    if (upper == "LAS:OFF")
        return "";

    // VFL# / FVFL#
    if (upper.size() >= 4 && (upper.compare(0, 3, "VFL") == 0 || upper.compare(0, 4, "FVFL") == 0))
    {
        BeginBusy(50);
        return "";
    }

    if (upper.find("CHAN") == 0 && upper.find('?') != std::string::npos)
        return std::to_string(m_channels);

    // DETECT#?
    if (upper.compare(0, 6, "DETECT") == 0)
    {
        // DETECT12? -> return 12 (brightest = self)
        std::string num;
        for (size_t i = 6; i < upper.size() && isdigit(static_cast<unsigned char>(upper[i])); ++i)
            num.push_back(upper[i]);
        int ch = num.empty() ? 1 : atoi(num.c_str());
        BeginBusy(m_measDelayMs);
        // For query forms, return after marking busy; driver re-queries after idle.
        if (upper.find('?') != std::string::npos)
            return std::to_string(ch);
        return "";
    }

    // TEST:CON#
    if (upper.compare(0, 8, "TEST:CON") == 0)
    {
        std::string num;
        for (size_t i = 8; i < upper.size() && isdigit(static_cast<unsigned char>(upper[i])); ++i)
            num.push_back(upper[i]);
        int n = num.empty() ? m_channels : atoi(num.c_str());
        if (n <= 0 || n > m_channels) n = m_channels;
        m_lastResult = BuildTypeAResult(n);
        m_lastStatus = "PASS";
        BeginBusy(m_measDelayMs);
        return "";
    }

    if (upper.find("TEST:RES") == 0)
    {
        if (m_lastResult.empty())
            m_lastResult = BuildTypeAResult(m_channels);
        return m_lastResult;
    }

    if (upper == "TEST:START")
    {
        m_lastResult = BuildTypeAResult(m_channels);
        m_lastStatus = m_errorMode ? "FAIL" : "PASS";
        BeginBusy(m_measDelayMs);
        return "";
    }
    if (upper.find("TEST:STA") == 0)
        return m_lastStatus;
    if (upper == "TEST:NEXT" || upper == "TEST:RETRY")
        return "";

    m_errorQueue.push_back("-113,\"Undefined header\"");
    Log("[WARN] Unknown command: %s", cmd.c_str());
    return "";
}

void CPTMSimServer::Log(const char* fmt, ...)
{
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    printf("%s\n", buf);
}
