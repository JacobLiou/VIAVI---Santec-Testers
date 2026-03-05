#include "stdafx.h"
#include "SantecDriver.h"
#include "SantecVisaAdapter.h"
#include <stdexcept>
#include <sstream>
#include <cmath>

namespace ViaviNSantecTester {

// ===========================================================================
// Santec RL1 官方 SCPI 命令定义
//
// 参考：RLM 用户手册 M-RL1-001-07，表11和表12
// ===========================================================================
namespace SCPI {
    // IEEE 488.2 通用命令（表11）
    static const char* IDN          = "*IDN?";
    static const char* RST          = "*RST";
    static const char* CLS          = "*CLS";
    static const char* OPC_Q        = "*OPC?";
    static const char* OPC          = "*OPC";
    static const char* WAI          = "*WAI";

    // 系统命令（表11）
    static const char* SYS_ERR      = "SYST:ERR?";
    static const char* SYS_VER      = "SYST:VER?";

    // 激光控制（表12）
    static const char* LAS_DISABLE  = "LAS:DISAB";
    static const char* LAS_ENABLE   = "LAS:ENAB";          // LAS:ENAB <波长_nm>
    static const char* LAS_ENABLE_Q = "LAS:ENAB?";
    static const char* LAS_INFO_Q   = "LAS:INFO?";

    // 光纤信息（表12）
    static const char* FIBER_INFO_Q = "FIBER:INFO?";

    // 回波损耗测量 - 同步（表12）
    static const char* READ_RL      = "READ:RL?";           // READ:RL? <nm> -> RLA,RLB,RLTOTAL,长度
    static const char* READ_IL      = "READ:IL";            // READ:IL:det#? <nm> -> 插入损耗值

    // 回波损耗参考（表12）
    static const char* REF_RL       = "REF:RL";             // REF:RL（自动测量）或 REF:RL #,[#]
    static const char* REF_RL_Q     = "REF:RL?";            // 返回 MTJ1 长度
    static const char* REF_IL       = "REF:IL";             // REF:IL:det# <nm>,<值>
    static const char* REF_IL_Q     = "REF:IL";             // REF:IL:det#? <nm>

    // 功率计（表12）
    static const char* POW_NUM_Q    = "POW:NUM?";
    static const char* POW_INFO     = "POW";                // POW:det#:INFO?
    static const char* READ_POW     = "READ:POW";           // READ:POW:det#? <nm>
    static const char* READ_POW_MON = "READ:POW:MON?";      // READ:POW:MON? <nm>

    // 回波损耗灵敏度（表12）
    static const char* RL_SENS      = "RL:SENS";            // RL:SENS <fast|standard>
    static const char* RL_SENS_Q    = "RL:SENS?";

    // 回波损耗B位置（表12）
    static const char* RL_POSB      = "RL:POSB";            // RL:POSB <eof|zero>
    static const char* RL_POSB_Q    = "RL:POSB?";

    // 回波损耗增益（表12）
    static const char* RL_GAIN      = "RL:GAIN";            // RL:GAIN <low|normal>
    static const char* RL_GAIN_Q    = "RL:GAIN?";

    // 被测器件配置（表12）
    static const char* DUT_LENGTH   = "DUT:LENGTH";         // DUT:LENGTH <100|1500|4000>
    static const char* DUT_LENGTH_Q = "DUT:LENGTH?";
    static const char* DUT_IL       = "DUT:IL";             // DUT:IL <值>
    static const char* DUT_IL_Q     = "DUT:IL?";

    // 内部开关/输出（表12）
    static const char* OUT_CLOSE    = "OUT:CLOS";           // OUT:CLOS <通道>
    static const char* OUT_CLOSE_Q  = "OUT:CLOS?";

    // 外部开关（表12）
    // SW#:CLOSe # / SW#:CLOSe? / SW#:INFO?

    // 本地控制（表12）
    static const char* LCL          = "LCL";                // LCL <0|1>
    static const char* LCL_Q        = "LCL?";

    // 自动启动（表12）
    static const char* AUTO_ENABLE  = "AUTO:ENAB";          // AUTO:ENAB <0|1>
    static const char* AUTO_ENABLE_Q= "AUTO:ENAB?";
    static const char* AUTO_TRIG_Q  = "AUTO:TRIG?";
    static const char* AUTO_TRIG_RST= "AUTO:TRIG:RST";     // AUTO:TRIG:RST <nm>,<det#>

    // 工厂数据（表12）
    static const char* READ_FACTORY = "READ:FACT:POW?";     // READ:FACT:POW? <输出>,<nm>

    // 条码（表12）
    static const char* READ_BARCODE = "READ:BARC?";

