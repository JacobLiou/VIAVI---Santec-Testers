# UDL.ViaviPCT4All API Reference

Viavi MAP PCT 全功能驱动 DLL，严格对照 **MAP-PCT Programming Guide 22112369-346 R002**。

## 概述

- 通信协议: SCPI, 命令以 `\n` 结尾
- 通信方式: TCP (端口 8301) 或 VISA
- DLL 前缀: `PCT4_`
- 命名空间: `ViaviPCT4All`

## DLL 导出函数

### 驱动生命周期

| 函数 | 说明 |
|------|------|
| `PCT4_CreateDriver(type, ip, port, slot)` | 创建 TCP 驱动实例 |
| `PCT4_CreateDriverEx(type, address, port, slot, commType)` | 创建驱动（指定通信类型） |
| `PCT4_DestroyDriver(hDriver)` | 销毁驱动 |

### 连接

| 函数 | 说明 |
|------|------|
| `PCT4_Connect(hDriver)` | 建立连接 |
| `PCT4_Disconnect(hDriver)` | 断开连接 |
| `PCT4_Initialize(hDriver)` | 初始化 (发送 *REM, 读取 :CONF?) |
| `PCT4_IsConnected(hDriver)` | 检查连接状态 |

### 测量控制

| 函数 | 对应 SCPI |
|------|-----------|
| `PCT4_StartMeasurement` | `:MEASure:STARt` |
| `PCT4_StopMeasurement` | `:MEASure:STOP` |
| `PCT4_GetMeasurementState` | `:MEASure:STATe?` |
| `PCT4_WaitForIdle` | 轮询 `:MEASure:STATe?` 直到 IDLE |
| `PCT4_MeasureReset` | `:MEASure:RESet` |

### 配置

| 函数 | 对应 SCPI |
|------|-----------|
| `PCT4_SetFunction(mode)` | `:SENSe:FUNCtion <mode>` |
| `PCT4_SetWavelength(nm)` | `:SOURce:WAVelength <nm>` |
| `PCT4_SetSourceList(wl)` | `:SOURce:LIST <wl>` |
| `PCT4_SetAveragingTime(s)` | `:SENSe:ATIMe <s>` |
| `PCT4_SetRange(range)` | `:SENSe:RANGe <range>` |
| `PCT4_SetILOnly(state)` | `:SENSe:ILONly <state>` |
| `PCT4_SetConnection(mode)` | `:PATH:CONNection <mode>` |
| `PCT4_SetBiDir(state)` | `:PATH:BIDIR <state>` |

### 光路控制 (含光开关)

| 函数 | 对应 SCPI |
|------|-----------|
| `PCT4_SetChannel(group, ch)` | `:PATH:CHANnel <group>,<ch>` |
| `PCT4_GetChannel(group)` | `:PATH:CHANnel? <group>` |
| `PCT4_SetPathList(sw, ch)` | `:PATH:LIST <sw>,<channels>` |
| `PCT4_SetLaunch(port)` | `:PATH:LAUNch <port>` |

### 原始 SCPI

| 函数 | 说明 |
|------|------|
| `PCT4_SendCommand(cmd, resp, size)` | 发送查询获取响应 |
| `PCT4_SendWrite(cmd)` | 发送写命令 |

## C++ 接口 (IViaviPCT4AllDriver)

完整接口覆盖文档全部章节:

### Appendix A: Common SCPI
`ClearStatus`, `SetESE/GetESE`, `GetESR`, `GetIdentification`, `OperationComplete/QueryOperationComplete`, `RecallState`, `ResetDevice`, `SaveState`, `SetSRE/GetSRE`, `GetSTB`, `SelfTest`, `Wait`

### Chapter 2: Common Cassette Commands
`IsBusy`, `GetConfig/GetConfigParsed`, `GetDeviceInformation`, `GetDeviceFault`, `GetSlotFault`, `GetCassetteInfo`, `SetLock/GetLock`, `ResetCassette`, `GetDeviceStatus`, `ResetDeviceStatus`, `GetSystemError`, `RunCassetteTest`

### Chapter 3: System Commands
`SuperExit/SuperLaunch/GetSuperStatus`, `SetSystemDate/GetSystemDate`, `GetSystemErrorSys`, `GetChassisFault/GetFaultSummary`, `SetGPIBAddress/GetGPIBAddress`, `GetSystemInfoRaw/GetSystemInfo`, `SetInterlock/GetInterlock/GetInterlockState`, `GetInventory`, `GetIPList`, `GetLayout/GetLayoutPort`, `SetLegacyMode/GetLegacyMode`, `GetLicenses`, `ReleaseLock`, `SystemShutdown`, `GetSystemStatusReg`, `GetTemperature`, `GetSystemTime`

