#include "stdafx.h"
#include "CViaviOSWDriver.h"
#include "ViaviOSWTcpAdapter.h"
#include "ViaviOSWVisaAdapter.h"
#include <stdexcept>
#include <sstream>
#include <cstdarg>

namespace ViaviOSW {

// ===========================================================================
// Viavi MAP OSW SCPI 命令定义
//
// 参考：MAP-PCT Programming Guide Chapter 2 (通用命令)
//       标准 SCPI 路由命令
// ===========================================================================
namespace SCPI {
    // 远程控制
    static const char* REM         = "*REM";

    // IEEE 488.2 通用命令
    static const char* IDN         = "*IDN?";
    static const char* RST         = "*RST";
    static const char* CLS         = "*CLS";

    // 系统命令
    static const char* SYS_ERR     = ":SYST:ERR?";

    // 模块信息
    static const char* INFO        = ":INFO?";
    static const char* DEV_INFO    = ":DEV:INFO?";   // :DEV:INFO? <device>
    static const char* CONF        = ":CONF?";

    // 路由命令
    static const char* PROC_ROUT   = ":PROC:ROUT";   // :PROC:ROUT <device>,<channel>
    static const char* PROC_ROUT_Q = ":PROC:ROUT?";  // :PROC:ROUT? <device>

    // 忙碌状态
    static const char* BUSY        = ":BUSY?";