    // 测试计划/通知（表12）
    static const char* TEST_NOTIFY  = "TEST:NOTIFY";        // TEST:NOTIFY# "字符串"
    static const char* TEST_RETRY   = "TEST:RETRY";
    static const char* TEST_NEXT    = "TEST:NEXT";
}


// ===========================================================================
// CSantecTcpAdapter - TCP SCPI 传输层
// ===========================================================================

CSantecTcpAdapter::CSantecTcpAdapter()
    : m_socket(INVALID_SOCKET)
    , m_bufferSize(4096)
{
}

CSantecTcpAdapter::~CSantecTcpAdapter()
{
    Close();
}

bool CSantecTcpAdapter::Open(const std::string& address, int port, double timeout)
{
    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_socket == INVALID_SOCKET) return false;

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<u_short>(port));
    if (inet_pton(AF_INET, address.c_str(), &addr.sin_addr) != 1)
    {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        return false;
    }

    u_long nonBlocking = 1;
    ioctlsocket(m_socket, FIONBIO, &nonBlocking);

    int ret = connect(m_socket, (sockaddr*)&addr, sizeof(addr));
    if (ret == SOCKET_ERROR)
    {
        int err = WSAGetLastError();
        if (err != WSAEWOULDBLOCK)
        {
            closesocket(m_socket);
            m_socket = INVALID_SOCKET;
            return false;
        }

        fd_set writefds, exceptfds;
        FD_ZERO(&writefds);
        FD_ZERO(&exceptfds);
        FD_SET(m_socket, &writefds);
        FD_SET(m_socket, &exceptfds);

        timeval tv;
        tv.tv_sec = static_cast<long>(timeout);
        tv.tv_usec = static_cast<long>((timeout - static_cast<long>(timeout)) * 1000000);

        ret = select(0, NULL, &writefds, &exceptfds, &tv);
        if (ret <= 0 || FD_ISSET(m_socket, &exceptfds) || !FD_ISSET(m_socket, &writefds))
        {
            closesocket(m_socket);
            m_socket = INVALID_SOCKET;
            return false;
        }

        int optError = 0;
        int optLen = sizeof(optError);
        getsockopt(m_socket, SOL_SOCKET, SO_ERROR, (char*)&optError, &optLen);
        if (optError != 0)
        {
            closesocket(m_socket);
            m_socket = INVALID_SOCKET;
            return false;
        }
    }

    u_long blocking = 0;
    ioctlsocket(m_socket, FIONBIO, &blocking);

    DWORD timeoutMs = static_cast<DWORD>(timeout * 1000);
    setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeoutMs, sizeof(timeoutMs));
    setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeoutMs, sizeof(timeoutMs));

    BOOL keepAlive = TRUE;
    setsockopt(m_socket, SOL_SOCKET, SO_KEEPALIVE, (const char*)&keepAlive, sizeof(keepAlive));

    tcp_keepalive ka = {};
    ka.onoff = 1;
    ka.keepalivetime = 10000;
    ka.keepaliveinterval = 2000;
    DWORD bytesReturned = 0;
    WSAIoctl(m_socket, SIO_KEEPALIVE_VALS, &ka, sizeof(ka), NULL, 0, &bytesReturned, NULL, NULL);

    return true;
}

void CSantecTcpAdapter::Close()
{
    if (m_socket != INVALID_SOCKET)
    {
        shutdown(m_socket, SD_BOTH);
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }
}

bool CSantecTcpAdapter::IsOpen() const
{
    return m_socket != INVALID_SOCKET;
}

void CSantecTcpAdapter::SetReadTimeout(DWORD timeoutMs)
{
    if (m_socket != INVALID_SOCKET)
    {
        setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO,
                   (const char*)&timeoutMs, sizeof(timeoutMs));
    }
}

std::string CSantecTcpAdapter::SendQuery(const std::string& command)
{
    SendWrite(command);

    std::string data;
    char buf[4096];
    while (true)
    {
        int received = recv(m_socket, buf, sizeof(buf) - 1, 0);
        if (received <= 0) break;
        buf[received] = '\0';
        data.append(buf, received);
        if (data.find('\n') != std::string::npos) break;
    }
    while (!data.empty() && (data.back() == '\n' || data.back() == '\r'))
        data.pop_back();
    return data;
}

void CSantecTcpAdapter::SendWrite(const std::string& command)
{
    if (m_socket == INVALID_SOCKET)
        throw std::runtime_error("Santec TCP adapter not connected.");

    std::string fullCmd = command + "\n";
    int sent = send(m_socket, fullCmd.c_str(), static_cast<int>(fullCmd.size()), 0);
    if (sent == SOCKET_ERROR)
        throw std::runtime_error(std::string("Santec TCP send failed: WSA ")
            + std::to_string(WSAGetLastError()));
}


// ===========================================================================
// CSantecDriver
// ===========================================================================

