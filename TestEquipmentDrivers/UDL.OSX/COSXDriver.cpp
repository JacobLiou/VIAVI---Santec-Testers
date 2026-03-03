#include "stdafx.h"
#include "COSXDriver.h"
#include <cstdarg>
#include <cmath>

namespace OSXSwitch {

// ===========================================================================
// Official SCPI Command Definitions for OSX Optical Switch
//
// Reference: OSX User Manual M-OSX_SX1-001.04, Table 4 & Table 5
// ===========================================================================
namespace SCPI {
    // IEEE 488.2 / Standard SCPI (Table 4)
    static const char* IDN          = "*IDN?";
    static const char* RST          = "*RST";
    static const char* CLS          = "*CLS";
    static const char* OPC_Q        = "*OPC?";
    static const char* WAI          = "*WAI";

    // System commands (Table 4)
    static const char* SYS_ERR      = ":SYST:ERR?";
    static const char* SYS_VER      = ":SYST:VERS?";

    // Status (Table 4) -- critical for async polling
    static const char* STAT_OPER_COND = ":STAT:OPER:COND?";

    // Network (Table 4)
    static const char* LAN_ADDR_Q   = ":SYST:COMM:LAN:ADDR?";
    static const char* LAN_GW_Q     = ":SYST:COMM:LAN:GATE?";
    static const char* LAN_MASK_Q   = ":SYST:COMM:LAN:MASK?";
    static const char* LAN_HOST_Q   = ":SYST:COMM:LAN:HOST?";
    static const char* LAN_MAC_Q    = ":SYST:COMM:LAN:MAC?";

    // OSX-specific switch commands (Table 5)
    static const char* CLOSE        = "CLOSe";              // CLOSe / CLOSe # / CLOSe?
    static const char* CFG_END_Q    = "CFG:SWT:END?";

    // Module commands (Table 5)
    static const char* MOD_CAT_Q    = "MODule:CATalog?";
    static const char* MOD_NUM_Q    = "MODule:NUMber?";
    static const char* MOD_SEL      = "MODule:SELect";      // MODule:SELect / MODule:SELect #
    static const char* MOD_SEL_Q    = "MODule:SELect?";
    // MODule#:INFO?  -- built dynamically

    // Route commands (Table 5)
    // ROUTe:CHANnel:ALL # / ROUTe#:CHANnel # / ROUTe#:CLOSe # / ROUTe#:CHANnel?
    // ROUTe#:COMMon # / ROUTe#:COMMon? / ROUTe#:HOMe -- built dynamically

    // Control (Table 5)
    static const char* LCL          = "LCL";                // LCL # / LCL?
    static const char* LCL_Q        = "LCL?";

    // Notification (Table 5)
    // TEST:NOTIFY# "<string>" -- built dynamically
}

// ===========================================================================
// Static members
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
// Construction / Destruction
// ===========================================================================

COSXDriver::COSXDriver(const std::string& ipAddress, int port, double timeout)
    : m_socket(INVALID_SOCKET)
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
}

// ===========================================================================
// Logging
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
// TCP Connection
// ===========================================================================

bool COSXDriver::ConnectSocket(SOCKET sock, const sockaddr_in& addr, double timeoutSec)
{
    u_long nonBlocking = 1;
    ioctlsocket(sock, FIONBIO, &nonBlocking);

    int ret = connect(sock, (const sockaddr*)&addr, sizeof(addr));
    if (ret == 0)
    {
        u_long blocking = 0;
        ioctlsocket(sock, FIONBIO, &blocking);
        return true;
    }

    int err = WSAGetLastError();
    if (err != WSAEWOULDBLOCK)
    {
        Log(LOG_ERROR, "connect() failed immediately: WSA %d", err);
        return false;
    }

    fd_set writefds, exceptfds;
    FD_ZERO(&writefds);
    FD_ZERO(&exceptfds);
    FD_SET(sock, &writefds);
    FD_SET(sock, &exceptfds);

    timeval tv;
    tv.tv_sec = static_cast<long>(timeoutSec);
    tv.tv_usec = static_cast<long>((timeoutSec - tv.tv_sec) * 1000000);

    ret = select(0, NULL, &writefds, &exceptfds, &tv);
    if (ret == 0)
    {
        Log(LOG_ERROR, "Connection timed out after %.1f seconds.", timeoutSec);
        return false;
    }
    if (ret == SOCKET_ERROR)
    {
        Log(LOG_ERROR, "select() failed: WSA %d", WSAGetLastError());
        return false;
    }

    if (FD_ISSET(sock, &exceptfds))
    {
        int optError = 0;
        int optLen = sizeof(optError);
        getsockopt(sock, SOL_SOCKET, SO_ERROR, (char*)&optError, &optLen);
        Log(LOG_ERROR, "Connection failed: socket error %d", optError);
        return false;
    }

    if (!FD_ISSET(sock, &writefds))
    {
        Log(LOG_ERROR, "Connection failed: socket not writable after select.");
        return false;
    }

    int optError = 0;
    int optLen = sizeof(optError);
    getsockopt(sock, SOL_SOCKET, SO_ERROR, (char*)&optError, &optLen);
    if (optError != 0)
    {
        Log(LOG_ERROR, "Connection failed: SO_ERROR = %d", optError);
        return false;
    }

    u_long blocking = 0;
    ioctlsocket(sock, FIONBIO, &blocking);
    return true;
}

