#include "stdafx.h"
#include "CViaviPCT4AllDriver.h"
#include "ViaviPCT4AllTcpAdapter.h"
#include "ViaviPCT4AllVisaAdapter.h"
#include "PCT4AllScpiCommands.h"
#include <stdexcept>
#include <sstream>
#include <cmath>
#include <chrono>
#include <thread>

namespace ViaviPCT4All {

// ===========================================================================
// Construction / Destruction
// ===========================================================================

CViaviPCT4AllDriver::CViaviPCT4AllDriver(const std::string& address,
                                         int port, double timeout,
                                         CommType commType)
    : m_commType(commType)
    , m_adapter(nullptr)
    , m_ownsAdapter(true)
    , m_state(STATE_DISCONNECTED)
    , m_logger("ViaviPCT4All")
{
    m_config.address = address;
    m_config.port = (port > 0) ? port : DEFAULT_PORT;
    m_config.timeout = timeout;
}

CViaviPCT4AllDriver::~CViaviPCT4AllDriver()
{
    try { Disconnect(); } catch (...) {}
}

void CViaviPCT4AllDriver::SetCommAdapter(IViaviPCT4AllCommAdapter* adapter, bool takeOwnership)
{
    if (m_adapter && m_ownsAdapter)
    {
        m_adapter->Close();
        delete m_adapter;
    }
    m_adapter = adapter;
    m_ownsAdapter = takeOwnership;
}

// ===========================================================================
// Internal helpers
// ===========================================================================

std::string CViaviPCT4AllDriver::Query(const std::string& command)
{
    ValidateConnection();
    m_logger.Debug("Query: %s", command.c_str());
    std::string resp = m_adapter->SendQuery(command);
    m_logger.Debug("Response: %s", resp.c_str());
    return resp;
}

std::string CViaviPCT4AllDriver::QueryLong(const std::string& command)
{
    ValidateConnection();
    m_logger.Debug("QueryLong: %s", command.c_str());

    auto* tcpAdapter = dynamic_cast<CViaviPCT4AllTcpAdapter*>(m_adapter);
    auto* visaAdapter = dynamic_cast<CViaviPCT4AllVisaAdapter*>(m_adapter);
    if (tcpAdapter) tcpAdapter->SetReadTimeout(MEAS_TIMEOUT_MS);
    if (visaAdapter) visaAdapter->SetReadTimeout(MEAS_TIMEOUT_MS);

    std::string resp = m_adapter->SendQuery(command);

    if (tcpAdapter) tcpAdapter->SetReadTimeout(static_cast<DWORD>(m_config.timeout * 1000));
    if (visaAdapter) visaAdapter->SetReadTimeout(static_cast<DWORD>(m_config.timeout * 1000));

    m_logger.Debug("Response: %s", resp.c_str());
    return resp;
}

void CViaviPCT4AllDriver::Write(const std::string& command)
{
    ValidateConnection();
    m_logger.Debug("Write: %s", command.c_str());
    m_adapter->SendWrite(command);
}

void CViaviPCT4AllDriver::ValidateConnection()
{
    if (!m_adapter || !m_adapter->IsOpen())
        throw std::runtime_error("Not connected to Viavi MAP PCT module.");
}

std::string CViaviPCT4AllDriver::Trim(const std::string& s)
{
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

int CViaviPCT4AllDriver::SafeAtoi(const std::string& s)
{
    try { return std::stoi(Trim(s)); }
    catch (...) { return 0; }
}

double CViaviPCT4AllDriver::SafeAtof(const std::string& s)
{
    try { return std::stod(Trim(s)); }
    catch (...) { return 0.0; }
}

ErrorInfo CViaviPCT4AllDriver::ParseError(const std::string& response)
{
    ErrorInfo err;
    size_t pos = response.find(',');
    if (pos != std::string::npos)
    {
        err.code = SafeAtoi(response.substr(0, pos));
        std::string msg = response.substr(pos + 1);
        size_t q1 = msg.find('"');
        size_t q2 = msg.rfind('"');
        if (q1 != std::string::npos && q2 != std::string::npos && q2 > q1)
            err.message = msg.substr(q1 + 1, q2 - q1 - 1);
        else
            err.message = Trim(msg);
    }
    else
    {
        err.code = SafeAtoi(response);
        err.message = (err.code == 0) ? "No Error" : "Unknown";
    }
    return err;
}

PCTConfig CViaviPCT4AllDriver::ParseConfig(const std::string& response)
{
    PCTConfig cfg;
    std::istringstream iss(response);
    std::string token;
    std::vector<std::string> tokens;
    while (iss >> token) tokens.push_back(token);

    if (tokens.size() >= 2) { cfg.deviceNum = SafeAtoi(tokens[0]); cfg.deviceType = tokens[1]; }
    if (tokens.size() >= 3) cfg.fiberType = tokens[2];
    if (tokens.size() >= 4) cfg.outputs = SafeAtoi(tokens[3]);
    if (tokens.size() >= 5) cfg.sw1Channels = SafeAtoi(tokens[4]);
    if (tokens.size() >= 6) cfg.sw2Channels = SafeAtoi(tokens[5]);

    for (size_t i = 6; i < tokens.size() && i < 12; ++i)
    {
        int wl = SafeAtoi(tokens[i]);
        if (wl > 0) cfg.wavelengths.push_back(wl);
    }

    if (tokens.size() >= 13) cfg.coreType = SafeAtoi(tokens[12]);
    if (tokens.size() >= 14) cfg.opmCount = SafeAtoi(tokens[13]);

    return cfg;
}

CassetteInfo CViaviPCT4AllDriver::ParseCassetteInfo(const std::string& response)
{
    CassetteInfo info;
    std::vector<std::string> parts;
    std::istringstream iss(response);
    std::string token;
    while (std::getline(iss, token, ',')) parts.push_back(Trim(token));

    if (parts.size() >= 1) info.serialNumber = parts[0];
    if (parts.size() >= 2) info.partNumber = parts[1];
    if (parts.size() >= 3) info.firmwareVersion = parts[2];
    if (parts.size() >= 4) info.moduleVersion1 = parts[3];
    if (parts.size() >= 5) info.moduleVersion2 = parts[4];
    if (parts.size() >= 6) info.hardwareVersion = parts[5];
    if (parts.size() >= 7) info.assemblyDate = parts[6];
    if (parts.size() >= 8) info.description = parts[7];

    return info;
}

IdentificationInfo CViaviPCT4AllDriver::ParseIDN(const std::string& response)
{
    IdentificationInfo info;
    std::vector<std::string> parts;
    std::istringstream iss(response);
    std::string token;
    while (std::getline(iss, token, ',')) parts.push_back(Trim(token));

    if (parts.size() >= 1) info.manufacturer = parts[0];
    if (parts.size() >= 2) info.platform = parts[1];
    if (parts.size() >= 3) info.serialNumber = parts[2];
    if (parts.size() >= 4) info.firmwareVersion = parts[3];

    return info;
}

SystemInfo CViaviPCT4AllDriver::ParseSystemInfo(const std::string& response)
{
    SystemInfo info;
    info.raw = response;
    std::vector<std::string> parts;
    std::istringstream iss(response);
    std::string token;
    while (std::getline(iss, token, ',')) parts.push_back(Trim(token));

    if (parts.size() >= 1) info.mainframePart = parts[0];
    if (parts.size() >= 2) info.mainframeSerial = parts[1];
    if (parts.size() >= 3) info.mainframeHW = parts[2];
    if (parts.size() >= 4) info.mainframeDate = parts[3];
    if (parts.size() >= 5) info.controllerPart = parts[4];
    if (parts.size() >= 6) info.controllerSerial = parts[5];
    if (parts.size() >= 7) info.controllerHW = parts[6];
    if (parts.size() >= 8) info.controllerDate = parts[7];

    return info;
}

// ===========================================================================
// Connection lifecycle
// ===========================================================================

bool CViaviPCT4AllDriver::Connect()
{
    if (m_adapter && m_adapter->IsOpen())
        return true;

    m_state = STATE_CONNECTING;
    m_logger.Info("Connecting to %s:%d (type=%d)",
                  m_config.address.c_str(), m_config.port, m_commType);

    for (int attempt = 0; attempt <= m_config.reconnectAttempts; ++attempt)
    {
        try
        {
            if (!m_adapter)
            {
                if (m_commType == COMM_VISA)
                    m_adapter = new CViaviPCT4AllVisaAdapter();
                else
                    m_adapter = new CViaviPCT4AllTcpAdapter();
                m_ownsAdapter = true;
            }

            if (m_adapter->Open(m_config.address, m_config.port, m_config.timeout))
            {
                m_state = STATE_CONNECTED;
                m_logger.Info("Connected successfully.");
                return true;
            }
        }
        catch (const std::exception& ex)
        {
            m_logger.Warning("Connection attempt %d failed: %s", attempt + 1, ex.what());
        }

        if (attempt < m_config.reconnectAttempts)
        {
            m_logger.Info("Retrying in %.1f seconds...", m_config.reconnectDelay);
            std::this_thread::sleep_for(
                std::chrono::milliseconds(static_cast<int>(m_config.reconnectDelay * 1000)));
        }
    }

    m_state = STATE_ERROR;
    m_logger.Error("Failed to connect after %d attempts.", m_config.reconnectAttempts + 1);
    return false;
}

void CViaviPCT4AllDriver::Disconnect()
{
    if (m_adapter)
    {
        try
        {
            if (m_adapter->IsOpen())
                m_adapter->SendWrite(SCPI::Common::CLS);
        }
        catch (...) {}

        m_adapter->Close();
        if (m_ownsAdapter) { delete m_adapter; m_adapter = nullptr; }
    }
    m_state = STATE_DISCONNECTED;
    m_logger.Info("Disconnected.");
}

bool CViaviPCT4AllDriver::Reconnect()
{
    Disconnect();
    return Connect();
}

bool CViaviPCT4AllDriver::IsConnected() const
{
    return m_adapter && m_adapter->IsOpen() && m_state == STATE_CONNECTED;
}

bool CViaviPCT4AllDriver::Initialize()
{
    if (!IsConnected()) return false;

    try
    {
        Write(SCPI::PCT::REM);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        std::string confResp = Query(SCPI::Cassette::CONF_Q);
        if (confResp.empty())
        {
            m_logger.Error("Initialize: :CONF? returned empty.");
            return false;
        }

        m_logger.Info("Configuration: %s", confResp.c_str());
        return true;
    }
    catch (const std::exception& ex)
    {
        m_logger.Error("Initialize failed: %s", ex.what());
        return false;
    }
}

// ===========================================================================
// Appendix A: Common SCPI Commands
// ===========================================================================

void CViaviPCT4AllDriver::ClearStatus()
{
    Write(SCPI::Common::CLS);
}

void CViaviPCT4AllDriver::SetESE(int value)
{
    Write(std::string(SCPI::Common::ESE) + " " + std::to_string(value));
}

int CViaviPCT4AllDriver::GetESE()
{
    return SafeAtoi(Query(SCPI::Common::ESE_Q));
}

int CViaviPCT4AllDriver::GetESR()
{
    return SafeAtoi(Query(SCPI::Common::ESR_Q));
}

IdentificationInfo CViaviPCT4AllDriver::GetIdentification()
{
    return ParseIDN(Query(SCPI::Common::IDN_Q));
}

void CViaviPCT4AllDriver::OperationComplete()
{
    Write(SCPI::Common::OPC);
}

int CViaviPCT4AllDriver::QueryOperationComplete()
{
    return SafeAtoi(Query(SCPI::Common::OPC_Q));
}

void CViaviPCT4AllDriver::RecallState(int stateNum)
{
    Write(std::string(SCPI::Common::RCL) + " " + std::to_string(stateNum));
}

void CViaviPCT4AllDriver::ResetDevice()
{
    Write(SCPI::Common::RST);
}

void CViaviPCT4AllDriver::SaveState(int stateNum)
{
    Write(std::string(SCPI::Common::SAV) + " " + std::to_string(stateNum));
}

void CViaviPCT4AllDriver::SetSRE(int value)
{
    Write(std::string(SCPI::Common::SRE) + " " + std::to_string(value));
}

int CViaviPCT4AllDriver::GetSRE()
{
    return SafeAtoi(Query(SCPI::Common::SRE_Q));
}

int CViaviPCT4AllDriver::GetSTB()
{
    return SafeAtoi(Query(SCPI::Common::STB_Q));
}

int CViaviPCT4AllDriver::SelfTest()
{
    return SafeAtoi(QueryLong(SCPI::Common::TST_Q));
}

void CViaviPCT4AllDriver::Wait()
{
    Write(SCPI::Common::WAI);
}

// ===========================================================================
// Chapter 2: Common Cassette Commands
// ===========================================================================

int CViaviPCT4AllDriver::IsBusy()
{
    return SafeAtoi(Query(SCPI::Cassette::BUSY_Q));
}

std::string CViaviPCT4AllDriver::GetConfig()
{
    return Query(SCPI::Cassette::CONF_Q);
}

PCTConfig CViaviPCT4AllDriver::GetConfigParsed()
{
    return ParseConfig(Query(SCPI::Cassette::CONF_Q));
}

std::string CViaviPCT4AllDriver::GetDeviceInformation(int device)
{
    return Query(std::string(SCPI::Cassette::DEV_INFO_Q) + " " + std::to_string(device));
}

int CViaviPCT4AllDriver::GetDeviceFault(int device)
{
    return SafeAtoi(Query(std::string(SCPI::Cassette::FAULT_DEV_Q)
                          + " " + std::to_string(device)));
}

int CViaviPCT4AllDriver::GetSlotFault()
{
    return SafeAtoi(Query(SCPI::Cassette::FAULT_SLOT_Q));
}

CassetteInfo CViaviPCT4AllDriver::GetCassetteInfo()
{
    return ParseCassetteInfo(Query(SCPI::Cassette::INFO_Q));
}

void CViaviPCT4AllDriver::SetLock(int state, const std::string& name, const std::string& ipID)
{
    Write(std::string(SCPI::Cassette::LOCK) + " "
          + std::to_string(state) + "," + name + "," + ipID);
}

std::string CViaviPCT4AllDriver::GetLock()
{
    return Query(SCPI::Cassette::LOCK_Q);
}

void CViaviPCT4AllDriver::ResetCassette()
{
    Write(SCPI::Cassette::RESET);
}

int CViaviPCT4AllDriver::GetDeviceStatus(int device)
{
    return SafeAtoi(Query(std::string(SCPI::Cassette::STATUS_DEV_Q)
                          + " " + std::to_string(device)));
}

void CViaviPCT4AllDriver::ResetDeviceStatus(int device)
{
    Write(std::string(SCPI::Cassette::STATUS_RESET) + " " + std::to_string(device));
}

ErrorInfo CViaviPCT4AllDriver::GetSystemError()
{
    return ParseError(Query(SCPI::Cassette::SYS_ERR_Q));
}

int CViaviPCT4AllDriver::RunCassetteTest()
{
    return SafeAtoi(QueryLong(SCPI::Cassette::TEST_Q));
}

// ===========================================================================
// Chapter 3: System Commands
// ===========================================================================

void CViaviPCT4AllDriver::SuperExit(const std::string& name)
{
    Write(std::string(SCPI::System::SUPER_EXIT) + " " + name);
}

void CViaviPCT4AllDriver::SuperLaunch(const std::string& name)
{
    Write(std::string(SCPI::System::SUPER_LAUNCH) + " " + name);
}

int CViaviPCT4AllDriver::GetSuperStatus(const std::string& name)
{
    return SafeAtoi(Query(std::string(SCPI::System::SUPER_STATUS_Q) + " " + name));
}

void CViaviPCT4AllDriver::SetSystemDate(const std::string& date)
{
    Write(std::string(SCPI::System::SYS_DATE) + " " + date);
}

std::string CViaviPCT4AllDriver::GetSystemDate()
{
    return Query(SCPI::System::SYS_DATE_Q);
}

ErrorInfo CViaviPCT4AllDriver::GetSystemErrorSys()
{
    return ParseError(Query(SCPI::System::SYS_ERR_Q));
}

int CViaviPCT4AllDriver::GetChassisFault()
{
    return SafeAtoi(Query(SCPI::System::SYS_FAULT_CHASSIS_Q));
}

int CViaviPCT4AllDriver::GetFaultSummary()
{
    return SafeAtoi(Query(SCPI::System::SYS_FAULT_SUMMARY_Q));
}

void CViaviPCT4AllDriver::SetGPIBAddress(int address)
{
    Write(std::string(SCPI::System::SYS_GPIB) + " " + std::to_string(address));
}

int CViaviPCT4AllDriver::GetGPIBAddress()
{
    return SafeAtoi(Query(SCPI::System::SYS_GPIB_Q));
}

std::string CViaviPCT4AllDriver::GetSystemInfoRaw()
{
    return Query(SCPI::System::SYS_INFO_Q);
}

SystemInfo CViaviPCT4AllDriver::GetSystemInfo()
{
    return ParseSystemInfo(Query(SCPI::System::SYS_INFO_Q));
}

void CViaviPCT4AllDriver::SetInterlock(int state)
{
    Write(std::string(SCPI::System::SYS_INTLOCK) + " " + std::to_string(state));
}

int CViaviPCT4AllDriver::GetInterlock()
{
    return SafeAtoi(Query(SCPI::System::SYS_INTLOCK_Q));
}

std::string CViaviPCT4AllDriver::GetInterlockState()
{
    return Query(SCPI::System::SYS_INTLOCK_STATE_Q);
}

std::string CViaviPCT4AllDriver::GetInventory()
{
    return Query(SCPI::System::SYS_INVENTORY_Q);
}

std::string CViaviPCT4AllDriver::GetIPList()
{
    return Query(SCPI::System::SYS_IP_LIST_Q);
}

std::string CViaviPCT4AllDriver::GetLayout()
{
    return Query(SCPI::System::SYS_LAYOUT_Q);
}

std::string CViaviPCT4AllDriver::GetLayoutPort()
{
    return Query(SCPI::System::SYS_LAYOUT_PORT_Q);
}

void CViaviPCT4AllDriver::SetLegacyMode(int state)
{
    Write(std::string(SCPI::System::SYS_LEGACY_MODE) + " " + std::to_string(state));
}

int CViaviPCT4AllDriver::GetLegacyMode()
{
    return SafeAtoi(Query(SCPI::System::SYS_LEGACY_MODE_Q));
}

std::string CViaviPCT4AllDriver::GetLicenses()
{
    return Query(SCPI::System::SYS_LICENSE_Q);
}

void CViaviPCT4AllDriver::ReleaseLock(const std::string& slotID)
{
    Write(std::string(SCPI::System::SYS_LOCK_RELEASE) + " " + slotID);
}

void CViaviPCT4AllDriver::SystemShutdown(int mode)
{
    Write(std::string(SCPI::System::SYS_SHUTDOWN) + " " + std::to_string(mode));
}

int CViaviPCT4AllDriver::GetSystemStatusReg()
{
    return SafeAtoi(Query(SCPI::System::SYS_STATUS_Q));
}

std::string CViaviPCT4AllDriver::GetTemperature()
{
    return Query(SCPI::System::SYS_TEMP_Q);
}

std::string CViaviPCT4AllDriver::GetSystemTime()
{
    return Query(SCPI::System::SYS_TIME_Q);
}

// ===========================================================================
// Chapter 4: Factory / System Integration Commands
// ===========================================================================

std::string CViaviPCT4AllDriver::GetCalibrationStatus()
{
    return Query(SCPI::Factory::CAL_Q);
}

std::string CViaviPCT4AllDriver::GetCalibrationDate()
{
    return Query(SCPI::Factory::CAL_DATE_Q);
}

void CViaviPCT4AllDriver::FactoryCommit(int step)
{
    Write(std::string(SCPI::Factory::COMMIT) + " " + std::to_string(step));
}

void CViaviPCT4AllDriver::SetFactoryBiDir(int status)
{
    Write(std::string(SCPI::Factory::CONF_BIDIR) + " " + std::to_string(status));
}

int CViaviPCT4AllDriver::GetFactoryBiDir()
{
    return SafeAtoi(Query(SCPI::Factory::CONF_BIDIR_Q));
}

void CViaviPCT4AllDriver::SetFactoryCore(int core)
{
    Write(std::string(SCPI::Factory::CONF_CORE) + " " + std::to_string(core));
}

int CViaviPCT4AllDriver::GetFactoryCore()
{
    return SafeAtoi(Query(SCPI::Factory::CONF_CORE_Q));
}

void CViaviPCT4AllDriver::SetFactoryLowPower(int status)
{
    Write(std::string(SCPI::Factory::CONF_LPOW) + " " + std::to_string(status));
}

int CViaviPCT4AllDriver::GetFactoryLowPower()
{
    return SafeAtoi(Query(SCPI::Factory::CONF_LPOW_Q));
}

void CViaviPCT4AllDriver::SetFactoryOPM(const std::string& status)
{
    Write(std::string(SCPI::Factory::CONF_OPM) + " " + status);
}

std::string CViaviPCT4AllDriver::GetFactoryOPM()
{
    return Query(SCPI::Factory::CONF_OPM_Q);
}

void CViaviPCT4AllDriver::SetFactorySwitch(int sw, const std::string& status)
{
    Write(std::string(SCPI::Factory::CONF_SWITCH) + " "
          + std::to_string(sw) + "," + status);
}

std::string CViaviPCT4AllDriver::GetFactorySwitch(int sw)
{
    return Query(std::string(SCPI::Factory::CONF_SWITCH_Q) + " " + std::to_string(sw));
}

void CViaviPCT4AllDriver::SetFactorySwitchSize(int sw, int size)
{
    Write(std::string(SCPI::Factory::CONF_SWITCH_SIZE) + " "
          + std::to_string(sw) + "," + std::to_string(size));
}

std::string CViaviPCT4AllDriver::GetFactoryFPDistance(int ilf)
{
    return Query(std::string(SCPI::Factory::MEAS_FP_DIST_Q) + " " + std::to_string(ilf));
}

std::string CViaviPCT4AllDriver::GetFactoryFPLoss(int ilf)
{
    return Query(std::string(SCPI::Factory::MEAS_FP_LOSS_Q) + " " + std::to_string(ilf));
}

std::string CViaviPCT4AllDriver::GetFactoryFPRatio(int ilf)
{
    return Query(std::string(SCPI::Factory::MEAS_FP_RATIO_Q) + " " + std::to_string(ilf));
}

std::string CViaviPCT4AllDriver::GetFactoryLoop(int ilf)
{
    return Query(std::string(SCPI::Factory::MEAS_LOOP_Q) + " " + std::to_string(ilf));
}

std::string CViaviPCT4AllDriver::GetFactoryOPMP(int index)
{
    return Query(std::string(SCPI::Factory::MEAS_OPMP_Q) + " " + std::to_string(index));
}

std::string CViaviPCT4AllDriver::GetFactoryRange(int fiber)
{
    return Query(std::string(SCPI::Factory::MEAS_RANGE_Q) + " " + std::to_string(fiber));
}

void CViaviPCT4AllDriver::StartFactoryMeasure(int step)
{
    if (step >= 0)
        Write(std::string(SCPI::Factory::MEAS_START) + " " + std::to_string(step));
    else
        Write(SCPI::Factory::MEAS_START);
}

std::string CViaviPCT4AllDriver::GetFactorySWDistance(int channel, int sw)
{
    return Query(std::string(SCPI::Factory::MEAS_SW_DIST_Q) + " "
                 + std::to_string(channel) + "," + std::to_string(sw));
}

std::string CViaviPCT4AllDriver::GetFactorySWLoss(int channel, int sw)
{
    return Query(std::string(SCPI::Factory::MEAS_SW_LOSS_Q) + " "
                 + std::to_string(channel) + "," + std::to_string(sw));
}

void CViaviPCT4AllDriver::SetFactorySetupSwitch(int sw)
{
    Write(std::string(SCPI::Factory::SETUP_SWITCH) + " " + std::to_string(sw));
}

int CViaviPCT4AllDriver::GetFactorySetupSwitch()
{
    return SafeAtoi(Query(SCPI::Factory::SETUP_SWITCH_Q));
}

void CViaviPCT4AllDriver::FactoryReset(int step)
{
    if (step >= 0)
        Write(std::string(SCPI::Factory::RESET) + " " + std::to_string(step));
    else
        Write(SCPI::Factory::RESET);
}

// ===========================================================================
// Chapter 5: PCT - Active / Remote
// ===========================================================================

int CViaviPCT4AllDriver::GetActive()
{
    return SafeAtoi(Query(SCPI::PCT::ACTIVE_Q));
}

void CViaviPCT4AllDriver::SetRemote()
{
    Write(SCPI::PCT::REM);
}

// ===========================================================================
// Chapter 5: PCT - Measurement Commands
// ===========================================================================

std::string CViaviPCT4AllDriver::GetMeasureAll(int launchCh, int recvCh)
{
    std::string cmd = std::string(SCPI::PCT::MEAS_ALL_Q) + " " + std::to_string(launchCh);
    if (recvCh > 0)
        cmd += "," + std::to_string(recvCh);
    return Query(cmd);
}

std::string CViaviPCT4AllDriver::GetDistance()
{
    return Query(SCPI::PCT::MEAS_DIST_Q);
}

void CViaviPCT4AllDriver::StartFastIL(int wavelength, double aTime,
                                       double delay, int startCh, int endCh)
{
    std::ostringstream oss;
    oss << SCPI::PCT::MEAS_FASTIL << " " << wavelength
        << "," << aTime << "," << delay
        << "," << startCh << "," << endCh;
    Write(oss.str());
}

std::string CViaviPCT4AllDriver::GetFastIL()
{
    return QueryLong(SCPI::PCT::MEAS_FASTIL_Q);
}

void CViaviPCT4AllDriver::SetHelixFactor(double value)
{
    std::ostringstream oss;
    oss << SCPI::PCT::MEAS_HELIX << " " << value;
    Write(oss.str());
}

double CViaviPCT4AllDriver::GetHelixFactor()
{
    return SafeAtof(Query(SCPI::PCT::MEAS_HELIX_Q));
}

std::string CViaviPCT4AllDriver::GetIL()
{
    return Query(SCPI::PCT::MEAS_IL_Q);
}

void CViaviPCT4AllDriver::SetILLA(int state)
{
    Write(std::string(SCPI::PCT::MEAS_ILLA) + " " + std::to_string(state));
}

int CViaviPCT4AllDriver::GetILLA()
{
    return SafeAtoi(Query(SCPI::PCT::MEAS_ILLA_Q));
}

std::string CViaviPCT4AllDriver::GetLength()
{
    return Query(SCPI::PCT::MEAS_LENGTH_Q);
}

std::string CViaviPCT4AllDriver::GetORL(int method, int origin,
                                         double aOffset, double bOffset)
{
    std::ostringstream oss;
    oss << SCPI::PCT::MEAS_ORL_Q << " " << method
        << "," << origin << "," << aOffset << "," << bOffset;
    return Query(oss.str());
}

std::string CViaviPCT4AllDriver::GetORLPreset(int origin)
{
    return Query(std::string(SCPI::PCT::MEAS_ORL_PRESET_Q) + " " + std::to_string(origin));
}

void CViaviPCT4AllDriver::SetORLSetupPreset(int zone, int preset)
{
    Write(std::string(SCPI::PCT::MEAS_ORL_SETUP) + " "
          + std::to_string(zone) + "," + std::to_string(preset));
}

void CViaviPCT4AllDriver::SetORLSetupCustom(int zone, int method, int origin,
                                             double aOff, double bOff)
{
    std::ostringstream oss;
    oss << SCPI::PCT::MEAS_ORL_SETUP << " " << zone
        << "," << method << "," << origin << "," << aOff << "," << bOff;
    Write(oss.str());
}

std::string CViaviPCT4AllDriver::GetORLSetup(int zone)
{
    return Query(std::string(SCPI::PCT::MEAS_ORL_SETUP_Q) + " " + std::to_string(zone));
}

std::string CViaviPCT4AllDriver::GetORLZone(int zone)
{
    return Query(std::string(SCPI::PCT::MEAS_ORL_ZONE_Q) + " " + std::to_string(zone));
}

std::string CViaviPCT4AllDriver::GetPower()
{
    return Query(SCPI::PCT::MEAS_POWER_Q);
}

void CViaviPCT4AllDriver::SetRef2Step(int state)
{
    Write(std::string(SCPI::PCT::MEAS_REF2STEP) + " " + std::to_string(state));
}

int CViaviPCT4AllDriver::GetRef2Step()
{
    return SafeAtoi(Query(SCPI::PCT::MEAS_REF2STEP_Q));
}

void CViaviPCT4AllDriver::SetRefAlt(int state)
{
    Write(std::string(SCPI::PCT::MEAS_REFALT) + " " + std::to_string(state));
}

int CViaviPCT4AllDriver::GetRefAlt()
{
    return SafeAtoi(Query(SCPI::PCT::MEAS_REFALT_Q));
}

void CViaviPCT4AllDriver::MeasureReset()
{
    Write(SCPI::PCT::MEAS_RESET);
}

void CViaviPCT4AllDriver::StartSEIL(int wavelength, int launchCh, int recvCh,
                                      int aTime, double binWidth, int adjustIL)
{
    std::ostringstream oss;
    oss << SCPI::PCT::MEAS_SEIL << " " << wavelength
        << "," << launchCh << "," << recvCh
        << "," << aTime << "," << binWidth << "," << adjustIL;
    Write(oss.str());
}

std::string CViaviPCT4AllDriver::GetSEIL()
{
    return QueryLong(SCPI::PCT::MEAS_SEIL_Q);
}

void CViaviPCT4AllDriver::StartMeasurement()
{
    Write(SCPI::PCT::MEAS_START);
}

int CViaviPCT4AllDriver::GetMeasurementState()
{
    return SafeAtoi(Query(SCPI::PCT::MEAS_STATE_Q));
}

void CViaviPCT4AllDriver::StopMeasurement()
{
    Write(SCPI::PCT::MEAS_STOP);
}

// ===========================================================================
// Chapter 5: PCT - PATH Commands
// ===========================================================================

void CViaviPCT4AllDriver::SetBiDir(int state)
{
    Write(std::string(SCPI::PCT::PATH_BIDIR) + " " + std::to_string(state));
}

int CViaviPCT4AllDriver::GetBiDir()
{
    return SafeAtoi(Query(SCPI::PCT::PATH_BIDIR_Q));
}

void CViaviPCT4AllDriver::SetChannel(int group, int channel)
{
    Write(std::string(SCPI::PCT::PATH_CHANNEL) + " "
          + std::to_string(group) + "," + std::to_string(channel));
}

int CViaviPCT4AllDriver::GetChannel(int group)
{
    return SafeAtoi(Query(std::string(SCPI::PCT::PATH_CHANNEL_Q) + " " + std::to_string(group)));
}

int CViaviPCT4AllDriver::GetAvailableChannels(int group)
{
    return SafeAtoi(Query(std::string(SCPI::PCT::PATH_CHANNEL_AVA_Q)
                          + " " + std::to_string(group)));
}

void CViaviPCT4AllDriver::SetConnection(int mode)
{
    Write(std::string(SCPI::PCT::PATH_CONNECTION) + " " + std::to_string(mode));
}

int CViaviPCT4AllDriver::GetConnection()
{
    return SafeAtoi(Query(SCPI::PCT::PATH_CONNECTION_Q));
}

void CViaviPCT4AllDriver::SetDUTLength(double length)
{
    std::ostringstream oss;
    oss << SCPI::PCT::PATH_DUT_LENGTH << " " << length;
    Write(oss.str());
}

double CViaviPCT4AllDriver::GetDUTLength()
{
    return SafeAtof(Query(SCPI::PCT::PATH_DUT_LENGTH_Q));
}

void CViaviPCT4AllDriver::SetDUTLengthAuto(int state)
{
    Write(std::string(SCPI::PCT::PATH_DUT_LENGTH_AUTO) + " " + std::to_string(state));
}

int CViaviPCT4AllDriver::GetDUTLengthAuto()
{
    return SafeAtoi(Query(SCPI::PCT::PATH_DUT_LENGTH_AUTO_Q));
}

void CViaviPCT4AllDriver::SetEOFMin(double distance)
{
    std::ostringstream oss;
    oss << SCPI::PCT::PATH_EOF_MIN << " " << distance;
    Write(oss.str());
}

double CViaviPCT4AllDriver::GetEOFMin()
{
    return SafeAtof(Query(SCPI::PCT::PATH_EOF_MIN_Q));
}

void CViaviPCT4AllDriver::SetJumperIL(int group, int channel, double il)
{
    std::ostringstream oss;
    oss << SCPI::PCT::PATH_JUMPER_IL << " " << group << "," << channel << "," << il;
    Write(oss.str());
}

double CViaviPCT4AllDriver::GetJumperIL(int group, int channel)
{
    return SafeAtof(Query(std::string(SCPI::PCT::PATH_JUMPER_IL_Q) + " "
                          + std::to_string(group) + "," + std::to_string(channel)));
}

void CViaviPCT4AllDriver::SetJumperILAuto(int group, int channel, int state)
{
    Write(std::string(SCPI::PCT::PATH_JUMPER_IL_AUTO) + " "
          + std::to_string(group) + "," + std::to_string(channel) + "," + std::to_string(state));
}

int CViaviPCT4AllDriver::GetJumperILAuto(int group, int channel)
{
    return SafeAtoi(Query(std::string(SCPI::PCT::PATH_JUMPER_IL_AUTO_Q) + " "
                          + std::to_string(group) + "," + std::to_string(channel)));
}

void CViaviPCT4AllDriver::SetJumperLength(int group, int channel, double length)
{
    std::ostringstream oss;
    oss << SCPI::PCT::PATH_JUMPER_LENGTH << " " << group << "," << channel << "," << length;
    Write(oss.str());
}

double CViaviPCT4AllDriver::GetJumperLength(int group, int channel)
{
    return SafeAtof(Query(std::string(SCPI::PCT::PATH_JUMPER_LENGTH_Q) + " "
                          + std::to_string(group) + "," + std::to_string(channel)));
}

void CViaviPCT4AllDriver::SetJumperLengthAuto(int group, int channel, int state)
{
    Write(std::string(SCPI::PCT::PATH_JUMPER_LENGTH_AUTO) + " "
          + std::to_string(group) + "," + std::to_string(channel) + "," + std::to_string(state));
}

int CViaviPCT4AllDriver::GetJumperLengthAuto(int group, int channel)
{
    return SafeAtoi(Query(std::string(SCPI::PCT::PATH_JUMPER_LENGTH_AUTO_Q) + " "
                          + std::to_string(group) + "," + std::to_string(channel)));
}

void CViaviPCT4AllDriver::ResetJumper(int group, int channel)
{
    std::string cmd = SCPI::PCT::PATH_JUMPER_RESET;
    if (group > 0)
    {
        cmd += " " + std::to_string(group);
        if (channel > 0) cmd += "," + std::to_string(channel);
    }
    Write(cmd);
}

void CViaviPCT4AllDriver::ResetJumperMeasure(int group, int channel)
{
    std::string cmd = SCPI::PCT::PATH_JUMPER_RESET_MEAS;
    if (group > 0)
    {
        cmd += " " + std::to_string(group);
        if (channel > 0) cmd += "," + std::to_string(channel);
    }
    Write(cmd);
}

void CViaviPCT4AllDriver::SetLaunch(int port)
{
    Write(std::string(SCPI::PCT::PATH_LAUNCH) + " " + std::to_string(port));
}

int CViaviPCT4AllDriver::GetLaunch()
{
    return SafeAtoi(Query(SCPI::PCT::PATH_LAUNCH_Q));
}

int CViaviPCT4AllDriver::GetLaunchAvailable()
{
    return SafeAtoi(Query(SCPI::PCT::PATH_LAUNCH_AVA_Q));
}

void CViaviPCT4AllDriver::SetPathList(int sw, const std::string& channels)
{
    Write(std::string(SCPI::PCT::PATH_LIST) + " " + std::to_string(sw) + "," + channels);
}

std::string CViaviPCT4AllDriver::GetPathList(int sw)
{
    return Query(std::string(SCPI::PCT::PATH_LIST_Q) + " " + std::to_string(sw));
}

void CViaviPCT4AllDriver::SetReceive(int state)
{
    Write(std::string(SCPI::PCT::PATH_RECEIVE) + " " + std::to_string(state));
}

int CViaviPCT4AllDriver::GetReceive()
{
    return SafeAtoi(Query(SCPI::PCT::PATH_RECEIVE_Q));
}

// ===========================================================================
// Chapter 5: PCT - Port Map Commands
// ===========================================================================

void CViaviPCT4AllDriver::SetPortMapEnable(int state)
{
    Write(std::string(SCPI::PCT::PMAP_ENABLE) + " " + std::to_string(state));
}

int CViaviPCT4AllDriver::GetPortMapEnable()
{
    return SafeAtoi(Query(SCPI::PCT::PMAP_ENABLE_Q));
}

void CViaviPCT4AllDriver::PortMapMeasureAll()
{
    Write(SCPI::PCT::PMAP_MEAS_ALL);
}

void CViaviPCT4AllDriver::SetPortMapLive(int channel)
{
    Write(std::string(SCPI::PCT::PMAP_MEAS_LIVE) + " " + std::to_string(channel));
}

int CViaviPCT4AllDriver::GetPortMapLive(int channel)
{
    return SafeAtoi(Query(std::string(SCPI::PCT::PMAP_MEAS_LIVE_Q)
                          + " " + std::to_string(channel)));
}

void CViaviPCT4AllDriver::PortMapValidate()
{
    Write(SCPI::PCT::PMAP_MEAS_VALID);
}

std::string CViaviPCT4AllDriver::GetPortMapValidation()
{
    return QueryLong(SCPI::PCT::PMAP_MEAS_VALID_Q);
}

std::string CViaviPCT4AllDriver::GetPortMapLink(int path)
{
    return Query(std::string(SCPI::PCT::PMAP_PATH_LINK_Q) + " " + std::to_string(path));
}

void CViaviPCT4AllDriver::SetPortMapSelect(int path)
{
    Write(std::string(SCPI::PCT::PMAP_PATH_SELECT) + " " + std::to_string(path));
}

int CViaviPCT4AllDriver::GetPortMapSelect()
{
    return SafeAtoi(Query(SCPI::PCT::PMAP_PATH_SELECT_Q));
}

int CViaviPCT4AllDriver::GetPortMapPathSize()
{
    return SafeAtoi(Query(SCPI::PCT::PMAP_PATH_SIZE_Q));
}

int CViaviPCT4AllDriver::GetPortMapFirst(int sw)
{
    return SafeAtoi(Query(std::string(SCPI::PCT::PMAP_SETUP_FIRST_Q)
                          + " " + std::to_string(sw)));
}

void CViaviPCT4AllDriver::PortMapInitList(const std::string& listSW1,
                                           const std::string& listSW2, int mode)
{
    Write(std::string(SCPI::PCT::PMAP_SETUP_INIT_LIST) + " "
          + listSW1 + "," + listSW2 + "," + std::to_string(mode));
}

void CViaviPCT4AllDriver::PortMapInitRange(int startSW1, int startSW2, int size, int mode)
{
    Write(std::string(SCPI::PCT::PMAP_SETUP_INIT_RANGE) + " "
          + std::to_string(startSW1) + "," + std::to_string(startSW2)
          + "," + std::to_string(size) + "," + std::to_string(mode));
}

int CViaviPCT4AllDriver::GetPortMapLast(int sw)
{
    return SafeAtoi(Query(std::string(SCPI::PCT::PMAP_SETUP_LAST_Q)
                          + " " + std::to_string(sw)));
}

void CViaviPCT4AllDriver::SetPortMapLink(int fixedCh, int variableCh)
{
    Write(std::string(SCPI::PCT::PMAP_SETUP_LINK) + " "
          + std::to_string(fixedCh) + "," + std::to_string(variableCh));
}

std::string CViaviPCT4AllDriver::GetPortMapList(int sw)
{
    return Query(std::string(SCPI::PCT::PMAP_SETUP_LIST_Q) + " " + std::to_string(sw));
}

void CViaviPCT4AllDriver::SetPortMapLock(int state)
{
    Write(std::string(SCPI::PCT::PMAP_SETUP_LOCK) + " " + std::to_string(state));
}

int CViaviPCT4AllDriver::GetPortMapLock()
{
    return SafeAtoi(Query(SCPI::PCT::PMAP_SETUP_LOCK_Q));
}

int CViaviPCT4AllDriver::GetPortMapMode()
{
    return SafeAtoi(Query(SCPI::PCT::PMAP_SETUP_MODE_Q));
}

void CViaviPCT4AllDriver::PortMapReset()
{
    Write(SCPI::PCT::PMAP_SETUP_RESET);
}

int CViaviPCT4AllDriver::GetPortMapSetupSize()
{
    return SafeAtoi(Query(SCPI::PCT::PMAP_SETUP_SIZE_Q));
}

// ===========================================================================
// Chapter 5: PCT - Sense Commands
// ===========================================================================

void CViaviPCT4AllDriver::SetAveragingTime(int seconds)
{
    Write(std::string(SCPI::PCT::SENS_ATIME) + " " + std::to_string(seconds));
}

int CViaviPCT4AllDriver::GetAveragingTime()
{
    return SafeAtoi(Query(SCPI::PCT::SENS_ATIME_Q));
}

std::string CViaviPCT4AllDriver::GetAvailableAveragingTimes()
{
    return Query(SCPI::PCT::SENS_ATIME_AVA_Q);
}

void CViaviPCT4AllDriver::SetFunction(int mode)
{
    Write(std::string(SCPI::PCT::SENS_FUNC) + " " + std::to_string(mode));
}

int CViaviPCT4AllDriver::GetFunction()
{
    return SafeAtoi(Query(SCPI::PCT::SENS_FUNC_Q));
}

void CViaviPCT4AllDriver::SetILOnly(int state)
{
    Write(std::string(SCPI::PCT::SENS_ILONLY) + " " + std::to_string(state));
}

int CViaviPCT4AllDriver::GetILOnly()
{
    return SafeAtoi(Query(SCPI::PCT::SENS_ILONLY_Q));
}

void CViaviPCT4AllDriver::SetOPM(int index)
{
    Write(std::string(SCPI::PCT::SENS_OPM) + " " + std::to_string(index));
}

int CViaviPCT4AllDriver::GetOPM()
{
    return SafeAtoi(Query(SCPI::PCT::SENS_OPM_Q));
}

void CViaviPCT4AllDriver::SetRange(int range)
{
    Write(std::string(SCPI::PCT::SENS_RANGE) + " " + std::to_string(range));
}

int CViaviPCT4AllDriver::GetRange()
{
    return SafeAtoi(Query(SCPI::PCT::SENS_RANGE_Q));
}

void CViaviPCT4AllDriver::SetTempSensitivity(int level)
{
    Write(std::string(SCPI::PCT::SENS_TEMP) + " " + std::to_string(level));
}

int CViaviPCT4AllDriver::GetTempSensitivity()
{
    return SafeAtoi(Query(SCPI::PCT::SENS_TEMP_Q));
}

// ===========================================================================
// Chapter 5: PCT - Source Commands
// ===========================================================================

void CViaviPCT4AllDriver::SetContinuous(int state)
{
    Write(std::string(SCPI::PCT::SOUR_CONTINUOUS) + " " + std::to_string(state));
}

int CViaviPCT4AllDriver::GetContinuous()
{
    return SafeAtoi(Query(SCPI::PCT::SOUR_CONTINUOUS_Q));
}

void CViaviPCT4AllDriver::SetSourceList(const std::string& wl)
{
    Write(std::string(SCPI::PCT::SOUR_LIST) + " " + wl);
}

std::string CViaviPCT4AllDriver::GetSourceList()
{
    return Query(SCPI::PCT::SOUR_LIST_Q);
}

void CViaviPCT4AllDriver::SetWarmup(const std::string& wl)
{
    Write(std::string(SCPI::PCT::SOUR_WARMUP) + " " + wl);
}

std::string CViaviPCT4AllDriver::GetWarmup()
{
    return Query(SCPI::PCT::SOUR_WARMUP_Q);
}

void CViaviPCT4AllDriver::SetWavelength(int wavelength)
{
    Write(std::string(SCPI::PCT::SOUR_WAV) + " " + std::to_string(wavelength));
}

std::string CViaviPCT4AllDriver::GetWavelength()
{
    return Query(SCPI::PCT::SOUR_WAV_Q);
}

std::string CViaviPCT4AllDriver::GetAvailableWavelengths()
{
    return Query(SCPI::PCT::SOUR_WAV_AVA_Q);
}

// ===========================================================================
// Chapter 5: PCT - System Warning
// ===========================================================================

ErrorInfo CViaviPCT4AllDriver::GetWarning()
{
    return ParseError(Query(SCPI::PCT::SYS_WARNING_Q));
}

// ===========================================================================
// Chapter 5: PCT - OPM Commands
// ===========================================================================

std::string CViaviPCT4AllDriver::FetchLoss()
{
    return Query(SCPI::PCT::FETCH_LOSS_Q);
}

std::string CViaviPCT4AllDriver::FetchORL()
{
    return Query(SCPI::PCT::FETCH_ORL_Q);
}

std::string CViaviPCT4AllDriver::FetchPower()
{
    return Query(SCPI::PCT::FETCH_POWER_Q);
}

void CViaviPCT4AllDriver::StartDarkMeasure()
{
    Write(SCPI::PCT::SENS_POW_DARK);
}

int CViaviPCT4AllDriver::GetDarkStatus()
{
    return SafeAtoi(Query(SCPI::PCT::SENS_POW_DARK_Q));
}

void CViaviPCT4AllDriver::RestoreDarkFactory()
{
    Write(SCPI::PCT::SENS_POW_DARK_FACTORY);
}

void CViaviPCT4AllDriver::SetPowerMode(int mode)
{
    Write(std::string(SCPI::PCT::SENS_POW_MODE) + " " + std::to_string(mode));
}

int CViaviPCT4AllDriver::GetPowerMode()
{
    return SafeAtoi(Query(SCPI::PCT::SENS_POW_MODE_Q));
}

// ===========================================================================
// Workflow helpers
// ===========================================================================

bool CViaviPCT4AllDriver::WaitForIdle(int timeoutMs)
{
    auto start = std::chrono::steady_clock::now();
    while (true)
    {
        int state = GetMeasurementState();
        if (state == MEAS_IDLE) return true;
        if (state == MEAS_FAULT) return false;

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();
        if (elapsed >= timeoutMs) return false;

        std::this_thread::sleep_for(std::chrono::milliseconds(POLL_INTERVAL_MS));
    }
}

bool CViaviPCT4AllDriver::WaitForMeasurement(int timeoutMs)
{
    return WaitForIdle(timeoutMs);
}

// ===========================================================================
// Raw SCPI passthrough
// ===========================================================================

std::string CViaviPCT4AllDriver::SendRawQuery(const std::string& command)
{
    return Query(command);
}

void CViaviPCT4AllDriver::SendRawWrite(const std::string& command)
{
    Write(command);
}

} // namespace ViaviPCT4All
