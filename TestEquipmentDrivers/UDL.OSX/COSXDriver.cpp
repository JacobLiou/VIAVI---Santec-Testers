#include "stdafx.h"
#include "COSXDriver.h"
#include "OSXTcpAdapter.h"
#include "OSXVisaAdapter.h"
#include <cstdarg>
#include <cmath>

namespace OSXSwitch {

// ===========================================================================
// OSX 光开关的官方 SCPI 命令定义
//
// 参考：OSX 用户手册 M-OSX_SX1-001.04，表 4 和表 5
// ===========================================================================
namespace SCPI {
    // IEEE 488.2 / 标准 SCPI（表 4）
    static const char* IDN          = "*IDN?";
    static const char* RST          = "*RST";
    static const char* CLS          = "*CLS";
    static const char* OPC_Q        = "*OPC?";
    static const char* WAI          = "*WAI";

    // 系统命令（表 4）
    static const char* SYS_ERR      = ":SYST:ERR?";
    static const char* SYS_VER      = ":SYST:VERS?";

    // 状态（表 4）—— 对异步轮询至关重要
    static const char* STAT_OPER_COND = ":STAT:OPER:COND?";

    // 网络（表 4）
    static const char* LAN_ADDR_Q   = ":SYST:COMM:LAN:ADDR?";
    static const char* LAN_GW_Q     = ":SYST:COMM:LAN:GATE?";
    static const char* LAN_MASK_Q   = ":SYST:COMM:LAN:MASK?";
    static const char* LAN_HOST_Q   = ":SYST:COMM:LAN:HOST?";
    static const char* LAN_MAC_Q    = ":SYST:COMM:LAN:MAC?";

    // OSX 专用开关命令（表 5）
    static const char* CLOSE        = "CLOSe";              // CLOSe / CLOSe # / CLOSe?
    static const char* CFG_END_Q    = "CFG:SWT:END?";

    // 模块命令（表 5）
    static const char* MOD_CAT_Q    = "MODule:CATalog?";
    static const char* MOD_NUM_Q    = "MODule:NUMber?";
    static const char* MOD_SEL      = "MODule:SELect";      // MODule:SELect / MODule:SELect #
    static const char* MOD_SEL_Q    = "MODule:SELect?";
    // MODule#:INFO?  -- 动态构建

    // 路由命令（表 5）
    // ROUTe:CHANnel:ALL # / ROUTe#:CHANnel # / ROUTe#:CLOSe # / ROUTe#:CHANnel?
    // ROUTe#:COMMon # / ROUTe#:COMMon? / ROUTe#:HOMe -- 动态构建

    // 控制（表 5）
    static const char* LCL          = "LCL";                // LCL # / LCL?
    static const char* LCL_Q        = "LCL?";

