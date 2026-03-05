#include "stdafx.h"
#include "ViaviSimServer.h"

// ---------------------------------------------------------------------------
// 构造 / 析构
// ---------------------------------------------------------------------------

CViaviSimServer::CViaviSimServer()
    : m_chassisListen(INVALID_SOCKET)
    , m_pctListen(INVALID_SOCKET)
    , m_chassisPort(8100)
    , m_pctPort(8301)
    , m_hChassisThread(NULL)
    , m_hPctThread(NULL)
    , m_running(false)
    , m_errorMode(false)
    , m_verbose(false)
    , m_measDelayMs(2000)
    , m_commandCount(0)
    , m_launchPort(1)
    , m_sensFunc(1)
    , m_measRunning(false)
    , m_measStartTick(0)
{
    m_wavelengths.push_back(1310.0);
    m_wavelengths.push_back(1550.0);
}

CViaviSimServer::~CViaviSimServer()
{
    Stop();
}

// ---------------------------------------------------------------------------
// 启动 / 关闭
// ---------------------------------------------------------------------------

static SOCKET CreateListenSocket(int port)
{
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) return INVALID_SOCKET;

    int opt = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(static_cast<u_short>(port));

    if (bind(s, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        closesocket(s);
        return INVALID_SOCKET;
    }

    if (listen(s, 2) == SOCKET_ERROR)
    {
        closesocket(s);
        return INVALID_SOCKET;
    }

    return s;
}

bool CViaviSimServer::Start(int chassisPort, int pctPort)
{
    if (m_running) return true;

    m_chassisPort = chassisPort;
    m_pctPort = pctPort;

    m_chassisListen = CreateListenSocket(chassisPort);
    if (m_chassisListen == INVALID_SOCKET)
    {
        Log("ERROR", "Failed to listen on Chassis port %d (WSA %d)", chassisPort, WSAGetLastError());
        return false;
    }

    m_pctListen = CreateListenSocket(pctPort);
    if (m_pctListen == INVALID_SOCKET)
    {
        Log("ERROR", "Failed to listen on PCT port %d (WSA %d)", pctPort, WSAGetLastError());
        closesocket(m_chassisListen);
        m_chassisListen = INVALID_SOCKET;
        return false;
    }

    m_running = true;

    m_hChassisThread = CreateThread(NULL, 0, ChassisThreadProc, this, 0, NULL);
    m_hPctThread = CreateThread(NULL, 0, PctThreadProc, this, 0, NULL);

    Log("Chassis", "Listening on port %d", chassisPort);
    Log("PCT    ", "Listening on port %d", pctPort);

    return true;
}

void CViaviSimServer::Stop()
{
    m_running = false;

    if (m_chassisListen != INVALID_SOCKET)
    {
        closesocket(m_chassisListen);
        m_chassisListen = INVALID_SOCKET;
    }
    if (m_pctListen != INVALID_SOCKET)
    {
        closesocket(m_pctListen);
        m_pctListen = INVALID_SOCKET;
    }

    if (m_hChassisThread)
    {
        WaitForSingleObject(m_hChassisThread, 3000);
        CloseHandle(m_hChassisThread);
        m_hChassisThread = NULL;
    }
    if (m_hPctThread)
    {
        WaitForSingleObject(m_hPctThread, 3000);
        CloseHandle(m_hPctThread);
        m_hPctThread = NULL;
    }
}

// ---------------------------------------------------------------------------
// 服务器线程
// ---------------------------------------------------------------------------

DWORD WINAPI CViaviSimServer::ChassisThreadProc(LPVOID param)
{
    reinterpret_cast<CViaviSimServer*>(param)->RunChassisServer();
    return 0;
}

DWORD WINAPI CViaviSimServer::PctThreadProc(LPVOID param)
{
    reinterpret_cast<CViaviSimServer*>(param)->RunPctServer();
    return 0;
}

