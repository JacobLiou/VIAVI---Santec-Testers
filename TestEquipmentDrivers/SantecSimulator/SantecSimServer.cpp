#include "stdafx.h"
#include "SantecSimServer.h"
#include <cstdarg>

CSantecSimServer::CSantecSimServer()
    : m_running(false)
    , m_port(5025)
    , m_listenSocket(INVALID_SOCKET)
    , m_serverThread(NULL)
    , m_model(SIM_RL1)
    , m_enabledLaser(0)
    , m_fiberType("SM")
    , m_dutLengthBin(100)
    , m_rlSensitivity("fast")
    , m_rlGain("normal")
    , m_rlPosB("eof")
    , m_outputChannel(1)
    , m_sw1Channel(1)
    , m_sw2Channel(1)
    , m_localMode(true)
    , m_autoStart(false)
    , m_dutIL(0.0)
    , m_measDelayMs(1500)
    , m_rlReferenced(false)
    , m_refMTJ1Length(3.0)
    , m_refMTJ2Length(0.0)
    , m_errorMode(false)
    , m_verbose(false)
    , m_commandCount(0)
{
    srand(static_cast<unsigned>(time(NULL)));

    m_supportedWavelengths.push_back(1310);
    m_supportedWavelengths.push_back(1490);
    m_supportedWavelengths.push_back(1550);
    m_supportedWavelengths.push_back(1625);
}

CSantecSimServer::~CSantecSimServer()
{
    Stop();
}

// ---------------------------------------------------------------------------
// 生命周期
// ---------------------------------------------------------------------------