CSantecDriver::CSantecDriver(const std::string& ipAddress,
                             int port,
                             double timeout,
                             CommType commType)
    : CBaseEquipmentDriver(ConnectionConfig(), std::string("Santec"))
    , m_commType(commType)
    , m_adapter(nullptr)
    , m_ownsAdapter(false)
    , m_model(MODEL_UNKNOWN)
    , m_sensitivity(SENSITIVITY_FAST)
    , m_dutLengthBin(LENGTH_BIN_100)
    , m_rlGain(GAIN_NORMAL)
    , m_rlPosB(POSB_EOF)
    , m_localMode(true)
    , m_autoStart(false)
    , m_dutIL(0.0)
    , m_referenced(false)
{
    m_config.ipAddress = ipAddress;
    m_config.port = port;
    m_config.timeout = timeout;
    m_config.bufferSize = 4096;
    m_config.reconnectAttempts = 3;
    m_config.reconnectDelay = 2.0;

    m_wavelengths.push_back(1310.0);
    m_wavelengths.push_back(1550.0);
    m_channels.push_back(1);
}

CSantecDriver::~CSantecDriver()
{
    Disconnect();
    if (m_ownsAdapter && m_adapter)
    {
        delete m_adapter;
        m_adapter = nullptr;
    }
}

void CSantecDriver::SetCommAdapter(ISantecCommAdapter* adapter, bool takeOwnership)
{
    if (m_ownsAdapter && m_adapter)
        delete m_adapter;
    m_adapter = adapter;
    m_ownsAdapter = takeOwnership;
}

// ---------------------------------------------------------------------------
// 统一命令接口
// ---------------------------------------------------------------------------

std::string CSantecDriver::Query(const std::string& command)
{
    m_logger.Debug("TX> %s", command.c_str());
    std::string response;
    if (m_adapter && m_adapter->IsOpen())
        response = m_adapter->SendQuery(command);
    else if (m_socket != INVALID_SOCKET)
        response = SendCommand(command, "\n", true);
    else
        throw std::runtime_error("Not connected to Santec device.");

    m_logger.Debug("RX< %s", response.c_str());
    return response;
}

std::string CSantecDriver::QueryLong(const std::string& command)
{
    CSantecTcpAdapter* tcpAdapter = dynamic_cast<CSantecTcpAdapter*>(m_adapter);
    CSantecVisaAdapter* visaAdapter = dynamic_cast<CSantecVisaAdapter*>(m_adapter);

    if (tcpAdapter)
        tcpAdapter->SetReadTimeout(MEAS_TIMEOUT_MS);
    else if (visaAdapter)
        visaAdapter->SetReadTimeout(MEAS_TIMEOUT_MS);

    std::string result = Query(command);

    DWORD normalTimeout = static_cast<DWORD>(m_config.timeout * 1000);
    if (tcpAdapter)
        tcpAdapter->SetReadTimeout(normalTimeout);
    else if (visaAdapter)
        visaAdapter->SetReadTimeout(normalTimeout);

    return result;
}

void CSantecDriver::Write(const std::string& command)
{
    m_logger.Debug("TX> %s", command.c_str());
    if (m_adapter && m_adapter->IsOpen())
        m_adapter->SendWrite(command);
    else if (m_socket != INVALID_SOCKET)
        SendCommand(command, "\n", false);
    else
        throw std::runtime_error("Not connected to Santec device.");
}

void CSantecDriver::WriteAndSync(const std::string& command)
{
    std::string syncCmd = command + ";" + SCPI::OPC_Q;
    std::string opc = Query(syncCmd);
    m_logger.Debug("OPC sync: %s -> %s", command.c_str(), opc.c_str());
}

// ---------------------------------------------------------------------------
// 连接
// ---------------------------------------------------------------------------