    // 通知（表 5）
    // TEST:NOTIFY# "<string>" -- 动态构建
}

// ===========================================================================
// 静态成员
// ===========================================================================
LogCallback COSXDriver::s_globalCallback = nullptr;
LogLevel    COSXDriver::s_globalLevel    = LOG_INFO;
std::mutex  COSXDriver::s_logMutex;

void COSXDriver::SetGlobalLogCallback(LogCallback callback)
{
    std::lock_guard<std::mutex> lock(s_logMutex);
    s_globalCallback = callback;
}

void COSXDriver::SetGlobalLogLevel(LogLevel level)
{
    s_globalLevel = level;
}

// ===========================================================================
// 构造 / 析构
// ===========================================================================

COSXDriver::COSXDriver(const std::string& ipAddress, int port, double timeout,
                       CommType commType)
    : m_commType(commType)
    , m_adapter(nullptr)
    , m_ownsAdapter(false)
    , m_state(STATE_DISCONNECTED)
{
    m_config.ipAddress = ipAddress;
    m_config.port = port;
    m_config.timeout = timeout;
    m_config.bufferSize = 4096;
    m_config.reconnectAttempts = 3;
    m_config.reconnectDelay = 2.0;
}

COSXDriver::~COSXDriver()
{
    Disconnect();
    if (m_ownsAdapter && m_adapter)
    {
        delete m_adapter;
        m_adapter = nullptr;
    }
}

void COSXDriver::SetCommAdapter(IOSXCommAdapter* adapter, bool takeOwnership)
{
    if (m_ownsAdapter && m_adapter)
        delete m_adapter;
    m_adapter = adapter;
    m_ownsAdapter = takeOwnership;
}

// ===========================================================================
// 日志
// ===========================================================================

void COSXDriver::Log(LogLevel level, const char* fmt, ...)
{
    if (level < s_globalLevel)
        return;

    char buf[2048];
    va_list args;
    va_start(args, fmt);
    vsnprintf_s(buf, sizeof(buf), _TRUNCATE, fmt, args);
    va_end(args);

    std::lock_guard<std::mutex> lock(s_logMutex);
    if (s_globalCallback)
    {
        s_globalCallback(level, "OSX", buf);
    }
}

// ===========================================================================
// 连接验证
// ===========================================================================

bool COSXDriver::ValidateConnection()
{
    Log(LOG_INFO, "Validating connection (*IDN?)...");
    try
    {
        std::string response = Query(SCPI::IDN);
        if (response.empty())
        {
            Log(LOG_ERROR, "Validation failed: no response from *IDN?");
            return false;
        }

        ParseIdentity(response);
        Log(LOG_INFO, "Validated: %s %s (S/N %s, FW %s)",
            m_deviceInfo.manufacturer.c_str(),
            m_deviceInfo.model.c_str(),
            m_deviceInfo.serialNumber.c_str(),
            m_deviceInfo.firmwareVersion.c_str());
        return true;
    }
    catch (const std::exception& e)
    {
        Log(LOG_ERROR, "Validation exception: %s", e.what());
        return false;
    }
}

// ---------------------------------------------------------------------------
// IOSXDriver: 连接生命周期
// ---------------------------------------------------------------------------

bool COSXDriver::Connect()
{
    if (IsConnected())
    {
        Log(LOG_WARNING, "Already connected. Disconnect first.");
        return true;
    }

    m_state = STATE_CONNECTING;

    switch (m_commType)
    {
    case COMM_TCP:
    {
        if (!m_adapter)
        {
            m_adapter = new COSXTcpAdapter();
            m_ownsAdapter = true;
        }

        for (int attempt = 1; attempt <= m_config.reconnectAttempts; ++attempt)
        {
            Log(LOG_INFO, "TCP 连接 %s:%d (尝试 %d/%d)",
                m_config.ipAddress.c_str(), m_config.port,
                attempt, m_config.reconnectAttempts);

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
                m_state = STATE_CONNECTING;
                if (attempt < m_config.reconnectAttempts)
                    Sleep(static_cast<DWORD>(m_config.reconnectDelay * 1000));
                continue;
            }

            Log(LOG_INFO, "TCP 连接已建立并验证。");
            return true;
        }

        m_state = STATE_ERROR;
        Log(LOG_ERROR, "所有 TCP 连接尝试已耗尽。");
        return false;
    }

    case COMM_USB:
    {
        if (!m_adapter)
        {
            m_adapter = new COSXVisaAdapter();
            m_ownsAdapter = true;
        }

        for (int attempt = 1; attempt <= m_config.reconnectAttempts; ++attempt)
        {
            Log(LOG_INFO, "VISA 连接 %s (尝试 %d/%d)",
                m_config.ipAddress.c_str(),
                attempt, m_config.reconnectAttempts);

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
                m_state = STATE_CONNECTING;
                if (attempt < m_config.reconnectAttempts)
                    Sleep(static_cast<DWORD>(m_config.reconnectDelay * 1000));
                continue;
            }

            Log(LOG_INFO, "VISA 连接已建立并验证。");
            return true;
        }

        m_state = STATE_ERROR;
        Log(LOG_ERROR, "所有 VISA 连接尝试已耗尽。");
        return false;
    }

    case COMM_GPIB:
        Log(LOG_WARNING, "GPIB 通信尚未实现。");
        return false;

    case COMM_DLL:
        Log(LOG_WARNING, "DLL 通信尚未实现。");
        return false;

    default:
        Log(LOG_ERROR, "未知通信类型: %d", static_cast<int>(m_commType));
        return false;
    }
}

void COSXDriver::Disconnect()
{
    if (m_adapter && m_adapter->IsOpen())
    {
        try { Write("*CLS"); } catch (...) {}
        m_adapter->Close();
        Log(LOG_INFO, "已断开连接。");
    }
    m_state = STATE_DISCONNECTED;
}

