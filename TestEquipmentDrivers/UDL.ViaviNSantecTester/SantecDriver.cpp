#include "stdafx.h"
#include "SantecDriver.h"
#include <stdexcept>
#include <sstream>
#include <cmath>

namespace ViaviNSantecTester {

// ===========================================================================
// SCPI Command Definitions for Santec RLM-100 / ILM-100
//
// Reference: Santec Programming Guide v1.23 & RLM-100 Sample Code
//            https://inst.santec.com/resources/programming
//
// If your firmware uses different mnemonics, update ONLY these constants.
// ===========================================================================
namespace SCPI {
    // IEEE 488.2 common commands
    static const char* IDN          = "*IDN?";
    static const char* RST          = "*RST";
    static const char* CLS          = "*CLS";
    static const char* OPC          = "*OPC?";

    // System
    static const char* SYS_ERR      = "SYST:ERR?";

    // Wavelength / source configuration
    static const char* CONF_WAV     = "CONF:WAV";          // CONF:WAV 1310
    static const char* CONF_WAV_Q   = "CONF:WAV?";
    static const char* CONF_SPEED   = "CONF:SPEED";        // CONF:SPEED FAST | STANDARD
    static const char* CONF_CHAN    = "CONF:CHAN";           // CONF:CHAN 1

    // Reference / calibration
    static const char* REF_START    = "REF:STAR";           // initiate auto-reference
    static const char* REF_STATUS   = "REF:STAT?";          // 0=not done, 1=done
    static const char* REF_CLEAR    = "REF:CLE";            // clear stored reference

    // Measurement
    static const char* MEAS_INIT    = "INIT:IMM";           // trigger a measurement
    static const char* MEAS_STATUS  = "STAT:OPER?";         // bit mask: measurement in progress
    static const char* MEAS_ABORT   = "ABOR";

    // Fetch results (after measurement completes)
    static const char* FETCH_IL     = "FETC:IL?";           // insertion loss in dB
    static const char* FETCH_RL     = "FETC:RL?";           // return loss in dB (RLM only)
    static const char* FETCH_ALL    = "FETC?";              // all results

    // Status register bits (typical SCPI operation register)
    static const int STAT_MEAS_RUNNING = 0x10;   // bit 4
}


// ===========================================================================
// CSantecTcpAdapter - TCP SCPI transport
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

    // Non-blocking connect with select()-based timeout
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

    // Restore blocking mode
    u_long blocking = 0;
    ioctlsocket(m_socket, FIONBIO, &blocking);

