#pragma once

#include "PCT4AllTypes.h"
#include <vector>
#include <string>

namespace ViaviPCT4All {

// ===========================================================================
// IViaviPCT4AllDriver -- Viavi MAP PCT 全功能驱动接口
//
// 严格对照文档: MAP-PCT Programming Guide 22112369-346 R002
// 覆盖全部 SCPI 指令:
//   - Appendix A: Common SCPI Commands (*CLS, *IDN?, *RST, etc.)
//   - Chapter 2:  Common Cassette Commands (:BUSY?, :CONF?, :INFO?, etc.)
//   - Chapter 3:  System Commands (:SUPer:*, :SYSTem:*)
//   - Chapter 4:  System Integration / Factory Commands (:FACTory:*)
//   - Chapter 5:  PCT Commands (:MEASure:*, :PATH:*, :PMAP:*,
//                                :SENSe:*, :SOURce:*, OPM commands)
//
// 通信方式: TCP (默认端口 8301) 或 VISA
// 协议: SCPI, 命令以 \n 结尾
// ===========================================================================
class PCT4ALL_API IViaviPCT4AllDriver
{
public:
    virtual ~IViaviPCT4AllDriver() {}

    // =====================================================================
    // 连接生命周期
    // =====================================================================
    virtual bool Connect() = 0;
    virtual void Disconnect() = 0;
    virtual bool Reconnect() = 0;
    virtual bool IsConnected() const = 0;
    virtual bool Initialize() = 0;

    // =====================================================================
    // Appendix A: Common SCPI Commands
    // =====================================================================
    virtual void        ClearStatus() = 0;                          // *CLS
    virtual void        SetESE(int value) = 0;                      // *ESE <value>
    virtual int         GetESE() = 0;                               // *ESE?
    virtual int         GetESR() = 0;                               // *ESR?
    virtual IdentificationInfo GetIdentification() = 0;             // *IDN?
    virtual void        OperationComplete() = 0;                    // *OPC
    virtual int         QueryOperationComplete() = 0;               // *OPC?
    virtual void        RecallState(int stateNum) = 0;              // *RCL <n>
    virtual void        ResetDevice() = 0;                          // *RST
    virtual void        SaveState(int stateNum) = 0;                // *SAV <n>
    virtual void        SetSRE(int value) = 0;                      // *SRE <value>
    virtual int         GetSRE() = 0;                               // *SRE?
    virtual int         GetSTB() = 0;                               // *STB?
    virtual int         SelfTest() = 0;                             // *TST?
    virtual void        Wait() = 0;                                 // *WAI

    // =====================================================================
    // Chapter 2: Common Cassette Commands
    // =====================================================================
    virtual int         IsBusy() = 0;                               // :BUSY?
    virtual std::string GetConfig() = 0;                            // :CONFig?
    virtual PCTConfig   GetConfigParsed() = 0;                      // :CONFig? (parsed)
    virtual std::string GetDeviceInformation(int device = 1) = 0;   // :DEVice:INFOrmation? <dev>
    virtual int         GetDeviceFault(int device = 1) = 0;         // :FAULt:DEVice? <dev>
    virtual int         GetSlotFault() = 0;                         // :FAULt:SLOT?
    virtual CassetteInfo GetCassetteInfo() = 0;                     // :INFOrmation?
    virtual void        SetLock(int state, const std::string& name,
                                const std::string& ipID) = 0;      // :LOCK <state>,<name>,<ipID>
    virtual std::string GetLock() = 0;                              // :LOCK?
    virtual void        ResetCassette() = 0;                        // :RESet
    virtual int         GetDeviceStatus(int device = 1) = 0;        // :STATus:DEVice? <dev>
    virtual void        ResetDeviceStatus(int device = 1) = 0;      // :STATus:RESet <dev>
    virtual ErrorInfo   GetSystemError() = 0;                       // :SYSTem:ERRor?
    virtual int         RunCassetteTest() = 0;                      // :TEST?

