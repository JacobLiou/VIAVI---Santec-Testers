#pragma once

#include "IViaviPCT4AllDriver.h"
#include "IViaviPCT4AllCommAdapter.h"
#include "Logger.h"
#include <string>
#include <vector>
#include <atomic>

namespace ViaviPCT4All {

class PCT4ALL_API CViaviPCT4AllDriver : public IViaviPCT4AllDriver
{
public:
    static const int DEFAULT_PORT           = 8301;
    static const int SYSTEM_PORT            = 8100;
    static const int MEAS_TIMEOUT_MS        = 180000;
    static const int DEFAULT_TIMEOUT_MS     = 5000;
    static const int POLL_INTERVAL_MS       = 500;

    CViaviPCT4AllDriver(const std::string& address,
                        int port = DEFAULT_PORT,
                        double timeout = 5.0,
                        CommType commType = COMM_TCP);
    virtual ~CViaviPCT4AllDriver();

    // -- Connection --
    virtual bool Connect() override;
    virtual void Disconnect() override;
    virtual bool Reconnect() override;
    virtual bool IsConnected() const override;
    virtual bool Initialize() override;

    // -- Common SCPI (Appendix A) --
    virtual void        ClearStatus() override;
    virtual void        SetESE(int value) override;
    virtual int         GetESE() override;
    virtual int         GetESR() override;
    virtual IdentificationInfo GetIdentification() override;
    virtual void        OperationComplete() override;
    virtual int         QueryOperationComplete() override;
    virtual void        RecallState(int stateNum) override;
    virtual void        ResetDevice() override;
    virtual void        SaveState(int stateNum) override;
    virtual void        SetSRE(int value) override;
    virtual int         GetSRE() override;
    virtual int         GetSTB() override;
    virtual int         SelfTest() override;
    virtual void        Wait() override;

    // -- Common Cassette (Chapter 2) --
    virtual int         IsBusy() override;
    virtual std::string GetConfig() override;
    virtual PCTConfig   GetConfigParsed() override;
    virtual std::string GetDeviceInformation(int device = 1) override;
    virtual int         GetDeviceFault(int device = 1) override;
    virtual int         GetSlotFault() override;
    virtual CassetteInfo GetCassetteInfo() override;
    virtual void        SetLock(int state, const std::string& name,
                                const std::string& ipID) override;
    virtual std::string GetLock() override;
    virtual void        ResetCassette() override;
    virtual int         GetDeviceStatus(int device = 1) override;
    virtual void        ResetDeviceStatus(int device = 1) override;
    virtual ErrorInfo   GetSystemError() override;
    virtual int         RunCassetteTest() override;

    // -- System Commands (Chapter 3) --
    virtual void        SuperExit(const std::string& name = "PCT") override;
    virtual void        SuperLaunch(const std::string& name = "PCT") override;
    virtual int         GetSuperStatus(const std::string& name = "PCT") override;
    virtual void        SetSystemDate(const std::string& date) override;
    virtual std::string GetSystemDate() override;
    virtual ErrorInfo   GetSystemErrorSys() override;
    virtual int         GetChassisFault() override;
    virtual int         GetFaultSummary() override;
    virtual void        SetGPIBAddress(int address) override;
    virtual int         GetGPIBAddress() override;
    virtual std::string GetSystemInfoRaw() override;
    virtual SystemInfo  GetSystemInfo() override;
    virtual void        SetInterlock(int state) override;
    virtual int         GetInterlock() override;
    virtual std::string GetInterlockState() override;
    virtual std::string GetInventory() override;
    virtual std::string GetIPList() override;
    virtual std::string GetLayout() override;
    virtual std::string GetLayoutPort() override;
    virtual void        SetLegacyMode(int state) override;
    virtual int         GetLegacyMode() override;
    virtual std::string GetLicenses() override;
    virtual void        ReleaseLock(const std::string& slotID) override;
    virtual void        SystemShutdown(int mode) override;
    virtual int         GetSystemStatusReg() override;
    virtual std::string GetTemperature() override;
    virtual std::string GetSystemTime() override;