bool CSantecSimServer::Start(int port, SimModel model)
{
    m_port = port;
    m_model = model;
    m_running = true;

    if (model == SIM_ILM_100)
    {
        m_fiberType = "SM";
    }

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
// 服务器线程
// ---------------------------------------------------------------------------

DWORD WINAPI CSantecSimServer::ServerThreadProc(LPVOID param)
{
    CSantecSimServer* self = static_cast<CSantecSimServer*>(param);
    self->RunServer();
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
        m_port, m_model == SIM_RL1 ? "RL1" : "ILM-100");

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

        size_t pos;
        while ((pos = buffer.find('\n')) != std::string::npos)
        {
            std::string cmd = buffer.substr(0, pos);
            buffer.erase(0, pos + 1);

            while (!cmd.empty() && (cmd.back() == '\r' || cmd.back() == ' '))
                cmd.pop_back();
            if (cmd.empty()) continue;

            // 处理以 ';' 分隔的链式命令
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
// SCPI 命令处理（M-RL1-001-07 官方 RL1 命令）
// ---------------------------------------------------------------------------

std::string CSantecSimServer::ProcessCommand(const std::string& cmd)
{
    std::string upper = cmd;
    for (size_t i = 0; i < upper.size(); ++i)
        upper[i] = static_cast<char>(toupper(static_cast<unsigned char>(upper[i])));

    // ---- IEEE 488.2 通用命令 (表 11) ----

    if (upper == "*IDN?")
    {
        if (m_model == SIM_RL1)
            return "Santec,RL1,SIM001,1.0.0";
        else
            return "Santec,ILM-100,SIM002,1.0.0";
    }

    if (upper == "*RST")
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_rlReferenced = false;
        m_enabledLaser = 0;
        m_outputChannel = 1;
        m_sw1Channel = 1;
        m_sw2Channel = 1;
        m_dutLengthBin = 100;
        m_rlSensitivity = "fast";
        m_rlGain = "normal";
        m_rlPosB = "eof";
        m_localMode = true;
        m_autoStart = false;
        m_dutIL = 0.0;
        m_errorQueue.clear();
        m_ilRefs.clear();
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
        return "";
    }

    // ---- 系统 (表 11) ----

    if (upper == "SYST:ERR?" || upper == ":SYST:ERR?" || upper == "SYST:ERR:NEXT?")
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

    if (upper == "SYST:VER?")
    {
        return "1999.0";
    }

    // ---- 激光器控制 (表 12) ----

    if (upper == "LAS:DISAB" || upper == "LASER:DISABLE" || upper == "LAS:DISABLE")
    {
        m_enabledLaser = 0;
        Log("  [Laser] Disabled.");
        return "";
    }

    if (upper.find("LAS:ENAB") == 0 || upper.find("LASER:ENAB") == 0)
    {
        if (upper.back() == '?')
        {
            char buf[32];
            sprintf_s(buf, "%d", m_enabledLaser);
            return buf;
        }

        size_t sp = upper.find(' ');
        if (sp != std::string::npos)
        {
            m_enabledLaser = atoi(cmd.substr(sp + 1).c_str());
            Log("  [Laser] Enabled at %d nm.", m_enabledLaser);
        }
        return "";
    }

    if (upper == "LAS:INFO?" || upper == "LASER:INFO?")
    {
        std::string result;
        for (size_t i = 0; i < m_supportedWavelengths.size(); ++i)
        {
            if (i > 0) result += ",";
            char buf[16];
            sprintf_s(buf, "%d", m_supportedWavelengths[i]);
            result += buf;
        }
        return result;
    }

    // ---- 光纤信息 (表 12) ----

    if (upper == "FIBER:INFO?")
    {
        return m_fiberType;
    }

    // ---- RL 灵敏度 (表 12) ----

    if (upper.find("RL:SENS") == 0)
    {
        if (upper.back() == '?')
            return m_rlSensitivity;

        size_t sp = upper.find(' ');
        if (sp != std::string::npos)
        {
            m_rlSensitivity = cmd.substr(sp + 1);
            for (size_t i = 0; i < m_rlSensitivity.size(); ++i)
                m_rlSensitivity[i] = static_cast<char>(
                    tolower(static_cast<unsigned char>(m_rlSensitivity[i])));
            Log("  [Config] RL sensitivity = %s", m_rlSensitivity.c_str());
        }
        return "";
    }

    // ---- RL 增益 (表 12) ----

    if (upper.find("RL:GAIN") == 0)
    {
        if (upper.back() == '?')
            return m_rlGain;

        size_t sp = upper.find(' ');
        if (sp != std::string::npos)
        {
            m_rlGain = cmd.substr(sp + 1);
            for (size_t i = 0; i < m_rlGain.size(); ++i)
                m_rlGain[i] = static_cast<char>(
                    tolower(static_cast<unsigned char>(m_rlGain[i])));
            Log("  [Config] RL gain = %s", m_rlGain.c_str());
        }
        return "";
    }

    // ---- RL POSB (表 12) ----

    if (upper.find("RL:POSB") == 0)
    {
        if (upper.back() == '?')
            return m_rlPosB;

        size_t sp = upper.find(' ');
        if (sp != std::string::npos)
        {
            m_rlPosB = cmd.substr(sp + 1);
            for (size_t i = 0; i < m_rlPosB.size(); ++i)
                m_rlPosB[i] = static_cast<char>(
                    tolower(static_cast<unsigned char>(m_rlPosB[i])));
            Log("  [Config] RL POSB = %s", m_rlPosB.c_str());
        }
        return "";
    }

    // ---- DUT 长度 (表 12) ----

    if (upper.find("DUT:LENGTH") == 0)
    {
        if (upper.back() == '?')
        {
            char buf[32];
            sprintf_s(buf, "%d", m_dutLengthBin);
            return buf;
        }

        size_t sp = upper.find(' ');
        if (sp != std::string::npos)
        {
            m_dutLengthBin = atoi(cmd.substr(sp + 1).c_str());
            Log("  [Config] DUT length bin = %d m", m_dutLengthBin);
        }
        return "";
    }

    // ---- DUT IL (表 12) ----

    if (upper.find("DUT:IL") == 0)
    {
        if (upper.back() == '?')
        {
            char buf[32];
            sprintf_s(buf, "%.4f", m_dutIL);
            return buf;
        }

        size_t sp = upper.find(' ');
        if (sp != std::string::npos)
        {
            m_dutIL = atof(cmd.substr(sp + 1).c_str());
            Log("  [Config] DUT IL = %.4f dB", m_dutIL);
        }
        return "";
    }

    // ---- 输出 / 内部开关 (表 12) ----

    if (upper.find("OUT:CLOS") == 0)
    {
        if (upper.back() == '?')
        {
            char buf[16];
            sprintf_s(buf, "%d", m_outputChannel);
            return buf;
        }

        size_t sp = upper.find(' ');
        if (sp != std::string::npos)
        {
            m_outputChannel = atoi(cmd.substr(sp + 1).c_str());
            Log("  [Switch] Output channel = %d", m_outputChannel);
        }
        return "";
    }

    // ---- 外部开关 SW#:CLOSe (表 12) ----

    if (upper.find("SW") == 0 && upper.find(":CLOS") != std::string::npos)
    {
        int swNum = 0;
        if (upper.size() > 2 && upper[2] >= '0' && upper[2] <= '9')
            swNum = upper[2] - '0';

        if (upper.back() == '?')
        {
            char buf[16];
            int ch = (swNum == 1) ? m_sw1Channel : m_sw2Channel;
            sprintf_s(buf, "%d", ch);
            return buf;
        }

        size_t sp = upper.find(' ');
        if (sp != std::string::npos)
        {
            int ch = atoi(cmd.substr(sp + 1).c_str());
            if (swNum == 1) m_sw1Channel = ch;
            else if (swNum == 2) m_sw2Channel = ch;
            Log("  [Switch] SW%d channel = %d", swNum, ch);
        }
        return "";
    }

    // ---- SW#:INFO? (表 12) ----

    if (upper.find("SW") == 0 && upper.find(":INFO?") != std::string::npos)
    {
        return "SX1";
    }

    // ---- 本地控制 (表 12) ----

    if (upper.find("LCL") == 0)
    {
        if (upper == "LCL?")
        {
            return m_localMode ? "1" : "0";
        }

        size_t sp = upper.find(' ');
        if (sp != std::string::npos)
        {
            m_localMode = (atoi(cmd.substr(sp + 1).c_str()) != 0);
            Log("  [Config] Local mode = %s", m_localMode ? "enabled" : "disabled");
        }
        return "";
    }

    // ---- 自动启动 (表 12) ----

    if (upper.find("AUTO:ENAB") == 0)
    {
        if (upper.back() == '?')
            return m_autoStart ? "1" : "0";

        size_t sp = upper.find(' ');
        if (sp != std::string::npos)
        {
            m_autoStart = (atoi(cmd.substr(sp + 1).c_str()) != 0);
            Log("  [Config] Auto-start = %s", m_autoStart ? "enabled" : "disabled");
        }
        return "";
    }

    if (upper == "AUTO:TRIG?")
    {
        return "0";
    }

    if (upper.find("AUTO:TRIG:RST") == 0)
    {
        return "";
    }

    // ---- RL 参考 (表 12) ----

    if (upper.find("REF:RL") == 0)
    {
        if (upper == "REF:RL?")
        {
            char buf[32];
            sprintf_s(buf, "%.2f", m_refMTJ1Length);
            return buf;
        }

        size_t sp = upper.find(' ');
        if (sp != std::string::npos)
        {
            // REF:RL #,[#] - 手动设置 MTJ1 和可选的 MTJ2 长度
            std::string params = cmd.substr(sp + 1);
            size_t comma = params.find(',');
            if (comma != std::string::npos)
            {
                m_refMTJ1Length = atof(params.substr(0, comma).c_str());
                m_refMTJ2Length = atof(params.substr(comma + 1).c_str());
            }
            else
            {
                m_refMTJ1Length = atof(params.c_str());
            }
            m_rlReferenced = true;
            Log("  [Reference] RL ref set manually: MTJ1=%.2f, MTJ2=%.2f",
                m_refMTJ1Length, m_refMTJ2Length);
        }
        else
        {
            // REF:RL 无参数 - 自动测量跳线
            Log("  [Reference] RL auto-reference measuring...");
            Sleep(m_measDelayMs > 0 ? m_measDelayMs : 1000);
            m_refMTJ1Length = 3.0 + (rand() % 100) * 0.001;
            m_rlReferenced = true;
            Log("  [Reference] RL auto-reference done: MTJ1=%.3f m", m_refMTJ1Length);
        }
        return "";
    }

    // ---- IL 参考 (表 12) ----
    // REF:IL:det# <wavelength>,<value>  或  REF:IL:det#? <wavelength>

    if (upper.find("REF:IL:DET") == 0 || upper.find("REF:IL:DET") == 0)
    {
        bool isQuery = (upper.find('?') != std::string::npos);

        size_t sp = cmd.find(' ');
        if (isQuery && sp != std::string::npos)
        {
            int wav = atoi(cmd.substr(sp + 1).c_str());
            for (size_t i = 0; i < m_ilRefs.size(); ++i)
            {
                if (m_ilRefs[i].wavelength == wav)
                {
                    char buf[32];
                    sprintf_s(buf, "%.4f", m_ilRefs[i].value);
                    return buf;
                }
            }
            return "0.0000";
        }
        else if (sp != std::string::npos)
        {
            std::string params = cmd.substr(sp + 1);
            size_t comma = params.find(',');
            if (comma != std::string::npos)
            {
                int wav = atoi(params.substr(0, comma).c_str());
                double val = atof(params.substr(comma + 1).c_str());

                bool found = false;
                for (size_t i = 0; i < m_ilRefs.size(); ++i)
                {
                    if (m_ilRefs[i].wavelength == wav)
                    {
                        m_ilRefs[i].value = val;
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    ILRef ref;
                    ref.wavelength = wav;
                    ref.value = val;
                    m_ilRefs.push_back(ref);
                }
                Log("  [Reference] IL ref set: det @%d nm = %.4f dB", wav, val);
            }
        }
        return "";
    }

    // ---- 功率计 (表 12) ----

    if (upper == "POW:NUM?")
    {
        return "1";
    }

    if (upper.find("POW:DET") != std::string::npos && upper.find(":INFO?") != std::string::npos)
    {
        return "SIM-DET001,2025-01-01,1.0.0,100,wired";
    }

    // ---- 同步测量读取 (表 12) ----

    // READ:IL:det#? <wavelength>
    if (upper.find("READ:IL:DET") == 0 || upper.find("READ:IL:DET") == 0)
    {
        size_t sp = cmd.find(' ');
        double wavelength = (sp != std::string::npos) ? atof(cmd.substr(sp + 1).c_str()) : 1310.0;

        Log("  [Meas] READ:IL Ch%d @%.0f nm...", m_outputChannel, wavelength);
        if (m_measDelayMs > 100)
            Sleep(m_measDelayMs / 4);

        double il = GenerateIL(m_outputChannel, wavelength);
        char buf[64];
        sprintf_s(buf, "%.4f", il);
        return buf;
    }

    // READ:RL? <wavelength> 或 READ:RL? <wavelength>,<refLenA>,<refLenB>
    if (upper.find("READ:RL?") == 0)
    {
        if (m_model == SIM_ILM_100)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_errorQueue.push_back("-200,\"RL measurement not supported on ILM-100\"");
            return "";
        }

        size_t sp = cmd.find(' ');
        double wavelength = 1310.0;
        if (sp != std::string::npos)
        {
            std::string params = cmd.substr(sp + 1);
            size_t comma = params.find(',');
            wavelength = atof(params.substr(0, (comma != std::string::npos) ? comma : params.size()).c_str());
        }

        Log("  [Meas] READ:RL Ch%d @%.0f nm (sensitivity=%s, lengthBin=%d)...",
            m_outputChannel, wavelength, m_rlSensitivity.c_str(), m_dutLengthBin);

        // 根据灵敏度模拟测量延迟
        int delay = m_measDelayMs;
        if (m_rlSensitivity == "standard" && m_dutLengthBin >= 4000)
            delay = (std::max)(delay, 8000);
        else if (m_rlSensitivity == "standard")
            delay = (std::max)(delay, 3000);
        if (delay > 100)
            Sleep(delay);

        double rla = GenerateRL(m_outputChannel, wavelength);
        double rlb = GenerateRL(m_outputChannel, wavelength) - 2.0;
        double rltotal = GenerateRLTotal(m_outputChannel, wavelength);
        double length = GenerateLength();

        char buf[128];
        sprintf_s(buf, "%.2f,%.2f,%.2f,%.2f", rla, rlb, rltotal, length);
        return buf;
    }

    // READ:POW:det#? <wavelength>
    if (upper.find("READ:POW:DET") == 0)
    {
        char buf[32];
        sprintf_s(buf, "%.2f", -10.0 + (rand() % 100) * 0.01);
        return buf;
    }

    // READ:POW:MON? <wavelength>
    if (upper.find("READ:POW:MON?") == 0)
    {
        char buf[32];
        sprintf_s(buf, "%.2f", -5.0 + (rand() % 100) * 0.01);
        return buf;
    }

    // READ:FACT:POW? <output>,<wavelength>
    if (upper.find("READ:FACT:POW?") == 0)
    {
        char buf[32];
        sprintf_s(buf, "%.2f", -8.0 + (rand() % 100) * 0.01);
        return buf;
    }

    // READ:BARC?
    if (upper == "READ:BARC?")
    {
        return "SIM-BARCODE-001";
    }

    // ---- 测试计划 (表 12) ----

    if (upper.find("TEST:NOTIFY") == 0)
    {
        Log("  [Notify] %s", cmd.c_str());
        return "";
    }

    if (upper == "TEST:RETRY")
    {
        Log("  [Test] Retry.");
        return "";
    }

    if (upper == "TEST:NEXT")
    {
        Log("  [Test] Next DUT.");
        return "";
    }

    // ---- 状态寄存器 (表 11) ----

    if (upper.find("STAT:OPER") != std::string::npos)
    {
        return "0";
    }

    if (upper.find("STAT:QUES") != std::string::npos)
    {
        return "0";
    }

    if (upper == "STAT:PRES" || upper == ":STAT:PRES")
    {
        return "";
    }

    // ---- 未知命令 ----
    Log("  [WARN] Unknown command: %s", cmd.c_str());
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_errorQueue.push_back("-100,\"Command error: " + cmd + "\"");
    }
    return "";
}

