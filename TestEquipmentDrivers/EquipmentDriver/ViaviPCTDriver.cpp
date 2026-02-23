#include "stdafx.h"
#include "ViaviPCTDriver.h"
#include <stdexcept>
#include <sstream>
#include <cstdlib>

namespace EquipmentDriver {

// ---------------------------------------------------------------------------
// Construction / Destruction
// ---------------------------------------------------------------------------

CViaviPCTDriver::CViaviPCTDriver(const std::string& ipAddress,
                                 int slot,
                                 double timeout,
                                 int launchPort)
    : CBaseEquipmentDriver(ConnectionConfig(), std::string("VIAVI_MAP300_PCT"))
    , m_chassisSocket(INVALID_SOCKET)
    , m_ipAddress(ipAddress)
    , m_slot(slot)
    , m_launchPort(launchPort)
{
    m_config.ipAddress = ipAddress;
    m_config.port = PCT_BASE_PORT + slot;
    m_config.timeout = timeout;
    m_config.bufferSize = 1024;
    m_config.reconnectAttempts = 3;
    m_config.reconnectDelay = 2.0;
}

CViaviPCTDriver::~CViaviPCTDriver()
{
    Disconnect();
}

// ---------------------------------------------------------------------------
// Two-Level Connection
// ---------------------------------------------------------------------------

bool CViaviPCTDriver::Connect()
{
    // Step 1: Connect to Chassis (port 8100)
    m_logger.Info("Connecting to Chassis at %s:%d", m_ipAddress.c_str(), CHASSIS_PORT);

    m_chassisSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_chassisSocket == INVALID_SOCKET)
    {
        m_logger.Error("Failed to create chassis socket: %d", WSAGetLastError());
        return false;
    }

    DWORD timeoutMs = static_cast<DWORD>(m_config.timeout * 1000);
    setsockopt(m_chassisSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeoutMs, sizeof(timeoutMs));
    setsockopt(m_chassisSocket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeoutMs, sizeof(timeoutMs));

    sockaddr_in chassisAddr = {};
    chassisAddr.sin_family = AF_INET;
    chassisAddr.sin_port = htons(static_cast<u_short>(CHASSIS_PORT));
    inet_pton(AF_INET, m_ipAddress.c_str(), &chassisAddr.sin_addr);

    if (connect(m_chassisSocket, (sockaddr*)&chassisAddr, sizeof(chassisAddr)) == SOCKET_ERROR)
    {
        m_logger.Error("Chassis connection failed: %d", WSAGetLastError());
        closesocket(m_chassisSocket);
        m_chassisSocket = INVALID_SOCKET;
        return false;
    }
    m_logger.Info("Chassis connection established.");

    // Step 2: Launch Super Application
    m_logger.Info("Launching Super Application (PCT)...");
    SendChassisCommand("SUPER:LAUNCH PCT");
    m_logger.Info("Waiting %d ms for Super App to start...", SUPER_APP_WAIT_MS);
    Sleep(SUPER_APP_WAIT_MS);

    // Step 3: Connect to PCT Module via base class
    m_logger.Info("Connecting to PCT Module at slot %d (port %d)", m_slot, m_config.port);
    return CBaseEquipmentDriver::Connect();
}

void CViaviPCTDriver::Disconnect()
{
    CBaseEquipmentDriver::Disconnect();

    if (m_chassisSocket != INVALID_SOCKET)
    {
        shutdown(m_chassisSocket, SD_BOTH);
        closesocket(m_chassisSocket);
        m_chassisSocket = INVALID_SOCKET;
        m_logger.Info("Chassis connection closed.");
    }
}