bool CSantecDriver::Connect()
{
    m_logger.Info("Connecting to Santec %s:%d (comm=%d)",
        m_config.ipAddress.c_str(), m_config.port, static_cast<int>(m_commType));

    switch (m_commType)
    {
    case COMM_TCP:
    {
        if (!m_adapter)
        {
            m_adapter = new CSantecTcpAdapter();
            m_ownsAdapter = true;
        }

        for (int attempt = 1; attempt <= m_config.reconnectAttempts; ++attempt)
        {
            m_logger.Info("Connection attempt %d/%d", attempt, m_config.reconnectAttempts);

            if (!m_adapter->Open(m_config.ipAddress, m_config.port, m_config.timeout))
            {
                m_logger.Error("Connection attempt %d failed.", attempt);
                if (attempt < m_config.reconnectAttempts)
                    Sleep(static_cast<DWORD>(m_config.reconnectDelay * 1000));
                continue;
            }

            m_state = STATE_CONNECTED;

            if (!ValidateConnection())
            {
                m_logger.Error("Validation failed (attempt %d).", attempt);
                m_adapter->Close();
                m_state = STATE_DISCONNECTED;
                if (attempt < m_config.reconnectAttempts)
                    Sleep(static_cast<DWORD>(m_config.reconnectDelay * 1000));
                continue;
            }

            m_logger.Info("Santec connected and validated.");
            return true;
        }

        m_state = STATE_ERROR;
        m_logger.Error("All connection attempts exhausted.");
        return false;
    }
    case COMM_GPIB:
        m_logger.Warning("GPIB communication not yet implemented.");
        return false;
    case COMM_USB:
    {
        if (!m_adapter)
        {
            CSantecVisaAdapter* visaAdapter = new CSantecVisaAdapter();
            m_adapter = visaAdapter;
            m_ownsAdapter = true;
        }

        for (int attempt = 1; attempt <= m_config.reconnectAttempts; ++attempt)
        {
            m_logger.Info("VISA 连接尝试 %d/%d", attempt, m_config.reconnectAttempts);

            try
            {
                if (!m_adapter->Open(m_config.ipAddress, m_config.port, m_config.timeout))
                {
                    m_logger.Error("VISA 连接尝试 %d 失败。", attempt);
                    if (attempt < m_config.reconnectAttempts)
                        Sleep(static_cast<DWORD>(m_config.reconnectDelay * 1000));
                    continue;
                }
            }
            catch (const std::exception& ex)
            {
                m_logger.Error("VISA 连接异常: %s", ex.what());
                if (attempt < m_config.reconnectAttempts)
                    Sleep(static_cast<DWORD>(m_config.reconnectDelay * 1000));
                continue;
            }

            m_state = STATE_CONNECTED;

            if (!ValidateConnection())
            {
                m_logger.Error("VISA 验证失败 (尝试 %d)。", attempt);
                m_adapter->Close();
                m_state = STATE_DISCONNECTED;
                if (attempt < m_config.reconnectAttempts)
                    Sleep(static_cast<DWORD>(m_config.reconnectDelay * 1000));
                continue;
            }

            m_logger.Info("Santec VISA 已连接并验证。");
            return true;
        }

        m_state = STATE_ERROR;
        m_logger.Error("所有 VISA 连接尝试已耗尽。");
        return false;
    }
    case COMM_DLL:
        m_logger.Warning("DLL communication not yet implemented.");
        return false;
    default:
        m_logger.Error("Unknown communication type: %d", static_cast<int>(m_commType));
        return false;
    }
}

void CSantecDriver::Disconnect()
{
    if (m_adapter && m_adapter->IsOpen())
    {
        try
        {
            Write(SCPI::LAS_DISABLE);
            Write(SCPI::CLS);
        }
        catch (...) {}
        m_adapter->Close();
        m_state = STATE_DISCONNECTED;
        m_referenced = false;
        m_lastResults.clear();
        m_logger.Info("Santec disconnected.");
    }
    else
    {
        CBaseEquipmentDriver::Disconnect();
    }
}

bool CSantecDriver::IsConnected() const
{
    if (m_adapter)
        return m_state == STATE_CONNECTED && m_adapter->IsOpen();
    return CBaseEquipmentDriver::IsConnected();
}

// ---------------------------------------------------------------------------
// 连接后验证：通过 *IDN? 验证设备身份
// ---------------------------------------------------------------------------

bool CSantecDriver::ValidateConnection()
{
    m_logger.Info("Validating Santec connection (*IDN?)...");
    try
    {
        std::string response = Query(SCPI::IDN);
        if (response.empty())
        {
            m_logger.Error("Validation failed: no response from *IDN?");
            return false;
        }

        ParseIdentity(response);
        m_logger.Info("Santec validated: %s %s (S/N %s, FW %s)",
            m_deviceInfo.manufacturer.c_str(),
            m_deviceInfo.model.c_str(),
            m_deviceInfo.serialNumber.c_str(),
            m_deviceInfo.firmwareVersion.c_str());
        return true;
    }
    catch (const std::exception& e)
    {
        m_logger.Error("Validation exception: %s", e.what());
        return false;
    }
}

