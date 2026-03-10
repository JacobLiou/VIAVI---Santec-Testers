#include "stdafx.h"
#include "CViaviPCTDriver.h"
#include "ViaviPCTTcpAdapter.h"
#include "ViaviPCTVisaAdapter.h"
#include <stdexcept>
#include <sstream>
#include <cmath>

namespace ViaviPCT {

// ===========================================================================
// Viavi MAP PCT SCPI 命令定义
//
// 参考：MAP-PCT Programming Guide 22112369-346, Chapter 5
// ===========================================================================
namespace SCPI {
    // 远程控制激活
    static const char* REM             = "*REM";

    // IEEE 488.2 通用命令
    static const char* IDN             = "*IDN?";
    static const char* RST             = "*RST";
    static const char* CLS             = "*CLS";
    static const char* OPC_Q           = "*OPC?";

    // 系统命令
    static const char* SYS_ERR         = ":SYST:ERR?";

    // 模块信息
    static const char* INFO            = ":INFO?";
    static const char* CONF            = ":CONF?";

    // 光源/波长配置 (Chapter 5.5)
    static const char* SOUR_WAV        = ":SOUR:WAV";           // :SOUR:WAV <nm>
    static const char* SOUR_WAV_Q      = ":SOUR:WAV?";
    static const char* SOUR_WAV_AVA    = ":SOUR:WAV:AVA?";      // 查询可用波长
    static const char* SOUR_LIST       = ":SOUR:LIST";           // :SOUR:LIST <w1>,<w2>,...
    static const char* SOUR_LIST_Q     = ":SOUR:LIST?";

    // 传感器/模式 (Chapter 5.7)
    static const char* SENS_FUNC       = ":SENS:FUNC";           // :SENS:FUNC <0|1> (0=参考, 1=DUT)
    static const char* SENS_FUNC_Q     = ":SENS:FUNC?";

    // 测量控制 (Chapter 5.8)
    static const char* MEAS_STAR       = ":MEAS:STAR";           // 开始测量
    static const char* MEAS_STAT       = ":MEAS:STAT?";          // 查询测量状态
    static const char* MEAS_ABOR       = ":MEAS:ABOR";           // 中止测量

    // 测量结果 (Chapter 5.9)
    static const char* MEAS_ALL        = ":MEAS:ALL?";           // :MEAS:ALL? <channel> 获取全部结果
    static const char* MEAS_IL         = ":MEAS:IL?";            // :MEAS:IL? <channel> 插入损耗
    static const char* MEAS_ORL        = ":MEAS:ORL?";           // :MEAS:ORL? <channel> 回波损耗
    static const char* MEAS_LEN        = ":MEAS:LEN?";           // :MEAS:LEN? <channel> 长度

    // 路径/通道 (Chapter 5.6)
    static const char* PATH_CHAN       = ":PATH:CHAN";            // :PATH:CHAN <group>,<channel>
    static const char* PATH_CHAN_Q     = ":PATH:CHAN?";

    // 平均时间
    static const char* SENS_AVG        = ":SENS:AVG";            // :SENS:AVG <seconds>
    static const char* SENS_AVG_Q      = ":SENS:AVG?";

    // DUT 范围
    static const char* SENS_RANG       = ":SENS:RANG";           // :SENS:RANG <meters>
    static const char* SENS_RANG_Q     = ":SENS:RANG?";

    // IL-only 模式
    static const char* SENS_ILONLY     = ":SENS:ILON";           // :SENS:ILON <0|1>
    static const char* SENS_ILONLY_Q   = ":SENS:ILON?";

    // 连接模式（1跳线/2跳线）
    static const char* SENS_CONN       = ":SENS:CONN";           // :SENS:CONN <1|2>
    static const char* SENS_CONN_Q     = ":SENS:CONN?";

    // 双向
    static const char* SENS_BIDI       = ":SENS:BIDI";           // :SENS:BIDI <0|1>
    static const char* SENS_BIDI_Q     = ":SENS:BIDI?";