    // 复位
    static const char* RES         = ":RES";
}


// ===========================================================================
// 静态成员
// ===========================================================================

LogCallback CViaviOSWDriver::s_globalCallback = nullptr;
LogLevel CViaviOSWDriver::s_globalLevel = LOG_DEBUG;
std::mutex CViaviOSWDriver::s_logMutex;

// ===========================================================================
// 构造 / 析构
// ===========================================================================

CViaviOSWDriver::CViaviOSWDriver(const std::string& address,
                                   int port,
                                   double timeout,
                                   CommType commType)
    : m_commType(commType)
    , m_adapter(nullptr)
    , m_ownsAdapter(false)
    , m_state(STATE_DISCONNECTED)
    , m_deviceCount(0)
{
    m_config.ipAddress = address;
    m_config.port = port;
    m_config.timeout = timeout;
    m_config.bufferSize = 4096;
    m_config.reconnectAttempts = 3;
    m_config.reconnectDelay = 2.0;
}

CViaviOSWDriver::~CViaviOSWDriver()
{
    Disconnect();
    if (m_ownsAdapter && m_adapter)
    {
        delete m_adapter;
        m_adapter = nullptr;
    }
}

void CViaviOSWDriver::SetCommAdapter(IViaviOSWCommAdapter* adapter, bool takeOwnership)
{
    if (m_ownsAdapter && m_adapter)
        delete m_adapter;
    m_adapter = adapter;
    m_ownsAdapter = takeOwnership;
}

void CViaviOSWDriver::SetGlobalLogCallback(LogCallback callback)
{
    std::lock_guard<std::mutex> lock(s_logMutex);
    s_globalCallback = callback;
}

void CViaviOSWDriver::SetGlobalLogLevel(LogLevel level)
{
    s_globalLevel = level;
}

// ---------------------------------------------------------------------------
// 日志
// ---------------------------------------------------------------------------

void CViaviOSWDriver::Log(LogLevel level, const char* fmt, ...)
{
    if (level < s_globalLevel)
        return;

    char buffer[2048];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    std::string message(buffer);
    std::string source("ViaviOSW");

    LogCallback cb;
    {
        std::lock_guard<std::mutex> lock(s_logMutex);
        cb = s_globalCallback;
    }

    if (cb)
    {
        cb(level, source, message);
    }
    else
    {
        time_t now = time(nullptr);
        struct tm tmNow;
        localtime_s(&tmNow, &now);

        char timeBuf[64];
        strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", &tmNow);

        static const char* levelStr[] = { "DEBUG", "INFO", "WARN", "ERROR" };
        const char* lvl = (level >= 0 && level <= 3) ? levelStr[level] : "???";

        char outBuf[2200];
        _snprintf_s(outBuf, sizeof(outBuf), "%s [%s] %s: %s\n",
            timeBuf, source.c_str(), lvl, buffer);
        OutputDebugStringA(outBuf);
    }
}

// ---------------------------------------------------------------------------
// 统一命令接口
// ---------------------------------------------------------------------------

std::string CViaviOSWDriver::Query(const std::string& command)
{
    Log(LOG_DEBUG, "TX> %s", command.c_str());
    if (!m_adapter || !m_adapter->IsOpen())
        throw std::runtime_error("Viavi OSW 未连接。");

    std::string response = m_adapter->SendQuery(command);
    Log(LOG_DEBUG, "RX< %s", response.c_str());
    return response;
}

void CViaviOSWDriver::Write(const std::string& command)
{
    Log(LOG_DEBUG, "TX> %s", command.c_str());
    if (!m_adapter || !m_adapter->IsOpen())
        throw std::runtime_error("Viavi OSW 未连接。");
    m_adapter->SendWrite(command);
}

void CViaviOSWDriver::WriteAndWait(const std::string& command, int timeoutMs)
{
    Write(command);
    WaitForIdle(timeoutMs);
}

// ---------------------------------------------------------------------------
// 连接
// ---------------------------------------------------------------------------

bool CViaviOSWDriver::Connect()
{
    Log(LOG_INFO, "连接 Viavi OSW %s:%d (comm=%d)",
        m_config.ipAddress.c_str(), m_config.port, static_cast<int>(m_commType));

    switch (m_commType)
    {
    case COMM_TCP:
    {
        if (!m_adapter)
        {
            m_adapter = new CViaviOSWTcpAdapter();
            m_ownsAdapter = true;
        }

        for (int attempt = 1; attempt <= m_config.reconnectAttempts; ++attempt)
        {
            Log(LOG_INFO, "连接尝试 %d/%d", attempt, m_config.reconnectAttempts);

            if (!m_adapter->Open(m_config.ipAddress, m_config.port, m_config.timeout))
            {
                Log(LOG_ERROR, "连接尝试 %d 失败。", attempt);
                if (attempt < m_config.reconnectAttempts)
                    Sleep(static_cast<DWORD>(m_config.reconnectDelay * 1000));
                continue;
            }

            m_state = STATE_CONNECTED;

            if (!ValidateConnection())
            {
                Log(LOG_ERROR, "验证失败 (尝试 %d)。", attempt);
                m_adapter->Close();
                m_state = STATE_DISCONNECTED;
                if (attempt < m_config.reconnectAttempts)
                    Sleep(static_cast<DWORD>(m_config.reconnectDelay * 1000));
                continue;
            }

            Log(LOG_INFO, "Viavi OSW TCP 已连接并验证。");
            return true;
        }

        m_state = STATE_ERROR;
        Log(LOG_ERROR, "所有连接尝试已耗尽。");
        return false;
    }
    case COMM_GPIB:
    case COMM_VISA:
    {
        if (!m_adapter)
        {
            CViaviOSWVisaAdapter* visaAdapter = new CViaviOSWVisaAdapter();
            m_adapter = visaAdapter;
            m_ownsAdapter = true;
        }

        for (int attempt = 1; attempt <= m_config.reconnectAttempts; ++attempt)
        {
            Log(LOG_INFO, "VISA 连接尝试 %d/%d", attempt, m_config.reconnectAttempts);

            try
            {
                if (!m_adapter->Open(m_config.ipAddress, m_config.port, m_config.timeout))
                {
                    Log(LOG_ERROR, "VISA 连接尝试 %d 失败。", attempt);
                    if (attempt < m_config.reconnectAttempts)
                        Sleep(static_cast<DWORD>(m_config.reconnectDelay * 1000));
                    continue;
                }
            }
            catch (const std::exception& ex)
            {
                Log(LOG_ERROR, "VISA 连接异常: %s", ex.what());
                if (attempt < m_config.reconnectAttempts)
                    Sleep(static_cast<DWORD>(m_config.reconnectDelay * 1000));
                continue;
            }

            m_state = STATE_CONNECTED;

            if (!ValidateConnection())
            {
                Log(LOG_ERROR, "VISA 验证失败 (尝试 %d)。", attempt);
                m_adapter->Close();
                m_state = STATE_DISCONNECTED;
                if (attempt < m_config.reconnectAttempts)
                    Sleep(static_cast<DWORD>(m_config.reconnectDelay * 1000));
                continue;
            }

            Log(LOG_INFO, "Viavi OSW VISA 已连接并验证。");
            return true;
        }

        m_state = STATE_ERROR;
        Log(LOG_ERROR, "所有 VISA 连接尝试已耗尽。");
        return false;
    }
    default:
        Log(LOG_ERROR, "未知通信类型: %d", static_cast<int>(m_commType));
        return false;
    }
}

void CViaviOSWDriver::Disconnect()
{
    if (m_adapter && m_adapter->IsOpen())
    {
        try
        {
            Write(SCPI::CLS);
        }
        catch (...) {}
        m_adapter->Close();
        m_state = STATE_DISCONNECTED;
        Log(LOG_INFO, "Viavi OSW 已断开。");
    }
}

bool CViaviOSWDriver::Reconnect()
{
    Disconnect();
    return Connect();
}

bool CViaviOSWDriver::IsConnected() const
{
    if (m_adapter)
        return m_state == STATE_CONNECTED && m_adapter->IsOpen();
    return false;
}

// ---------------------------------------------------------------------------
// 连接验证：发送 *REM + :CONF? 验证
// ---------------------------------------------------------------------------

bool CViaviOSWDriver::ValidateConnection()
{
    Log(LOG_INFO, "验证 Viavi OSW 连接...");
    try
    {
        // 激活远程控制模式
        Write(SCPI::REM);
        Sleep(200);

        // 查询配置以验证连接
        std::string confResp = Query(SCPI::CONF);
        if (confResp.empty())
        {
            Log(LOG_ERROR, "验证失败: :CONF? 无响应");
            return false;
        }

        m_switchInfo = ParseSwitchConfig(confResp);
        Log(LOG_INFO, "OSW 验证成功: 设备 %d, 通道数 %d",
            m_switchInfo.deviceNum, m_switchInfo.channelCount);

        // 获取设备信息
        try
        {
            std::string infoResp = Query(SCPI::INFO);
            ParseDeviceInfo(infoResp);
            Log(LOG_INFO, "设备信息: S/N %s, P/N %s, FW %s",
                m_deviceInfo.serialNumber.c_str(),
                m_deviceInfo.partNumber.c_str(),
                m_deviceInfo.firmwareVersion.c_str());
        }
        catch (const std::exception& e)
        {
            Log(LOG_WARNING, ":INFO? 失败: %s", e.what());
        }

        return true;
    }
    catch (const std::exception& e)
    {
        Log(LOG_ERROR, "验证异常: %s", e.what());
        return false;
    }
}

// ---------------------------------------------------------------------------
// 设备信息
// ---------------------------------------------------------------------------

DeviceInfo CViaviOSWDriver::GetDeviceInfo()
{
    if (m_deviceInfo.serialNumber.empty() && IsConnected())
    {
        try
        {
            std::string infoResp = Query(SCPI::INFO);
            ParseDeviceInfo(infoResp);
        }
        catch (...) {}
    }
    return m_deviceInfo;
}

void CViaviOSWDriver::ParseDeviceInfo(const std::string& response)
{
    std::vector<std::string> parts;
    std::istringstream iss(response);
    std::string token;
    while (std::getline(iss, token, ','))
        parts.push_back(Trim(token));

    if (parts.size() >= 1) m_deviceInfo.serialNumber = parts[0];
    if (parts.size() >= 2) m_deviceInfo.partNumber = parts[1];
    if (parts.size() >= 3) m_deviceInfo.firmwareVersion = parts[2];
    if (parts.size() >= 4) m_deviceInfo.hardwareVersion = parts[3];
}

ErrorInfo CViaviOSWDriver::CheckError()
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
        info.message = std::string("CheckError 异常: ") + e.what();
    }
    return info;
}