void CSantecDriver::ParseIdentity(const std::string& idnResponse)
{
    std::vector<std::string> parts;
    std::istringstream iss(idnResponse);
    std::string token;
    while (std::getline(iss, token, ','))
        parts.push_back(Trim(token));

    if (parts.size() >= 1) m_deviceInfo.manufacturer = parts[0];
    if (parts.size() >= 2) m_deviceInfo.model = parts[1];
    if (parts.size() >= 3) m_deviceInfo.serialNumber = parts[2];
    if (parts.size() >= 4) m_deviceInfo.firmwareVersion = parts[3];

    std::string modelUpper = m_deviceInfo.model;
    for (size_t i = 0; i < modelUpper.size(); ++i)
        modelUpper[i] = static_cast<char>(toupper(static_cast<unsigned char>(modelUpper[i])));

    if (modelUpper.find("RL1") != std::string::npos || modelUpper.find("RLM") != std::string::npos)
        m_model = MODEL_RL1;
    else if (modelUpper.find("IL1") != std::string::npos || modelUpper.find("ILM") != std::string::npos)
        m_model = MODEL_ILM_100;
    else if (modelUpper.find("BR1") != std::string::npos || modelUpper.find("BRM") != std::string::npos)
        m_model = MODEL_BRM_100;
    else
        m_model = MODEL_UNKNOWN;

    m_logger.Info("Auto-detected model: %d (%s)", static_cast<int>(m_model), m_deviceInfo.model.c_str());
}

// ---------------------------------------------------------------------------
// 错误处理
// ---------------------------------------------------------------------------

ErrorInfo CSantecDriver::CheckError()
{
    ErrorInfo info;
    try
    {
        std::string response = Query(SCPI::SYS_ERR);
        if (!response.empty())
        {
            size_t commaPos = response.find(',');
            if (commaPos != std::string::npos)
            {
                info.code = atoi(response.substr(0, commaPos).c_str());
                std::string msg = response.substr(commaPos + 1);
                if (msg.size() >= 2 && msg.front() == '"' && msg.back() == '"')
                    msg = msg.substr(1, msg.size() - 2);
                info.message = msg;
            }
            else
            {
                info.code = atoi(response.c_str());
            }
        }
    }
    catch (const std::exception& e)
    {
        info.code = -1;
        info.message = std::string("CheckError exception: ") + e.what();
    }
    return info;
}

DeviceInfo CSantecDriver::GetDeviceInfo()
{
    if (m_deviceInfo.model.empty() && IsConnected())
    {
        try
        {
            std::string idn = Query(SCPI::IDN);
            ParseIdentity(idn);
        }
        catch (...) {}
    }
    return m_deviceInfo;
}

// ---------------------------------------------------------------------------
// 初始化 - 发现设备能力并应用配置
// ---------------------------------------------------------------------------

bool CSantecDriver::Initialize()
{
    m_logger.Info("Initializing Santec RL1...");

    try
    {
        Write(SCPI::CLS);
        Sleep(100);

        // 发现光纤类型（SM 或 MM）
        try
        {
            m_fiberType = Trim(Query(SCPI::FIBER_INFO_Q));
            m_logger.Info("Fiber type: %s", m_fiberType.c_str());
        }
        catch (...)
        {
            m_logger.Warning("FIBER:INFO? not supported, assuming SM.");
            m_fiberType = "SM";
        }

        // 发现支持的波长
        try
        {
            std::string wavList = Query(SCPI::LAS_INFO_Q);
            m_logger.Info("Supported wavelengths: %s", wavList.c_str());
            std::vector<double> wavs = ParseDoubleList(wavList);
            m_supportedWavelengths.clear();
            for (size_t i = 0; i < wavs.size(); ++i)
                m_supportedWavelengths.push_back(static_cast<int>(wavs[i]));
        }
        catch (...)
        {
            m_logger.Warning("LAS:INFO? not supported.");
        }

        // 应用回波损耗灵敏度（仅单模光纤）
        if (m_fiberType != "MM")
        {
            SetRLSensitivity(m_sensitivity);
            SetDUTLength(m_dutLengthBin);
            SetRLGain(m_rlGain);
        }

        // 禁用本地模式以进行远程控制
        SetLocalMode(false);

        ErrorInfo err = CheckError();
        if (!err.IsOK())
        {
            m_logger.Warning("Init completed with error: [%d] %s", err.code, err.message.c_str());
        }

        std::string opc = Query(SCPI::OPC_Q);
        m_logger.Info("Initialization complete (OPC=%s).", opc.c_str());

        return true;
    }
    catch (const std::exception& e)
    {
        m_logger.Error("Initialize failed: %s", e.what());
        return false;
    }
}

// ---------------------------------------------------------------------------
// RL1 特定配置命令（表12）
// ---------------------------------------------------------------------------

void CSantecDriver::SetRLSensitivity(RLSensitivity sens)
{
    m_sensitivity = sens;
    if (!IsConnected()) return;

    std::string cmd = std::string(SCPI::RL_SENS) + " " +
        (sens == SENSITIVITY_FAST ? "fast" : "standard");
    WriteAndSync(cmd);
    m_logger.Info("RL sensitivity set to %s.", sens == SENSITIVITY_FAST ? "fast" : "standard");
}

void CSantecDriver::SetDUTLength(DUTLengthBin bin)
{
    m_dutLengthBin = bin;
    if (!IsConnected()) return;

    std::ostringstream cmd;
    cmd << SCPI::DUT_LENGTH << " " << static_cast<int>(bin);
    WriteAndSync(cmd.str());
    m_logger.Info("DUT length bin set to %d m.", static_cast<int>(bin));
}

