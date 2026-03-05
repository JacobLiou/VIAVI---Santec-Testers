#include "stdafx.h"
#include "BaseEquipmentDriver.h"
#include <stdexcept>

namespace ViaviNSantecTester {

CBaseEquipmentDriver::CBaseEquipmentDriver(const ConnectionConfig& config, const std::string& deviceType)
    : m_config(config)
    , m_deviceType(deviceType)
    , m_socket(INVALID_SOCKET)
    , m_state(STATE_DISCONNECTED)
    , m_logger(std::string("Driver.") + deviceType)
{
}

CBaseEquipmentDriver::~CBaseEquipmentDriver()
{
    Disconnect();
}

// ---------------------------------------------------------------------------
// 带超时的非阻塞连接（基于 select）
// ---------------------------------------------------------------------------

bool CBaseEquipmentDriver::ConnectSocket(SOCKET sock, const sockaddr_in& addr, double timeoutSec)
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
        m_logger.Error("connect() failed immediately: WSA %d", err);
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
        m_logger.Error("Connection timed out after %.1f seconds.", timeoutSec);
        return false;
    }
    if (ret == SOCKET_ERROR)
    {
        m_logger.Error("select() failed: WSA %d", WSAGetLastError());
        return false;
    }

    if (FD_ISSET(sock, &exceptfds))
    {
        int optError = 0;
        int optLen = sizeof(optError);
        getsockopt(sock, SOL_SOCKET, SO_ERROR, (char*)&optError, &optLen);
        m_logger.Error("Connection failed: socket error %d", optError);
        return false;
    }

    if (!FD_ISSET(sock, &writefds))
    {
        m_logger.Error("Connection failed: socket not writable after select.");
        return false;
    }

    int optError = 0;
    int optLen = sizeof(optError);
    getsockopt(sock, SOL_SOCKET, SO_ERROR, (char*)&optError, &optLen);
    if (optError != 0)
    {
        m_logger.Error("Connection failed: SO_ERROR = %d", optError);
        return false;
    }

    u_long blocking = 0;
    ioctlsocket(sock, FIONBIO, &blocking);
    return true;
}

// ---------------------------------------------------------------------------
// TCP 保活
// ---------------------------------------------------------------------------

void CBaseEquipmentDriver::EnableKeepAlive(SOCKET sock)
{
    BOOL keepAlive = TRUE;
    setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (const char*)&keepAlive, sizeof(keepAlive));

    struct tcp_keepalive ka = {};
    ka.onoff = 1;
    ka.keepalivetime = 10000;       // 空闲10秒后发送第一个探测包
    ka.keepaliveinterval = 2000;    // 探测包间隔2秒
    DWORD bytesReturned = 0;
    WSAIoctl(sock, SIO_KEEPALIVE_VALS, &ka, sizeof(ka),
             NULL, 0, &bytesReturned, NULL, NULL);

    m_logger.Debug("TCP keepalive enabled (idle=10s, interval=2s).");
}

// ---------------------------------------------------------------------------
// 连接后验证（默认：检查套接字健康状态）
// ---------------------------------------------------------------------------

bool CBaseEquipmentDriver::ValidateConnection()
{
    if (m_socket == INVALID_SOCKET)
        return false;

    fd_set writefds;
    FD_ZERO(&writefds);
    FD_SET(m_socket, &writefds);

    timeval tv = {};
    tv.tv_sec = 0;
    tv.tv_usec = 100000; // 100毫秒

    int ret = select(0, NULL, &writefds, NULL, &tv);
    if (ret <= 0)
    {
        m_logger.Error("Validation failed: socket not writable.");
        return false;
    }

    int optError = 0;
    int optLen = sizeof(optError);
    getsockopt(m_socket, SOL_SOCKET, SO_ERROR, (char*)&optError, &optLen);
    if (optError != 0)
    {
        m_logger.Error("Validation failed: SO_ERROR = %d", optError);
        return false;
    }

    m_logger.Debug("Default socket validation passed.");
    return true;
}