    // 忙碌状态
    static const char* BUSY            = ":BUSY?";
}


// ===========================================================================
// 构造 / 析构
// ===========================================================================

CViaviPCTDriver::CViaviPCTDriver(const std::string& address,
                                   int port,
                                   double timeout,
                                   CommType commType)
    : m_commType(commType)
    , m_adapter(nullptr)
    , m_ownsAdapter(false)
    , m_state(STATE_DISCONNECTED)
    , m_logger("ViaviPCT")
    , m_measMode(MODE_REFERENCE)
    , m_averagingTime(1)
    , m_dutRange(100)
    , m_ilOnly(false)
    , m_connectionMode(1)
    , m_bidirectional(false)
    , m_referenced(false)
    , m_abortRequested(false)
{
    m_config.ipAddress = address;
    m_config.port = port;
    m_config.timeout = timeout;
    m_config.bufferSize = 4096;
    m_config.reconnectAttempts = 3;
    m_config.reconnectDelay = 2.0;

    m_wavelengths.push_back(1310.0);
    m_wavelengths.push_back(1550.0);
    m_channels.push_back(1);
}

CViaviPCTDriver::~CViaviPCTDriver()
{
    Disconnect();
    if (m_ownsAdapter && m_adapter)
    {
        delete m_adapter;
        m_adapter = nullptr;
    }
}

void CViaviPCTDriver::SetCommAdapter(IViaviPCTCommAdapter* adapter, bool takeOwnership)
{
    if (m_ownsAdapter && m_adapter)
        delete m_adapter;
    m_adapter = adapter;
    m_ownsAdapter = takeOwnership;
}

// ---------------------------------------------------------------------------
// 统一命令接口
// ---------------------------------------------------------------------------

std::string CViaviPCTDriver::Query(const std::string& command)
{
    m_logger.Debug("TX> %s", command.c_str());
    if (!m_adapter || !m_adapter->IsOpen())
        throw std::runtime_error("Viavi PCT 未连接。");

    std::string response = m_adapter->SendQuery(command);
    m_logger.Debug("RX< %s", response.c_str());
    return response;
}

std::string CViaviPCTDriver::QueryLong(const std::string& command)
{
    CViaviPCTTcpAdapter* tcpAdapter = dynamic_cast<CViaviPCTTcpAdapter*>(m_adapter);
    CViaviPCTVisaAdapter* visaAdapter = dynamic_cast<CViaviPCTVisaAdapter*>(m_adapter);

    if (tcpAdapter)
        tcpAdapter->SetReadTimeout(MEAS_TIMEOUT_MS);
    else if (visaAdapter)
        visaAdapter->SetReadTimeout(MEAS_TIMEOUT_MS);

    std::string result = Query(command);

    DWORD normalTimeout = static_cast<DWORD>(m_config.timeout * 1000);
    if (tcpAdapter)
        tcpAdapter->SetReadTimeout(normalTimeout);
    else if (visaAdapter)
        visaAdapter->SetReadTimeout(normalTimeout);

    return result;
}

void CViaviPCTDriver::Write(const std::string& command)
{
    m_logger.Debug("TX> %s", command.c_str());
    if (!m_adapter || !m_adapter->IsOpen())
        throw std::runtime_error("Viavi PCT 未连接。");
    m_adapter->SendWrite(command);
}

// ---------------------------------------------------------------------------
// 连接
// ---------------------------------------------------------------------------

bool CViaviPCTDriver::Connect()
{
    m_logger.Info("连接 Viavi PCT %s:%d (comm=%d)",
        m_config.ipAddress.c_str(), m_config.port, static_cast<int>(m_commType));

    switch (m_commType)
    {
    case COMM_TCP:
    {
        if (!m_adapter)
        {
            m_adapter = new CViaviPCTTcpAdapter();
            m_ownsAdapter = true;
        }

        for (int attempt = 1; attempt <= m_config.reconnectAttempts; ++attempt)
        {
            m_logger.Info("连接尝试 %d/%d", attempt, m_config.reconnectAttempts);

            if (!m_adapter->Open(m_config.ipAddress, m_config.port, m_config.timeout))
            {
                m_logger.Error("连接尝试 %d 失败。", attempt);
                if (attempt < m_config.reconnectAttempts)
                    Sleep(static_cast<DWORD>(m_config.reconnectDelay * 1000));
                continue;
            }

            m_state = STATE_CONNECTED;

            if (!ValidateConnection())
            {
                m_logger.Error("验证失败 (尝试 %d)。", attempt);
                m_adapter->Close();
                m_state = STATE_DISCONNECTED;
                if (attempt < m_config.reconnectAttempts)
                    Sleep(static_cast<DWORD>(m_config.reconnectDelay * 1000));
                continue;
            }

            m_logger.Info("Viavi PCT TCP 已连接并验证。");
            return true;
        }

        m_state = STATE_ERROR;
        m_logger.Error("所有连接尝试已耗尽。");
        return false;
    }
    case COMM_GPIB:
    case COMM_VISA:
    {
        if (!m_adapter)
        {
            CViaviPCTVisaAdapter* visaAdapter = new CViaviPCTVisaAdapter();
            m_adapter = visaAdapter;
            m_ownsAdapter = true;
        }

        for (int attempt = 1; attempt <= m_config.reconnectAttempts; ++attempt)
        {
            m_logger.Info("VISA 连接尝试 %d/%d", attempt, m_config.reconnectAttempts);

            try
            {
                if (!m_adapter->Open(m_config.ipAddress, m_config.port, m_config.timeout))
                {
                    m_logger.Error("VISA 连接尝试 %d 失败。", attempt);
                    if (attempt < m_config.reconnectAttempts)
                        Sleep(static_cast<DWORD>(m_config.reconnectDelay * 1000));
                    continue;
                }
            }
            catch (const std::exception& ex)
            {
                m_logger.Error("VISA 连接异常: %s", ex.what());
                if (attempt < m_config.reconnectAttempts)
                    Sleep(static_cast<DWORD>(m_config.reconnectDelay * 1000));
                continue;
            }

            m_state = STATE_CONNECTED;

            if (!ValidateConnection())
            {
                m_logger.Error("VISA 验证失败 (尝试 %d)。", attempt);
                m_adapter->Close();
                m_state = STATE_DISCONNECTED;
                if (attempt < m_config.reconnectAttempts)
                    Sleep(static_cast<DWORD>(m_config.reconnectDelay * 1000));
                continue;
            }

            m_logger.Info("Viavi PCT VISA 已连接并验证。");
            return true;
        }

        m_state = STATE_ERROR;
        m_logger.Error("所有 VISA 连接尝试已耗尽。");
        return false;
    }
    default:
        m_logger.Error("未知通信类型: %d", static_cast<int>(m_commType));
        return false;
    }
}

void CViaviPCTDriver::Disconnect()
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
        m_referenced = false;
        m_lastResults.clear();
        m_logger.Info("Viavi PCT 已断开。");
    }
}