void CSantecDriver::SetRLGain(RLGainMode gain)
{
    m_rlGain = gain;
    if (!IsConnected()) return;

    std::string cmd = std::string(SCPI::RL_GAIN) + " " +
        (gain == GAIN_LOW ? "low" : "normal");
    WriteAndSync(cmd);
    m_logger.Info("RL gain set to %s.", gain == GAIN_LOW ? "low" : "normal");
}

void CSantecDriver::SetRLPosB(RLPosB posb)
{
    m_rlPosB = posb;
    if (!IsConnected()) return;

    std::string cmd = std::string(SCPI::RL_POSB) + " " +
        (posb == POSB_EOF ? "eof" : "zero");
    WriteAndSync(cmd);
    m_logger.Info("RL POSB set to %s.", posb == POSB_EOF ? "eof" : "zero");
}

void CSantecDriver::SetLocalMode(bool enabled)
{
    m_localMode = enabled;
    if (!IsConnected()) return;

    std::string cmd = std::string(SCPI::LCL) + " " + (enabled ? "1" : "0");
    WriteAndSync(cmd);
    m_logger.Info("Local mode %s.", enabled ? "enabled" : "disabled");
}

void CSantecDriver::SetAutoStart(bool enabled)
{
    m_autoStart = enabled;
    if (!IsConnected()) return;

    std::string cmd = std::string(SCPI::AUTO_ENABLE) + " " + (enabled ? "1" : "0");
    WriteAndSync(cmd);
    m_logger.Info("Auto-start %s.", enabled ? "enabled" : "disabled");
}

void CSantecDriver::SetDUTInsertionLoss(double ilDB)
{
    m_dutIL = ilDB;
    if (!IsConnected()) return;

    std::ostringstream cmd;
    cmd << SCPI::DUT_IL << " " << ilDB;
    WriteAndSync(cmd.str());
    m_logger.Info("DUT IL set to %.2f dB.", ilDB);
}

// ---------------------------------------------------------------------------
// 激光控制
// ---------------------------------------------------------------------------

void CSantecDriver::EnableLaser(int wavelengthNm)
{
    std::ostringstream cmd;
    cmd << SCPI::LAS_ENABLE << " " << wavelengthNm;
    WriteAndSync(cmd.str());
    m_logger.Info("Laser enabled at %d nm.", wavelengthNm);
}

void CSantecDriver::DisableLaser()
{
    WriteAndSync(SCPI::LAS_DISABLE);
    m_logger.Info("Laser disabled.");
}

int CSantecDriver::QueryEnabledLaser()
{
    std::string resp = Query(SCPI::LAS_ENABLE_Q);
    return atoi(resp.c_str());
}

// ---------------------------------------------------------------------------
// 开关控制
// ---------------------------------------------------------------------------

void CSantecDriver::SetOutputChannel(int channel)
{
    std::ostringstream cmd;
    cmd << SCPI::OUT_CLOSE << " " << channel;
    WriteAndSync(cmd.str());
    m_logger.Info("Output channel set to %d.", channel);
}

int CSantecDriver::GetOutputChannel()
{
    std::string resp = Query(SCPI::OUT_CLOSE_Q);
    return atoi(resp.c_str());
}

void CSantecDriver::SetSwitchChannel(int switchNum, int channel)
{
    std::ostringstream cmd;
    cmd << "SW" << switchNum << ":CLOS " << channel;
    WriteAndSync(cmd.str());
    m_logger.Info("Switch %d set to channel %d.", switchNum, channel);
}

int CSantecDriver::GetSwitchChannel(int switchNum)
{
    std::ostringstream cmd;
    cmd << "SW" << switchNum << ":CLOS?";
    std::string resp = Query(cmd.str());
    return atoi(resp.c_str());
}

// ---------------------------------------------------------------------------
// 功率计
// ---------------------------------------------------------------------------

int CSantecDriver::GetDetectorCount()
{
    std::string resp = Query(SCPI::POW_NUM_Q);
    return atoi(resp.c_str());
}

std::string CSantecDriver::GetDetectorInfo(int detectorNum)
{
    std::ostringstream cmd;
    cmd << "POW:det" << detectorNum << ":INFO?";
    return Query(cmd.str());
}

// ---------------------------------------------------------------------------
// 配置 - 波长和通道
// ---------------------------------------------------------------------------

void CSantecDriver::ConfigureWavelengths(const std::vector<double>& wavelengths)
{
    m_wavelengths = wavelengths;
    m_logger.Info("Configured %d wavelength(s).", (int)wavelengths.size());
}

