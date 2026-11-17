#include "stdafx.h"
#include "SantecPTMDriver.h"
#include "SantecPTMTcpAdapter.h"
#include "SantecPTMVisaAdapter.h"
#include "PTMScpiCommands.h"
#include <stdexcept>
#include <sstream>
#include <cctype>

namespace SantecPTM {

CSantecPTMDriver::CSantecPTMDriver(const std::string& address,
                                   int port,
                                   double timeout,
                                   CommType commType)
    : m_commType(commType)
    , m_adapter(nullptr)
    , m_ownsAdapter(false)
    , m_state(STATE_DISCONNECTED)
    , m_localMode(true)
    , m_channelCount(0)
    , m_log("Driver.SantecPTM")
{
    m_config.ipAddress = address;
    m_config.port = (port > 0) ? port : DEFAULT_PORT;
    m_config.timeout = timeout;
}

CSantecPTMDriver::~CSantecPTMDriver()
{
    Disconnect();
    if (m_ownsAdapter && m_adapter)
    {
        delete m_adapter;
        m_adapter = nullptr;
    }
}

void CSantecPTMDriver::SetCommAdapter(ISantecPTMCommAdapter* adapter, bool takeOwnership)
{
    if (m_ownsAdapter && m_adapter && m_adapter != adapter)
        delete m_adapter;
    m_adapter = adapter;
    m_ownsAdapter = takeOwnership;
}

bool CSantecPTMDriver::Connect()
{
    if (IsConnected())
        return true;

    m_state = STATE_CONNECTING;

    try
    {
        if (!m_adapter)
        {
            if (m_commType == COMM_USB || m_commType == COMM_GPIB)
            {
                if (m_commType == COMM_GPIB)
                    m_log.Warning("GPIB requested; using VISA adapter.");
                SetCommAdapter(new CSantecPTMVisaAdapter(), true);
            }
            else if (m_commType == COMM_DLL)
            {
                m_log.Warning("COMM_DLL not implemented.");
                m_state = STATE_ERROR;
                return false;
            }
            else
            {
                SetCommAdapter(new CSantecPTMTcpAdapter(), true);
            }
        }

        for (int attempt = 1; attempt <= m_config.reconnectAttempts; ++attempt)
        {
            m_log.Info("Connect attempt %d/%d to %s:%d (comm=%d)",
                       attempt, m_config.reconnectAttempts,
                       m_config.ipAddress.c_str(), m_config.port,
                       static_cast<int>(m_commType));

            if (m_adapter->Open(m_config.ipAddress, m_config.port, m_config.timeout))
            {
                std::string idn = Query(SCPI::IDN);
                ParseIdentity(idn);
                m_state = STATE_CONNECTED;
                m_log.Info("Connected: %s", idn.c_str());
                return true;
            }

            Sleep(static_cast<DWORD>(m_config.reconnectDelay * 1000));
        }
    }
    catch (const std::exception& ex)
    {
        m_log.Error("Connect failed: %s", ex.what());
    }

    m_state = STATE_ERROR;
    return false;
}

void CSantecPTMDriver::Disconnect()
{
    try
    {
        if (m_adapter && m_adapter->IsOpen())
        {
            try { Write(SCPI::LAS_OFF); } catch (...) {}
            try { Write(SCPI::CLS); } catch (...) {}
            m_adapter->Close();
        }
    }
    catch (...) {}

    m_state = STATE_DISCONNECTED;
    m_results.clear();
    m_rawResult.clear();
}

bool CSantecPTMDriver::Reconnect()
{
    Disconnect();
    return Connect();
}

bool CSantecPTMDriver::IsConnected() const
{
    return m_state == STATE_CONNECTED && m_adapter && m_adapter->IsOpen();
}

bool CSantecPTMDriver::Initialize()
{
    if (!IsConnected()) return false;

    try
    {
        Write(SCPI::CLS);

        std::string ch = Query(SCPI::CHANNEL_Q);
        m_channelCount = atoi(Trim(ch).c_str());
        if (m_channelCount <= 0)
            m_channelCount = 12;

        SetLocalMode(false);

        ErrorInfo err = CheckError();
        if (!err.IsOK())
            m_log.Warning("SYST:ERR? after init: %d,%s", err.code, err.message.c_str());

        m_log.Info("Initialized. Channels=%d", m_channelCount);
        return true;
    }
    catch (const std::exception& ex)
    {
        m_log.Error("Initialize failed: %s", ex.what());
        return false;
    }
}

DeviceInfo CSantecPTMDriver::GetDeviceInfo()
{
    if (IsConnected())
    {
        try
        {
            ParseIdentity(Query(SCPI::IDN));
        }
        catch (...) {}
    }
    return m_deviceInfo;
}

ErrorInfo CSantecPTMDriver::CheckError()
{
    try
    {
        std::string resp = Query(SCPI::SYS_ERR);
        // format: code,"message"
        ErrorInfo info;
        size_t comma = resp.find(',');
        if (comma != std::string::npos)
        {
            info.code = atoi(resp.substr(0, comma).c_str());
            info.message = Trim(resp.substr(comma + 1));
            if (!info.message.empty() && info.message.front() == '"')
                info.message.erase(info.message.begin());
            if (!info.message.empty() && info.message.back() == '"')
                info.message.pop_back();
        }
        else
        {
            info.code = atoi(resp.c_str());
            info.message = resp;
        }
        return info;
    }
    catch (const std::exception& ex)
    {
        return ErrorInfo(-1, ex.what());
    }
}

int CSantecPTMDriver::GetChannelCount()
{
    if (!IsConnected()) return m_channelCount;
    try
    {
        std::string ch = Query(SCPI::CHANNEL_Q);
        int n = atoi(Trim(ch).c_str());
        if (n > 0) m_channelCount = n;
    }
    catch (...) {}
    return m_channelCount;
}

bool CSantecPTMDriver::TakeMeasurement(int nChannels)
{
    if (!IsConnected()) return false;

    try
    {
        int n = nChannels;
        if (n <= 0)
            n = GetChannelCount();
        if (n <= 0)
            n = 12;

        std::ostringstream cmd;
        cmd << SCPI::TEST_CON << n;
        Write(cmd.str());

        if (!WaitUntilIdle())
        {
            m_log.Error("TakeMeasurement timed out waiting for STAT:OPER:COND?");
            return false;
        }

        m_rawResult = Query(SCPI::TEST_RESULT_Q);
        m_results = ParsePolarityMap(m_rawResult);
        m_log.Info("Measurement done. raw='%s' parsed=%d",
                   m_rawResult.c_str(), static_cast<int>(m_results.size()));
        return true;
    }
    catch (const std::exception& ex)
    {
        m_log.Error("TakeMeasurement failed: %s", ex.what());
        return false;
    }
}

std::vector<PolarityChannelResult> CSantecPTMDriver::GetResults()
{
    return m_results;
}

std::string CSantecPTMDriver::GetRawResult()
{
    return m_rawResult;
}

int CSantecPTMDriver::Detect(int outputChannel)
{
    if (!IsConnected() || outputChannel <= 0) return -1;
    try
    {
        // DETECT#? turns on laser # and returns brightest detector (async).
        std::ostringstream cmd;
        cmd << SCPI::DETECT << outputChannel << "?";
        Write(cmd.str());
        if (!WaitUntilIdle())
            m_log.Warning("DETECT wait idle timed out");

        // Re-query to fetch the detector number after operation completes.
        std::string resp = Query(cmd.str());
        return atoi(Trim(resp).c_str());
    }
    catch (const std::exception& ex)
    {
        m_log.Error("Detect failed: %s", ex.what());
        return -1;
    }
}

void CSantecPTMDriver::SetLocalMode(bool enabled)
{
    if (!IsConnected()) return;
    try
    {
        std::ostringstream cmd;
        cmd << SCPI::LCL << " " << (enabled ? 1 : 0);
        Write(cmd.str());
        m_localMode = enabled;
    }
    catch (const std::exception& ex)
    {
        m_log.Error("SetLocalMode failed: %s", ex.what());
    }
}

bool CSantecPTMDriver::GetLocalMode()
{
    if (!IsConnected()) return m_localMode;
    try
    {
        std::string resp = Query(SCPI::LCL_Q);
        m_localMode = (atoi(Trim(resp).c_str()) != 0);
    }
    catch (...) {}
    return m_localMode;
}

void CSantecPTMDriver::LaserOff()
{
    if (!IsConnected()) return;
    try { Write(SCPI::LAS_OFF); }
    catch (const std::exception& ex) { m_log.Error("LAS:OFF failed: %s", ex.what()); }
}

void CSantecPTMDriver::SetVfl(int outputChannel, VflMode mode)
{
    if (!IsConnected() || outputChannel <= 0) return;
    try
    {
        if (mode == VFL_OFF)
        {
            Write(SCPI::LAS_OFF);
            return;
        }
        std::ostringstream cmd;
        cmd << ((mode == VFL_FLASH) ? SCPI::FVFL : SCPI::VFL) << outputChannel;
        Write(cmd.str());
    }
    catch (const std::exception& ex)
    {
        m_log.Error("SetVfl failed: %s", ex.what());
    }
}

std::string CSantecPTMDriver::GetBarcode()
{
    if (!IsConnected()) return "";
    try { return Query(SCPI::READ_BARCODE); }
    catch (...) { return ""; }
}

void CSantecPTMDriver::Notify(int icon, const std::string& message)
{
    if (!IsConnected()) return;
    try
    {
        std::ostringstream cmd;
        cmd << SCPI::TEST_NOTIFY << icon << " \"" << message << "\"";
        Write(cmd.str());
    }
    catch (const std::exception& ex)
    {
        m_log.Error("Notify failed: %s", ex.what());
    }
}

bool CSantecPTMDriver::TestStart()
{
    if (!IsConnected()) return false;
    try
    {
        Write(SCPI::TEST_START);
        return WaitUntilIdle();
    }
    catch (...) { return false; }
}

std::string CSantecPTMDriver::TestStatus()
{
    if (!IsConnected()) return "";
    try { return Query(SCPI::TEST_STATUS_Q); }
    catch (...) { return ""; }
}

bool CSantecPTMDriver::TestNext()
{
    if (!IsConnected()) return false;
    try { Write(SCPI::TEST_NEXT); return true; }
    catch (...) { return false; }
}

bool CSantecPTMDriver::TestRetry()
{
    if (!IsConnected()) return false;
    try { Write(SCPI::TEST_RETRY); return true; }
    catch (...) { return false; }
}

std::string CSantecPTMDriver::SendRawQuery(const std::string& command)
{
    return Query(command);
}

void CSantecPTMDriver::SendRawWrite(const std::string& command)
{
    Write(command);
}

// ---------------------------------------------------------------------------
std::string CSantecPTMDriver::Query(const std::string& command)
{
    if (!m_adapter || !m_adapter->IsOpen())
        throw std::runtime_error("PTM not connected");
    m_log.Debug("TX> %s", command.c_str());
    std::string resp = m_adapter->SendQuery(command);
    m_log.Debug("RX< %s", resp.c_str());
    return resp;
}

void CSantecPTMDriver::Write(const std::string& command)
{
    if (!m_adapter || !m_adapter->IsOpen())
        throw std::runtime_error("PTM not connected");
    m_log.Debug("TX> %s", command.c_str());
    m_adapter->SendWrite(command);
}

bool CSantecPTMDriver::WaitUntilIdle(int timeoutMs)
{
    const int polls = (timeoutMs > 0) ? (timeoutMs / IDLE_POLL_MS) : 1;
    for (int i = 0; i < polls; ++i)
    {
        std::string resp = Query(SCPI::STAT_OPER_COND);
        int cond = atoi(Trim(resp).c_str());
        if (cond == 0)
            return true;
        Sleep(IDLE_POLL_MS);
    }
    return false;
}

void CSantecPTMDriver::ParseIdentity(const std::string& idnResponse)
{
    // MANUFACTURER,MODEL,SERIAL,FIRMWARE
    std::istringstream iss(idnResponse);
    std::string part;
    std::vector<std::string> parts;
    while (std::getline(iss, part, ','))
        parts.push_back(Trim(part));

    if (parts.size() > 0) m_deviceInfo.manufacturer = parts[0];
    if (parts.size() > 1) m_deviceInfo.model = parts[1];
    if (parts.size() > 2) m_deviceInfo.serialNumber = parts[2];
    if (parts.size() > 3) m_deviceInfo.firmwareVersion = parts[3];
}

std::string CSantecPTMDriver::Trim(const std::string& s)
{
    size_t b = 0;
    while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    size_t e = s.size();
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return s.substr(b, e - b);
}

// ---------------------------------------------------------------------------
// Provisional TEST:RESult? parsers (same-company CSV style as RLM).
// Formats accepted:
//   1) out,in,out,in,...          e.g. 1,1,2,2,3,3
//   2) out:in,out:in,...          e.g. 1:1,2:2
//   3) groups separated by ';'
// On failure returns empty vector (raw string still available via GetRawResult).
// ---------------------------------------------------------------------------
std::vector<PolarityChannelResult> CSantecPTMDriver::ParsePolarityMap(const std::string& raw)
{
    std::vector<PolarityChannelResult> results;
    std::string text = Trim(raw);
    if (text.empty()) return results;

    // Normalize ';' groups into commas for tokenization of pairs
    for (char& c : text)
    {
        if (c == ';') c = ',';
    }

    // Prefer out:in tokens
    if (text.find(':') != std::string::npos)
    {
        std::istringstream iss(text);
        std::string token;
        while (std::getline(iss, token, ','))
        {
            token = Trim(token);
            if (token.empty()) continue;
            size_t colon = token.find(':');
            if (colon == std::string::npos) continue;
            PolarityChannelResult r;
            r.outputChannel = atoi(token.substr(0, colon).c_str());
            r.inputChannel = atoi(token.substr(colon + 1).c_str());
            r.rawData[0] = r.outputChannel;
            r.rawData[1] = r.inputChannel;
            r.rawDataCount = 2;
            if (r.outputChannel > 0)
                results.push_back(r);
        }
        return results;
    }

    // Flat CSV: out,in,out,in,...
    std::vector<int> vals;
    {
        std::istringstream iss(text);
        std::string token;
        while (std::getline(iss, token, ','))
        {
            token = Trim(token);
            if (token.empty()) continue;
            vals.push_back(atoi(token.c_str()));
        }
    }

    for (size_t i = 0; i + 1 < vals.size(); i += 2)
    {
        PolarityChannelResult r;
        r.outputChannel = vals[i];
        r.inputChannel = vals[i + 1];
        r.rawData[0] = r.outputChannel;
        r.rawData[1] = r.inputChannel;
        r.rawDataCount = 2;
        if (r.outputChannel > 0)
            results.push_back(r);
    }
    return results;
}

} // namespace SantecPTM
