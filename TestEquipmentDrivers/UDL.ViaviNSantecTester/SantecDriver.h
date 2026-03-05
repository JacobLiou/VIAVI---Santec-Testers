#pragma once

#include "BaseEquipmentDriver.h"

namespace ViaviNSantecTester {

// ---------------------------------------------------------------------------
// Santec 通信适配器接口
// 允许在 TCP、GPIB、USB 或基于 DLL 的通信之间切换。
// ---------------------------------------------------------------------------
class DRIVER_API ISantecCommAdapter
{
public:
    virtual ~ISantecCommAdapter() {}
    virtual bool Open(const std::string& address, int port, double timeout) = 0;
    virtual void Close() = 0;
    virtual bool IsOpen() const = 0;
    virtual std::string SendQuery(const std::string& command) = 0;
    virtual void SendWrite(const std::string& command) = 0;
};

// 默认 TCP 适配器（后续可替换为 GPIB/USB/DLL 适配器）
class DRIVER_API CSantecTcpAdapter : public ISantecCommAdapter
{
public:
    CSantecTcpAdapter();
    virtual ~CSantecTcpAdapter();

    virtual bool Open(const std::string& address, int port, double timeout) override;
    virtual void Close() override;
    virtual bool IsOpen() const override;
    virtual std::string SendQuery(const std::string& command) override;
    virtual void SendWrite(const std::string& command) override;

    void SetReadTimeout(DWORD timeoutMs);

private:
    SOCKET m_socket;
    int    m_bufferSize;
};


// ---------------------------------------------------------------------------
// Santec RL1 驱动（回波损耗测量仪）
//
// SCPI 命令参考：
//   RLM 用户手册 M-RL1-001-07，编程指南（表11和表12）
//
// 通信方式：
//   - TCP/IP 端口 5025（符合 LXI 标准的 SCPI-RAW）
//   - USB 通过 USBTMC（需要 VISA 驱动）
//   - 命令以 \n（换行符）结尾
//   - RL1 同步执行 SCPI 命令
//   - 使用 *OPC? 进行命令间同步
// ---------------------------------------------------------------------------
class DRIVER_API CSantecDriver : public CBaseEquipmentDriver
{
public:
    static const int DEFAULT_PORT           = 5025;
    static const int MEAS_TIMEOUT_MS        = 15000;  // READ:RL? 最多可能需要10秒
    static const int DEFAULT_TIMEOUT_MS     = 5000;
    static const int MAX_POLL_TIME_MS       = 300000;  // 5分钟

    // 通过 *IDN? 识别的设备型号
    enum SantecModel
    {
        MODEL_UNKNOWN = 0,
        MODEL_RL1,          // RL1 自动回波损耗测量仪（插入损耗+回波损耗）
        MODEL_RLM_100,      // RL1 的旧名称别名
        MODEL_ILM_100,      // 插入损耗测量仪（仅插入损耗）
        MODEL_BRM_100       // 背向反射测量仪
    };

    // 回波损耗灵敏度/测量速度（RL:SENSitivity 命令）
    enum RLSensitivity
    {
        SENSITIVITY_FAST = 0,       // 每波长 <1.5秒，回波损耗限制为75 dB
        SENSITIVITY_STANDARD = 1    // 每波长 <5秒，回波损耗可达85 dB
    };

    // 被测器件长度档位（DUT:LENGTH 命令）
    enum DUTLengthBin
    {
        LENGTH_BIN_100  = 100,      // <100米（最快）
        LENGTH_BIN_1500 = 1500,     // <1500米
        LENGTH_BIN_4000 = 4000      // <4000米
    };

    // 回波损耗增益模式（RL:GAIN 命令）
    enum RLGainMode
    {
        GAIN_NORMAL = 0,    // 40至85 dB（推荐用于大多数应用）
        GAIN_LOW    = 1     // 30至40 dB
    };

    // 回波损耗B位置定义（RL:POSB 命令）
    enum RLPosB
    {
        POSB_EOF  = 0,      // 从光纤末端向后（默认）
        POSB_ZERO = 1       // 从零位向前
    };

    CSantecDriver(const std::string& ipAddress,
                  int port = DEFAULT_PORT,
                  double timeout = 5.0,
                  CommType commType = COMM_TCP);
    virtual ~CSantecDriver();

