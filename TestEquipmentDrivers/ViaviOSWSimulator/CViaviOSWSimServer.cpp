#include "stdafx.h"
#include "CViaviOSWSimServer.h"
#include <cstdarg>

CViaviOSWSimServer::CViaviOSWSimServer()
    : m_running(false)
    , m_port(8203)
    , m_listenSocket(INVALID_SOCKET)
    , m_serverThread(NULL)
    , m_remoteMode(false)
    , m_deviceCount(1)
    , m_busy(false)
    , m_switchStartTime(0)
    , m_switchDelayMs(300)
    , m_errorMode(false)
    , m_verbose(false)
    , m_commandCount(0)
{
    srand(static_cast<unsigned>(time(NULL)));

    DeviceState dev;
    dev.channelCount = 8;
    dev.currentChannel = 1;
    m_devices[1] = dev;
}

CViaviOSWSimServer::~CViaviOSWSimServer()
{
    Stop();
}

// ---------------------------------------------------------------------------
// 生命周期
// ---------------------------------------------------------------------------

bool CViaviOSWSimServer::Start(int port)
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

void CViaviOSWSimServer::Stop()
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

DWORD WINAPI CViaviOSWSimServer::ServerThreadProc(LPVOID param)
{
    CViaviOSWSimServer* self = static_cast<CViaviOSWSimServer*>(param);
    self->RunServer();
    return 0;
}

void CViaviOSWSimServer::RunServer()
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
    Log("[Viavi OSW Sim] 监听端口 %d", m_port);

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
        Log("[Viavi OSW Sim] 客户端连接: %s:%d", addrBuf, ntohs(clientAddr.sin_port));

        HandleClient(clientSocket);

        Log("[Viavi OSW Sim] 客户端断开。");
    }
}

void CViaviOSWSimServer::HandleClient(SOCKET clientSocket)
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

std::string CViaviOSWSimServer::ProcessCommand(const std::string& cmd)
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
        return "VIAVI,mOSW-C1,SIMOSW001,1.0.0";
    }

    if (upper == "*RST")
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (std::map<int, DeviceState>::iterator it = m_devices.begin(); it != m_devices.end(); ++it)
            it->second.currentChannel = 1;
        m_busy = false;
        m_errorQueue.clear();
        return "";
    }

    if (upper == "*CLS")
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_errorQueue.clear();
        return "";
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
        return "SIMOSW001,mOSW-C1,1.2.0,HW2.0";
    }

    if (upper.find(":DEV:INFO?") == 0 || upper.find("DEV:INFO?") == 0)
    {
        size_t sp = cmd.find('?');
        int devNum = 1;
        if (sp != std::string::npos && sp + 1 < cmd.size())
        {
            std::string param = cmd.substr(sp + 1);
            while (!param.empty() && param.front() == ' ') param.erase(0, 1);
            if (!param.empty()) devNum = atoi(param.c_str());
        }

        std::map<int, DeviceState>::iterator it = m_devices.find(devNum);
        if (it != m_devices.end())
        {
            char buf[128];
            sprintf_s(buf, "SIMOSW-DEV%d,mOSW-C1,1.2.0,HW2.0,%dch",
                devNum, it->second.channelCount);
            return buf;
        }

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_errorQueue.push_back("-200,\"Device not found\"");
        }
        return "";
    }

    if (upper == ":CONF?" || upper == "CONF?")
    {
        std::ostringstream oss;
        oss << m_deviceCount << ",mOSW-C1";
        if (!m_devices.empty())
            oss << "," << m_devices.begin()->second.channelCount;
        else
            oss << ",8";
        return oss.str();
    }

    // ---- 通道路由 ----

    if (upper.find(":PROC:ROUT?") == 0 || upper.find("PROC:ROUT?") == 0)
    {
        int devNum = 1;
        size_t sp = cmd.find('?');
        if (sp != std::string::npos && sp + 1 < cmd.size())
        {
            std::string param = cmd.substr(sp + 1);
            while (!param.empty() && param.front() == ' ') param.erase(0, 1);
            if (!param.empty()) devNum = atoi(param.c_str());
        }

        std::lock_guard<std::mutex> lock(m_mutex);
        std::map<int, DeviceState>::iterator it = m_devices.find(devNum);
        if (it != m_devices.end())
        {
            char buf[16];
            sprintf_s(buf, "%d", it->second.currentChannel);
            return buf;
        }
        return "1";
    }

    if (upper.find(":PROC:ROUT ") == 0 || upper.find("PROC:ROUT ") == 0)
    {
        size_t sp = cmd.find(' ');
        if (sp != std::string::npos)
        {
            std::string params = cmd.substr(sp + 1);
            while (!params.empty() && params.front() == ' ') params.erase(0, 1);

            int devNum = 1;
            int channel = 1;
            size_t comma = params.find(',');
            if (comma != std::string::npos)
            {
                devNum = atoi(params.substr(0, comma).c_str());
                channel = atoi(params.substr(comma + 1).c_str());
            }
            else
            {
                channel = atoi(params.c_str());
            }

            std::lock_guard<std::mutex> lock(m_mutex);
            std::map<int, DeviceState>::iterator it = m_devices.find(devNum);
            if (it == m_devices.end())
            {
                DeviceState newDev;
                newDev.channelCount = 8;
                newDev.currentChannel = channel;
                m_devices[devNum] = newDev;
            }
            else
            {
                if (channel < 1 || channel > it->second.channelCount)
                {
                    m_errorQueue.push_back("-200,\"Channel out of range\"");
                    Log("  [WARN] 通道 %d 超出范围 (设备 %d, 最大 %d)。",
                        channel, devNum, it->second.channelCount);
                    return "";
                }
                it->second.currentChannel = channel;
            }

            m_busy = true;
            m_switchStartTime = GetTickCount();
            Log("  [开关] 设备 %d 切换到通道 %d（延迟 %d ms）。", devNum, channel, m_switchDelayMs);
        }
        return "";
    }

    // ---- 忙碌状态 ----

    if (upper == ":BUSY?" || upper == "BUSY?")
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_busy)
        {
            DWORD elapsed = GetTickCount() - m_switchStartTime;
            if (elapsed >= static_cast<DWORD>(m_switchDelayMs))
            {
                m_busy = false;
                return "0";
            }
            return "1";
        }
        return "0";
    }

    // ---- 复位 ----

    if (upper == ":RES" || upper == "RES")
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (std::map<int, DeviceState>::iterator it = m_devices.begin(); it != m_devices.end(); ++it)
            it->second.currentChannel = 1;
        m_busy = false;
        Log("  [复位] 所有设备已复位到通道 1。");
        return "";
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
// 日志
// ---------------------------------------------------------------------------

void CViaviOSWSimServer::Log(const char* fmt, ...)
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