    // =====================================================================
    // Chapter 3: System Commands (port 8100)
    // =====================================================================
    virtual void        SuperExit(const std::string& name = "PCT") = 0;     // :SUPer:EXIT <NAME>
    virtual void        SuperLaunch(const std::string& name = "PCT") = 0;   // :SUPer:LAUNch <NAME>
    virtual int         GetSuperStatus(const std::string& name = "PCT") = 0;// :SUPer:STATus? <NAME>
    virtual void        SetSystemDate(const std::string& date) = 0;         // :SYSTem:DATe <yyyy,mm,dd>
    virtual std::string GetSystemDate() = 0;                                // :SYSTem:DATe?
    virtual ErrorInfo   GetSystemErrorSys() = 0;                            // :SYSTem:ERRor? (system)
    virtual int         GetChassisFault() = 0;                              // :SYSTem:FAULt:CHASsis?
    virtual int         GetFaultSummary() = 0;                              // :SYSTem:FAULt:SUMMary?
    virtual void        SetGPIBAddress(int address) = 0;                    // :SYSTem:GPIB <n>
    virtual int         GetGPIBAddress() = 0;                               // :SYSTem:GPIB?
    virtual std::string GetSystemInfoRaw() = 0;                             // :SYSTem:INFO?
    virtual SystemInfo  GetSystemInfo() = 0;                                // :SYSTem:INFO? (parsed)
    virtual void        SetInterlock(int state) = 0;                        // :SYSTem:INTLock <n>
    virtual int         GetInterlock() = 0;                                 // :SYSTem:INTLock?
    virtual std::string GetInterlockState() = 0;                            // :SYSTem:INTLock:STATe?
    virtual std::string GetInventory() = 0;                                 // :SYSTem:INVentory?
    virtual std::string GetIPList() = 0;                                    // :SYSTem:IP:LIST?
    virtual std::string GetLayout() = 0;                                    // :SYSTem:LAYout?
    virtual std::string GetLayoutPort() = 0;                                // :SYSTem:LAYout:PORT?
    virtual void        SetLegacyMode(int state) = 0;                       // :SYSTem:LEGAcy:MODE <n>
    virtual int         GetLegacyMode() = 0;                                // :SYSTem:LEGAcy:MODE?
    virtual std::string GetLicenses() = 0;                                  // :SYSTem:LIC?
    virtual void        ReleaseLock(const std::string& slotID) = 0;         // :SYSTem:LOCK:RELEase <slot>
    virtual void        SystemShutdown(int mode) = 0;                       // :SYSTem:SHUTdown <n>
    virtual int         GetSystemStatusReg() = 0;                           // :SYSTem:STATus?
    virtual std::string GetTemperature() = 0;                               // :SYSTem:TEMP?
    virtual std::string GetSystemTime() = 0;                                // :SYSTem:TIMe?