bool CViaviPCTDriver::Reconnect()
{
    Disconnect();
    return Connect();
}

bool CViaviPCTDriver::IsConnected() const
{
    if (m_adapter)
        return m_state == STATE_CONNECTED && m_adapter->IsOpen();
    return false;
}

// ---------------------------------------------------------------------------
// 连接验证：发送 *REM 激活远程控制，然后 :CONF? 验证
// ---------------------------------------------------------------------------

bool CViaviPCTDriver::ValidateConnection()
{
    m_logger.Info("验证 Viavi PCT 连接...");
    try
    {
        // 激活远程控制模式
        Write(SCPI::REM);
        Sleep(200);

        // 查询配置以验证连接
        std::string confResp = Query(SCPI::CONF);
        if (confResp.empty())
        {
            m_logger.Error("验证失败: :CONF? 无响应");
            return false;
        }

        m_pctConfig = ParseConfiguration(confResp);
        m_logger.Info("PCT 验证成功: 设备 %d, 类型 %s",
            m_pctConfig.deviceNum, m_pctConfig.deviceType.c_str());

        // 获取设备信息
        try
        {
            std::string infoResp = Query(SCPI::INFO);
            ParseDeviceInfo(infoResp);
            m_logger.Info("设备信息: S/N %s, P/N %s, FW %s",
                m_deviceInfo.serialNumber.c_str(),
                m_deviceInfo.partNumber.c_str(),
                m_deviceInfo.firmwareVersion.c_str());
        }
        catch (const std::exception& e)
        {
            m_logger.Warning(":INFO? 失败: %s", e.what());
        }

        return true;
    }
    catch (const std::exception& e)
    {
        m_logger.Error("验证异常: %s", e.what());
        return false;
    }
}

