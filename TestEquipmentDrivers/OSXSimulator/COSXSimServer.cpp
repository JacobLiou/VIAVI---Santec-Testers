#include "stdafx.h"
#include "COSXSimServer.h"

COSXSimServer::COSXSimServer()
    : m_running(false)
    , m_port(5025)
    , m_listenSocket(INVALID_SOCKET)
    , m_serverThread(NULL)
    , m_selectedModule(0)
    , m_localMode(true)
    , m_switchDelayMs(300)
    , m_ipAddress("192.168.1.100")
    , m_gateway("192.168.1.1")
    , m_netmask("255.255.255.0")
    , m_hostname("OSX-SIM")
    , m_macAddress("00:11:22:33:44:55")
    , m_operationStartTick(0)
    , m_operationDurationMs(0)
    , m_errorMode(false)
    , m_verbose(false)
    , m_commandCount(0)
    , m_eseRegister(0)
    , m_sreRegister(0)
{
    srand(static_cast<unsigned>(time(NULL)));

    // Default: one 1Ax24 module
    SimModule mod;
    mod.name = "SX 1Ax24";
    mod.configType = 0;
    mod.channelCount = 24;
    mod.currentChannel = 1;
    mod.currentCommon = 0;
    m_modules.push_back(mod);
}

COSXSimServer::~COSXSimServer()
{
    Stop();
}

void COSXSimServer::AddModule(const std::string& name, int configType, int channelCount)
{
    SimModule mod;
    mod.name = name;
    mod.configType = configType;
    mod.channelCount = channelCount;
    mod.currentChannel = 1;
    mod.currentCommon = 0;
    m_modules.push_back(mod);
}

void COSXSimServer::ClearModules()
{
    m_modules.clear();
    m_selectedModule = 0;
}

// ---------------------------------------------------------------------------
// Async operation tracking
// ---------------------------------------------------------------------------

void COSXSimServer::BeginOperation()
{
    m_operationStartTick = GetTickCount();
    m_operationDurationMs = m_switchDelayMs;
}

bool COSXSimServer::IsOperationBusy() const
{
    if (m_operationDurationMs <= 0)
        return false;
    DWORD elapsed = GetTickCount() - m_operationStartTick;
    return elapsed < static_cast<DWORD>(m_operationDurationMs);
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

bool COSXSimServer::Start(int port)
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

void COSXSimServer::Stop()
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

DWORD WINAPI COSXSimServer::ServerThreadProc(LPVOID param)
{
    COSXSimServer* self = static_cast<COSXSimServer*>(param);
    self->RunServer();
    return 0;
}

void COSXSimServer::RunServer()
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
    Log("[OSX Sim] Listening on port %d (%d module(s))", m_port, (int)m_modules.size());

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
        Log("[OSX Sim] Client connected from %s:%d", addrBuf, ntohs(clientAddr.sin_port));

        HandleClient(clientSocket);

        Log("[OSX Sim] Client disconnected.");
    }
}

void COSXSimServer::HandleClient(SOCKET clientSocket)
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

            // Handle chained commands separated by ';'
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
// SCPI Command Processing (OSX User Manual Tables 4 & 5)
// ---------------------------------------------------------------------------

