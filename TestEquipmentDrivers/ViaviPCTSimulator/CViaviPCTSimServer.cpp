#include "stdafx.h"
#include "CViaviPCTSimServer.h"
#include <cstdarg>

CViaviPCTSimServer::CViaviPCTSimServer()
    : m_running(false)
    , m_port(8301)
    , m_listenSocket(INVALID_SOCKET)
    , m_serverThread(NULL)
    , m_remoteMode(false)
    , m_deviceNum(1)
    , m_deviceType("mORL-A1")
    , m_fiberType("SM")
    , m_outputs(2)
    , m_sw1Channels(8)
    , m_sw2Channels(0)
    , m_measFunc(0)
    , m_avgTime(1)
    , m_dutRange(100)
    , m_ilOnly(false)
    , m_connMode(1)
    , m_bidi(false)
    , m_group(1)
    , m_channel(1)
    , m_measBusy(false)
    , m_measStartTime(0)
    , m_measDelayMs(2000)
    , m_referenced(false)
    , m_errorMode(false)
    , m_verbose(false)
    , m_commandCount(0)
{
    srand(static_cast<unsigned>(time(NULL)));

    m_availableWavelengths.push_back(1310);
    m_availableWavelengths.push_back(1550);
    m_availableWavelengths.push_back(1625);

    m_configuredWavelengths.push_back(1310);
    m_configuredWavelengths.push_back(1550);
}

CViaviPCTSimServer::~CViaviPCTSimServer()
{
    Stop();
}

// ---------------------------------------------------------------------------
// 生命周期
// ---------------------------------------------------------------------------

bool CViaviPCTSimServer::Start(int port)
{
    m_port = port;
    m_running = true;

    m_serverThread = CreateThread(NULL, 0, ServerThreadProc, this, 0, NULL);
    if (!m_serverThread)
    {
        m_running = false;
        return false;
    }
    return true;
}

void CViaviPCTSimServer::Stop()
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
// 服务器线程
// ---------------------------------------------------------------------------

DWORD WINAPI CViaviPCTSimServer::ServerThreadProc(LPVOID param)
{
    CViaviPCTSimServer* self = static_cast<CViaviPCTSimServer*>(param);
    self->RunServer();
    return 0;
}

void CViaviPCTSimServer::RunServer()
{
    m_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_listenSocket == INVALID_SOCKET)
    {
        Log("[ERROR] 创建监听套接字失败。");
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
        Log("[ERROR] 绑定端口 %d 失败 (WSA %d)。", m_port, WSAGetLastError());
        closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
        m_running = false;
        return;
    }

    listen(m_listenSocket, SOMAXCONN);
    Log("[Viavi PCT Sim] 监听端口 %d", m_port);

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
        Log("[Viavi PCT Sim] 客户端连接: %s:%d", addrBuf, ntohs(clientAddr.sin_port));

        HandleClient(clientSocket);

        Log("[Viavi PCT Sim] 客户端断开。");
    }
}

void CViaviPCTSimServer::HandleClient(SOCKET clientSocket)
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

        size_t pos;
        while ((pos = buffer.find('\n')) != std::string::npos)
        {
            std::string cmd = buffer.substr(0, pos);
            buffer.erase(0, pos + 1);

            while (!cmd.empty() && (cmd.back() == '\r' || cmd.back() == ' '))
                cmd.pop_back();
            if (cmd.empty()) continue;

            std::vector<std::string> subCmds;
            std::istringstream cmdStream(cmd);
            std::string subCmd;
            while (std::getline(cmdStream, subCmd, ';'))
            {
                while (!subCmd.empty() && subCmd.front() == ' ') subCmd.erase(0, 1);
                while (!subCmd.empty() && subCmd.back() == ' ') subCmd.pop_back();
                if (!subCmd.empty())
                    subCmds.push_back(subCmd);
            }

            std::string lastResponse;
            for (size_t i = 0; i < subCmds.size(); ++i)
            {
                m_commandCount++;
                if (m_verbose)
                    Log("  RX> %s", subCmds[i].c_str());

                std::string response = ProcessCommand(subCmds[i]);
                if (!response.empty())
                    lastResponse = response;
            }

            if (!lastResponse.empty())
            {
                lastResponse += "\n";
                send(clientSocket, lastResponse.c_str(),
                     static_cast<int>(lastResponse.size()), 0);
                if (m_verbose)
                    Log("  TX< %s", lastResponse.c_str());
            }
        }
    }

    shutdown(clientSocket, SD_BOTH);
    closesocket(clientSocket);
}