std::string CViaviPCTDriver::SendChassisCommand(const std::string& command)
{
    if (m_chassisSocket == INVALID_SOCKET)
        throw std::runtime_error("Not connected to chassis.");

    std::string fullCmd = command + "\n";
    m_logger.Debug("TX (Chassis): %s", command.c_str());

    int sent = send(m_chassisSocket, fullCmd.c_str(), static_cast<int>(fullCmd.size()), 0);
    if (sent == SOCKET_ERROR)
        throw std::runtime_error("Failed to send chassis command.");

    if (command.find('?') != std::string::npos)
    {
        std::string data;
        char buf[1024];
        while (true)
        {
            int received = recv(m_chassisSocket, buf, sizeof(buf) - 1, 0);
            if (received <= 0) break;
            buf[received] = '\0';
            data.append(buf, received);
            if (data.find('\n') != std::string::npos) break;
        }
        while (!data.empty() && (data.back() == '\n' || data.back() == '\r'))
            data.pop_back();
        m_logger.Debug("RX (Chassis): %s", data.c_str());
        return data;
    }
    return std::string();
}

// ---------------------------------------------------------------------------
// Error Handling
// ---------------------------------------------------------------------------

ErrorInfo CViaviPCTDriver::CheckError()
{
    ErrorInfo info;
    try
    {
        std::string response = SendCommand("SYST:ERR?");
        if (!response.empty())
        {
            size_t commaPos = response.find(',');
            if (commaPos != std::string::npos)
            {
                info.code = atoi(response.substr(0, commaPos).c_str());
                info.message = response.substr(commaPos + 1);
                // Trim leading space
                if (!info.message.empty() && info.message[0] == ' ')
                    info.message = info.message.substr(1);
            }
            else
            {
                info.code = atoi(response.c_str());
            }
        }
    }
    catch (const std::exception& e)
    {
        m_logger.Warning("Error check failed: %s", e.what());
        info.code = -1;
        info.message = e.what();
    }
    return info;
}

// ---------------------------------------------------------------------------
// Device Info
// ---------------------------------------------------------------------------

DeviceInfo CViaviPCTDriver::GetDeviceInfo()
{
    DeviceInfo info;
    info.manufacturer = "VIAVI";
    info.model = "MAP300 PCT";
    info.slot = m_slot;
    return info;
}

// ---------------------------------------------------------------------------
// Initialization
// ---------------------------------------------------------------------------

bool CViaviPCTDriver::Initialize()
{
    m_logger.Info("Initializing VIAVI MAP300 PCT...");
    try
    {
        std::ostringstream cmd;
        cmd << "PATH:LAUNCH " << m_launchPort;
        SendCommand(cmd.str());
        AssertNoError();
        m_logger.Info("Launch port set to J%d.", m_launchPort);
        return true;
    }
    catch (const std::exception& e)
    {
        m_logger.Error("Initialization failed: %s", e.what());
        return false;
    }
}

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

void CViaviPCTDriver::ConfigureWavelengths(const std::vector<double>& wavelengths)
{
    m_wavelengths = wavelengths;

    std::ostringstream cmd;
    cmd << "SOURCE:LIST ";
    for (size_t i = 0; i < wavelengths.size(); ++i)
    {
        if (i > 0) cmd << ", ";
        cmd << static_cast<int>(wavelengths[i]);
    }
    SendCommand(cmd.str());
    AssertNoError();

    // Verify
    std::string response = SendCommand("SOURCE:LIST?");
    m_logger.Info("Wavelengths configured: %s nm", response.c_str());
}

void CViaviPCTDriver::ConfigureChannels(const std::vector<int>& channels)
{
    m_channels = channels;
    if (channels.empty()) return;

    std::string chStr = BuildChannelString(channels);
    std::ostringstream cmd;
    cmd << "PATH:LIST " << m_launchPort << "," << chStr;
    SendCommand(cmd.str());
    AssertNoError();
    m_logger.Info("Channels configured: %s", chStr.c_str());
}

void CViaviPCTDriver::ConfigureORL(const ORLConfig& config)
{
    std::ostringstream cmd;
    cmd << "MEAS:ORL:SETUP "
        << config.channel << ","
        << static_cast<int>(config.method) << ","
        << static_cast<int>(config.origin) << ","
        << config.aOffset << ","
        << config.bOffset;
    SendCommand(cmd.str());
    AssertNoError();
    m_logger.Info("ORL configured: method=%d, origin=%d, A=%.2fm, B=%.2fm",
        static_cast<int>(config.method), static_cast<int>(config.origin),
        config.aOffset, config.bOffset);
}