    // =====================================================================
    // Chapter 4: System Integration / Factory Commands
    // =====================================================================
    virtual std::string GetCalibrationStatus() = 0;                 // :FACTory:CALibration?
    virtual std::string GetCalibrationDate() = 0;                   // :FACTory:CALibration:DATe?
    virtual void        FactoryCommit(int step) = 0;                // :FACTory:COMMit <step>
    virtual void        SetFactoryBiDir(int status) = 0;            // :FACTory:CONFig:BIDir <status>
    virtual int         GetFactoryBiDir() = 0;                      // :FACTory:CONFig:BIDir?
    virtual void        SetFactoryCore(int core) = 0;               // :FACTory:CONFig:CORE <core>
    virtual int         GetFactoryCore() = 0;                       // :FACTory:CONFig:CORE?
    virtual void        SetFactoryLowPower(int status) = 0;         // :FACTory:CONFig:LPOW <status>
    virtual int         GetFactoryLowPower() = 0;                   // :FACTory:CONFig:LPOW?
    virtual void        SetFactoryOPM(const std::string& status) = 0;   // :FACTory:CONFig:OPM <status>
    virtual std::string GetFactoryOPM() = 0;                        // :FACTory:CONFig:OPM?
    virtual void        SetFactorySwitch(int sw,
                                         const std::string& status) = 0;// :FACTory:CONFig:SWITch <sw>,<status>
    virtual std::string GetFactorySwitch(int sw) = 0;               // :FACTory:CONFig:SWITch? <sw>
    virtual void        SetFactorySwitchSize(int sw, int size) = 0; // :FACTory:CONFig:SWITch:SIZe <sw>,<size>
    virtual std::string GetFactoryFPDistance(int ilf) = 0;          // :FACTory:MEASure:FPDistance? <ILF>
    virtual std::string GetFactoryFPLoss(int ilf) = 0;              // :FACTory:MEASure:FPLoss? <ILF>
    virtual std::string GetFactoryFPRatio(int ilf) = 0;             // :FACTory:MEASure:FPRatio? <ILF>
    virtual std::string GetFactoryLoop(int ilf) = 0;                // :FACTory:MEASure:LOOP? <ILF>
    virtual std::string GetFactoryOPMP(int index) = 0;              // :FACTory:MEASure:OPMP? <index>
    virtual std::string GetFactoryRange(int fiber) = 0;             // :FACTory:MEASure:RANGe? <fiber>
    virtual void        StartFactoryMeasure(int step = -1) = 0;    // :FACTory:MEASure:STARt [<step>]
    virtual std::string GetFactorySWDistance(int channel,
                                             int sw) = 0;          // :FACTory:MEASure:SWDistance? <ch>,<sw>
    virtual std::string GetFactorySWLoss(int channel,
                                          int sw) = 0;             // :FACTory:MEASure:SWLoss? <ch>,<sw>
    virtual void        SetFactorySetupSwitch(int sw) = 0;          // :FACTory:SETup:SWITch <sw>
    virtual int         GetFactorySetupSwitch() = 0;                // :FACTory:SETup:SWITch?
    virtual void        FactoryReset(int step = -1) = 0;            // :FACTory:RESet [<step>]

    // =====================================================================
    // Chapter 5: PCT - Active / Remote
    // =====================================================================
    virtual int         GetActive() = 0;                            // *ACTive?
    virtual void        SetRemote() = 0;                            // *REM