// ---------------------------------------------------------------------------
// 初始化 - 发现设备能力并应用配置
// ---------------------------------------------------------------------------

bool CViaviPCTDriver::Initialize()
{
    m_logger.Info("初始化 Viavi PCT...");

    try
    {
        Write(SCPI::CLS);
        Sleep(100);

        // 查询可用波长
        try
        {
            std::string wavResp = Query(SCPI::SOUR_WAV_AVA);
            m_logger.Info("可用波长: %s", wavResp.c_str());

            std::vector<int> wavs = ParseIntList(wavResp);
            m_pctConfig.wavelengths = wavs;
        }
        catch (const std::exception& e)
        {
            m_logger.Warning("查询可用波长失败: %s", e.what());
        }

        // 配置波长列表
        if (!m_wavelengths.empty())
        {
            std::ostringstream wavCmd;
            wavCmd << SCPI::SOUR_LIST;
            for (size_t i = 0; i < m_wavelengths.size(); ++i)
            {
                if (i > 0) wavCmd << ",";
                wavCmd << " " << static_cast<int>(m_wavelengths[i]);
            }
            Write(wavCmd.str());
            Sleep(100);
        }

        // 设置默认参考模式
        Write(std::string(SCPI::SENS_FUNC) + " 0");
        m_measMode = MODE_REFERENCE;

        ErrorInfo err = CheckError();
        if (!err.IsOK())
        {
            m_logger.Warning("初始化完成但有错误: [%d] %s", err.code, err.message.c_str());
        }

        m_logger.Info("Viavi PCT 初始化完成。");
        return true;
    }
    catch (const std::exception& e)
    {
        m_logger.Error("初始化失败: %s", e.what());
        return false;
    }
}

// ---------------------------------------------------------------------------
// 设备信息
// ---------------------------------------------------------------------------

DeviceInfo CViaviPCTDriver::GetDeviceInfo()
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

void CViaviPCTDriver::ParseDeviceInfo(const std::string& response)
{
    // :INFO? 返回格式: serialNumber,partNumber,fwVersion,modVer1,modVer2,hwVer,date
    std::vector<std::string> parts;
    std::istringstream iss(response);
    std::string token;
    while (std::getline(iss, token, ','))
        parts.push_back(Trim(token));

    if (parts.size() >= 1) m_deviceInfo.serialNumber = parts[0];
    if (parts.size() >= 2) m_deviceInfo.partNumber = parts[1];
    if (parts.size() >= 3) m_deviceInfo.firmwareVersion = parts[2];
    if (parts.size() >= 4) m_deviceInfo.moduleVersion1 = parts[3];
    if (parts.size() >= 5) m_deviceInfo.moduleVersion2 = parts[4];
    if (parts.size() >= 6) m_deviceInfo.hardwareVersion = parts[5];
    if (parts.size() >= 7) m_deviceInfo.assemblyDate = parts[6];
}

ErrorInfo CViaviPCTDriver::CheckError()
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

PCTConfig CViaviPCTDriver::GetConfiguration()
{
    if (IsConnected())
    {
        try
        {
            std::string confResp = Query(SCPI::CONF);
            m_pctConfig = ParseConfiguration(confResp);
        }
        catch (...) {}
    }
    return m_pctConfig;
}

PCTConfig CViaviPCTDriver::ParseConfiguration(const std::string& response)
{
    // :CONF? 返回格式: deviceNum,type,fiberType,outputs,sw1Channels,sw2Channels,wav1,wav2,...
    PCTConfig config;
    std::vector<std::string> parts;
    std::istringstream iss(response);
    std::string token;
    while (std::getline(iss, token, ','))
        parts.push_back(Trim(token));

    if (parts.size() >= 1) config.deviceNum = atoi(parts[0].c_str());
    if (parts.size() >= 2) config.deviceType = parts[1];
    if (parts.size() >= 3) config.fiberType = parts[2];
    if (parts.size() >= 4) config.outputs = atoi(parts[3].c_str());
    if (parts.size() >= 5) config.sw1Channels = atoi(parts[4].c_str());
    if (parts.size() >= 6) config.sw2Channels = atoi(parts[5].c_str());

    for (size_t i = 6; i < parts.size(); ++i)
    {
        int wav = atoi(parts[i].c_str());
        if (wav > 0) config.wavelengths.push_back(wav);
    }

    return config;
}