std::string COSXSimServer::ProcessCommand(const std::string& cmd)
{
    std::string upper = cmd;
    for (size_t i = 0; i < upper.size(); ++i)
        upper[i] = static_cast<char>(toupper(static_cast<unsigned char>(upper[i])));

    // ==== IEEE 488.2 Common Commands (Table 4) ====

    if (upper == "*IDN?")
    {
        return "Santec,OSX-100,SIM001,1.0.0";
    }

    if (upper == "*RST")
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (size_t i = 0; i < m_modules.size(); ++i)
        {
            m_modules[i].currentChannel = 1;
            m_modules[i].currentCommon = 0;
        }
        m_selectedModule = 0;
        m_localMode = true;
        m_errorQueue.clear();
        m_eseRegister = 0;
        m_sreRegister = 0;
        m_operationDurationMs = 0;
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

    if (upper == "*WAI")
    {
        while (IsOperationBusy())
            Sleep(10);
        return "";
    }

    if (upper == "*STB?")
    {
        return "0";
    }

    if (upper.find("*ESE") == 0)
    {
        if (upper == "*ESE?")
        {
            char buf[16];
            sprintf_s(buf, "%d", m_eseRegister);
            return buf;
        }
        size_t sp = upper.find(' ');
        if (sp != std::string::npos)
            m_eseRegister = atoi(cmd.substr(sp + 1).c_str());
        return "";
    }

    if (upper == "*ESR?")
    {
        return "0";
    }

    if (upper.find("*SRE") == 0)
    {
        if (upper == "*SRE?")
        {
            char buf[16];
            sprintf_s(buf, "%d", m_sreRegister);
            return buf;
        }
        size_t sp = upper.find(' ');
        if (sp != std::string::npos)
            m_sreRegister = atoi(cmd.substr(sp + 1).c_str());
        return "";
    }

    if (upper == "*OPT?")
    {
        return "0";
    }

    if (upper == "*TST?")
    {
        return "0";
    }

    // ==== Status Registers (Table 4) ====

    if (upper == ":STAT:OPER:COND?" || upper == "STAT:OPER:COND?")
    {
        return IsOperationBusy() ? "1" : "0";
    }

    if (upper.find("STAT:OPER") != std::string::npos)
    {
        if (upper.find("ENAB") != std::string::npos && upper.back() != '?')
            return "";
        return "0";
    }

    if (upper.find("STAT:QUES") != std::string::npos)
    {
        if (upper.find("ENAB") != std::string::npos && upper.back() != '?')
            return "";
        return "0";
    }

    if (upper == ":STAT:PRES" || upper == "STAT:PRES")
    {
        return "";
    }

    // ==== System Commands (Table 4) ====

    if (upper == ":SYST:ERR?" || upper == "SYST:ERR?" ||
        upper == ":SYST:ERR:NEXT?" || upper == "SYST:ERR:NEXT?")
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

    if (upper == ":SYST:VERS?" || upper == "SYST:VERS?")
    {
        return "1999.0";
    }

    // ==== Network (Table 4) ====

    if (upper == ":SYST:COMM:LAN:ADDR?" || upper == "SYST:COMM:LAN:ADDR?")
        return m_ipAddress;

    if (upper.find("SYST:COMM:LAN:ADDR") != std::string::npos && upper.back() != '?')
    {
        size_t sp = cmd.find(' ');
        if (sp != std::string::npos)
            m_ipAddress = cmd.substr(sp + 1);
        return "";
    }

    if (upper == ":SYST:COMM:LAN:GATE?" || upper == "SYST:COMM:LAN:GATE?" ||
        upper == ":SYST:COMM:LAN:GATEWAY?" || upper == "SYST:COMM:LAN:GATEWAY?")
        return m_gateway;

    if (upper.find("SYST:COMM:LAN:GATE") != std::string::npos && upper.back() != '?')
    {
        size_t sp = cmd.find(' ');
        if (sp != std::string::npos)
            m_gateway = cmd.substr(sp + 1);
        return "";
    }

    if (upper == ":SYST:COMM:LAN:MASK?" || upper == "SYST:COMM:LAN:MASK?")
        return m_netmask;

    if (upper.find("SYST:COMM:LAN:MASK") != std::string::npos && upper.back() != '?')
    {
        size_t sp = cmd.find(' ');
        if (sp != std::string::npos)
            m_netmask = cmd.substr(sp + 1);
        return "";
    }

    if (upper == ":SYST:COMM:LAN:HOST?" || upper == "SYST:COMM:LAN:HOST?" ||
        upper == ":SYST:COMM:LAN:HOSTNAME?" || upper == "SYST:COMM:LAN:HOSTNAME?")
        return m_hostname;

    if (upper.find("SYST:COMM:LAN:HOST") != std::string::npos && upper.back() != '?')
    {
        size_t sp = cmd.find(' ');
        if (sp != std::string::npos)
            m_hostname = cmd.substr(sp + 1);
        return "";
    }

    if (upper == ":SYST:COMM:LAN:MAC?" || upper == "SYST:COMM:LAN:MAC?")
        return m_macAddress;

    // ==== CLOSe commands (Table 5) ====

    if (upper == "CLOSE?" || upper == "CLOSE?")
    {
        if (m_selectedModule < (int)m_modules.size())
        {
            char buf[16];
            sprintf_s(buf, "%d", m_modules[m_selectedModule].currentChannel);
            return buf;
        }
        return "0";
    }

    if (upper == "CLOSE" || upper == "CLOSE")
    {
        // CLOSe with no argument = move to next channel
        if (m_selectedModule < (int)m_modules.size())
        {
            SimModule& mod = m_modules[m_selectedModule];
            mod.currentChannel++;
            if (mod.currentChannel > mod.channelCount)
                mod.currentChannel = 1;
            BeginOperation();
            Log("  [Switch] Module %d -> channel %d (next)", m_selectedModule, mod.currentChannel);
        }
        return "";
    }

    if (upper.find("CLOSE") == 0 && upper != "CLOSE?" && upper.size() > 5)
    {
        // CLOSe #
        size_t sp = cmd.find(' ');
        if (sp != std::string::npos && m_selectedModule < (int)m_modules.size())
        {
            int ch = atoi(cmd.substr(sp + 1).c_str());
            SimModule& mod = m_modules[m_selectedModule];
            if (ch >= 1 && ch <= mod.channelCount)
            {
                mod.currentChannel = ch;
                BeginOperation();
                Log("  [Switch] Module %d -> channel %d", m_selectedModule, ch);
            }
            else
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_errorQueue.push_back("-222,\"Data out of range\"");
            }
        }
        return "";
    }

    // ==== CFG:SWT:END? (Table 5) ====

    if (upper == "CFG:SWT:END?")
    {
        if (m_selectedModule < (int)m_modules.size())
        {
            char buf[16];
            sprintf_s(buf, "%d", m_modules[m_selectedModule].channelCount);
            return buf;
        }
        return "0";
    }

    // ==== MODule commands (Table 5) ====

    if (upper == "MODULE:CATALOG?" || upper == "MODULE:CAT?")
    {
        std::string result;
        for (size_t i = 0; i < m_modules.size(); ++i)
        {
            if (i > 0) result += ",";
            result += "\"" + m_modules[i].name + "\"";
        }
        return result;
    }

    if (upper == "MODULE:NUMBER?" || upper == "MODULE:NUM?")
    {
        char buf[16];
        sprintf_s(buf, "%d", (int)m_modules.size());
        return buf;
    }

    // MODule#:INFO?
    if (upper.find("MODULE") == 0 && upper.find(":INFO?") != std::string::npos)
    {
        int modIdx = 0;
        // Extract module index from "MODULE#:INFO?"
        size_t numStart = 6; // after "MODULE"
        size_t colonPos = upper.find(':', numStart);
        if (colonPos != std::string::npos && colonPos > numStart)
            modIdx = atoi(upper.substr(numStart, colonPos - numStart).c_str());

        if (modIdx >= 0 && modIdx < (int)m_modules.size())
        {
            const SimModule& mod = m_modules[modIdx];
            std::ostringstream oss;
            oss << mod.name << ",channels=" << mod.channelCount
                << ",current=" << mod.currentChannel
                << ",common=" << mod.currentCommon;
            return oss.str();
        }
        return "";
    }

    if (upper == "MODULE:SELECT?" || upper == "MODULE:SEL?")
    {
        char buf[16];
        sprintf_s(buf, "%d", m_selectedModule);
        return buf;
    }

    if (upper == "MODULE:SELECT" || upper == "MODULE:SEL")
    {
        // Select next module
        m_selectedModule++;
        if (m_selectedModule >= (int)m_modules.size())
            m_selectedModule = 0;
        Log("  [Module] Selected next -> %d", m_selectedModule);
        return "";
    }

    if ((upper.find("MODULE:SELECT") == 0 || upper.find("MODULE:SEL") == 0) && upper.back() != '?')
    {
        size_t sp = cmd.find(' ');
        if (sp != std::string::npos)
        {
            int idx = atoi(cmd.substr(sp + 1).c_str());
            if (idx >= 0 && idx < (int)m_modules.size())
            {
                m_selectedModule = idx;
                Log("  [Module] Selected %d", idx);
            }
            else
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_errorQueue.push_back("-222,\"Data out of range\"");
            }
        }
        return "";
    }

    // ==== ROUTe commands (Table 5) ====

    // ROUTe:CHANnel:ALL #
    if (upper.find("ROUTE:CHANNEL:ALL") == 0 || upper.find("ROUTE:CHAN:ALL") == 0)
    {
        size_t sp = cmd.find(' ');
        if (sp != std::string::npos)
        {
            int ch = atoi(cmd.substr(sp + 1).c_str());
            for (size_t i = 0; i < m_modules.size(); ++i)
            {
                if (ch >= 1 && ch <= m_modules[i].channelCount)
                    m_modules[i].currentChannel = ch;
            }
            BeginOperation();
            Log("  [Route] All modules -> channel %d", ch);
        }
        return "";
    }

    // ROUTe#:CHANnel # / ROUTe#:CLOSe # / ROUTe#:CHANnel? / ROUTe#:COMMon # / ROUTe#:COMMon? / ROUTe#:HOMe
    if (upper.find("ROUTE") == 0)
    {
        int modIdx = 0;
        size_t numStart = 5; // after "ROUTE"
        size_t colonPos = upper.find(':', numStart);
        if (colonPos != std::string::npos && colonPos > numStart)
            modIdx = atoi(upper.substr(numStart, colonPos - numStart).c_str());

        if (modIdx < 0 || modIdx >= (int)m_modules.size())
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_errorQueue.push_back("-222,\"Data out of range\"");
            return "";
        }

        SimModule& mod = m_modules[modIdx];
        std::string afterColon = upper.substr(colonPos + 1);

        // ROUTe#:CHANnel?
        if (afterColon == "CHANNEL?" || afterColon == "CHAN?")
        {
            char buf[16];
            sprintf_s(buf, "%d", mod.currentChannel);
            return buf;
        }

        // ROUTe#:CHANnel # or ROUTe#:CLOSe #
        if (afterColon.find("CHANNEL") == 0 || afterColon.find("CHAN") == 0 ||
            afterColon.find("CLOSE") == 0 || afterColon.find("CLOS") == 0)
        {
            if (afterColon.back() == '?')
            {
                char buf[16];
                sprintf_s(buf, "%d", mod.currentChannel);
                return buf;
            }

            size_t sp = cmd.find(' ');
            if (sp != std::string::npos)
            {
                int ch = atoi(cmd.substr(sp + 1).c_str());
                if (ch >= 1 && ch <= mod.channelCount)
                {
                    mod.currentChannel = ch;
                    BeginOperation();
                    Log("  [Route] Module %d -> channel %d", modIdx, ch);
                }
                else
                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    m_errorQueue.push_back("-222,\"Data out of range\"");
                }
            }
            return "";
        }

        // ROUTe#:COMMon?
        if (afterColon == "COMMON?" || afterColon == "COMM?")
        {
            char buf[16];
            sprintf_s(buf, "%d", mod.currentCommon);
            return buf;
        }

        // ROUTe#:COMMon #
        if (afterColon.find("COMMON") == 0 || afterColon.find("COMM") == 0)
        {
            size_t sp = cmd.find(' ');
            if (sp != std::string::npos)
            {
                mod.currentCommon = atoi(cmd.substr(sp + 1).c_str());
                Log("  [Route] Module %d common -> %d", modIdx, mod.currentCommon);
            }
            return "";
        }

        // ROUTe#:HOMe
        if (afterColon == "HOME" || afterColon == "HOM")
        {
            mod.currentChannel = 1;
            BeginOperation();
            Log("  [Route] Module %d homed", modIdx);
            return "";
        }
    }

    // ==== LCL (Table 5) ====

    if (upper == "LCL?")
    {
        return m_localMode ? "1" : "0";
    }

    if (upper.find("LCL") == 0 && upper != "LCL?")
    {
        size_t sp = upper.find(' ');
        if (sp != std::string::npos)
        {
            m_localMode = (atoi(cmd.substr(sp + 1).c_str()) != 0);
            Log("  [Control] Local mode = %s", m_localMode ? "enabled" : "disabled");
        }
        return "";
    }

    // ==== TEST:NOTIFY (Table 5) ====

    if (upper.find("TEST:NOTIFY") == 0)
    {
        Log("  [Notify] %s", cmd.c_str());
        return "";
    }

    // ==== *RCL / *SAV (Table 4) ====

    if (upper.find("*RCL") == 0 || upper.find("*SAV") == 0)
    {
        return "";
    }

    // ==== Unknown command ====
    Log("  [WARN] Unknown command: %s", cmd.c_str());
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_errorQueue.push_back("-100,\"Command error: " + cmd + "\"");
    }
    return "";
}

// ---------------------------------------------------------------------------
// Logging
// ---------------------------------------------------------------------------

void COSXSimServer::Log(const char* fmt, ...)
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
