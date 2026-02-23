#include "stdafx.h"
#include "BaseEquipmentDriver.h"
#include <stdexcept>

namespace EquipmentDriver {

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
// Connection Management
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

        // Set timeout
        DWORD timeoutMs = static_cast<DWORD>(m_config.timeout * 1000);
        setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeoutMs, sizeof(timeoutMs));
        setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeoutMs, sizeof(timeoutMs));

        sockaddr_in addr = {};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(static_cast<u_short>(m_config.port));
        inet_pton(AF_INET, m_config.ipAddress.c_str(), &addr.sin_addr);

        if (connect(m_socket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
        {
            m_logger.Error("Connection attempt %d failed: %d", attempt, WSAGetLastError());
            CleanupSocket();
            if (attempt < m_config.reconnectAttempts)
                Sleep(static_cast<DWORD>(m_config.reconnectDelay * 1000));
            continue;
        }

        m_state = STATE_CONNECTED;
        m_logger.Info("Connection established successfully.");
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
// Low-level Communication
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
        m_state = STATE_ERROR;
        throw std::runtime_error(std::string("Failed to send command, WSA error: ")
            + std::to_string(WSAGetLastError()));
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
            throw std::runtime_error(std::string("Receive error: ") + std::to_string(err));
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

    // Trim trailing whitespace/newline
    while (!data.empty() && (data.back() == '\n' || data.back() == '\r' || data.back() == ' '))
        data.pop_back();

    m_logger.Debug("RX: %s", data.c_str());
    return data;
}

// ---------------------------------------------------------------------------
// Error Handling
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
// High-level Workflow (Template Method)
// ---------------------------------------------------------------------------

std::vector<MeasurementResult> CBaseEquipmentDriver::RunFullTest(
    const std::vector<double>& wavelengths,
    const std::vector<int>& channels,
    bool doReference,
    const ReferenceConfig& refConfig)
{
    m_logger.Info("=== Starting Full Test Workflow ===");

    // Step 1: Configure
    m_logger.Info("Configuring %d wavelength(s)...", (int)wavelengths.size());
    ConfigureWavelengths(wavelengths);
    AssertNoError();

    m_logger.Info("Configuring %d channel(s)...", (int)channels.size());
    ConfigureChannels(channels);
    AssertNoError();

    // Step 2: Reference
    if (doReference)
    {
        m_logger.Info("Taking reference (override=%s)...",
            refConfig.useOverride ? "true" : "false");
        if (!TakeReference(refConfig))
            throw std::runtime_error("Reference measurement failed.");
        AssertNoError();
    }

    // Step 3: Measurement
    m_logger.Info("Taking measurement...");
    if (!TakeMeasurement())
        throw std::runtime_error("Measurement failed.");
    AssertNoError();

    // Step 4: Results
    m_logger.Info("Retrieving results...");
    std::vector<MeasurementResult> results = GetResults();
    m_logger.Info("=== Test Complete: %d results ===", (int)results.size());

    return results;
}

} // namespace EquipmentDriver