bool COSXDriver::Reconnect()
{
    Log(LOG_INFO, "正在重新连接...");
    Disconnect();
    Sleep(static_cast<DWORD>(m_config.reconnectDelay * 1000));
    return Connect();
}

bool COSXDriver::IsConnected() const
{
    return m_state == STATE_CONNECTED && m_adapter != nullptr && m_adapter->IsOpen();
}

// ===========================================================================
// SCPI 通信（通过适配器路由）
// ===========================================================================

std::string COSXDriver::Query(const std::string& command)
{
    if (!IsConnected())
        throw std::runtime_error("未连接到 OSX 设备。");

    Log(LOG_DEBUG, "TX: %s", command.c_str());

    try
    {
        std::string response = m_adapter->SendQuery(command);
        Log(LOG_DEBUG, "RX: %s", response.c_str());
        return response;
    }
    catch (const std::exception& ex)
    {
        Log(LOG_WARNING, "查询失败: %s。尝试重连...", ex.what());
        if (Reconnect())
        {
            Log(LOG_INFO, "已重连。重试: %s", command.c_str());
            std::string response = m_adapter->SendQuery(command);
            Log(LOG_DEBUG, "RX: %s", response.c_str());
            return response;
        }
        m_state = STATE_ERROR;
        throw;
    }
}

void COSXDriver::Write(const std::string& command)
{
    if (!IsConnected())
        throw std::runtime_error("未连接到 OSX 设备。");

    Log(LOG_DEBUG, "TX: %s", command.c_str());

    try
    {
        m_adapter->SendWrite(command);
    }
    catch (const std::exception& ex)
    {
        Log(LOG_WARNING, "写入失败: %s。尝试重连...", ex.what());
        if (Reconnect())
        {
            Log(LOG_INFO, "已重连。重试: %s", command.c_str());
            m_adapter->SendWrite(command);
            return;
        }
        m_state = STATE_ERROR;
        throw;
    }
}

void COSXDriver::WriteAndWait(const std::string& command, int timeoutMs)
{
    Write(command);
    if (!WaitForOperation(timeoutMs))
    {
        throw std::runtime_error(
            std::string("Operation timed out after command: ") + command);
    }
}

// ===========================================================================
// IOSXDriver: 设备识别
// ===========================================================================

DeviceInfo COSXDriver::GetDeviceInfo()
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

ErrorInfo COSXDriver::CheckError()
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

std::string COSXDriver::GetSystemVersion()
{
    return Query(SCPI::SYS_VER);
}

// ===========================================================================
// IOSXDriver: 模块管理
// ===========================================================================

int COSXDriver::GetModuleCount()
{
    std::string resp = Query(SCPI::MOD_NUM_Q);
    return atoi(resp.c_str());
}

std::vector<ModuleInfo> COSXDriver::GetModuleCatalog()
{
    std::vector<ModuleInfo> modules;

    std::string catalog = Query(SCPI::MOD_CAT_Q);
    Log(LOG_INFO, "Module catalog: %s", catalog.c_str());

    // 解析逗号分隔的列表："SX 1Ax24","SX 2Bx12",...
    std::vector<std::string> entries;
    std::istringstream iss(catalog);
    std::string token;
    while (std::getline(iss, token, ','))
    {
        std::string trimmed = Trim(token);
        if (trimmed.size() >= 2 && trimmed.front() == '"' && trimmed.back() == '"')
            trimmed = trimmed.substr(1, trimmed.size() - 2);
        if (!trimmed.empty())
            entries.push_back(trimmed);
    }

    for (size_t i = 0; i < entries.size(); ++i)
    {
        ModuleInfo mi;
        mi.index = static_cast<int>(i);
        mi.catalogEntry = entries[i];
        mi.configType = ParseConfigType(entries[i]);
        mi.channelCount = ParseChannelCount(entries[i]);

        // 查询详细信息
        try
        {
            std::ostringstream cmd;
            cmd << "MODule" << i << ":INFO?";
            mi.detailedInfo = Query(cmd.str());
        }
        catch (...) {}

        modules.push_back(mi);
    }

    return modules;
}