// ---------------------------------------------------------------------------
// Reference Measurement
// ---------------------------------------------------------------------------

bool CViaviPCTDriver::TakeReference(const ReferenceConfig& config)
{
    if (config.useOverride)
        return OverrideReference(config.ilValue, config.lengthValue);
    else
        return AutoReference();
}

bool CViaviPCTDriver::OverrideReference(double ilValue, double lengthValue)
{
    m_logger.Info("Overriding reference: IL=%.2f dB, Length=%.1f m", ilValue, lengthValue);

    for (size_t i = 0; i < m_channels.size(); ++i)
    {
        int ch = m_channels[i];
        std::ostringstream cmd;

        // Set IL override
        cmd.str(""); cmd.clear();
        cmd << "PATH:JUMPER:IL " << m_launchPort << "," << ch << "," << ilValue;
        SendCommand(cmd.str());
        AssertNoError();

        // Activate IL override (0 = use override)
        cmd.str(""); cmd.clear();
        cmd << "PATH:JUMPER:IL:AUTO " << m_launchPort << "," << ch << ",0";
        SendCommand(cmd.str());
        AssertNoError();

        // Set length override
        cmd.str(""); cmd.clear();
        cmd << "PATH:JUMPER:LENGTH " << m_launchPort << "," << ch << "," << lengthValue;
        SendCommand(cmd.str());
        AssertNoError();

        // Activate length override (0 = use override)
        cmd.str(""); cmd.clear();
        cmd << "PATH:JUMPER:LENGTH:AUTO " << m_launchPort << "," << ch << ",0";
        SendCommand(cmd.str());
        AssertNoError();
    }

    m_logger.Info("Reference override applied for %d channels.", (int)m_channels.size());
    return true;
}

bool CViaviPCTDriver::AutoReference()
{
    m_logger.Info("Starting automatic reference measurement...");

    // Deactivate all overrides (1 = auto/deactivate override)
    for (size_t i = 0; i < m_channels.size(); ++i)
    {
        int ch = m_channels[i];
        std::ostringstream cmd;

        cmd.str(""); cmd.clear();
        cmd << "PATH:JUMPER:IL:AUTO " << m_launchPort << "," << ch << ",1";
        SendCommand(cmd.str());
        AssertNoError();

        cmd.str(""); cmd.clear();
        cmd << "PATH:JUMPER:LENGTH:AUTO " << m_launchPort << "," << ch << ",1";
        SendCommand(cmd.str());
        AssertNoError();
    }

    // Switch to reference mode
    SendCommand("SENS:FUNC 0");
    AssertNoError();

    // Start reference measurement
    SendCommand("MEAS:START");
    return WaitForCompletion("Reference");
}

// ---------------------------------------------------------------------------
// IL/RL Measurement
// ---------------------------------------------------------------------------

bool CViaviPCTDriver::TakeMeasurement()
{
    m_logger.Info("Starting IL/RL measurement...");

    // Switch to measurement mode
    SendCommand("SENS:FUNC 1");
    AssertNoError();

    // Start measurement
    SendCommand("MEAS:START");
    return WaitForCompletion("Measurement");
}