// ---------------------------------------------------------------------------
// 配置
// ---------------------------------------------------------------------------

void CViaviPCTDriver::ConfigureWavelengths(const std::vector<double>& wavelengths)
{
    m_wavelengths = wavelengths;
    m_logger.Info("配置 %d 个波长。", (int)wavelengths.size());

    if (IsConnected() && !wavelengths.empty())
    {
        std::ostringstream cmd;
        cmd << SCPI::SOUR_LIST;
        for (size_t i = 0; i < wavelengths.size(); ++i)
        {
            if (i > 0) cmd << ",";
            cmd << " " << static_cast<int>(wavelengths[i]);
        }
        Write(cmd.str());
    }
}

void CViaviPCTDriver::ConfigureChannels(const std::vector<int>& channels)
{
    m_channels = channels;
    m_logger.Info("配置 %d 个通道。", (int)channels.size());
}

void CViaviPCTDriver::SetMeasurementMode(MeasurementMode mode)
{
    m_measMode = mode;
    if (IsConnected())
    {
        std::ostringstream cmd;
        cmd << SCPI::SENS_FUNC << " " << static_cast<int>(mode);
        Write(cmd.str());
        m_logger.Info("测量模式设置为 %s。", mode == MODE_REFERENCE ? "参考" : "DUT");
    }
}

MeasurementMode CViaviPCTDriver::GetMeasurementMode()
{
    if (IsConnected())
    {
        try
        {
            std::string resp = Query(SCPI::SENS_FUNC_Q);
            int val = atoi(resp.c_str());
            m_measMode = static_cast<MeasurementMode>(val);
        }
        catch (...) {}
    }
    return m_measMode;
}

void CViaviPCTDriver::SetAveragingTime(int seconds)
{
    m_averagingTime = seconds;
    if (IsConnected())
    {
        std::ostringstream cmd;
        cmd << SCPI::SENS_AVG << " " << seconds;
        Write(cmd.str());
        m_logger.Info("平均时间设置为 %d 秒。", seconds);
    }
}

int CViaviPCTDriver::GetAveragingTime()
{
    if (IsConnected())
    {
        try
        {
            std::string resp = Query(SCPI::SENS_AVG_Q);
            m_averagingTime = atoi(resp.c_str());
        }
        catch (...) {}
    }
    return m_averagingTime;
}

void CViaviPCTDriver::SetDUTRange(int rangeMeters)
{
    m_dutRange = rangeMeters;
    if (IsConnected())
    {
        std::ostringstream cmd;
        cmd << SCPI::SENS_RANG << " " << rangeMeters;
        Write(cmd.str());
        m_logger.Info("DUT 范围设置为 %d 米。", rangeMeters);
    }
}

void CViaviPCTDriver::SetILOnly(bool ilOnly)
{
    m_ilOnly = ilOnly;
    if (IsConnected())
    {
        std::ostringstream cmd;
        cmd << SCPI::SENS_ILONLY << " " << (ilOnly ? "1" : "0");
        Write(cmd.str());
        m_logger.Info("IL-only 模式 %s。", ilOnly ? "启用" : "禁用");
    }
}

void CViaviPCTDriver::SetConnectionMode(int mode)
{
    m_connectionMode = mode;
    if (IsConnected())
    {
        std::ostringstream cmd;
        cmd << SCPI::SENS_CONN << " " << mode;
        Write(cmd.str());
        m_logger.Info("连接模式设置为 %d 跳线。", mode);
    }
}