    // IEquipmentDriver 覆盖
    virtual bool Connect() override;
    virtual void Disconnect() override;
    virtual bool IsConnected() const override;
    virtual bool Initialize() override;
    virtual DeviceInfo GetDeviceInfo() override;
    virtual ErrorInfo CheckError() override;
    virtual void ConfigureWavelengths(const std::vector<double>& wavelengths) override;
    virtual void ConfigureChannels(const std::vector<int>& channels) override;
    virtual bool TakeReference(const ReferenceConfig& config) override;
    virtual bool TakeMeasurement() override;
    virtual std::vector<MeasurementResult> GetResults() override;

    // 设置自定义通信适配器（用于 GPIB/USB/DLL 支持）
    void SetCommAdapter(ISantecCommAdapter* adapter, bool takeOwnership = true);

    // Santec 特定访问器
    SantecModel GetModel() const { return m_model; }
    std::string GetFiberType() const { return m_fiberType; }
    std::vector<int> GetSupportedWavelengths() const { return m_supportedWavelengths; }

    // RL1 特定配置（表12命令）
    void SetRLSensitivity(RLSensitivity sens);
    RLSensitivity GetRLSensitivity() const { return m_sensitivity; }

    void SetDUTLength(DUTLengthBin bin);
    DUTLengthBin GetDUTLengthBin() const { return m_dutLengthBin; }

    void SetRLGain(RLGainMode gain);
    RLGainMode GetRLGain() const { return m_rlGain; }

    void SetRLPosB(RLPosB posb);
    RLPosB GetRLPosB() const { return m_rlPosB; }

    void SetLocalMode(bool enabled);
    bool GetLocalMode() const { return m_localMode; }

    void SetAutoStart(bool enabled);
    bool GetAutoStart() const { return m_autoStart; }

    void SetDUTInsertionLoss(double ilDB);
    double GetDUTInsertionLoss() const { return m_dutIL; }

    // 激光控制
    void EnableLaser(int wavelengthNm);
    void DisableLaser();
    int  QueryEnabledLaser();

    // 开关控制
    void SetOutputChannel(int channel);
    int  GetOutputChannel();
    void SetSwitchChannel(int switchNum, int channel);
    int  GetSwitchChannel(int switchNum);

    // 功率计
    int  GetDetectorCount();
    std::string GetDetectorInfo(int detectorNum);

    // 直接同步测量读取（按照官方协议同步执行）
    MeasurementResult ReadRL(int wavelengthNm);
    MeasurementResult ReadRL(int wavelengthNm, double refLenA, double refLenB);
    double ReadIL(int detectorNum, int wavelengthNm);
    double ReadPower(int detectorNum, int wavelengthNm);
    double ReadMonitorPower(int wavelengthNm);

protected:
    virtual bool ValidateConnection() override;

private:
    // 统一命令接口：路由到适配器或基类套接字
    std::string Query(const std::string& command);
    std::string QueryLong(const std::string& command);  // READ:RL? 的延长超时
    void        Write(const std::string& command);

    // 发送命令并等待 OPC（官方同步方法）
    void WriteAndSync(const std::string& command);

    // 辅助函数
    void ParseIdentity(const std::string& idnResponse);
    static std::vector<double> ParseDoubleList(const std::string& csv);
    static std::string Trim(const std::string& s);

    CommType                m_commType;
    ISantecCommAdapter*     m_adapter;
    bool                    m_ownsAdapter;
    SantecModel             m_model;
    DeviceInfo              m_deviceInfo;

    // 设备能力（在 Initialize 期间发现）
    std::string             m_fiberType;        // "SM" 或 "MM"
    std::vector<int>        m_supportedWavelengths;

    // 配置状态
    RLSensitivity           m_sensitivity;
    DUTLengthBin            m_dutLengthBin;
    RLGainMode              m_rlGain;
    RLPosB                  m_rlPosB;
    bool                    m_localMode;
    bool                    m_autoStart;
    double                  m_dutIL;

    std::vector<double>     m_wavelengths;
    std::vector<int>        m_channels;
    bool                    m_referenced;

    // TakeMeasurement() 的缓存结果
    std::vector<MeasurementResult> m_lastResults;
};

} // namespace ViaviNSantecTester