    // =====================================================================
    // Chapter 5: PCT - Measurement Commands
    // =====================================================================
    virtual std::string GetMeasureAll(int launchCh,
                                      int recvCh = -1) = 0;        // :MEASure:ALL? <lch>[,<rch>]
    virtual std::string GetDistance() = 0;                          // :MEASure:DISTance?
    virtual void        StartFastIL(int wavelength, double aTime,
                                    double delay, int startCh,
                                    int endCh) = 0;                // :MEASure:FASTil <wl>,<at>,<d>,<s>,<e>
    virtual std::string GetFastIL() = 0;                           // :MEASure:FASTil?
    virtual void        SetHelixFactor(double value) = 0;          // :MEASure:HELix <value>
    virtual double      GetHelixFactor() = 0;                      // :MEASure:HELix?
    virtual std::string GetIL() = 0;                               // :MEASure:IL?
    virtual void        SetILLA(int state) = 0;                    // :MEASure:ILLA <state>
    virtual int         GetILLA() = 0;                             // :MEASure:ILLA?
    virtual std::string GetLength() = 0;                           // :MEASure:LENGth?
    virtual std::string GetORL(int method, int origin,
                               double aOffset, double bOffset) = 0;// :MEASure:ORL? <m>,<o>,<a>,<b>
    virtual std::string GetORLPreset(int origin) = 0;              // :MEASure:ORL:PRESet? <origin>
    virtual void        SetORLSetupPreset(int zone,
                                          int preset) = 0;        // :MEASure:ORL:SETup <z>,<preset>
    virtual void        SetORLSetupCustom(int zone, int method,
                                          int origin, double aOff,
                                          double bOff) = 0;       // :MEASure:ORL:SETup <z>,<m>,<o>,<a>,<b>
    virtual std::string GetORLSetup(int zone) = 0;                 // :MEASure:ORL:SETup? <zone>
    virtual std::string GetORLZone(int zone) = 0;                  // :MEASure:ORL:ZONe? <zone>
    virtual std::string GetPower() = 0;                            // :MEASure:POWer?
    virtual void        SetRef2Step(int state) = 0;                // :MEASure:REF2step <state>
    virtual int         GetRef2Step() = 0;                         // :MEASure:REF2step?
    virtual void        SetRefAlt(int state) = 0;                  // :MEASure:REFAlt <state>
    virtual int         GetRefAlt() = 0;                           // :MEASure:REFAlt?
    virtual void        MeasureReset() = 0;                        // :MEASure:RESet
    virtual void        StartSEIL(int wavelength, int launchCh,
                                  int recvCh, int aTime,
                                  double binWidth,
                                  int adjustIL) = 0;              // :MEASure:SEIL <wl>,<lch>,<rch>,<at>,<bw>,<adj>
    virtual std::string GetSEIL() = 0;                             // :MEASure:SEIL?
    virtual void        StartMeasurement() = 0;                    // :MEASure:STARt
    virtual int         GetMeasurementState() = 0;                 // :MEASure:STATe?
    virtual void        StopMeasurement() = 0;                     // :MEASure:STOP

    // =====================================================================
    // Chapter 5: PCT - PATH Commands
    // =====================================================================
    virtual void        SetBiDir(int state) = 0;                   // :PATH:BIDIR <state>
    virtual int         GetBiDir() = 0;                            // :PATH:BIDIR?
    virtual void        SetChannel(int group, int channel) = 0;    // :PATH:CHANnel <group>,<ch>
    virtual int         GetChannel(int group) = 0;                 // :PATH:CHANnel? <group>
    virtual int         GetAvailableChannels(int group) = 0;       // :PATH:CHANnel:AVAilable? <group>
    virtual void        SetConnection(int mode) = 0;               // :PATH:CONNection <mode>
    virtual int         GetConnection() = 0;                       // :PATH:CONNection?
    virtual void        SetDUTLength(double length) = 0;           // :PATH:DUT:LENGth <length>
    virtual double      GetDUTLength() = 0;                        // :PATH:DUT:LENGth?
    virtual void        SetDUTLengthAuto(int state) = 0;           // :PATH:DUT:LENGth:AUTO <state>
    virtual int         GetDUTLengthAuto() = 0;                    // :PATH:DUT:LENGth:AUTO?
    virtual void        SetEOFMin(double distance) = 0;            // :PATH:EOF:MIN <distance>
    virtual double      GetEOFMin() = 0;                           // :PATH:EOF:MIN?
    virtual void        SetJumperIL(int group, int channel,
                                    double il) = 0;                // :PATH:JUMPer:IL <g>,<ch>,<il>
    virtual double      GetJumperIL(int group, int channel) = 0;   // :PATH:JUMPer:IL? <g>,<ch>
    virtual void        SetJumperILAuto(int group, int channel,
                                        int state) = 0;           // :PATH:JUMPer:IL:AUTO <g>,<ch>,<s>
    virtual int         GetJumperILAuto(int group,
                                        int channel) = 0;         // :PATH:JUMPer:IL:AUTO? <g>,<ch>
    virtual void        SetJumperLength(int group, int channel,
                                        double length) = 0;       // :PATH:JUMPer:LENGth <g>,<ch>,<l>
    virtual double      GetJumperLength(int group,
                                        int channel) = 0;         // :PATH:JUMPer:LENGth? <g>,<ch>
    virtual void        SetJumperLengthAuto(int group, int channel,
                                            int state) = 0;       // :PATH:JUMPer:LENGth:AUTO <g>,<ch>,<s>
    virtual int         GetJumperLengthAuto(int group,
                                            int channel) = 0;     // :PATH:JUMPer:LENGth:AUTO? <g>,<ch>
    virtual void        ResetJumper(int group = 0,
                                    int channel = 0) = 0;         // :PATH:JUMPer:RESet [<g>][,<ch>]
    virtual void        ResetJumperMeasure(int group = 0,
                                           int channel = 0) = 0;  // :PATH:JUMPer:RESet:MEASure [<g>][,<ch>]
    virtual void        SetLaunch(int port) = 0;                   // :PATH:LAUNch <port>
    virtual int         GetLaunch() = 0;                           // :PATH:LAUNch?
    virtual int         GetLaunchAvailable() = 0;                  // :PATH:LAUNch:AVAilable?
    virtual void        SetPathList(int sw,
                                    const std::string& channels) = 0;// :PATH:LIST <sw>,<channels>
    virtual std::string GetPathList(int sw) = 0;                   // :PATH:LIST? <sw>
    virtual void        SetReceive(int state) = 0;                 // :PATH:RECeive <state>
    virtual int         GetReceive() = 0;                          // :PATH:RECeive?