    // -- Factory Commands (Chapter 4) --
    virtual std::string GetCalibrationStatus() override;
    virtual std::string GetCalibrationDate() override;
    virtual void        FactoryCommit(int step) override;
    virtual void        SetFactoryBiDir(int status) override;
    virtual int         GetFactoryBiDir() override;
    virtual void        SetFactoryCore(int core) override;
    virtual int         GetFactoryCore() override;
    virtual void        SetFactoryLowPower(int status) override;
    virtual int         GetFactoryLowPower() override;
    virtual void        SetFactoryOPM(const std::string& status) override;
    virtual std::string GetFactoryOPM() override;
    virtual void        SetFactorySwitch(int sw, const std::string& status) override;
    virtual std::string GetFactorySwitch(int sw) override;
    virtual void        SetFactorySwitchSize(int sw, int size) override;
    virtual std::string GetFactoryFPDistance(int ilf) override;
    virtual std::string GetFactoryFPLoss(int ilf) override;
    virtual std::string GetFactoryFPRatio(int ilf) override;
    virtual std::string GetFactoryLoop(int ilf) override;
    virtual std::string GetFactoryOPMP(int index) override;
    virtual std::string GetFactoryRange(int fiber) override;
    virtual void        StartFactoryMeasure(int step = -1) override;
    virtual std::string GetFactorySWDistance(int channel, int sw) override;
    virtual std::string GetFactorySWLoss(int channel, int sw) override;
    virtual void        SetFactorySetupSwitch(int sw) override;
    virtual int         GetFactorySetupSwitch() override;
    virtual void        FactoryReset(int step = -1) override;

    // -- PCT Active/Remote --
    virtual int         GetActive() override;
    virtual void        SetRemote() override;

    // -- PCT Measurement --
    virtual std::string GetMeasureAll(int launchCh, int recvCh = -1) override;
    virtual std::string GetDistance() override;
    virtual void        StartFastIL(int wavelength, double aTime,
                                    double delay, int startCh, int endCh) override;
    virtual std::string GetFastIL() override;
    virtual void        SetHelixFactor(double value) override;
    virtual double      GetHelixFactor() override;
    virtual std::string GetIL() override;
    virtual void        SetILLA(int state) override;
    virtual int         GetILLA() override;
    virtual std::string GetLength() override;
    virtual std::string GetORL(int method, int origin,
                               double aOffset, double bOffset) override;
    virtual std::string GetORLPreset(int origin) override;
    virtual void        SetORLSetupPreset(int zone, int preset) override;
    virtual void        SetORLSetupCustom(int zone, int method, int origin,
                                          double aOff, double bOff) override;
    virtual std::string GetORLSetup(int zone) override;
    virtual std::string GetORLZone(int zone) override;
    virtual std::string GetPower() override;
    virtual void        SetRef2Step(int state) override;
    virtual int         GetRef2Step() override;
    virtual void        SetRefAlt(int state) override;
    virtual int         GetRefAlt() override;
    virtual void        MeasureReset() override;
    virtual void        StartSEIL(int wavelength, int launchCh, int recvCh,
                                  int aTime, double binWidth, int adjustIL) override;
    virtual std::string GetSEIL() override;
    virtual void        StartMeasurement() override;
    virtual int         GetMeasurementState() override;
    virtual void        StopMeasurement() override;

    // -- PATH --
    virtual void        SetBiDir(int state) override;
    virtual int         GetBiDir() override;
    virtual void        SetChannel(int group, int channel) override;
    virtual int         GetChannel(int group) override;
    virtual int         GetAvailableChannels(int group) override;
    virtual void        SetConnection(int mode) override;
    virtual int         GetConnection() override;
    virtual void        SetDUTLength(double length) override;
    virtual double      GetDUTLength() override;
    virtual void        SetDUTLengthAuto(int state) override;
    virtual int         GetDUTLengthAuto() override;
    virtual void        SetEOFMin(double distance) override;
    virtual double      GetEOFMin() override;
    virtual void        SetJumperIL(int group, int channel, double il) override;
    virtual double      GetJumperIL(int group, int channel) override;
    virtual void        SetJumperILAuto(int group, int channel, int state) override;
    virtual int         GetJumperILAuto(int group, int channel) override;
    virtual void        SetJumperLength(int group, int channel, double length) override;
    virtual double      GetJumperLength(int group, int channel) override;
    virtual void        SetJumperLengthAuto(int group, int channel, int state) override;
    virtual int         GetJumperLengthAuto(int group, int channel) override;
    virtual void        ResetJumper(int group = 0, int channel = 0) override;
    virtual void        ResetJumperMeasure(int group = 0, int channel = 0) override;
    virtual void        SetLaunch(int port) override;
    virtual int         GetLaunch() override;
    virtual int         GetLaunchAvailable() override;
    virtual void        SetPathList(int sw, const std::string& channels) override;
    virtual std::string GetPathList(int sw) override;
    virtual void        SetReceive(int state) override;
    virtual int         GetReceive() override;