bool CViaviPCTDriver::WaitForCompletion(const std::string& operationName)
{
    DWORD startTick = GetTickCount();

    while (true)
    {
        std::string state = SendCommand("MEAS:STATE?");

        if (state == "1")   // Complete
        {
            m_logger.Info("%s completed successfully.", operationName.c_str());
            AssertNoError();
            return true;
        }
        else if (state == "3")  // Error
        {
            m_logger.Error("%s encountered an error.", operationName.c_str());
            ErrorInfo err = CheckError();
            m_logger.Error("Error [%d]: %s", err.code, err.message.c_str());
            return false;
        }
        else if (state == "2")  // Running
        {
            DWORD elapsed = GetTickCount() - startTick;
            if (elapsed > static_cast<DWORD>(MAX_POLL_TIME_MS))
            {
                m_logger.Error("%s timed out after %d ms.", operationName.c_str(), elapsed);
                return false;
            }
            Sleep(POLL_INTERVAL_MS);
        }
        else
        {
            m_logger.Warning("Unexpected MEAS:STATE? response: %s", state.c_str());
            Sleep(POLL_INTERVAL_MS);
        }
    }
}

// ---------------------------------------------------------------------------
// Results Retrieval
// ---------------------------------------------------------------------------

std::vector<MeasurementResult> CViaviPCTDriver::GetResults()
{
    std::vector<MeasurementResult> results;

    // Re-query wavelengths for accurate mapping
    std::string wlResponse = SendCommand("SOURCE:LIST?");
    std::vector<double> wavelengths = ParseDoubleList(wlResponse);
    if (wavelengths.empty())
        wavelengths = m_wavelengths;

    for (size_t ci = 0; ci < m_channels.size(); ++ci)
    {
        int ch = m_channels[ci];

        std::ostringstream cmd;
        cmd << "MEAS:ALL? " << ch << "," << m_launchPort;
        std::string response = SendCommand(cmd.str());

        std::vector<double> rawValues = ParseDoubleList(response);

        for (size_t wi = 0; wi < wavelengths.size(); ++wi)
        {
            // Each wavelength produces 5 values
            size_t startIdx = wi * 5;
            size_t endIdx = startIdx + 5;

            MeasurementResult result;
            result.channel = ch;
            result.wavelength = wavelengths[wi];
            result.rawDataCount = 0;

            if (endIdx <= rawValues.size())
            {
                // Index 3 = RL (confirmed from protocol analysis)
                result.returnLoss = rawValues[startIdx + 3];
                // Index 0 used tentatively for IL
                result.insertionLoss = rawValues[startIdx];

                int count = 0;
                for (size_t k = startIdx; k < endIdx && count < 10; ++k, ++count)
                    result.rawData[count] = rawValues[k];
                result.rawDataCount = count;
            }
            else
            {
                m_logger.Warning("Incomplete data for ch=%d, wl=%.0f", ch, wavelengths[wi]);
            }

            results.push_back(result);
        }

        AssertNoError();
    }

    m_logger.Info("Retrieved %d measurement results.", (int)results.size());
    return results;
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

std::string CViaviPCTDriver::BuildChannelString(const std::vector<int>& channels)
{
    if (channels.empty()) return "";

    std::vector<int> sorted(channels);
    std::sort(sorted.begin(), sorted.end());

    // Check contiguous range
    bool contiguous = true;
    for (size_t i = 1; i < sorted.size(); ++i)
    {
        if (sorted[i] != sorted[i - 1] + 1)
        {
            contiguous = false;
            break;
        }
    }

    if (contiguous && sorted.size() > 1)
    {
        std::ostringstream oss;
        oss << sorted.front() << "-" << sorted.back();
        return oss.str();
    }

    std::ostringstream oss;
    for (size_t i = 0; i < sorted.size(); ++i)
    {
        if (i > 0) oss << ",";
        oss << sorted[i];
    }
    return oss.str();
}

std::vector<double> CViaviPCTDriver::ParseDoubleList(const std::string& csv)
{
    std::vector<double> values;
    std::istringstream iss(csv);
    std::string token;
    while (std::getline(iss, token, ','))
    {
        // Trim whitespace
        size_t start = token.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) continue;
        token = token.substr(start);
        size_t end = token.find_last_not_of(" \t\r\n");
        if (end != std::string::npos)
            token = token.substr(0, end + 1);

        if (!token.empty())
            values.push_back(atof(token.c_str()));
    }
    return values;
}

} // namespace EquipmentDriver