void COSXDriver::EnableKeepAlive(SOCKET sock)
{
    BOOL keepAlive = TRUE;
    setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (const char*)&keepAlive, sizeof(keepAlive));

    struct tcp_keepalive ka = {};
    ka.onoff = 1;
    ka.keepalivetime = 10000;
    ka.keepaliveinterval = 2000;
    DWORD bytesReturned = 0;
    WSAIoctl(sock, SIO_KEEPALIVE_VALS, &ka, sizeof(ka),
             NULL, 0, &bytesReturned, NULL, NULL);
}

void COSXDriver::CleanupSocket()
{
    if (m_socket != INVALID_SOCKET)
    {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }
}

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
// IOSXDriver: Connection lifecycle
// ---------------------------------------------------------------------------

bool COSXDriver::Connect()
{
    if (IsConnected())
    {
        Log(LOG_WARNING, "Already connected. Disconnect first.");
        return true;
    }

    m_state = STATE_CONNECTING;

    for (int attempt = 1; attempt <= m_config.reconnectAttempts; ++attempt)
    {
        Log(LOG_INFO, "Connecting to %s:%d (attempt %d/%d)",
            m_config.ipAddress.c_str(), m_config.port,
            attempt, m_config.reconnectAttempts);

        m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (m_socket == INVALID_SOCKET)
        {
            Log(LOG_ERROR, "Failed to create socket: %d", WSAGetLastError());
            continue;
        }

        sockaddr_in addr = {};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(static_cast<u_short>(m_config.port));

        if (inet_pton(AF_INET, m_config.ipAddress.c_str(), &addr.sin_addr) != 1)
        {
            Log(LOG_ERROR, "Invalid IP address: %s", m_config.ipAddress.c_str());
            CleanupSocket();
            m_state = STATE_ERROR;
            return false;
        }

        if (!ConnectSocket(m_socket, addr, m_config.timeout))
        {
            Log(LOG_ERROR, "Connection attempt %d failed.", attempt);
            CleanupSocket();
            if (attempt < m_config.reconnectAttempts)
                Sleep(static_cast<DWORD>(m_config.reconnectDelay * 1000));
            continue;
        }

        DWORD timeoutMs = static_cast<DWORD>(m_config.timeout * 1000);
        setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeoutMs, sizeof(timeoutMs));
        setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeoutMs, sizeof(timeoutMs));
        EnableKeepAlive(m_socket);

        m_state = STATE_CONNECTED;
        if (!ValidateConnection())
        {
            Log(LOG_ERROR, "Post-connection validation failed (attempt %d).", attempt);
            m_state = STATE_CONNECTING;
            CleanupSocket();
            if (attempt < m_config.reconnectAttempts)
                Sleep(static_cast<DWORD>(m_config.reconnectDelay * 1000));
            continue;
        }

        Log(LOG_INFO, "Connection established and validated.");
        return true;
    }

    m_state = STATE_ERROR;
    Log(LOG_ERROR, "All connection attempts exhausted.");
    return false;
}

void COSXDriver::Disconnect()
{
    if (m_socket != INVALID_SOCKET)
    {
        try { Write(SCPI::CLS); } catch (...) {}
        shutdown(m_socket, SD_BOTH);
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        Log(LOG_INFO, "Disconnected.");
    }
    m_state = STATE_DISCONNECTED;
}

bool COSXDriver::Reconnect()
{
    Log(LOG_INFO, "Reconnecting...");
    Disconnect();
    Sleep(static_cast<DWORD>(m_config.reconnectDelay * 1000));
    return Connect();
}

bool COSXDriver::IsConnected() const
{
    return m_state == STATE_CONNECTED && m_socket != INVALID_SOCKET;
}

// ===========================================================================
// Low-level SCPI communication
// ===========================================================================