// ---------------------------------------------------------------------------
// 主动健康检查
// ---------------------------------------------------------------------------

bool CBaseEquipmentDriver::IsAlive() const
{
    if (m_socket == INVALID_SOCKET || m_state != STATE_CONNECTED)
        return false;

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(m_socket, &readfds);

    timeval tv = {};
    int ret = select(0, &readfds, NULL, NULL, &tv);
    if (ret > 0)
    {
        char buf;
        int result = recv(m_socket, &buf, 1, MSG_PEEK);
        if (result == 0 || result == SOCKET_ERROR)
            return false;
    }

    return true;
}

// ---------------------------------------------------------------------------
// 连接管理
// ---------------------------------------------------------------------------

bool CBaseEquipmentDriver::Connect()
{
    if (IsConnected())
    {
        m_logger.Warning("Already connected. Disconnect first.");
        return true;
    }

    m_state = STATE_CONNECTING;

    for (int attempt = 1; attempt <= m_config.reconnectAttempts; ++attempt)
    {
        m_logger.Info("Connecting to %s:%d (attempt %d/%d)",
            m_config.ipAddress.c_str(), m_config.port,
            attempt, m_config.reconnectAttempts);

        m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (m_socket == INVALID_SOCKET)
        {
            m_logger.Error("Failed to create socket: %d", WSAGetLastError());
            continue;
        }

        sockaddr_in addr = {};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(static_cast<u_short>(m_config.port));

        if (inet_pton(AF_INET, m_config.ipAddress.c_str(), &addr.sin_addr) != 1)
        {
            m_logger.Error("Invalid IP address format: %s", m_config.ipAddress.c_str());
            CleanupSocket();
            m_state = STATE_ERROR;
            return false;
        }

        if (!ConnectSocket(m_socket, addr, m_config.timeout))
        {
            m_logger.Error("Connection attempt %d failed.", attempt);
            CleanupSocket();
            if (attempt < m_config.reconnectAttempts)
                Sleep(static_cast<DWORD>(m_config.reconnectDelay * 1000));
            continue;
        }

        // TCP 握手成功 - 配置套接字选项
        DWORD timeoutMs = static_cast<DWORD>(m_config.timeout * 1000);
        setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeoutMs, sizeof(timeoutMs));
        setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeoutMs, sizeof(timeoutMs));
        EnableKeepAlive(m_socket);

        // 验证是否连接到真实设备
        m_state = STATE_CONNECTED;
        if (!ValidateConnection())
        {
            m_logger.Error("Post-connection validation failed (attempt %d).", attempt);
            m_state = STATE_CONNECTING;
            CleanupSocket();
            if (attempt < m_config.reconnectAttempts)
                Sleep(static_cast<DWORD>(m_config.reconnectDelay * 1000));
            continue;
        }

        m_logger.Info("Connection established and validated successfully.");
        return true;
    }

    m_state = STATE_ERROR;
    m_logger.Error("All connection attempts exhausted.");
    return false;
}

void CBaseEquipmentDriver::Disconnect()
{
    if (m_socket != INVALID_SOCKET)
    {
        shutdown(m_socket, SD_BOTH);
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        m_logger.Info("Disconnected from device.");
    }
    m_state = STATE_DISCONNECTED;
}

bool CBaseEquipmentDriver::Reconnect()
{
    m_logger.Info("Reconnecting...");
    Disconnect();
    Sleep(static_cast<DWORD>(m_config.reconnectDelay * 1000));
    return Connect();
}

bool CBaseEquipmentDriver::IsConnected() const
{
    return m_state == STATE_CONNECTED && m_socket != INVALID_SOCKET;
}

void CBaseEquipmentDriver::CleanupSocket()
{
    if (m_socket != INVALID_SOCKET)
    {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }
}