// ---------------------------------------------------------------------------
// SCPI 命令处理
// ---------------------------------------------------------------------------

std::string CViaviPCTSimServer::ProcessCommand(const std::string& cmd)
{
    std::string upper = cmd;
    for (size_t i = 0; i < upper.size(); ++i)
        upper[i] = static_cast<char>(toupper(static_cast<unsigned char>(upper[i])));

    // ---- 远程控制 ----

    if (upper == "*REM")
    {
        m_remoteMode = true;
        Log("  [远程] 远程控制模式已激活。");
        return "";
    }

    // ---- IEEE 488.2 通用命令 ----

    if (upper == "*IDN?")
    {
        return "VIAVI,mORL-A1,SIMPCT001,1.0.0";
    }

    if (upper == "*RST")
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_measFunc = 0;
        m_avgTime = 1;
        m_dutRange = 100;
        m_ilOnly = false;
        m_connMode = 1;
        m_bidi = false;
        m_group = 1;
        m_channel = 1;
        m_measBusy = false;
        m_referenced = false;
        m_errorQueue.clear();
        m_configuredWavelengths.clear();
        m_configuredWavelengths.push_back(1310);
        m_configuredWavelengths.push_back(1550);
        return "";
    }

    if (upper == "*CLS")
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_errorQueue.clear();
        return "";
    }

    if (upper == "*OPC?" || upper == "*OPC")
    {
        return "1";
    }

    // ---- 系统命令 ----

    if (upper == ":SYST:ERR?" || upper == "SYST:ERR?")
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

    // ---- 模块信息 ----

    if (upper == ":INFO?" || upper == "INFO?")
    {
        return "SIMPCT001,mORL-A1,2.1.0,1.0.0,1.0.0,HW1.0,2025-01-15";
    }

    if (upper == ":CONF?" || upper == "CONF?")
    {
        std::ostringstream oss;
        oss << m_deviceNum << "," << m_deviceType << "," << m_fiberType
            << "," << m_outputs << "," << m_sw1Channels << "," << m_sw2Channels;
        for (size_t i = 0; i < m_availableWavelengths.size(); ++i)
            oss << "," << m_availableWavelengths[i];
        return oss.str();
    }

    // ---- 光源/波长配置 ----

    if (upper == ":SOUR:WAV?" || upper == "SOUR:WAV?")
    {
        if (!m_configuredWavelengths.empty())
        {
            char buf[32];
            sprintf_s(buf, "%d", m_configuredWavelengths[0]);
            return buf;
        }
        return "1310";
    }

    if (upper.find(":SOUR:WAV ") == 0 || upper.find("SOUR:WAV ") == 0)
    {
        size_t sp = cmd.find(' ');
        if (sp != std::string::npos)
        {
            int wav = atoi(cmd.substr(sp + 1).c_str());
            m_configuredWavelengths.clear();
            m_configuredWavelengths.push_back(wav);
            Log("  [光源] 波长设置为 %d nm。", wav);
        }
        return "";
    }

    if (upper == ":SOUR:WAV:AVA?" || upper == "SOUR:WAV:AVA?")
    {
        std::ostringstream oss;
        for (size_t i = 0; i < m_availableWavelengths.size(); ++i)
        {
            if (i > 0) oss << ",";
            oss << m_availableWavelengths[i];
        }
        return oss.str();
    }

    if (upper == ":SOUR:LIST?" || upper == "SOUR:LIST?")
    {
        std::ostringstream oss;
        for (size_t i = 0; i < m_configuredWavelengths.size(); ++i)
        {
            if (i > 0) oss << ",";
            oss << m_configuredWavelengths[i];
        }
        return oss.str();
    }

    if (upper.find(":SOUR:LIST ") == 0 || upper.find("SOUR:LIST ") == 0)
    {
        size_t sp = cmd.find(' ');
        if (sp != std::string::npos)
        {
            m_configuredWavelengths.clear();
            std::string params = cmd.substr(sp + 1);
            std::istringstream iss(params);
            std::string token;
            while (std::getline(iss, token, ','))
            {
                while (!token.empty() && token.front() == ' ') token.erase(0, 1);
                int wav = atoi(token.c_str());
                if (wav > 0)
                    m_configuredWavelengths.push_back(wav);
            }
            Log("  [光源] 波长列表已配置: %d 个波长。", (int)m_configuredWavelengths.size());
        }
        return "";
    }

    // ---- 传感器/模式 ----

    if (upper == ":SENS:FUNC?" || upper == "SENS:FUNC?")
    {
        char buf[16];
        sprintf_s(buf, "%d", m_measFunc);
        return buf;
    }

    if (upper.find(":SENS:FUNC ") == 0 || upper.find("SENS:FUNC ") == 0)
    {
        size_t sp = cmd.find(' ');
        if (sp != std::string::npos)
        {
            m_measFunc = atoi(cmd.substr(sp + 1).c_str());
            Log("  [传感器] 功能模式 = %s。", m_measFunc == 0 ? "参考" : "DUT");
        }
        return "";
    }

    if (upper == ":SENS:AVG?" || upper == "SENS:AVG?")
    {
        char buf[16];
        sprintf_s(buf, "%d", m_avgTime);
        return buf;
    }

    if (upper.find(":SENS:AVG ") == 0 || upper.find("SENS:AVG ") == 0)
    {
        size_t sp = cmd.find(' ');
        if (sp != std::string::npos)
        {
            m_avgTime = atoi(cmd.substr(sp + 1).c_str());
            Log("  [传感器] 平均时间 = %d 秒。", m_avgTime);
        }
        return "";
    }

    if (upper == ":SENS:RANG?" || upper == "SENS:RANG?")
    {
        char buf[16];
        sprintf_s(buf, "%d", m_dutRange);
        return buf;
    }

    if (upper.find(":SENS:RANG ") == 0 || upper.find("SENS:RANG ") == 0)
    {
        size_t sp = cmd.find(' ');
        if (sp != std::string::npos)
        {
            m_dutRange = atoi(cmd.substr(sp + 1).c_str());
            Log("  [传感器] DUT 范围 = %d 米。", m_dutRange);
        }
        return "";
    }

    if (upper == ":SENS:ILON?" || upper == "SENS:ILON?")
    {
        return m_ilOnly ? "1" : "0";
    }

    if (upper.find(":SENS:ILON ") == 0 || upper.find("SENS:ILON ") == 0)
    {
        size_t sp = cmd.find(' ');
        if (sp != std::string::npos)
        {
            m_ilOnly = (atoi(cmd.substr(sp + 1).c_str()) != 0);
            Log("  [传感器] IL-only = %s。", m_ilOnly ? "启用" : "禁用");
        }
        return "";
    }

    if (upper == ":SENS:CONN?" || upper == "SENS:CONN?")
    {
        char buf[16];
        sprintf_s(buf, "%d", m_connMode);
        return buf;
    }

    if (upper.find(":SENS:CONN ") == 0 || upper.find("SENS:CONN ") == 0)
    {
        size_t sp = cmd.find(' ');
        if (sp != std::string::npos)
        {
            m_connMode = atoi(cmd.substr(sp + 1).c_str());
            Log("  [传感器] 连接模式 = %d 跳线。", m_connMode);
        }
        return "";
    }

    if (upper == ":SENS:BIDI?" || upper == "SENS:BIDI?")
    {
        return m_bidi ? "1" : "0";
    }

    if (upper.find(":SENS:BIDI ") == 0 || upper.find("SENS:BIDI ") == 0)
    {
        size_t sp = cmd.find(' ');
        if (sp != std::string::npos)
        {
            m_bidi = (atoi(cmd.substr(sp + 1).c_str()) != 0);
            Log("  [传感器] 双向 = %s。", m_bidi ? "启用" : "禁用");
        }
        return "";
    }

    // ---- 路径/通道 ----

    if (upper == ":PATH:CHAN?" || upper == "PATH:CHAN?")
    {
        char buf[32];
        sprintf_s(buf, "%d,%d", m_group, m_channel);
        return buf;
    }

    if (upper.find(":PATH:CHAN ") == 0 || upper.find("PATH:CHAN ") == 0)
    {
        size_t sp = cmd.find(' ');
        if (sp != std::string::npos)
        {
            std::string params = cmd.substr(sp + 1);
            size_t comma = params.find(',');
            if (comma != std::string::npos)
            {
                m_group = atoi(params.substr(0, comma).c_str());
                m_channel = atoi(params.substr(comma + 1).c_str());
            }
            else
            {
                m_channel = atoi(params.c_str());
            }
            Log("  [路径] 组 = %d, 通道 = %d。", m_group, m_channel);
        }
        return "";
    }

    // ---- 测量控制 ----

    if (upper == ":MEAS:STAR" || upper == "MEAS:STAR")
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_measBusy = true;
        m_measStartTime = GetTickCount();

        if (m_measFunc == 0)
            m_referenced = true;

        Log("  [测量] 开始 %s 测量（延迟 %d ms）。",
            m_measFunc == 0 ? "参考" : "DUT", m_measDelayMs);
        return "";
    }

    if (upper == ":MEAS:STAT?" || upper == "MEAS:STAT?")
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_measBusy)
        {
            DWORD elapsed = GetTickCount() - m_measStartTime;
            if (elapsed >= static_cast<DWORD>(m_measDelayMs))
            {
                m_measBusy = false;
                return "1";
            }
            return "2";
        }
        return "1";
    }

    if (upper == ":MEAS:ABOR" || upper == "MEAS:ABOR")
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_measBusy = false;
        Log("  [测量] 已中止。");
        return "";
    }

    // ---- 测量结果 ----

    if (upper.find(":MEAS:ALL?") == 0 || upper.find("MEAS:ALL?") == 0)
    {
        int ch = m_channel;
        size_t sp = cmd.find('?');
        if (sp != std::string::npos && sp + 1 < cmd.size())
        {
            std::string param = cmd.substr(sp + 1);
            while (!param.empty() && param.front() == ' ') param.erase(0, 1);
            if (!param.empty()) ch = atoi(param.c_str());
        }

        std::ostringstream oss;
        for (size_t wi = 0; wi < m_configuredWavelengths.size(); ++wi)
        {
            if (wi > 0) oss << ";";
            double wav = static_cast<double>(m_configuredWavelengths[wi]);
            double il = GenerateIL(ch, wav);
            double orl = GenerateORL(ch, wav);
            double orlZ1 = orl - 0.5 - (rand() % 50) * 0.01;
            double orlZ2 = orl - 1.0 - (rand() % 50) * 0.01;
            double len = GenerateLength();
            double pwr = GeneratePower(wav);

            char buf[256];
            sprintf_s(buf, "%.1f,%.4f,%.2f,%.2f,%.2f,%.2f,%.2f",
                wav, il, orl, orlZ1, orlZ2, len, pwr);
            oss << buf;
        }
        return oss.str();
    }

    if (upper.find(":MEAS:IL?") == 0 || upper.find("MEAS:IL?") == 0)
    {
        int ch = m_channel;
        size_t sp = cmd.find('?');
        if (sp != std::string::npos && sp + 1 < cmd.size())
        {
            std::string param = cmd.substr(sp + 1);
            while (!param.empty() && param.front() == ' ') param.erase(0, 1);
            if (!param.empty()) ch = atoi(param.c_str());
        }

        double wav = m_configuredWavelengths.empty() ? 1310.0 : static_cast<double>(m_configuredWavelengths[0]);
        double il = GenerateIL(ch, wav);
        char buf[32];
        sprintf_s(buf, "%.4f", il);
        return buf;
    }

    if (upper.find(":MEAS:ORL?") == 0 || upper.find("MEAS:ORL?") == 0)
    {
        int ch = m_channel;
        size_t sp = cmd.find('?');
        if (sp != std::string::npos && sp + 1 < cmd.size())
        {
            std::string param = cmd.substr(sp + 1);
            while (!param.empty() && param.front() == ' ') param.erase(0, 1);
            if (!param.empty()) ch = atoi(param.c_str());
        }

        double wav = m_configuredWavelengths.empty() ? 1310.0 : static_cast<double>(m_configuredWavelengths[0]);
        double orl = GenerateORL(ch, wav);
        char buf[32];
        sprintf_s(buf, "%.2f", orl);
        return buf;
    }

    if (upper.find(":MEAS:LEN?") == 0 || upper.find("MEAS:LEN?") == 0)
    {
        double len = GenerateLength();
        char buf[32];
        sprintf_s(buf, "%.2f", len);
        return buf;
    }

    // ---- 忙碌状态 ----

    if (upper == ":BUSY?" || upper == "BUSY?")
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_measBusy)
        {
            DWORD elapsed = GetTickCount() - m_measStartTime;
            if (elapsed >= static_cast<DWORD>(m_measDelayMs))
            {
                m_measBusy = false;
                return "0";
            }
            return "1";
        }
        return "0";
    }

    // ---- 未知命令 ----
    Log("  [WARN] 未知命令: %s", cmd.c_str());
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_errorQueue.push_back("-100,\"Command error: " + cmd + "\"");
    }
    return "";
}