// ---------------------------------------------------------------------------
// 开关信息
// ---------------------------------------------------------------------------

int CViaviOSWDriver::GetDeviceCount()
{
    if (m_deviceCount <= 0 && IsConnected())
    {
        // 从 :CONF? 响应解析设备数
        try
        {
            std::string confResp = Query(SCPI::CONF);
            m_switchInfo = ParseSwitchConfig(confResp);
            m_deviceCount = m_switchInfo.deviceNum;
        }
        catch (...)
        {
            m_deviceCount = 1;
        }
    }
    return (m_deviceCount > 0) ? m_deviceCount : 1;
}

SwitchInfo CViaviOSWDriver::GetSwitchInfo(int deviceNum)
{
    if (IsConnected())
    {
        try
        {
            std::string confResp = Query(SCPI::CONF);
            m_switchInfo = ParseSwitchConfig(confResp);

            // 获取当前通道
            std::ostringstream cmd;
            cmd << SCPI::PROC_ROUT_Q << " " << deviceNum;
            std::string resp = Query(cmd.str());
            m_switchInfo.currentChannel = atoi(resp.c_str());
        }
        catch (...) {}
    }
    return m_switchInfo;
}

SwitchInfo CViaviOSWDriver::ParseSwitchConfig(const std::string& response)
{
    // :CONF? 返回格式: deviceNum,type,channelCount,...
    SwitchInfo info;
    std::vector<std::string> parts;
    std::istringstream iss(response);
    std::string token;
    while (std::getline(iss, token, ','))
        parts.push_back(Trim(token));

    if (parts.size() >= 1) info.deviceNum = atoi(parts[0].c_str());
    if (parts.size() >= 2) info.description = parts[1];
    if (parts.size() >= 3) info.channelCount = atoi(parts[2].c_str());

    // 根据描述推断开关类型
    std::string desc = info.description;
    for (size_t i = 0; i < desc.size(); ++i)
        desc[i] = static_cast<char>(toupper(static_cast<unsigned char>(desc[i])));

    if (desc.find("2D") != std::string::npos) info.switchType = SWITCH_2D;
    else if (desc.find("2E") != std::string::npos) info.switchType = SWITCH_2E;
    else if (desc.find("2X") != std::string::npos) info.switchType = SWITCH_2X;
    else info.switchType = SWITCH_1C;

    return info;
}

