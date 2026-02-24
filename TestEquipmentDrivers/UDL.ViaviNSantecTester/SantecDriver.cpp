#include "stdafx.h"
#include "SantecDriver.h"
#include <stdexcept>
#include <sstream>

namespace ViaviNSantecTester {

// ===========================================================================
// CSantecTcpAdapter - Default TCP communication adapter
// ===========================================================================

CSantecTcpAdapter::CSantecTcpAdapter()
    : m_socket(INVALID_SOCKET)
    , m_bufferSize(1024)
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

    // Non-blocking connect with timeout (same pattern as BaseEquipmentDriver)
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

    // Restore blocking mode and set I/O timeouts
    u_long blocking = 0;
    ioctlsocket(m_socket, FIONBIO, &blocking);

    DWORD timeoutMs = static_cast<DWORD>(timeout * 1000);
    setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeoutMs, sizeof(timeoutMs));
    setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeoutMs, sizeof(timeoutMs));

    // Enable TCP keepalive
    BOOL keepAlive = TRUE;
    setsockopt(m_socket, SOL_SOCKET, SO_KEEPALIVE, (const char*)&keepAlive, sizeof(keepAlive));

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

std::string CSantecTcpAdapter::SendQuery(const std::string& command)
{
    SendWrite(command);

    std::string data;
    char buf[1024];
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
{
    m_config.ipAddress = ipAddress;
    m_config.port = port;
    m_config.timeout = timeout;
    m_config.bufferSize = 1024;
    m_config.reconnectAttempts = 3;
    m_config.reconnectDelay = 2.0;
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
// Connection
// ---------------------------------------------------------------------------

bool CSantecDriver::Connect()
{
    m_logger.Info("Connecting to Santec equipment at %s:%d (comm=%d)",
        m_config.ipAddress.c_str(), m_config.port, static_cast<int>(m_commType));

    switch (m_commType)
    {
    case COMM_TCP:
    {
        if (m_adapter)
        {
            for (int attempt = 1; attempt <= m_config.reconnectAttempts; ++attempt)
            {
                m_logger.Info("Adapter connect attempt %d/%d", attempt, m_config.reconnectAttempts);

                if (!m_adapter->Open(m_config.ipAddress, m_config.port, m_config.timeout))
                {
                    m_logger.Error("Adapter connection attempt %d failed.", attempt);
                    if (attempt < m_config.reconnectAttempts)
                        Sleep(static_cast<DWORD>(m_config.reconnectDelay * 1000));
                    continue;
                }

                m_state = STATE_CONNECTED;

                if (!ValidateConnection())
                {
                    m_logger.Error("Adapter validation failed (attempt %d).", attempt);
                    m_adapter->Close();
                    m_state = STATE_DISCONNECTED;
                    if (attempt < m_config.reconnectAttempts)
                        Sleep(static_cast<DWORD>(m_config.reconnectDelay * 1000));
                    continue;
                }

                m_logger.Info("Santec connected and validated via adapter.");
                return true;
            }

            m_state = STATE_ERROR;
            m_logger.Error("All adapter connection attempts exhausted.");
            return false;
        }

        // No adapter: use base class TCP (includes ConnectSocket + ValidateConnection)
        return CBaseEquipmentDriver::Connect();
    }
    case COMM_GPIB:
        m_logger.Warning("GPIB communication not yet implemented for Santec.");
        return false;
    case COMM_USB:
        m_logger.Warning("USB communication not yet implemented for Santec.");
        return false;
    case COMM_DLL:
        m_logger.Warning("DLL communication not yet implemented for Santec.");
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
        m_adapter->Close();
        m_state = STATE_DISCONNECTED;
        m_logger.Info("Santec adapter disconnected.");
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
// Post-connection validation
// ---------------------------------------------------------------------------

bool CSantecDriver::ValidateConnection()
{
    m_logger.Info("Validating Santec connection...");
    try
    {
        // Try a standard SCPI identity query to confirm the device is responsive.
        // TODO: Replace with confirmed Santec command once protocol docs are available.
        std::string response;
        if (m_adapter && m_adapter->IsOpen())
            response = m_adapter->SendQuery("*IDN?");
        else if (m_socket != INVALID_SOCKET)
            response = SendCommand("*IDN?");
        else
            return false;

        if (response.empty())
        {
            m_logger.Error("Validation failed: no response from *IDN?");
            return false;
        }

        m_logger.Info("Santec validation OK (*IDN? -> %s)", response.c_str());
        return true;
    }
    catch (const std::exception& e)
    {
        m_logger.Error("Validation failed: %s", e.what());
        return false;
    }
}

// ---------------------------------------------------------------------------
// Error Handling (placeholder)
// ---------------------------------------------------------------------------

ErrorInfo CSantecDriver::CheckError()
{
    // TODO: Implement with actual Santec error query command
    ErrorInfo info;
    try
    {
        std::string response;
        if (m_adapter && m_adapter->IsOpen())
            response = m_adapter->SendQuery("SYST:ERR?");
        else if (IsConnected())
            response = SendCommand("SYST:ERR?");

        if (!response.empty())
        {
            size_t commaPos = response.find(',');
            if (commaPos != std::string::npos)
            {
                info.code = atoi(response.substr(0, commaPos).c_str());
                info.message = response.substr(commaPos + 1);
            }
        }
    }
    catch (...)
    {
        // Placeholder: suppress errors until protocol is confirmed
    }
    return info;
}

// ---------------------------------------------------------------------------
// Device Info (placeholder)
// ---------------------------------------------------------------------------

DeviceInfo CSantecDriver::GetDeviceInfo()
{
    DeviceInfo info;
    info.manufacturer = "Santec";
    info.model = "TBD";    // TODO: Query via *IDN? when protocol is known
    return info;
}

// ---------------------------------------------------------------------------
// Initialization (placeholder)
// ---------------------------------------------------------------------------

bool CSantecDriver::Initialize()
{
    m_logger.Info("Initializing Santec equipment...");
    m_logger.Warning("Santec initialization is PLACEHOLDER. "
                     "Implement after receiving protocol documentation.");

    // TODO: Possible initialization steps:
    // - Identity query: *IDN?
    // - System reset: *RST
    // - Default parameter configuration
    // - Calibration check

    return true;
}

// ---------------------------------------------------------------------------
// Configuration (placeholder)
// ---------------------------------------------------------------------------

void CSantecDriver::ConfigureWavelengths(const std::vector<double>& wavelengths)
{
    m_wavelengths = wavelengths;
    m_logger.Warning("Santec wavelength configuration is PLACEHOLDER. (%d wavelengths)",
        (int)wavelengths.size());

    // TODO: Implement with actual Santec wavelength commands
}

void CSantecDriver::ConfigureChannels(const std::vector<int>& channels)
{
    m_channels = channels;
    m_logger.Warning("Santec channel configuration is PLACEHOLDER. (%d channels)",
        (int)channels.size());

    // TODO: Implement with actual Santec channel commands
}

// ---------------------------------------------------------------------------
// Reference (placeholder)
// ---------------------------------------------------------------------------

bool CSantecDriver::TakeReference(const ReferenceConfig& config)
{
    m_logger.Info("Taking reference measurement (override=%s)...",
        config.useOverride ? "true" : "false");
    m_logger.Warning("Santec reference measurement is PLACEHOLDER.");

    // TODO: Implement Santec reference/calibration procedure
    return true;
}

// ---------------------------------------------------------------------------
// Measurement (placeholder)
// ---------------------------------------------------------------------------

bool CSantecDriver::TakeMeasurement()
{
    m_logger.Info("Starting Santec measurement...");
    m_logger.Warning("Santec measurement is PLACEHOLDER.");

    // TODO: Implement Santec measurement commands
    return true;
}

bool CSantecDriver::WaitForCompletion(const std::string& operationName)
{
    // TODO: Implement polling based on Santec status query mechanism
    DWORD startTick = GetTickCount();
    while (true)
    {
        // Placeholder: immediately return success
        m_logger.Info("%s completed (placeholder).", operationName.c_str());
        return true;

        DWORD elapsed = GetTickCount() - startTick;
        if (elapsed > static_cast<DWORD>(MAX_POLL_TIME_MS))
        {
            m_logger.Error("%s timed out.", operationName.c_str());
            return false;
        }
        Sleep(POLL_INTERVAL_MS);
    }
}

// ---------------------------------------------------------------------------
// Results (placeholder)
// ---------------------------------------------------------------------------

std::vector<MeasurementResult> CSantecDriver::GetResults()
{
    m_logger.Warning("Santec result retrieval is PLACEHOLDER.");

    std::vector<MeasurementResult> results;

    for (size_t ci = 0; ci < m_channels.size(); ++ci)
    {
        for (size_t wi = 0; wi < m_wavelengths.size(); ++wi)
        {
            MeasurementResult result;
            result.channel = m_channels[ci];
            result.wavelength = m_wavelengths[wi];
            result.insertionLoss = 0.0;
            result.returnLoss = 0.0;
            result.rawDataCount = 0;
            results.push_back(result);
        }
    }

    return results;
}

} // namespace ViaviNSantecTester
