#pragma once

#include "BaseEquipmentDriver.h"

namespace ViaviNSantecTester {

// ---------------------------------------------------------------------------
// Communication adapter interface for Santec
// Allows swapping between TCP, GPIB, USB, or DLL-based communication.
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

// Default TCP adapter (can be replaced with GPIB/USB/DLL adapters later)
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
// Santec Driver for RL1 (Return Loss Meter)
//
// SCPI Command Reference:
//   RLM User Manual M-RL1-001-07, Programming Guide (Table 11 & Table 12)
//
// Communication:
//   - TCP/IP on port 5025 (SCPI-RAW per LXI standard)
//   - USB via USBTMC (requires VISA drivers)
//   - Commands terminated with \n (linefeed)
//   - RL1 runs SCPI commands synchronously
//   - Use *OPC? for synchronization between commands
// ---------------------------------------------------------------------------
class DRIVER_API CSantecDriver : public CBaseEquipmentDriver
{
public:
    static const int DEFAULT_PORT           = 5025;
    static const int MEAS_TIMEOUT_MS        = 15000;  // READ:RL? can take up to 10s
    static const int DEFAULT_TIMEOUT_MS     = 5000;
    static const int MAX_POLL_TIME_MS       = 300000;  // 5 minutes

    // Device model as identified from *IDN?
    enum SantecModel
    {
        MODEL_UNKNOWN = 0,
        MODEL_RL1,          // RL1 Automated Return Loss Meter (IL+RL)
        MODEL_RLM_100,      // Legacy name alias for RL1
        MODEL_ILM_100,      // Insertion Loss Meter (IL only)
        MODEL_BRM_100       // Backreflection Meter
    };

    // RL sensitivity / measurement speed (RL:SENSitivity command)
    enum RLSensitivity
    {
        SENSITIVITY_FAST = 0,       // <1.5s per wavelength, RL limited to 75 dB
        SENSITIVITY_STANDARD = 1    // <5s per wavelength, RL up to 85 dB
    };

    // DUT length bin (DUT:LENGTH command)
    enum DUTLengthBin
    {
        LENGTH_BIN_100  = 100,      // <100m (fastest)
        LENGTH_BIN_1500 = 1500,     // <1500m
        LENGTH_BIN_4000 = 4000      // <4000m
    };

    // RL gain mode (RL:GAIN command)
    enum RLGainMode
    {
        GAIN_NORMAL = 0,    // 40 to 85 dB (recommended for most applications)
        GAIN_LOW    = 1     // 30 to 40 dB
    };

    // RLB position definition (RL:POSB command)
    enum RLPosB
    {
        POSB_EOF  = 0,      // backward from end of fiber (default)
        POSB_ZERO = 1       // forward from position zero
    };

    CSantecDriver(const std::string& ipAddress,
                  int port = DEFAULT_PORT,
                  double timeout = 5.0,
                  CommType commType = COMM_TCP);
    virtual ~CSantecDriver();

    // IEquipmentDriver overrides
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

    // Set a custom communication adapter (for GPIB/USB/DLL support)
    void SetCommAdapter(ISantecCommAdapter* adapter, bool takeOwnership = true);

    // Santec-specific accessors
    SantecModel GetModel() const { return m_model; }
    std::string GetFiberType() const { return m_fiberType; }
    std::vector<int> GetSupportedWavelengths() const { return m_supportedWavelengths; }

    // RL1-specific configuration (Table 12 commands)
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

    // Laser control
    void EnableLaser(int wavelengthNm);
    void DisableLaser();
    int  QueryEnabledLaser();

    // Switch control
    void SetOutputChannel(int channel);
    int  GetOutputChannel();
    void SetSwitchChannel(int switchNum, int channel);
    int  GetSwitchChannel(int switchNum);

    // Power meter
    int  GetDetectorCount();
    std::string GetDetectorInfo(int detectorNum);

    // Direct measurement reads (synchronous per official protocol)
    MeasurementResult ReadRL(int wavelengthNm);
    MeasurementResult ReadRL(int wavelengthNm, double refLenA, double refLenB);
    double ReadIL(int detectorNum, int wavelengthNm);
    double ReadPower(int detectorNum, int wavelengthNm);
    double ReadMonitorPower(int wavelengthNm);

protected:
    virtual bool ValidateConnection() override;

private:
    // Unified command interface: routes to adapter or base socket
    std::string Query(const std::string& command);
    std::string QueryLong(const std::string& command);  // extended timeout for READ:RL?
    void        Write(const std::string& command);

    // Send command and wait for OPC (official synchronization method)
    void WriteAndSync(const std::string& command);

    // Helpers
    void ParseIdentity(const std::string& idnResponse);
    static std::vector<double> ParseDoubleList(const std::string& csv);
    static std::string Trim(const std::string& s);

    CommType                m_commType;
    ISantecCommAdapter*     m_adapter;
    bool                    m_ownsAdapter;
    SantecModel             m_model;
    DeviceInfo              m_deviceInfo;

    // Device capabilities (discovered during Initialize)
    std::string             m_fiberType;        // "SM" or "MM"
    std::vector<int>        m_supportedWavelengths;

    // Configuration state
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

    // Cached results from TakeMeasurement()
    std::vector<MeasurementResult> m_lastResults;
};

} // namespace ViaviNSantecTester