    // =====================================================================
    // Chapter 5: PCT - Port Map Commands
    // =====================================================================
    virtual void        SetPortMapEnable(int state) = 0;           // :PMAP:ENABle <state>
    virtual int         GetPortMapEnable() = 0;                    // :PMAP:ENABle?
    virtual void        PortMapMeasureAll() = 0;                   // :PMAP:MEASure:ALL
    virtual void        SetPortMapLive(int channel) = 0;           // :PMAP:MEASure:LIVE <ch>
    virtual int         GetPortMapLive(int channel) = 0;           // :PMAP:MEASure:LIVE? <ch>
    virtual void        PortMapValidate() = 0;                     // :PMAP:MEASure:VALid
    virtual std::string GetPortMapValidation() = 0;                // :PMAP:MEASure:VALid?
    virtual std::string GetPortMapLink(int path) = 0;              // :PMAP:PATH:LINK? <path>
    virtual void        SetPortMapSelect(int path) = 0;            // :PMAP:PATH:SELect <path>
    virtual int         GetPortMapSelect() = 0;                    // :PMAP:PATH:SELect?
    virtual int         GetPortMapPathSize() = 0;                  // :PMAP:PATH:SIZE?
    virtual int         GetPortMapFirst(int sw) = 0;               // :PMAP:SETup:FIRSt? <sw>
    virtual void        PortMapInitList(const std::string& listSW1,
                                        const std::string& listSW2,
                                        int mode) = 0;            // :PMAP:SETup:INIT:LIST <l1>,<l2>,<m>
    virtual void        PortMapInitRange(int startSW1, int startSW2,
                                         int size, int mode) = 0; // :PMAP:SETup:INIT:RANGe <s1>,<s2>,<sz>,<m>
    virtual int         GetPortMapLast(int sw) = 0;                // :PMAP:SETup:LAST? <sw>
    virtual void        SetPortMapLink(int fixedCh,
                                       int variableCh) = 0;       // :PMAP:SETup:LINK <f>,<v>
    virtual std::string GetPortMapList(int sw) = 0;                // :PMAP:SETup:LIST? <sw>
    virtual void        SetPortMapLock(int state) = 0;             // :PMAP:SETup:LOCK <state>
    virtual int         GetPortMapLock() = 0;                      // :PMAP:SETup:LOCK?
    virtual int         GetPortMapMode() = 0;                      // :PMAP:SETup:MODE?
    virtual void        PortMapReset() = 0;                        // :PMAP:SETup:RESet
    virtual int         GetPortMapSetupSize() = 0;                 // :PMAP:SETup:SIZE?