// ---------------------------------------------------------------------------
// 模拟数据生成
// ---------------------------------------------------------------------------

double CViaviPCTSimServer::GenerateIL(int channel, double wavelength)
{
    double base = 0.10 + (channel - 1) * 0.02;
    if (wavelength > 1500) base += 0.03;
    double noise = (rand() % 100 - 50) * 0.001;
    double result = base + noise;
    if (!m_referenced) result += 0.5;
    if (m_errorMode) result += (rand() % 100) * 0.01;
    return result > 0 ? result : 0.01;
}

double CViaviPCTSimServer::GenerateORL(int channel, double wavelength)
{
    double base = 55.0 + (channel - 1) * 0.5;
    if (wavelength > 1500) base -= 1.0;
    double noise = (rand() % 100 - 50) * 0.1;
    double result = base + noise;
    if (!m_referenced) result -= 10.0;
    if (m_errorMode) result -= (rand() % 200) * 0.1;
    return result > 10 ? result : 10.0;
}

double CViaviPCTSimServer::GenerateLength()
{
    double base = 3.0;
    double noise = (rand() % 100) * 0.01;
    return base + noise;
}

double CViaviPCTSimServer::GeneratePower(double wavelength)
{
    double base = -10.0;
    if (wavelength > 1500) base -= 1.0;
    double noise = (rand() % 100) * 0.01;
    return base + noise;
}

// ---------------------------------------------------------------------------
// 日志
// ---------------------------------------------------------------------------

void CViaviPCTSimServer::Log(const char* fmt, ...)
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