void CViaviSimServer::RunChassisServer()
{
    DWORD timeout = 500;
    setsockopt(m_chassisListen, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    while (m_running)
    {
        sockaddr_in clientAddr = {};
        int addrLen = sizeof(clientAddr);
        SOCKET client = accept(m_chassisListen, (sockaddr*)&clientAddr, &addrLen);

        if (client == INVALID_SOCKET) continue;

        char ipBuf[INET_ADDRSTRLEN] = {};
        inet_ntop(AF_INET, &clientAddr.sin_addr, ipBuf, sizeof(ipBuf));
        Log("Chassis", "Client connected: %s:%d", ipBuf, ntohs(clientAddr.sin_port));

        HandleClient(client, "Chassis");

        Log("Chassis", "Client disconnected.");
    }
}

void CViaviSimServer::RunPctServer()
{
    DWORD timeout = 500;
    setsockopt(m_pctListen, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    while (m_running)
    {
        sockaddr_in clientAddr = {};
        int addrLen = sizeof(clientAddr);
        SOCKET client = accept(m_pctListen, (sockaddr*)&clientAddr, &addrLen);

        if (client == INVALID_SOCKET) continue;

        char ipBuf[INET_ADDRSTRLEN] = {};
        inet_ntop(AF_INET, &clientAddr.sin_addr, ipBuf, sizeof(ipBuf));
        Log("PCT    ", "Client connected: %s:%d", ipBuf, ntohs(clientAddr.sin_port));

        // 为新会话重置协议状态
        {
            std::lock_guard<std::mutex> lock(m_stateMutex);
            m_measRunning = false;
            m_sensFunc = 1;
        }

        HandleClient(client, "PCT    ");

        Log("PCT    ", "Client disconnected.");
    }
}

// ---------------------------------------------------------------------------
// 客户端处理器（通用接收循环）
// ---------------------------------------------------------------------------

void CViaviSimServer::HandleClient(SOCKET clientSocket, const char* serverTag)
{
    DWORD timeout = 200;
    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    std::string buffer;
    char recvBuf[2048];

    while (m_running)
    {
        int received = recv(clientSocket, recvBuf, sizeof(recvBuf) - 1, 0);

        if (received == SOCKET_ERROR)
        {
            int err = WSAGetLastError();
            if (err == WSAETIMEDOUT) continue;
            break;
        }
        if (received == 0) break;

        recvBuf[received] = '\0';
        buffer.append(recvBuf, received);

        // 处理完整的行
        size_t pos;
        while ((pos = buffer.find('\n')) != std::string::npos)
        {
            std::string cmd = buffer.substr(0, pos);
            buffer.erase(0, pos + 1);

            // 去除 \r
            while (!cmd.empty() && cmd.back() == '\r')
                cmd.pop_back();
            if (cmd.empty()) continue;

            m_commandCount++;

            bool isChassis = (strcmp(serverTag, "Chassis") == 0);
            std::string response;

            if (isChassis)
                response = ProcessChassisCommand(cmd);
            else
                response = ProcessPctCommand(cmd);

            if (!response.empty())
            {
                Log(serverTag, "<< %s  >> %s", cmd.c_str(), response.c_str());
                std::string fullResp = response + "\n";
                send(clientSocket, fullResp.c_str(), static_cast<int>(fullResp.size()), 0);
            }
            else
            {
                LogVerbose(serverTag, "<< %s", cmd.c_str());
            }
        }
    }

    shutdown(clientSocket, SD_BOTH);
    closesocket(clientSocket);
}

// ---------------------------------------------------------------------------
// 机箱协议
// ---------------------------------------------------------------------------

std::string CViaviSimServer::ProcessChassisCommand(const std::string& cmd)
{
    if (cmd.find("SUPER:LAUNCH") == 0)
    {
        Log("Chassis", "<< %s  (Super App launched)", cmd.c_str());
        return "";
    }

    if (cmd.find('?') != std::string::npos)
    {
        Log("Chassis", "<< %s  >> (default response)", cmd.c_str());
        return "0";
    }

    LogVerbose("Chassis", "<< %s", cmd.c_str());
    return "";
}

// ---------------------------------------------------------------------------
// PCT 协议
// ---------------------------------------------------------------------------

std::string CViaviSimServer::ProcessPctCommand(const std::string& cmd)
{
    std::lock_guard<std::mutex> lock(m_stateMutex);

    // ---- 错误查询 ----
    if (cmd == "SYST:ERR?")
    {
        if (m_errorMode)
            return "-100,Command error";
        return "0,No error";
    }

    // ---- 波长配置 ----
    if (cmd.find("SOURCE:LIST ") == 0)
    {
        m_wavelengths.clear();
        std::string params = cmd.substr(12);
        std::istringstream iss(params);
        std::string token;
        while (std::getline(iss, token, ','))
        {
            size_t s = token.find_first_not_of(" ");
            if (s != std::string::npos)
                m_wavelengths.push_back(atof(token.substr(s).c_str()));
        }
        Log("PCT    ", "<< %s  (wavelengths: %d set)", cmd.c_str(), (int)m_wavelengths.size());
        return "";
    }

    if (cmd == "SOURCE:LIST?")
    {
        std::ostringstream oss;
        for (size_t i = 0; i < m_wavelengths.size(); ++i)
        {
            if (i > 0) oss << ",";
            oss << static_cast<int>(m_wavelengths[i]);
        }
        return oss.str();
    }

    // ---- 启动端口 ----
    if (cmd.find("PATH:LAUNCH ") == 0)
    {
        m_launchPort = atoi(cmd.substr(12).c_str());
        Log("PCT    ", "<< %s  (launch port: J%d)", cmd.c_str(), m_launchPort);
        return "";
    }

    // ---- 通道配置 ----
    if (cmd.find("PATH:LIST ") == 0)
    {
        Log("PCT    ", "<< %s  (channels configured)", cmd.c_str());
        return "";
    }

    // ---- 参考覆盖（全部只写）----
    if (cmd.find("PATH:JUMPER:") == 0)
    {
        LogVerbose("PCT    ", "<< %s", cmd.c_str());
        return "";
    }

    // ---- ORL 设置 ----
    if (cmd.find("MEAS:ORL:SETUP ") == 0)
    {
        Log("PCT    ", "<< %s  (ORL configured)", cmd.c_str());
        return "";
    }

    // ---- 测量模式 ----
    if (cmd.find("SENS:FUNC ") == 0)
    {
        m_sensFunc = atoi(cmd.substr(10).c_str());
        Log("PCT    ", "<< %s  (mode: %s)",
            cmd.c_str(), m_sensFunc == 0 ? "REFERENCE" : "MEASUREMENT");
        return "";
    }

    // ---- 开始测量 ----
    if (cmd == "MEAS:START")
    {
        m_measRunning = true;
        m_measStartTick = GetTickCount();
        Log("PCT    ", "<< MEAS:START  (running, delay=%dms)", m_measDelayMs.load());
        return "";
    }

    // ---- 测量状态查询 ----
    if (cmd == "MEAS:STATE?")
    {
        if (!m_measRunning)
            return "0";

        DWORD elapsed = GetTickCount() - m_measStartTick;
        if (elapsed < static_cast<DWORD>(m_measDelayMs))
            return "2";     // 仍在运行

        m_measRunning = false;

        if (m_errorMode)
            return "3";     // 错误

        return "1";         // 完成
    }

    // ---- 结果查询: MEAS:ALL? <ch>,<port> ----
    if (cmd.find("MEAS:ALL?") == 0)
    {
        std::string params = cmd.substr(9);
        // 去除前导空格
        size_t s = params.find_first_not_of(" ");
        if (s != std::string::npos) params = params.substr(s);

        int channel = 1, port = 1;
        size_t comma = params.find(',');
        if (comma != std::string::npos)
        {
            channel = atoi(params.substr(0, comma).c_str());
            port = atoi(params.substr(comma + 1).c_str());
        }

        return GenerateMeasResults(channel, port);
    }

    // ---- 未知查询：返回通用响应 ----
    if (cmd.find('?') != std::string::npos)
    {
        Log("PCT    ", "<< %s  >> 0 (unknown query)", cmd.c_str());
        return "0";
    }

    // ---- 未知写入命令 ----
    LogVerbose("PCT    ", "<< %s  (ignored)", cmd.c_str());
    return "";
}

// ---------------------------------------------------------------------------
// 生成模拟测量数据
//
// 每个波长 5 个值：
//   [0] IL (dB)     - 插入损耗         (典型值 0.1 ~ 0.4)
//   [1] ORL (dB)    - 总回波损耗       (典型值 40 ~ 55)
//   [2] Length (m)   - 光纤长度         (典型值 1.0 ~ 5.0)
//   [3] RL (dB)     - 连接器回波损耗   (典型值 45 ~ 60)
//   [4] Power (dBm) - 光功率           (典型值 -10 ~ -3)
// ---------------------------------------------------------------------------

std::string CViaviSimServer::GenerateMeasResults(int channel, int launchPort)
{
    std::ostringstream oss;
    oss.precision(4);
    oss << std::fixed;

    for (size_t wi = 0; wi < m_wavelengths.size(); ++wi)
    {
        if (wi > 0) oss << ",";

        // 基于种子的通道/波长变化，确保可重复性
        double chVar = (channel - 1) * 0.02;
        double wlVar = (m_wavelengths[wi] > 1400) ? 0.05 : 0.0;

        double il  = 0.15 + chVar + wlVar + (rand() % 100) * 0.001;
        double orl = 48.0 - chVar * 5.0 + (rand() % 100) * 0.05;
        double len = 3.0 + (rand() % 100) * 0.01;
        double rl  = 52.0 - chVar * 3.0 - wlVar * 2.0 + (rand() % 100) * 0.05;
        double pwr = -6.0 + chVar + (rand() % 100) * 0.01;

        oss << il  << "," << orl << "," << len << ","
            << rl  << "," << pwr;
    }

    return oss.str();
}

// ---------------------------------------------------------------------------
// 日志
// ---------------------------------------------------------------------------

void CViaviSimServer::Log(const char* tag, const char* fmt, ...)
{
    std::lock_guard<std::mutex> lock(m_logMutex);

    SYSTEMTIME st;
    GetLocalTime(&st);
    printf("[%02d:%02d:%02d][%s] ",
           st.wHour, st.wMinute, st.wSecond, tag);

    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    printf("\n");
}

void CViaviSimServer::LogVerbose(const char* tag, const char* fmt, ...)
{
    if (!m_verbose) return;

    std::lock_guard<std::mutex> lock(m_logMutex);

    SYSTEMTIME st;
    GetLocalTime(&st);
    printf("[%02d:%02d:%02d][%s] ",
           st.wHour, st.wMinute, st.wSecond, tag);

    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    printf("\n");
}