void CSantecDriver::ConfigureChannels(const std::vector<int>& channels)
{
    m_channels = channels;
    m_logger.Info("Configured %d channel(s).", (int)channels.size());
}

// ---------------------------------------------------------------------------
// 参考/校准（官方 RL1 协议）
//
// 回波损耗参考：REF:RL（自动测量测试跳线长度）
//               REF:RL #,[#]（手动设置 MTJ1 和可选的 MTJ2 长度）
// 插入损耗参考：REF:IL:det# <波长>,<值>
// ---------------------------------------------------------------------------

bool CSantecDriver::TakeReference(const ReferenceConfig& config)
{
    m_logger.Info("Taking reference (override=%s)...", config.useOverride ? "true" : "false");
    m_referenced = false;

    try
    {
        if (config.useOverride)
        {
            // 手动设置回波损耗参考的测试跳线长度
            std::ostringstream cmd;
            cmd << SCPI::REF_RL << " " << config.lengthValue;
            WriteAndSync(cmd.str());
            m_logger.Info("RL reference set manually: MTJ1 = %.2f m", config.lengthValue);

            // 为每个检测器/波长设置插入损耗参考
            for (size_t wi = 0; wi < m_wavelengths.size(); ++wi)
            {
                std::ostringstream ilCmd;
                ilCmd << "REF:IL:det1 " << static_cast<int>(m_wavelengths[wi])
                      << "," << config.ilValue;
                WriteAndSync(ilCmd.str());
                m_logger.Info("IL reference set: det1 @%d nm = %.4f dB",
                    static_cast<int>(m_wavelengths[wi]), config.ilValue);
            }
        }
        else
        {
            // 自动测量测试跳线长度（无参数的 REF:RL）
            // 这是一个同步命令，用于测量 MTJ
            WriteAndSync(SCPI::REF_RL);
            m_logger.Info("RL reference auto-measured.");
        }

        // 验证参考值是否已存储
        std::string refLen = Query(SCPI::REF_RL_Q);
        m_logger.Info("Stored RL reference length: %s", refLen.c_str());

        if (!refLen.empty() && refLen != "0")
        {
            m_referenced = true;
            m_logger.Info("Reference completed successfully.");
        }
        else
        {
            m_logger.Warning("Reference may not have been set (REF:RL? returned: %s).",
                refLen.c_str());
            m_referenced = true;
        }

        ErrorInfo err = CheckError();
        if (!err.IsOK())
        {
            m_logger.Warning("Reference completed with error: [%d] %s", err.code, err.message.c_str());
        }

        return true;
    }
    catch (const std::exception& e)
    {
        m_logger.Error("TakeReference exception: %s", e.what());
        return false;
    }
}

// ---------------------------------------------------------------------------
// 测量 - 按照官方协议进行同步 READ:RL? / READ:IL:det#?
//
// RL1 同步执行 SCPI 命令。READ:RL? 在一次调用中触发
// 测量并返回结果，无需单独的触发/获取步骤。
// 在4km长度档位的标准模式下可能需要最多10秒。
// ---------------------------------------------------------------------------

bool CSantecDriver::TakeMeasurement()
{
    m_logger.Info("Starting measurement...");

    if (!m_referenced)
        m_logger.Warning("No reference taken - measurement results may be inaccurate.");

    m_lastResults.clear();

    try
    {
        for (size_t ci = 0; ci < m_channels.size(); ++ci)
        {
            // 通过内部开关选择通道
            if (m_channels.size() > 1 || m_channels[0] != 1)
            {
                SetOutputChannel(m_channels[ci]);
                Sleep(50);
            }

            for (size_t wi = 0; wi < m_wavelengths.size(); ++wi)
            {
                int wavNm = static_cast<int>(m_wavelengths[wi]);
                MeasurementResult result;
                result.channel = m_channels[ci];
                result.wavelength = m_wavelengths[wi];

                // 在此波长启用激光
                EnableLaser(wavNm);

                // READ:IL:det1? <nm> - 同步插入损耗读取
                try
                {
                    result.insertionLoss = ReadIL(1, wavNm);
                }
                catch (const std::exception& e)
                {
                    m_logger.Warning("IL read failed: %s", e.what());
                }

                // READ:RL? <nm> - 同步回波损耗读取（返回 RLA,RLB,RLTOTAL,长度）
                if (m_model == MODEL_RL1 || m_model == MODEL_RLM_100 ||
                    m_model == MODEL_BRM_100 || m_model == MODEL_UNKNOWN)
                {
                    try
                    {
                        MeasurementResult rlResult = ReadRL(wavNm);
                        result.returnLossA = rlResult.returnLossA;
                        result.returnLossB = rlResult.returnLossB;
                        result.returnLossTotal = rlResult.returnLossTotal;
                        result.dutLength = rlResult.dutLength;
                        result.returnLoss = rlResult.returnLossA;
                    }
                    catch (const std::exception& e)
                    {
                        m_logger.Warning("RL read failed: %s", e.what());
                        if (m_model == MODEL_UNKNOWN)
                            m_model = MODEL_ILM_100;
                    }
                }

                m_logger.Info("  Ch%d @%dnm: IL=%.4f dB, RLA=%.2f dB, RLB=%.2f dB, "
                    "RLTOTAL=%.2f dB, Length=%.2f m",
                    result.channel, wavNm,
                    result.insertionLoss, result.returnLossA,
                    result.returnLossB, result.returnLossTotal, result.dutLength);

                m_lastResults.push_back(result);
            }
        }

        DisableLaser();

        ErrorInfo err = CheckError();
        if (!err.IsOK())
        {
            m_logger.Warning("Measurement completed with error: [%d] %s",
                err.code, err.message.c_str());
        }

        m_logger.Info("Measurement completed: %d results.", (int)m_lastResults.size());
        return true;
    }
    catch (const std::exception& e)
    {
        m_logger.Error("TakeMeasurement exception: %s", e.what());
        return false;
    }
}