void CViaviPCTDriver::SetBidirectional(bool enabled)
{
    m_bidirectional = enabled;
    if (IsConnected())
    {
        std::ostringstream cmd;
        cmd << SCPI::SENS_BIDI << " " << (enabled ? "1" : "0");
        Write(cmd.str());
        m_logger.Info("双向模式 %s。", enabled ? "启用" : "禁用");
    }
}

// ---------------------------------------------------------------------------
// 测量状态轮询
// ---------------------------------------------------------------------------

MeasurementState CViaviPCTDriver::GetMeasurementState()
{
    try
    {
        std::string resp = Query(SCPI::MEAS_STAT);
        int val = atoi(resp.c_str());
        return static_cast<MeasurementState>(val);
    }
    catch (...)
    {
        return MEAS_FAULT;
    }
}

bool CViaviPCTDriver::WaitForMeasurement(int timeoutMs)
{
    int elapsed = 0;
    int interval = POLL_INTERVAL_MS;

    while (elapsed < timeoutMs)
    {
        if (m_abortRequested.load())
        {
            m_logger.Info("收到中止请求，发送中止序列...");
            try {
                Write(SCPI::MEAS_ABOR);
                Sleep(100);
                Write(std::string(SCPI::SENS_FUNC) + " 1");
            } catch (...) {}
            m_abortRequested = false;
            m_logger.Info("测量已中止 (耗时 %d ms)。", elapsed);
            return false;
        }

        MeasurementState state = GetMeasurementState();
        if (state == MEAS_IDLE)
        {
            m_logger.Debug("测量完成 (耗时 %d ms)。", elapsed);
            return true;
        }
        if (state == MEAS_FAULT)
        {
            m_logger.Error("测量故障。");
            return false;
        }

        Sleep(interval);
        elapsed += interval;

        if (interval < 2000)
            interval = (std::min)(interval * 2, 2000);
    }

    m_logger.Error("测量超时 (%d ms)。", timeoutMs);
    return false;
}

void CViaviPCTDriver::AbortMeasurement()
{
    m_abortRequested = true;
}

// ---------------------------------------------------------------------------
// 参考 / 校准
// ---------------------------------------------------------------------------

bool CViaviPCTDriver::TakeReference(const ReferenceConfig& config)
{
    m_logger.Info("开始参考测量...");
    m_referenced = false;
    m_abortRequested = false;

    try
    {
        // 设置参考模式
        SetMeasurementMode(MODE_REFERENCE);
        Sleep(200);

        // 开始测量
        Write(SCPI::MEAS_STAR);

        // 轮询等待完成
        if (!WaitForMeasurement(MEAS_TIMEOUT_MS))
        {
            m_logger.Error("参考测量超时。");
            return false;
        }

        // 检查错误
        ErrorInfo err = CheckError();
        if (!err.IsOK())
        {
            m_logger.Warning("参考测量完成但有错误: [%d] %s", err.code, err.message.c_str());
        }

        m_referenced = true;
        m_logger.Info("参考测量完成。");
        return true;
    }
    catch (const std::exception& e)
    {
        m_logger.Error("参考测量异常: %s", e.what());
        return false;
    }
}

// ---------------------------------------------------------------------------
// DUT 测量
// ---------------------------------------------------------------------------