    DWORD timeoutMs = static_cast<DWORD>(timeout * 1000);
    setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeoutMs, sizeof(timeoutMs));
    setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeoutMs, sizeof(timeoutMs));

    // TCP keepalive for dead-peer detection
    BOOL keepAlive = TRUE;
    setsockopt(m_socket, SOL_SOCKET, SO_KEEPALIVE, (const char*)&keepAlive, sizeof(keepAlive));

    tcp_keepalive ka = {};
    ka.onoff = 1;
    ka.keepalivetime = 10000;       // 10s idle before first probe
    ka.keepaliveinterval = 2000;    // 2s between probes
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
    // Trim trailing CR/LF
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
    , m_speed(SPEED_FAST)
    , m_referenced(false)
{
    m_config.ipAddress = ipAddress;
    m_config.port = port;
    m_config.timeout = timeout;
    m_config.bufferSize = 4096;
    m_config.reconnectAttempts = 3;
    m_config.reconnectDelay = 2.0;

    // Default wavelengths for SM fiber
    m_wavelengths.push_back(1310.0);
    m_wavelengths.push_back(1550.0);

    // Default to single channel
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
// Unified command interface
// ---------------------------------------------------------------------------

std::string CSantecDriver::Query(const std::string& command)
{
    m_logger.Debug("TX> %s", command.c_str());
    std::string response;
    if (m_adapter && m_adapter->IsOpen())
        response = m_adapter->SendQuery(command);
    else if (m_socket != INVALID_SOCKET)
    {
        response = SendCommand(command, "\n", true);
    }
    else
        throw std::runtime_error("Not connected to Santec device.");

    m_logger.Debug("RX< %s", response.c_str());
    return response;
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

// ---------------------------------------------------------------------------
// Connection
// ---------------------------------------------------------------------------

bool CSantecDriver::Connect()
{
    m_logger.Info("Connecting to Santec %s:%d (comm=%d)",
        m_config.ipAddress.c_str(), m_config.port, static_cast<int>(m_commType));

    switch (m_commType)
    {
    case COMM_TCP:
    {
        // Create default TCP adapter if none was provided externally
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
        m_logger.Warning("USB communication not yet implemented.");
        return false;
    case COMM_DLL:
        m_logger.Warning("DLL communication not yet implemented. "
                         "See Santec.Hardware.dll at https://inst.santec.com/resources/programming");
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
        try { Write(SCPI::CLS); } catch (...) {}
        m_adapter->Close();
        m_state = STATE_DISCONNECTED;
        m_referenced = false;
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
// Post-connection validation: *IDN? to verify device identity
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
    // IEEE 488.2 *IDN? format: "Manufacturer,Model,Serial,Firmware"
    std::vector<std::string> parts;
    std::istringstream iss(idnResponse);
    std::string token;
    while (std::getline(iss, token, ','))
        parts.push_back(Trim(token));

    if (parts.size() >= 1) m_deviceInfo.manufacturer = parts[0];
    if (parts.size() >= 2) m_deviceInfo.model = parts[1];
    if (parts.size() >= 3) m_deviceInfo.serialNumber = parts[2];
    if (parts.size() >= 4) m_deviceInfo.firmwareVersion = parts[3];

    // Auto-detect model from the model string
    std::string modelUpper = m_deviceInfo.model;
    for (size_t i = 0; i < modelUpper.size(); ++i)
        modelUpper[i] = static_cast<char>(toupper(static_cast<unsigned char>(modelUpper[i])));

    if (modelUpper.find("RLM") != std::string::npos || modelUpper.find("RL1") != std::string::npos)
        m_model = MODEL_RLM_100;
    else if (modelUpper.find("ILM") != std::string::npos || modelUpper.find("IL1") != std::string::npos)
        m_model = MODEL_ILM_100;
    else if (modelUpper.find("BRM") != std::string::npos || modelUpper.find("BR1") != std::string::npos)
        m_model = MODEL_BRM_100;
    else
        m_model = MODEL_UNKNOWN;

    m_logger.Info("Auto-detected model: %d (%s)", static_cast<int>(m_model), m_deviceInfo.model.c_str());
}

// ---------------------------------------------------------------------------
// Error handling
// ---------------------------------------------------------------------------

ErrorInfo CSantecDriver::CheckError()
{
    ErrorInfo info;
    try
    {
        std::string response = Query(SCPI::SYS_ERR);
        if (!response.empty())
        {
            // SCPI format: <code>,"<message>"
            size_t commaPos = response.find(',');
            if (commaPos != std::string::npos)
            {
                info.code = atoi(response.substr(0, commaPos).c_str());
                std::string msg = response.substr(commaPos + 1);
                // Strip surrounding quotes
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
// Initialization
// ---------------------------------------------------------------------------

bool CSantecDriver::Initialize()
{
    m_logger.Info("Initializing Santec equipment...");

    try
    {
        // 1. Clear status registers
        Write(SCPI::CLS);
        Sleep(100);

        // 2. Set measurement speed
        std::string speedCmd = std::string(SCPI::CONF_SPEED) + " " +
            (m_speed == SPEED_FAST ? "FAST" : "STANDARD");
        Write(speedCmd);
        Sleep(100);

        // 3. Configure default wavelengths
        for (size_t i = 0; i < m_wavelengths.size(); ++i)
        {
            std::ostringstream cmd;
            cmd << SCPI::CONF_WAV << " " << static_cast<int>(m_wavelengths[i]);
            Write(cmd.str());
            Sleep(50);
        }

        // 4. Verify no errors after initialization
        ErrorInfo err = CheckError();
        if (!err.IsOK())
        {
            m_logger.Warning("Init completed with error: [%d] %s", err.code, err.message.c_str());
        }

        // 5. Query operation completion
        std::string opc = Query(SCPI::OPC);
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
// Configuration
// ---------------------------------------------------------------------------

void CSantecDriver::ConfigureWavelengths(const std::vector<double>& wavelengths)
{
    m_wavelengths = wavelengths;
    m_logger.Info("Configuring %d wavelength(s)...", (int)wavelengths.size());

    if (!IsConnected()) return;

    try
    {
        for (size_t i = 0; i < wavelengths.size(); ++i)
        {
            std::ostringstream cmd;
            cmd << SCPI::CONF_WAV << " " << static_cast<int>(wavelengths[i]);
            Write(cmd.str());
            m_logger.Info("  Wavelength: %d nm", static_cast<int>(wavelengths[i]));
            Sleep(50);
        }
    }
    catch (const std::exception& e)
    {
        m_logger.Error("ConfigureWavelengths failed: %s", e.what());
    }
}

void CSantecDriver::ConfigureChannels(const std::vector<int>& channels)
{
    m_channels = channels;
    m_logger.Info("Configuring %d channel(s)...", (int)channels.size());

    if (!IsConnected()) return;

    try
    {
        for (size_t i = 0; i < channels.size(); ++i)
        {
            std::ostringstream cmd;
            cmd << SCPI::CONF_CHAN << " " << channels[i];
            Write(cmd.str());
            m_logger.Info("  Channel: %d", channels[i]);
            Sleep(50);
        }
    }
    catch (const std::exception& e)
    {
        m_logger.Error("ConfigureChannels failed: %s", e.what());
    }
}

// ---------------------------------------------------------------------------
// Reference / Calibration
// ---------------------------------------------------------------------------

bool CSantecDriver::TakeReference(const ReferenceConfig& config)
{
    m_logger.Info("Taking reference (override=%s)...", config.useOverride ? "true" : "false");
    m_referenced = false;

    try
    {
        // Clear any previous reference
        Write(SCPI::REF_CLEAR);
        Sleep(200);

        // Initiate reference measurement
        Write(SCPI::REF_START);

        // Poll for reference completion
        if (!WaitForCompletion("Reference"))
        {
            m_logger.Error("Reference did not complete within timeout.");
            return false;
        }

        // Verify reference status
        std::string status = Query(SCPI::REF_STATUS);
        if (status.find("1") != std::string::npos || status.find("DONE") != std::string::npos)
        {
            m_referenced = true;
            m_logger.Info("Reference completed successfully.");
        }
        else
        {
            m_logger.Error("Reference failed (status=%s).", status.c_str());
            return false;
        }

        // Check for errors
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
// Measurement
// ---------------------------------------------------------------------------

bool CSantecDriver::TakeMeasurement()
{
    m_logger.Info("Starting measurement...");

    if (!m_referenced)
        m_logger.Warning("No reference taken - measurement results may be inaccurate.");

    try
    {
        // Initiate measurement on all configured wavelengths
        Write(SCPI::MEAS_INIT);

        // Wait for completion
        if (!WaitForCompletion("Measurement"))
        {
            m_logger.Error("Measurement did not complete within timeout.");
            return false;
        }

        // Check for errors
        ErrorInfo err = CheckError();
        if (!err.IsOK())
        {
            m_logger.Warning("Measurement completed with error: [%d] %s", err.code, err.message.c_str());
        }

        m_logger.Info("Measurement completed.");
        return true;
    }
    catch (const std::exception& e)
    {
        m_logger.Error("TakeMeasurement exception: %s", e.what());
        return false;
    }
}

bool CSantecDriver::WaitForCompletion(const std::string& operationName)
{
    m_logger.Info("Waiting for %s to complete...", operationName.c_str());
    DWORD startTick = GetTickCount();

    while (true)
    {
        DWORD elapsed = GetTickCount() - startTick;
        if (elapsed > static_cast<DWORD>(MAX_POLL_TIME_MS))
        {
            m_logger.Error("%s timed out after %d ms.", operationName.c_str(), elapsed);
            return false;
        }

        try
        {
            // Poll operation status register
            std::string status = Query(SCPI::MEAS_STATUS);
            int statusBits = atoi(status.c_str());

            if ((statusBits & SCPI::STAT_MEAS_RUNNING) == 0)
            {
                m_logger.Info("%s completed in %d ms.", operationName.c_str(), elapsed);
                return true;
            }
        }
        catch (const std::exception& e)
        {
            m_logger.Warning("Status poll error: %s (retrying)", e.what());
        }

        Sleep(POLL_INTERVAL_MS);
    }
}

// ---------------------------------------------------------------------------
// Results
// ---------------------------------------------------------------------------

std::vector<MeasurementResult> CSantecDriver::GetResults()
{
    m_logger.Info("Retrieving results...");
    std::vector<MeasurementResult> results;

    try
    {
        // Query per channel, per wavelength
        for (size_t ci = 0; ci < m_channels.size(); ++ci)
        {
            // Select channel if multi-channel (OSX switch connected)
            if (m_channels.size() > 1)
            {
                std::ostringstream chanCmd;
                chanCmd << SCPI::CONF_CHAN << " " << m_channels[ci];
                Write(chanCmd.str());
                Sleep(50);
            }

            for (size_t wi = 0; wi < m_wavelengths.size(); ++wi)
            {
                // Select wavelength
                std::ostringstream wavCmd;
                wavCmd << SCPI::CONF_WAV << " " << static_cast<int>(m_wavelengths[wi]);
                Write(wavCmd.str());
                Sleep(50);

                MeasurementResult result;
                result.channel = m_channels[ci];
                result.wavelength = m_wavelengths[wi];
                result.insertionLoss = 0.0;
                result.returnLoss = 0.0;
                result.rawDataCount = 0;

                // Fetch insertion loss (all models)
                std::string ilResp = Query(SCPI::FETCH_IL);
                if (!ilResp.empty())
                {
                    std::vector<double> ilValues = ParseDoubleList(ilResp);
                    if (!ilValues.empty())
                        result.insertionLoss = ilValues[0];

                    for (size_t r = 0; r < ilValues.size() && r < 10; ++r)
                    {
                        result.rawData[result.rawDataCount++] = ilValues[r];
                    }
                }

                // Fetch return loss (RLM-100 and BRM-100 only)
                if (m_model == MODEL_RLM_100 || m_model == MODEL_BRM_100 || m_model == MODEL_UNKNOWN)
                {
                    try
                    {
                        std::string rlResp = Query(SCPI::FETCH_RL);
                        if (!rlResp.empty())
                        {
                            std::vector<double> rlValues = ParseDoubleList(rlResp);
                            if (!rlValues.empty())
                                result.returnLoss = rlValues[0];
                        }
                    }
                    catch (...)
                    {
                        // ILM-100 does not support RL fetch
                        if (m_model == MODEL_UNKNOWN)
                            m_model = MODEL_ILM_100;
                    }
                }

                m_logger.Info("  Ch%d @%dnm: IL=%.4f dB, RL=%.4f dB",
                    result.channel, static_cast<int>(result.wavelength),
                    result.insertionLoss, result.returnLoss);
                results.push_back(result);
            }
        }

        m_logger.Info("Retrieved %d results.", (int)results.size());
    }
    catch (const std::exception& e)
    {
        m_logger.Error("GetResults exception: %s", e.what());
    }

    return results;
}

// ---------------------------------------------------------------------------
// Utility
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