// ---------------------------------------------------------------------------
// 直接同步测量读取（官方 RL1 协议）
// ---------------------------------------------------------------------------

MeasurementResult CSantecDriver::ReadRL(int wavelengthNm)
{
    std::ostringstream cmd;
    cmd << SCPI::READ_RL << " " << wavelengthNm;

    std::string resp = QueryLong(cmd.str());

    MeasurementResult result;
    result.wavelength = static_cast<double>(wavelengthNm);

    std::vector<double> values = ParseDoubleList(resp);
    if (values.size() >= 1) result.returnLossA = values[0];
    if (values.size() >= 2) result.returnLossB = values[1];
    if (values.size() >= 3) result.returnLossTotal = values[2];
    if (values.size() >= 4) result.dutLength = values[3];
    result.returnLoss = result.returnLossA;

    for (size_t i = 0; i < values.size() && i < 10; ++i)
        result.rawData[result.rawDataCount++] = values[i];

    return result;
}

MeasurementResult CSantecDriver::ReadRL(int wavelengthNm, double refLenA, double refLenB)
{
    std::ostringstream cmd;
    cmd << SCPI::READ_RL << " " << wavelengthNm << "," << refLenA << "," << refLenB;

    std::string resp = QueryLong(cmd.str());

    MeasurementResult result;
    result.wavelength = static_cast<double>(wavelengthNm);

    std::vector<double> values = ParseDoubleList(resp);
    if (values.size() >= 1) result.returnLossA = values[0];
    if (values.size() >= 2) result.returnLossB = values[1];
    if (values.size() >= 3) result.returnLossTotal = values[2];
    if (values.size() >= 4) result.dutLength = values[3];
    result.returnLoss = result.returnLossA;

    for (size_t i = 0; i < values.size() && i < 10; ++i)
        result.rawData[result.rawDataCount++] = values[i];

    return result;
}

double CSantecDriver::ReadIL(int detectorNum, int wavelengthNm)
{
    std::ostringstream cmd;
    cmd << "READ:IL:det" << detectorNum << "? " << wavelengthNm;

    std::string resp = QueryLong(cmd.str());
    if (resp.empty())
        throw std::runtime_error("Empty response from READ:IL");

    return atof(resp.c_str());
}

double CSantecDriver::ReadPower(int detectorNum, int wavelengthNm)
{
    std::ostringstream cmd;
    cmd << "READ:POW:det" << detectorNum << "? " << wavelengthNm;
    std::string resp = Query(cmd.str());
    return atof(resp.c_str());
}

double CSantecDriver::ReadMonitorPower(int wavelengthNm)
{
    std::ostringstream cmd;
    cmd << SCPI::READ_POW_MON << " " << wavelengthNm;
    std::string resp = Query(cmd.str());
    return atof(resp.c_str());
}

// ---------------------------------------------------------------------------
// 结果
// ---------------------------------------------------------------------------

std::vector<MeasurementResult> CSantecDriver::GetResults()
{
    return m_lastResults;
}

// ---------------------------------------------------------------------------
// 工具函数
// ---------------------------------------------------------------------------

std::vector<double> CSantecDriver::ParseDoubleList(const std::string& csv)
{
    std::vector<double> values;
    std::istringstream iss(csv);
    std::string token;
    while (std::getline(iss, token, ','))
    {
        std::string trimmed = Trim(token);
        if (!trimmed.empty())
        {
            char* end = nullptr;
            double val = strtod(trimmed.c_str(), &end);
            if (end != trimmed.c_str())
                values.push_back(val);
        }
    }
    return values;
}

std::string CSantecDriver::Trim(const std::string& s)
{
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

} // namespace ViaviNSantecTester