// ---------------------------------------------------------------------------
// 底层通信
// ---------------------------------------------------------------------------

std::string CBaseEquipmentDriver::SendCommand(const std::string& command,
                                               const std::string& endChar,
                                               bool expectResponse)
{
    if (!IsConnected())
        throw std::runtime_error("Not connected to device.");

    std::string fullCommand = command + endChar;
    m_logger.Debug("TX: %s", command.c_str());

    int sent = send(m_socket, fullCommand.c_str(), static_cast<int>(fullCommand.size()), 0);
    if (sent == SOCKET_ERROR)
    {
        int wsaErr = WSAGetLastError();
        m_logger.Warning("Send failed (WSA %d). Attempting auto-reconnect...", wsaErr);

        // 可以安全重试：命令从未送达
        if (Reconnect())
        {
            m_logger.Info("Reconnected. Retrying command: %s", command.c_str());
            sent = send(m_socket, fullCommand.c_str(), static_cast<int>(fullCommand.size()), 0);
        }

        if (sent == SOCKET_ERROR)
        {
            m_state = STATE_ERROR;
            throw std::runtime_error(std::string("Send failed, WSA error: ")
                + std::to_string(WSAGetLastError()));
        }
    }

    bool isQuery = (command.find('?') != std::string::npos);
    if (expectResponse || isQuery)
    {
        return ReceiveResponse(m_config.bufferSize);
    }
    return std::string();
}

std::string CBaseEquipmentDriver::ReceiveResponse(int bufferSize)
{
    std::string data;
    std::vector<char> buf(bufferSize + 1, 0);

    while (true)
    {
        int received = recv(m_socket, buf.data(), bufferSize, 0);
        if (received == SOCKET_ERROR)
        {
            int err = WSAGetLastError();
            if (err == WSAETIMEDOUT)
            {
                m_logger.Warning("Response timeout. Partial data: %s", data.c_str());
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

    m_logger.Debug("RX: %s", data.c_str());
    return data;
}

// ---------------------------------------------------------------------------
// 错误处理
// ---------------------------------------------------------------------------

void CBaseEquipmentDriver::AssertNoError()
{
    ErrorInfo err = CheckError();
    if (!err.IsOK())
    {
        m_logger.Error("Device error [%d]: %s", err.code, err.message.c_str());
        throw std::runtime_error(
            std::string("Device error [") + std::to_string(err.code)
            + "]: " + err.message);
    }
}

// ---------------------------------------------------------------------------
// 高级工作流（模板方法）
// ---------------------------------------------------------------------------

std::vector<MeasurementResult> CBaseEquipmentDriver::RunFullTest(
    const std::vector<double>& wavelengths,
    const std::vector<int>& channels,
    bool doReference,
    const ReferenceConfig& refConfig)
{
    m_logger.Info("=== Starting Full Test Workflow ===");

    // 步骤1：配置
    m_logger.Info("Configuring %d wavelength(s)...", (int)wavelengths.size());
    ConfigureWavelengths(wavelengths);
    AssertNoError();

    m_logger.Info("Configuring %d channel(s)...", (int)channels.size());
    ConfigureChannels(channels);
    AssertNoError();

    // 步骤2：参考
    if (doReference)
    {
        m_logger.Info("Taking reference (override=%s)...",
            refConfig.useOverride ? "true" : "false");
        if (!TakeReference(refConfig))
            throw std::runtime_error("Reference measurement failed.");
        AssertNoError();
    }

    // 步骤3：测量
    m_logger.Info("Taking measurement...");
    if (!TakeMeasurement())
        throw std::runtime_error("Measurement failed.");
    AssertNoError();

    // 步骤4：结果
    m_logger.Info("Retrieving results...");
    std::vector<MeasurementResult> results = GetResults();
    m_logger.Info("=== Test Complete: %d results ===", (int)results.size());

    return results;
}

} // namespace ViaviNSantecTester
