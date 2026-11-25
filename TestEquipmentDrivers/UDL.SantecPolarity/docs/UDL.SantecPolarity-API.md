# UDL.SantecPolarity 驱动接口文档

## 概述

| 属性 | 值 |
|---|---|
| DLL 名称 | `UDL.SantecPolarity.dll` |
| 用途 | 公司量产极性 Mapping（多设备编排） |
| 目标硬件 | RLM-100 + OSX-100 1x24 + RD-SP-100-SM |
| 通信 | RLM/OSX：TCP 5025 或 USB(VISA)；RD-SP：官方 SDK/Worker |
| 函数前缀 | `SP_` |
| 调用约定 | `__stdcall` |
| 加载方式 | `LoadLibrary` + `GetProcAddress` |
| 设计依据 | `SantecPolarity/RLM100_OSX100_RDSP100SM_Polarity_Cpp_MFC_UDL_Final_Plan.md` |

**当前实现阶段：** ABI 骨架。在供应商 RD-SP Mapping SDK/Worker 到位前，`SP_Open` / `SP_RunMapping` 固定返回 `SP_E_BACKEND_NOT_SUPPORTED`。禁止与已冻结的虚构 `POLARITY:*` stub（`SantecPolarity/Driver`）混淆。

## 与其它驱动的关系

| 驱动 | 关系 |
|---|---|
| `UDL.SantecRLM` | IL/RL 既有驱动；极性 Session 期间需独占协调，禁止双主控 |
| `UDL.SantecOSX` | 光开关 SCPI 经验可复用；本机固定 SW1 1..24 |
| `UDL.SantecPTM` | **另一产品线**（独立 PTM）；不在当前公司 BOM 验收范围 |
| `SantecPolarity/` stub | 虚构一体机协议；已冻结，禁止量产 |

## 测量流程（目标）

1. `SP_Create`
2. `SP_Open`（校验 RLM/OSX/RD-SP 序列号与 Profile）
3. `SP_GetHardwareSnapshot`
4. `SP_RunMapping`（1310 nm、逐通道点亮、官方 Mapping）
5. `SP_GetSummary` / `SP_GetFiberResults` / 诊断图路径
6. `SP_Close` / `SP_Destroy`

Cleanup 必须强制关激光、结束 Mapping、释放设备锁。

## 状态码（节选）

| 码 | 含义 |
|---|---|
| `SP_OK` | 成功 |
| `SP_E_BACKEND_NOT_SUPPORTED` | 当前骨架：SDK/Worker 未集成 |
| `SP_E_DEVICE_BUSY` | 全局 RLM 锁被占用 |
| `SP_E_RDSP_NOT_FOUND` / `SP_E_RLM_*` / `SP_E_OSX_*` | 硬件校验失败 |
| `SP_E_MAPPING_FAILED` | 官方 Mapping 失败 |
| `SP_E_CANCELLED` | 用户取消 |

## C ABI 导出

见 `SPExports.h` 与 `UDL.SantecPolarity.def`：

`SP_Create` / `SP_Destroy` / `SP_Open` / `SP_Close` / `SP_GetHardwareSnapshot` /
`SP_RunMapping` / `SP_Cancel` / `SP_GetSummary` / `SP_GetFiberResultCount` /
`SP_GetFiberResults` / `SP_GetDiagnosticPathUtf8` / `SP_GetLastErrorUtf8`

结构体：`SP_OPEN_OPTIONS`、`SP_HARDWARE_SNAPSHOT`、`SP_FIBER_RESULT`、`SP_RESULT_SUMMARY`（`SPTypes.h`）。

## 门禁

实现 Mapping 前必须完成 Final Plan Gate A（Native OPL-SDK 覆盖 RD-SP）或 Gate B（Managed Worker）。索取清单见仓库 `docs/RD-SP-SDK-供应商索取清单.md`。
