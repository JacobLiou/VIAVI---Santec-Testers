# UDL.SantecPolarity 驱动接口文档

交付与联调流程见 [UDL.SantecPolarity-SDK交付说明.md](UDL.SantecPolarity-SDK交付说明.md)。

## 概述

| 属性 | 值 |
|---|---|
| DLL 名称 | `UDL.SantecPolarity.dll` |
| 用途 | 公司量产极性 Mapping（多设备编排） |
| 目标硬件 | RLM-100 + OSX-100 1x24 + RD-SP-100-SM |
| 通信 | RLM/OSX：由 Santec.Hardware 管理（TCP/VISA 等）；RD-SP：相机 SDK（同进程 Cli） |
| 函数前缀 | `SP_` |
| 调用约定 | `__stdcall` |
| 加载方式 | `LoadLibrary` + `GetProcAddress` |
| 后端 | 同进程 `UDL.SantecPolarity.Cli` → **Santec.Hardware 2.3.1.0**（钉死） |

Host **只**加载本 DLL；`UDL.SantecPolarity.Cli.dll`、`Santec.Hardware.dll`、`Santec.Core.dll` 及打包 `bin` 内全部依赖须与本 DLL **同目录、同位数**。  
`SP_Open` 会抢占 `Global\UDL_SANTEC_RLM100_<serial>`（与 `UDL.SantecRLM` 互斥）。  
禁止与已冻结的虚构 `POLARITY:*` stub（`SantecPolarity/`）混淆。

## 与其它驱动的关系

| 驱动 | 关系 |
|---|---|
| `UDL.SantecRLM` | IL/RL 既有驱动；极性 Session 期间需独占协调，禁止双主控 |
| `UDL.SantecOSX` | 光开关；本方案由 Hardware 编排，Host 不必另开 OSX Session |
| `UDL.SantecPTM` | **另一产品线**（独立 PTM）；不在当前公司 BOM 验收范围 |
| `SantecPolarity/` stub | 虚构一体机协议；已冻结，禁止量产 |
| `UDL.SantecPolarity.Cli` | 同进程 C++/CLI；Host 勿直接加载 |

## 测量流程

1. `SP_Create`
2. `SP_Open`（设备锁 + 加载 Cli + Hardware Open；建议传 `rlmSerialUtf8`）
3. `SP_GetHardwareSnapshot`（可选）
4. `SP_RunMapping`（**工作线程**；分段 Mapping）
5. `SP_GetSummary` / `SP_GetFiberResults`
6. `SP_Close` / `SP_Destroy`（关激光、释放锁）

可随时 `SP_Cancel` 请求取消进行中的 Mapping；失败用 `SP_GetLastErrorUtf8`。

## 同进程集成约定

- Host 进程会加载 CLR（因 Cli 为 C++/CLI）；业务工程本身可为纯 Native/MFC。
- 钉死 Hardware **2.3.1.0**，勿用 Samples / NuGet 其它版本覆盖。
- 将 `Host.exe.config.sample` 中的 `assemblyBinding` 合并进 Host 的 `.exe.config`。

## 状态码（节选）

| 码 | 含义 |
|---|---|
| `SP_OK` | 成功 |
| `SP_E_INVALID_ARGUMENT` / `SP_E_INVALID_STATE` | 参数或生命周期错误 |
| `SP_E_DEVICE_BUSY` | 全局 RLM 锁被占用 |
| `SP_E_RDSP_NOT_FOUND` / `SP_E_RLM_*` / `SP_E_OSX_*` | 硬件校验 / 发现失败 |
| `SP_E_TIMEOUT` | 超时 |
| `SP_E_CANCELLED` | 用户取消 |
| `SP_E_MAPPING_FAILED` | 官方 Mapping 失败 |
| `SP_E_COMMUNICATION` / `SP_E_DEVICE_ERROR` | 通信或设备错误 |
| `SP_E_INTERNAL` | 内部错误 |

完整枚举见 `SPTypes.h`。

## 数据结构（摘要）

定义在 `TestEquipmentDrivers/UDL.SantecPolarity/SPTypes.h`（交付包 `include/`）。

### SP_OPEN_OPTIONS

| 字段 | 说明 |
|---|---|
| `structSize` / `version` | 填 `sizeof` / `1` |
| `stationIdUtf8` | 工位 ID，可空 |
| `rlmSerialUtf8` | RLM 序列号，**建议必填** |
| `rdspSerialUtf8` | RD-SP 序列号，可空 |
| `profilePathUtf8` | profile 路径，可空 |
| `connectTimeoutMs` / `operationTimeoutMs` / `mappingTimeoutMs` | 超时（ms） |

### SP_HARDWARE_SNAPSHOT

Open 后可读：RLM/OSX/RDSP 型号与串号、输出数、开关通道、波长、远端探测器索引等。

### SP_RESULT_SUMMARY / SP_FIBER_RESULT

Summary：总判定、纤芯计数、pass/fail、诊断图数量、耗时。  
Fiber：输入纤、开关通道、期望/实测输出、是否有光、`SP_FIBER_STATUS`、采集耗时。

## C ABI 导出

见 `SPExports.h` 与 `UDL.SantecPolarity.def`：

`SP_Create` / `SP_Destroy` / `SP_Open` / `SP_Close` /  
`SP_GetHardwareSnapshot` / `SP_RunMapping` / `SP_Cancel` /  
`SP_GetSummary` / `SP_GetFiberResultCount` / `SP_GetFiberResults` /  
`SP_GetDiagnosticPathUtf8` / `SP_GetLastErrorUtf8`

动态加载示例：`TestEquipmentDrivers/PolarityTestApp/SantecPolarityDllLoader.h`。

## 打包

```powershell
cd TestEquipmentDrivers\UDL.SantecPolarity.Pack
powershell -ExecutionPolicy Bypass -File .\pack-polarity-dist.ps1
```

产物：`dist/UDL.SantecPolarity-x64.zip`、`Win32.zip`、`Both.zip`（含 `PolarityTestApp` 与全部依赖）。

## 修订记录

| 日期 | 说明 |
|---|---|
| 2026-11-25 | 对齐 Gate B 同进程 Cli 实现；去掉过时骨架/门禁表述；链到 SDK 交付说明 |