// ---------------------------------------------------------------------------
// 数据生成
// ---------------------------------------------------------------------------

double CSantecSimServer::GenerateIL(int channel, double wavelength)
{
    double base = 0.10 + (channel - 1) * 0.02;
    if (wavelength > 1500) base += 0.03;
    double noise = (rand() % 100 - 50) * 0.001;
    double result = base + noise;
    if (!m_rlReferenced) result += 0.5;
    if (m_errorMode) result += (rand() % 100) * 0.01;
    return result > 0 ? result : 0.01;
}

double CSantecSimServer::GenerateRL(int channel, double wavelength)
{
    double base = 55.0 + (channel - 1) * 0.5;
    if (wavelength > 1500) base -= 1.0;
    if (m_rlGain == "low") base -= 20.0;
    double noise = (rand() % 100 - 50) * 0.1;
    double result = base + noise;
    if (!m_rlReferenced) result -= 10.0;
    if (m_errorMode) result -= (rand() % 200) * 0.1;
    return result > 10 ? result : 10.0;
}

double CSantecSimServer::GenerateRLTotal(int channel, double wavelength)
{
    double rl = GenerateRL(channel, wavelength);
    return rl - 3.0 - (rand() % 50) * 0.1;
}

double CSantecSimServer::GenerateLength()
{
    double base = 3.0;
    double noise = (rand() % 100) * 0.01;
    return base + noise;
}

// ---------------------------------------------------------------------------
// 日志
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