ModuleInfo COSXDriver::GetModuleInfo(int moduleIndex)
{
    ModuleInfo mi;
    mi.index = moduleIndex;

    try
    {
        std::ostringstream cmd;
        cmd << "MODule" << moduleIndex << ":INFO?";
        mi.detailedInfo = Query(cmd.str());
    }
    catch (...) {}

    return mi;
}

void COSXDriver::SelectModule(int moduleIndex)
{
    std::ostringstream cmd;
    cmd << SCPI::MOD_SEL << " " << moduleIndex;
    Write(cmd.str());
    Log(LOG_INFO, "Selected module %d.", moduleIndex);
}

void COSXDriver::SelectNextModule()
{
    Write(SCPI::MOD_SEL);
    Log(LOG_INFO, "Selected next module.");
}

int COSXDriver::GetSelectedModule()
{
    std::string resp = Query(SCPI::MOD_SEL_Q);
    return atoi(resp.c_str());
}

// ===========================================================================
// IOSXDriver: 通道切换
// ===========================================================================

void COSXDriver::SwitchChannel(int channel)
{
    std::ostringstream cmd;
    cmd << SCPI::CLOSE << " " << channel;
    WriteAndWait(cmd.str());
    Log(LOG_INFO, "Switched to channel %d.", channel);
}

void COSXDriver::SwitchNext()
{
    WriteAndWait(SCPI::CLOSE);
    Log(LOG_INFO, "Switched to next channel.");
}

int COSXDriver::GetCurrentChannel()
{
    std::string resp = Query("CLOSe?");
    return atoi(resp.c_str());
}

int COSXDriver::GetChannelCount()
{
    std::string resp = Query(SCPI::CFG_END_Q);
    return atoi(resp.c_str());
}

// ===========================================================================
// IOSXDriver: 多模块路由
// ===========================================================================

void COSXDriver::RouteChannel(int moduleIndex, int channel)
{
    std::ostringstream cmd;
    cmd << "ROUTe" << moduleIndex << ":CHANnel " << channel;
    WriteAndWait(cmd.str());
    Log(LOG_INFO, "Route module %d -> channel %d.", moduleIndex, channel);
}

int COSXDriver::GetRouteChannel(int moduleIndex)
{
    std::ostringstream cmd;
    cmd << "ROUTe" << moduleIndex << ":CHANnel?";
    std::string resp = Query(cmd.str());
    return atoi(resp.c_str());
}

void COSXDriver::RouteAllModules(int channel)
{
    std::ostringstream cmd;
    cmd << "ROUTe:CHANnel:ALL " << channel;
    WriteAndWait(cmd.str());
    Log(LOG_INFO, "Routed all modules -> channel %d.", channel);
}

void COSXDriver::SetCommonInput(int moduleIndex, int common)
{
    std::ostringstream cmd;
    cmd << "ROUTe" << moduleIndex << ":COMMon " << common;
    WriteAndWait(cmd.str());
    Log(LOG_INFO, "Module %d common input set to %d.", moduleIndex, common);
}

int COSXDriver::GetCommonInput(int moduleIndex)
{
    std::ostringstream cmd;
    cmd << "ROUTe" << moduleIndex << ":COMMon?";
    std::string resp = Query(cmd.str());
    return atoi(resp.c_str());
}

void COSXDriver::HomeModule(int moduleIndex)
{
    std::ostringstream cmd;
    cmd << "ROUTe" << moduleIndex << ":HOMe";
    WriteAndWait(cmd.str(), MAX_OPERATION_WAIT_MS);
    Log(LOG_INFO, "Module %d homed.", moduleIndex);
}

// ===========================================================================
// IOSXDriver: 控制
// ===========================================================================

void COSXDriver::SetLocalMode(bool local)
{
    std::ostringstream cmd;
    cmd << SCPI::LCL << " " << (local ? "1" : "0");
    Write(cmd.str());
    Log(LOG_INFO, "Local mode %s.", local ? "enabled" : "disabled");
}

bool COSXDriver::GetLocalMode()
{
    std::string resp = Query(SCPI::LCL_Q);
    return atoi(resp.c_str()) != 0;
}

void COSXDriver::SendNotification(int icon, const std::string& message)
{
    std::ostringstream cmd;
    cmd << "TEST:NOTIFY" << icon << " \"" << message << "\"";
    Write(cmd.str());
    Log(LOG_INFO, "Notification sent (icon=%d): %s", icon, message.c_str());
}