std::string COSXDriver::ReceiveResponse()
{
    std::string data;
    std::vector<char> buf(m_config.bufferSize + 1, 0);

    while (true)
    {
        int received = recv(m_socket, buf.data(), m_config.bufferSize, 0);
        if (received == SOCKET_ERROR)
        {
            int err = WSAGetLastError();
            if (err == WSAETIMEDOUT)
            {
                Log(LOG_WARNING, "Response timeout. Partial: %s", data.c_str());
                throw std::runtime_error("Response timeout.");
            }
            m_state = STATE_ERROR;
            throw std::runtime_error(std::string("Receive error: WSA ") + std::to_string(err));
        }
        if (received == 0)
        {
            m_state = STATE_ERROR;
            throw std::runtime_error("Connection closed by remote host.");
        }

        buf[received] = '\0';
        data.append(buf.data(), received);

        if (data.find('\n') != std::string::npos)
            break;
    }

    while (!data.empty() && (data.back() == '\n' || data.back() == '\r' || data.back() == ' '))
        data.pop_back();

    Log(LOG_DEBUG, "RX: %s", data.c_str());
    return data;
}

std::string COSXDriver::Query(const std::string& command)
{
    if (!IsConnected())
        throw std::runtime_error("Not connected to OSX device.");

    std::string fullCmd = command + "\n";
    Log(LOG_DEBUG, "TX: %s", command.c_str());

    int sent = send(m_socket, fullCmd.c_str(), static_cast<int>(fullCmd.size()), 0);
    if (sent == SOCKET_ERROR)
    {
        int wsaErr = WSAGetLastError();
        Log(LOG_WARNING, "Send failed (WSA %d). Attempting reconnect...", wsaErr);

        if (Reconnect())
        {
            Log(LOG_INFO, "Reconnected. Retrying: %s", command.c_str());
            sent = send(m_socket, fullCmd.c_str(), static_cast<int>(fullCmd.size()), 0);
        }

        if (sent == SOCKET_ERROR)
        {
            m_state = STATE_ERROR;
            throw std::runtime_error(std::string("Send failed, WSA error: ")
                + std::to_string(WSAGetLastError()));
        }
    }

    return ReceiveResponse();
}

void COSXDriver::Write(const std::string& command)
{
    if (!IsConnected())
        throw std::runtime_error("Not connected to OSX device.");

    std::string fullCmd = command + "\n";
    Log(LOG_DEBUG, "TX: %s", command.c_str());

    int sent = send(m_socket, fullCmd.c_str(), static_cast<int>(fullCmd.size()), 0);
    if (sent == SOCKET_ERROR)
    {
        int wsaErr = WSAGetLastError();
        Log(LOG_WARNING, "Send failed (WSA %d). Attempting reconnect...", wsaErr);

        if (Reconnect())
        {
            Log(LOG_INFO, "Reconnected. Retrying: %s", command.c_str());
            sent = send(m_socket, fullCmd.c_str(), static_cast<int>(fullCmd.size()), 0);
        }

        if (sent == SOCKET_ERROR)
        {
            m_state = STATE_ERROR;
            throw std::runtime_error(std::string("Send failed, WSA error: ")
                + std::to_string(WSAGetLastError()));
        }
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
// IOSXDriver: Device identification
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
// IOSXDriver: Module management
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

    // Parse comma-separated list: "SX 1Ax24","SX 2Bx12",...
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

        // Query detailed info
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
// IOSXDriver: Channel switching
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
// IOSXDriver: Multi-module routing
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
// IOSXDriver: Control
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
// IOSXDriver: Network configuration
// ===========================================================================

std::string COSXDriver::GetIPAddress()  { return Trim(Query(SCPI::LAN_ADDR_Q)); }
std::string COSXDriver::GetGateway()    { return Trim(Query(SCPI::LAN_GW_Q)); }
std::string COSXDriver::GetNetmask()    { return Trim(Query(SCPI::LAN_MASK_Q)); }
std::string COSXDriver::GetHostname()   { return Trim(Query(SCPI::LAN_HOST_Q)); }
std::string COSXDriver::GetMAC()        { return Trim(Query(SCPI::LAN_MAC_Q)); }

// ===========================================================================
// IOSXDriver: Operation synchronization
//
// The OSX runs SCPI commands asynchronously. After issuing a switch command,
// poll STAT:OPER:COND? until 0 to confirm the operation has completed.
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
// IOSXDriver: Raw SCPI passthrough
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
// Parsing utilities
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
    // Catalog entries look like "SX 1Ax24", "SX 2Bx12", etc.
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
    // Extract number after 'x' in entries like "SX 1Ax24"
    size_t xPos = catalog.find('x');
    if (xPos == std::string::npos)
        xPos = catalog.find('X');
    if (xPos == std::string::npos || xPos + 1 >= catalog.size())
        return 0;

    // Find the last 'x' which separates config from channel count
    size_t lastX = catalog.rfind('x');
    if (lastX == std::string::npos)
        lastX = catalog.rfind('X');
    if (lastX == std::string::npos || lastX + 1 >= catalog.size())
        return 0;

    return atoi(catalog.substr(lastX + 1).c_str());
}

} // namespace OSXSwitch