    // =====================================================================
    // Chapter 5: PCT - Sense Commands
    // =====================================================================
    virtual void        SetAveragingTime(int seconds) = 0;         // :SENSe:ATIMe <secs>
    virtual int         GetAveragingTime() = 0;                    // :SENSe:ATIMe?
    virtual std::string GetAvailableAveragingTimes() = 0;          // :SENSe:ATIMe:AVAilable?
    virtual void        SetFunction(int mode) = 0;                 // :SENSe:FUNCtion <mode>
    virtual int         GetFunction() = 0;                         // :SENSe:FUNCtion?
    virtual void        SetILOnly(int state) = 0;                  // :SENSe:ILONly <state>
    virtual int         GetILOnly() = 0;                           // :SENSe:ILONly?
    virtual void        SetOPM(int index) = 0;                     // :SENSe:OPM <index>
    virtual int         GetOPM() = 0;                              // :SENSe:OPM?
    virtual void        SetRange(int range) = 0;                   // :SENSe:RANGe <range>
    virtual int         GetRange() = 0;                            // :SENSe:RANGe?
    virtual void        SetTempSensitivity(int level) = 0;         // :SENSe:TEMPerature <level>
    virtual int         GetTempSensitivity() = 0;                  // :SENSe:TEMPerature?

    // =====================================================================
    // Chapter 5: PCT - Source Commands
    // =====================================================================
    virtual void        SetContinuous(int state) = 0;              // :SOURce:CONTinuous <state>
    virtual int         GetContinuous() = 0;                       // :SOURce:CONTinuous?
    virtual void        SetSourceList(const std::string& wl) = 0;  // :SOURce:LIST <wl1>,<wl2>,...
    virtual std::string GetSourceList() = 0;                       // :SOURce:LIST?
    virtual void        SetWarmup(const std::string& wl) = 0;      // :SOURce:WARMup <wl|None>
    virtual std::string GetWarmup() = 0;                           // :SOURce:WARMup?
    virtual void        SetWavelength(int wavelength) = 0;         // :SOURce:WAVelength <nm>
    virtual std::string GetWavelength() = 0;                       // :SOURce:WAVelength?
    virtual std::string GetAvailableWavelengths() = 0;             // :SOURce:WAVelength:AVAilable?

    // =====================================================================
    // Chapter 5: PCT - System Warning
    // =====================================================================
    virtual ErrorInfo   GetWarning() = 0;                          // :SYSTem:WARNing?

    // =====================================================================
    // Chapter 5: PCT - OPM Commands
    // =====================================================================
    virtual std::string FetchLoss() = 0;                           // :FETCh:LOSS?
    virtual std::string FetchORL() = 0;                            // :FETCh:ORL?
    virtual std::string FetchPower() = 0;                          // :FETCh:POWer?
    virtual void        StartDarkMeasure() = 0;                    // :SENSe:POWer:DARK
    virtual int         GetDarkStatus() = 0;                       // :SENSe:POWer:DARK?
    virtual void        RestoreDarkFactory() = 0;                  // :SENSe:POWer:DARK:FACTory
    virtual void        SetPowerMode(int mode) = 0;                // :SENSe:POWer:MODE <mode>
    virtual int         GetPowerMode() = 0;                        // :SENSe:POWer:MODE?

    // =====================================================================
    // 高级工作流
    // =====================================================================
    virtual bool        WaitForIdle(int timeoutMs = 180000) = 0;
    virtual bool        WaitForMeasurement(int timeoutMs = 180000) = 0;

    // =====================================================================
    // 原始 SCPI 透传
    // =====================================================================
    virtual std::string SendRawQuery(const std::string& command) = 0;
    virtual void        SendRawWrite(const std::string& command) = 0;
};

} // namespace ViaviPCT4All