void COSXDriver::Reset()
{
    Write(SCPI::RST);
    Log(LOG_INFO, "Device reset (*RST).");
}

// ===========================================================================
// IOSXDriver: 网络配置
// ===========================================================================

std::string COSXDriver::GetIPAddress()  { return Trim(Query(SCPI::LAN_ADDR_Q)); }
std::string COSXDriver::GetGateway()    { return Trim(Query(SCPI::LAN_GW_Q)); }
std::string COSXDriver::GetNetmask()    { return Trim(Query(SCPI::LAN_MASK_Q)); }
std::string COSXDriver::GetHostname()   { return Trim(Query(SCPI::LAN_HOST_Q)); }
std::string COSXDriver::GetMAC()        { return Trim(Query(SCPI::LAN_MAC_Q)); }

// ===========================================================================
// IOSXDriver: 操作同步
//
// OSX 以异步方式运行 SCPI 命令。发出切换命令后，
// 需轮询 STAT:OPER:COND? 直到返回 0 以确认操作已完成。
// ===========================================================================

bool COSXDriver::WaitForOperation(int timeoutMs)
{
    int elapsed = 0;
    while (elapsed < timeoutMs)
    {
        try
        {
            std::string resp = Query(SCPI::STAT_OPER_COND);
            int status = atoi(resp.c_str());
            if (status == 0)
                return true;
        }
        catch (const std::exception& e)
        {
            Log(LOG_WARNING, "Poll error: %s", e.what());
        }

        Sleep(MAX_POLL_INTERVAL_MS);
        elapsed += MAX_POLL_INTERVAL_MS;
    }

    Log(LOG_ERROR, "WaitForOperation timed out after %d ms.", timeoutMs);
    return false;
}

// ===========================================================================
// IOSXDriver: 原始 SCPI 透传
// ===========================================================================

std::string COSXDriver::SendRawQuery(const std::string& command)
{
    return Query(command);
}

void COSXDriver::SendRawWrite(const std::string& command)
{
    Write(command);
}

// ===========================================================================
// 解析工具
// ===========================================================================

void COSXDriver::ParseIdentity(const std::string& idnResponse)
{
    std::vector<std::string> parts;
    std::istringstream iss(idnResponse);
    std::string token;
    while (std::getline(iss, token, ','))
        parts.push_back(Trim(token));

    if (parts.size() >= 1) m_deviceInfo.manufacturer    = parts[0];
    if (parts.size() >= 2) m_deviceInfo.model            = parts[1];
    if (parts.size() >= 3) m_deviceInfo.serialNumber     = parts[2];
    if (parts.size() >= 4) m_deviceInfo.firmwareVersion  = parts[3];
}

std::string COSXDriver::Trim(const std::string& s)
{
    size_t start = s.find_first_not_of(" \t\r\n\"");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n\"");
    return s.substr(start, end - start + 1);
}

SwitchConfigType COSXDriver::ParseConfigType(const std::string& catalog)
{
    // 目录条目格式如 "SX 1Ax24"、"SX 2Bx12" 等
    std::string upper = catalog;
    for (size_t i = 0; i < upper.size(); ++i)
        upper[i] = static_cast<char>(toupper(static_cast<unsigned char>(upper[i])));

    if (upper.find("2C") != std::string::npos) return CONFIG_2C;
    if (upper.find("2B") != std::string::npos) return CONFIG_2B;
    if (upper.find("2A") != std::string::npos) return CONFIG_2A;
    return CONFIG_1A;
}

int COSXDriver::ParseChannelCount(const std::string& catalog)
{
    // 从 "SX 1Ax24" 等条目中提取 'x' 后面的数字
    size_t xPos = catalog.find('x');
    if (xPos == std::string::npos)
        xPos = catalog.find('X');
    if (xPos == std::string::npos || xPos + 1 >= catalog.size())
        return 0;

    // 找到最后一个 'x'，它分隔配置类型和通道数
    size_t lastX = catalog.rfind('x');
    if (lastX == std::string::npos)
        lastX = catalog.rfind('X');
    if (lastX == std::string::npos || lastX + 1 >= catalog.size())
        return 0;

    return atoi(catalog.substr(lastX + 1).c_str());
}

} // namespace OSXSwitch