    // -- Port Map --
    virtual void        SetPortMapEnable(int state) override;
    virtual int         GetPortMapEnable() override;
    virtual void        PortMapMeasureAll() override;
    virtual void        SetPortMapLive(int channel) override;
    virtual int         GetPortMapLive(int channel) override;
    virtual void        PortMapValidate() override;
    virtual std::string GetPortMapValidation() override;
    virtual std::string GetPortMapLink(int path) override;
    virtual void        SetPortMapSelect(int path) override;
    virtual int         GetPortMapSelect() override;
    virtual int         GetPortMapPathSize() override;
    virtual int         GetPortMapFirst(int sw) override;
    virtual void        PortMapInitList(const std::string& listSW1,
                                        const std::string& listSW2, int mode) override;
    virtual void        PortMapInitRange(int startSW1, int startSW2,
                                         int size, int mode) override;
    virtual int         GetPortMapLast(int sw) override;
    virtual void        SetPortMapLink(int fixedCh, int variableCh) override;
    virtual std::string GetPortMapList(int sw) override;
    virtual void        SetPortMapLock(int state) override;
    virtual int         GetPortMapLock() override;
    virtual int         GetPortMapMode() override;
    virtual void        PortMapReset() override;
    virtual int         GetPortMapSetupSize() override;

    // -- Sense --
    virtual void        SetAveragingTime(int seconds) override;
    virtual int         GetAveragingTime() override;
    virtual std::string GetAvailableAveragingTimes() override;
    virtual void        SetFunction(int mode) override;
    virtual int         GetFunction() override;
    virtual void        SetILOnly(int state) override;
    virtual int         GetILOnly() override;
    virtual void        SetOPM(int index) override;
    virtual int         GetOPM() override;
    virtual void        SetRange(int range) override;
    virtual int         GetRange() override;
    virtual void        SetTempSensitivity(int level) override;
    virtual int         GetTempSensitivity() override;

    // -- Source --
    virtual void        SetContinuous(int state) override;
    virtual int         GetContinuous() override;
    virtual void        SetSourceList(const std::string& wl) override;
    virtual std::string GetSourceList() override;
    virtual void        SetWarmup(const std::string& wl) override;
    virtual std::string GetWarmup() override;
    virtual void        SetWavelength(int wavelength) override;
    virtual std::string GetWavelength() override;
    virtual std::string GetAvailableWavelengths() override;

    // -- Warning --
    virtual ErrorInfo   GetWarning() override;

    // -- OPM --
    virtual std::string FetchLoss() override;
    virtual std::string FetchORL() override;
    virtual std::string FetchPower() override;
    virtual void        StartDarkMeasure() override;
    virtual int         GetDarkStatus() override;
    virtual void        RestoreDarkFactory() override;
    virtual void        SetPowerMode(int mode) override;
    virtual int         GetPowerMode() override;

    // -- Workflow --
    virtual bool        WaitForIdle(int timeoutMs = 180000) override;
    virtual bool        WaitForMeasurement(int timeoutMs = 180000) override;

    // -- Raw SCPI --
    virtual std::string SendRawQuery(const std::string& command) override;
    virtual void        SendRawWrite(const std::string& command) override;

    // -- Accessors --
    void SetCommAdapter(IViaviPCT4AllCommAdapter* adapter, bool takeOwnership = true);
    ConnectionState GetState() const { return m_state; }
    CLogger& GetLogger() { return m_logger; }

private:
    std::string Query(const std::string& command);
    std::string QueryLong(const std::string& command);
    void        Write(const std::string& command);
    void        ValidateConnection();

    ErrorInfo   ParseError(const std::string& response);
    PCTConfig   ParseConfig(const std::string& response);
    CassetteInfo ParseCassetteInfo(const std::string& response);
    IdentificationInfo ParseIDN(const std::string& response);
    SystemInfo  ParseSystemInfo(const std::string& response);

    static std::string Trim(const std::string& s);
    static int SafeAtoi(const std::string& s);
    static double SafeAtof(const std::string& s);

    CommType                    m_commType;
    IViaviPCT4AllCommAdapter*   m_adapter;
    bool                        m_ownsAdapter;
    ConnectionConfig            m_config;
    ConnectionState             m_state;
    CLogger                     m_logger;
};

} // namespace ViaviPCT4All