// ---------------------------------------------------------------------------
// 通道切换
// ---------------------------------------------------------------------------

void CViaviOSWDriver::SwitchChannel(int deviceNum, int channel)
{
    Log(LOG_INFO, "切换设备 %d 到通道 %d", deviceNum, channel);

    std::ostringstream cmd;
    cmd << SCPI::PROC_ROUT << " " << deviceNum << "," << channel;
    WriteAndWait(cmd.str(), SWITCH_TIMEOUT_MS);

    Log(LOG_INFO, "设备 %d 已切换到通道 %d。", deviceNum, channel);
}

int CViaviOSWDriver::GetCurrentChannel(int deviceNum)
{
    std::ostringstream cmd;
    cmd << SCPI::PROC_ROUT_Q << " " << deviceNum;
    std::string resp = Query(cmd.str());
    return atoi(resp.c_str());
}

int CViaviOSWDriver::GetChannelCount(int deviceNum)
{
    if (m_switchInfo.channelCount <= 0)
    {
        GetSwitchInfo(deviceNum);
    }
    return m_switchInfo.channelCount;
}

// ---------------------------------------------------------------------------
// 控制
// ---------------------------------------------------------------------------

void CViaviOSWDriver::SetLocalMode(bool local)
{
    if (local)
    {
        // 退出远程模式（恢复本地控制）
        Log(LOG_INFO, "恢复本地控制模式。");
    }
    else
    {
        Write(SCPI::REM);
        Log(LOG_INFO, "设置远程控制模式。");
    }
}

void CViaviOSWDriver::Reset()
{
    Log(LOG_INFO, "复位 Viavi OSW 模块...");
    Write(SCPI::RES);
    Sleep(1000);
    Log(LOG_INFO, "Viavi OSW 模块已复位。");
}

// ---------------------------------------------------------------------------
// 操作同步
// ---------------------------------------------------------------------------

bool CViaviOSWDriver::WaitForIdle(int timeoutMs)
{
    int elapsed = 0;

    while (elapsed < timeoutMs)
    {
        try
        {
            std::string resp = Query(SCPI::BUSY);
            int busy = atoi(resp.c_str());
            if (busy == 0)
            {
                Log(LOG_DEBUG, "设备空闲 (耗时 %d ms)。", elapsed);
                return true;
            }
        }
        catch (const std::exception& e)
        {
            Log(LOG_WARNING, "轮询 :BUSY? 异常: %s", e.what());
        }

        Sleep(POLL_INTERVAL_MS);
        elapsed += POLL_INTERVAL_MS;
    }

    Log(LOG_ERROR, "等待设备空闲超时 (%d ms)。", timeoutMs);
    return false;
}

// ---------------------------------------------------------------------------
// 原始 SCPI 透传
// ---------------------------------------------------------------------------

std::string CViaviOSWDriver::SendRawQuery(const std::string& command)
{
    return Query(command);
}

void CViaviOSWDriver::SendRawWrite(const std::string& command)
{
    Write(command);
}

// ---------------------------------------------------------------------------
// 辅助函数
// ---------------------------------------------------------------------------

std::string CViaviOSWDriver::Trim(const std::string& s)
{
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

} // namespace ViaviOSW