bool CViaviPCTDriver::TakeMeasurement()
{
    m_logger.Info("开始 DUT 测量...");
    m_abortRequested = false;

    if (!m_referenced)
        m_logger.Warning("未进行参考测量 - 结果可能不准确。");

    m_lastResults.clear();

    try
    {
        // 设置 DUT 测量模式
        SetMeasurementMode(MODE_DUT);
        Sleep(200);

        // 开始测量
        Write(SCPI::MEAS_STAR);

        // 轮询等待完成
        if (!WaitForMeasurement(MEAS_TIMEOUT_MS))
        {
            m_logger.Error("DUT 测量超时。");
            return false;
        }

        // 获取每个通道的结果（多波长用 ';' 分隔）
        for (size_t ci = 0; ci < m_channels.size(); ++ci)
        {
            int ch = m_channels[ci];

            try
            {
                std::ostringstream cmd;
                cmd << SCPI::MEAS_ALL << " " << ch;
                std::string resp = Query(cmd.str());

                std::istringstream groups(resp);
                std::string group;
                while (std::getline(groups, group, ';'))
                {
                    std::string trimmedGroup = Trim(group);
                    if (trimmedGroup.empty()) continue;

                    MeasurementResult result = ParseMeasurementAll(trimmedGroup, ch);
                    m_lastResults.push_back(result);

                    m_logger.Info("  Ch%d @%.0fnm: IL=%.2f dB, ORL=%.2f dB, Len=%.2f m",
                        ch, result.wavelength, result.insertionLoss,
                        result.returnLoss, result.dutLength);
                }
            }
            catch (const std::exception& e)
            {
                m_logger.Warning("获取通道 %d 结果失败: %s", ch, e.what());
            }
        }

        // 检查错误
        ErrorInfo err = CheckError();
        if (!err.IsOK())
        {
            m_logger.Warning("测量完成但有错误: [%d] %s", err.code, err.message.c_str());
        }

        m_logger.Info("DUT 测量完成: %d 个结果。", (int)m_lastResults.size());
        return true;
    }
    catch (const std::exception& e)
    {
        m_logger.Error("DUT 测量异常: %s", e.what());
        return false;
    }
}

std::vector<MeasurementResult> CViaviPCTDriver::GetResults()
{
    return m_lastResults;
}

// ---------------------------------------------------------------------------
// 高级工作流
// ---------------------------------------------------------------------------

std::vector<MeasurementResult> CViaviPCTDriver::RunFullTest(
    const std::vector<double>& wavelengths,
    const std::vector<int>& channels,
    bool doReference,
    const ReferenceConfig& refConfig)
{
    ConfigureWavelengths(wavelengths);
    ConfigureChannels(channels);

    if (doReference)
    {
        if (!TakeReference(refConfig))
        {
            m_logger.Error("RunFullTest: 参考测量失败。");
            return std::vector<MeasurementResult>();
        }
    }

    if (!TakeMeasurement())
    {
        m_logger.Error("RunFullTest: DUT 测量失败。");
        return std::vector<MeasurementResult>();
    }

    return GetResults();
}

// ---------------------------------------------------------------------------
// 原始 SCPI 透传
// ---------------------------------------------------------------------------

std::string CViaviPCTDriver::SendRawQuery(const std::string& command)
{
    return Query(command);
}

void CViaviPCTDriver::SendRawWrite(const std::string& command)
{
    Write(command);
}

// ---------------------------------------------------------------------------
// 解析 :MEAS:ALL? 响应
// 格式: wavelength,IL,ORL,ORL_zone1,ORL_zone2,length,power,...
// ---------------------------------------------------------------------------

MeasurementResult CViaviPCTDriver::ParseMeasurementAll(const std::string& response, int channel)
{
    MeasurementResult result;
    result.channel = channel;

    std::vector<double> values = ParseDoubleList(response);

    if (values.size() >= 1) result.wavelength = values[0];
    if (values.size() >= 2) result.insertionLoss = values[1];
    if (values.size() >= 3) result.returnLoss = values[2];
    if (values.size() >= 4) result.returnLossZone1 = values[3];
    if (values.size() >= 5) result.returnLossZone2 = values[4];
    if (values.size() >= 6) result.dutLength = values[5];
    if (values.size() >= 7) result.power = values[6];

    for (size_t i = 0; i < values.size() && i < 10; ++i)
        result.rawData[result.rawDataCount++] = values[i];

    return result;
}

// ---------------------------------------------------------------------------
// 辅助函数
// ---------------------------------------------------------------------------

std::vector<double> CViaviPCTDriver::ParseDoubleList(const std::string& csv)
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

std::vector<int> CViaviPCTDriver::ParseIntList(const std::string& csv)
{
    std::vector<int> values;
    std::istringstream iss(csv);
    std::string token;
    while (std::getline(iss, token, ','))
    {
        std::string trimmed = Trim(token);
        if (!trimmed.empty())
        {
            int val = atoi(trimmed.c_str());
            if (val != 0) values.push_back(val);
        }
    }
    return values;
}

std::string CViaviPCTDriver::Trim(const std::string& s)
{
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

} // namespace ViaviPCT