### Chapter 4: Factory Commands
`GetCalibrationStatus/GetCalibrationDate`, `FactoryCommit`, `SetFactoryBiDir/GetFactoryBiDir`, `SetFactoryCore/GetFactoryCore`, `SetFactoryLowPower/GetFactoryLowPower`, `SetFactoryOPM/GetFactoryOPM`, `SetFactorySwitch/GetFactorySwitch`, `SetFactorySwitchSize`, `GetFactoryFPDistance/FPLoss/FPRatio`, `GetFactoryLoop`, `GetFactoryOPMP`, `GetFactoryRange`, `StartFactoryMeasure`, `GetFactorySWDistance/SWLoss`, `SetFactorySetupSwitch/GetFactorySetupSwitch`, `FactoryReset`

### Chapter 5: PCT - Measurement
`GetMeasureAll`, `GetDistance`, `StartFastIL/GetFastIL`, `SetHelixFactor/GetHelixFactor`, `GetIL`, `SetILLA/GetILLA`, `GetLength`, `GetORL/GetORLPreset`, `SetORLSetupPreset/SetORLSetupCustom/GetORLSetup/GetORLZone`, `GetPower`, `SetRef2Step/GetRef2Step`, `SetRefAlt/GetRefAlt`, `MeasureReset`, `StartSEIL/GetSEIL`, `StartMeasurement`, `GetMeasurementState`, `StopMeasurement`

### Chapter 5: PCT - PATH
`SetBiDir/GetBiDir`, `SetChannel/GetChannel/GetAvailableChannels`, `SetConnection/GetConnection`, `SetDUTLength/GetDUTLength/SetDUTLengthAuto/GetDUTLengthAuto`, `SetEOFMin/GetEOFMin`, `SetJumperIL/GetJumperIL/SetJumperILAuto/GetJumperILAuto`, `SetJumperLength/GetJumperLength/SetJumperLengthAuto/GetJumperLengthAuto`, `ResetJumper/ResetJumperMeasure`, `SetLaunch/GetLaunch/GetLaunchAvailable`, `SetPathList/GetPathList`, `SetReceive/GetReceive`

### Chapter 5: PCT - Port Map
`SetPortMapEnable/GetPortMapEnable`, `PortMapMeasureAll`, `SetPortMapLive/GetPortMapLive`, `PortMapValidate/GetPortMapValidation`, `GetPortMapLink`, `SetPortMapSelect/GetPortMapSelect`, `GetPortMapPathSize`, `GetPortMapFirst`, `PortMapInitList/PortMapInitRange`, `GetPortMapLast`, `SetPortMapLink`, `GetPortMapList`, `SetPortMapLock/GetPortMapLock`, `GetPortMapMode`, `PortMapReset`, `GetPortMapSetupSize`

### Chapter 5: PCT - Sense
`SetAveragingTime/GetAveragingTime/GetAvailableAveragingTimes`, `SetFunction/GetFunction`, `SetILOnly/GetILOnly`, `SetOPM/GetOPM`, `SetRange/GetRange`, `SetTempSensitivity/GetTempSensitivity`

### Chapter 5: PCT - Source
`SetContinuous/GetContinuous`, `SetSourceList/GetSourceList`, `SetWarmup/GetWarmup`, `SetWavelength/GetWavelength/GetAvailableWavelengths`

### Chapter 5: PCT - OPM
`FetchLoss`, `FetchORL`, `FetchPower`, `StartDarkMeasure/GetDarkStatus`, `RestoreDarkFactory`, `SetPowerMode/GetPowerMode`

### Warning
`GetWarning`

### Workflow
`WaitForIdle(timeoutMs)`, `WaitForMeasurement(timeoutMs)`

### Raw SCPI
`SendRawQuery(cmd)`, `SendRawWrite(cmd)`

## 典型工作流

```
// 1. 创建并连接
HANDLE h = PCT4_CreateDriver("viavi", "10.14.140.32", 8301, 1);
PCT4_Connect(h);
PCT4_Initialize(h);

// 2. 配置
PCT4_SetSourceList(h, "1310,1550");
PCT4_SetAveragingTime(h, 5);
PCT4_SetRange(h, 2);

// 3. 设置光路 (切换光开关)
PCT4_SetPathList(h, 1, "1,2,3");    // SW1 通道列表
PCT4_SetChannel(h, 1, 1);           // 切换到 SW1 通道 1

// 4. 参考测量
PCT4_SetFunction(h, 0);             // MODE_REFERENCE
PCT4_StartMeasurement(h);
PCT4_WaitForIdle(h, 60000);

// 5. DUT 测量
PCT4_SetFunction(h, 1);             // MODE_DUT
PCT4_StartMeasurement(h);
PCT4_WaitForIdle(h, 60000);

// 6. 停止测量（若需要中途停止）
PCT4_StopMeasurement(h);

// 7. 清理
PCT4_Disconnect(h);
PCT4_DestroyDriver(h);
```
