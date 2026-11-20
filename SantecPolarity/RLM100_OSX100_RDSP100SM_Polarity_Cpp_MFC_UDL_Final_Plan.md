# Santec RLM-100 + OSX-100 + RD-SP-100-SM 极性测试 C/C++ MFC UDL 最终实施方案

> 文档级别：最终硬件基线方案（用于公司 UDL 设计评审、PoC、开发和验收）  
> 开发平台：Windows + Visual Studio + Native C/C++ / MFC  
> UDL 对外接口：C ABI（`extern "C"` + `__stdcall`）  
> 日期：2026-11-20

---

## 1. 最终基线与范围

### 1.1 公司实际硬件

本方案不再以 RLM-110、通用 OSX 或 RD-P/RD-SP 二选一为假设，最终且唯一的量产硬件基线为：

| 角色 | 公司实际料号 | 本方案中的确定含义 |
|---|---|---|
| Return Loss Meter | `RLM-100-01-3500-S-09-FA-R` | RLM-100；单输出；1310/1550 nm；单模 9/125 µm；FC/APC；Remote Head |
| Optical Switch | `OSX-100-1A-024-09-FA-00B` | OSX-100；1A 拓扑；1x24；单模 9/125 µm；FC/APC；00B 机箱；订货号未带显示屏选项 |
| Combined detector | `RD-SP-100-SM` | RD-SP-100 单模版；远端积分球 + 极性检测器；不再按 RD-P 独立极性头设计 |
| 主要规格资料 | `RLM-100-C-E_Ver1-6_CODE-202506-MB-KT-CPY.pdf` | RLM-100 与 OSX-100 的订货编码、规格和组合方式基准 |
| SCPI 资料 | `RLM-User-Manual_2025.8.18.pdf` | RLM 通信、SCPI、Laser、Output、Switch、IL/RL 命令依据 |

### 1.2 订货号解码依据

根据 `RLM-100-C-E_Ver1-6_CODE-202506-MB-KT-CPY.pdf` 第 5 页订货表：

#### `RLM-100-01-3500-S-09-FA-R`

```text
RLM-100
  01    = Single Output
  3500  = 1310 / 1550 nm
  S-09  = Single-mode, 9/125 µm
  FA    = FC/APC
  R     = Remote Head
```

这会直接改变此前方案：

- 不允许继续写成 RLM-110。
- 不允许选择 1300、1490 或 1625 nm。
- 极性 Mapping 的默认光源固定为 **1310 nm**；1550 nm保留给 IL/RL，不作为第一版极性采集波长。
- RLM 只有一个 Output；极性循环不再做多 Output 路由，固定 `OUT:CLOSe 1`。
- 检测器是 Remote Head 配置；初始化时必须识别并绑定实际 RD-SP 序列号/Detector Index，不能默认使用前面板 Detector。

#### `OSX-100-1A-024-09-FA-00B`

```text
OSX-100-1A
  024  = 1x24
  09   = 9/125 µm single-mode
  FA   = FC/APC
  00B  = Switch chassis code
  末尾无 D = 无 5-inch touchscreen 选项
```

这会直接改变此前方案：

- 第一版只支持 **一台 1x24 OSX-100**。
- 默认物理 Route 是 `SW1` 的 1..24 通道，但上线前必须通过 `SW1:INFO?`、`SW1:CLOSe?` 和实机点亮验证确认 Switch 编号。
- 不再设计 SW2、第二台 Switch、1xN 动态发现或任意通道数量作为首版功能。
- RD-SP 的价值正是避免传统双 Switch 极性/IL Reference 结构；不得重新引入第二 Switch。

#### `RD-SP-100-SM`

本料号由公司实物配置给定。当前 Santec 官方页面确认 RD-SP 是远端积分球与极性检测器的组合，可与 RLM 和 OSX 一次连接完成 Polarity、IL 和 RL，并支持 Custom Mapping 和 Dark Channel。

`RLM-100-C-E_Ver1-6_CODE-202506-MB-KT-CPY.pdf` 第 6 页仍显示上一代 `RD-P-100`/`PTM-100` 选项，没有更新到 RD-SP。因此：

- 该 PDF 用于确认 RLM-100 和 OSX-100 的订货编码；
- 公司实物料号 `RD-SP-100-SM` 与当前 Santec RD-SP 官方资料用于确认极性检测头；
- 不得因为旧 PDF 写有 RD-P 就把驱动目标降级为 RD-P；
- 厂商 SDK 中可能仍保留 `RDPFactory`、`IRDP` 等历史名称，但实际创建对象必须验证为 `IRDSP`/`RDSP` 或明确支持 RD-SP 的 Factory 路径。

### 1.3 本期范围

**包含：**

- 24 芯单模 MPO/客户自定义 Mapping；
- RLM-100 1310 nm 极性照明；
- OSX-100 `SW1` 1..24 顺序或配置化 Route；
- RD-SP 图像采集与官方 Mapping 算法；
- Expected/Observed Mapping、逐芯结果、Dark Channel 和诊断图像；
- Native C ABI、MFC 调用、日志、取消、超时和安全清理；
- 与现有 `UDL.SantecRLM` 的独占协调。

**不包含：**

- RLM-110 Ultra-fast 特性或 RLM-110 节拍；
- RD-P-100、PTM-100 或 Multimode RD-SP；
- 1300/1490/1625 nm；
- 第二台 OSX、SW2 或双输出 RLM；
- 自研相机协议、曝光算法和 Mapping 视觉算法；
- 第一版直接写 XNS。

---

## 2. 对此前方案的修正结论

| 此前假设 | 最终修正 |
|---|---|
| RLM-110 | 固定为 `RLM-100-01-3500-S-09-FA-R` |
| RD-SP 或 RD-P 二选一 | 固定为 `RD-SP-100-SM`，RD-P 只作为 API 历史背景 |
| 1300/1310 nm 任选 | 本机是 SM 3500 光源，极性固定 1310 nm |
| RLM 可能双输出 | 本机 `01` 是单输出，固定 Output 1 |
| OSX 通道数动态泛化 | 本机固定 1x24；首版验证并锁定 SW1 CH1..24 |
| 可能使用第二 Switch | 删除；RD-SP 方案不需要第二 Switch |
| RLM-110 12 芯 <25 秒指标 | 删除；改用 RLM-100 实机测量建立 12/24 芯 P50/P95 |
| `RDPFactory.CreateRDP()` 示例可直接套用 | 不可直接假设；必须使用明确支持 RD-SP 的官方创建路径 |
| In-process C++/CLI 可作为默认 | 不作为默认；公司 UDL 保持 Native，优先 Native OPL-SDK，否则隔离 Worker |

### 2.1 最重要的工程判断

> 这不是新增一条 `READ:POL?` SCPI 命令，而是由 Native C++ UDL 编排 `RLM-100 + OSX-100 + RD-SP-100-SM`，调用 Santec 官方 RD-SP 采集/Mapping 能力完成的复合测试。

附件 RLM 手册的公开 SCPI 命令表包含 Laser、IL/RL、Detector、Output 和 Switch 等命令，但没有极性 Mapping 查询。RD-SP 的极性结果来自逐通道点亮、相机采集、光点定位和 Mapping 处理。

---

## 3. 最终软件架构

### 3.1 架构原则

公司要求固定采用 C 或 Visual Studio C++ MFC 风格，因此：

1. `UDL.SantecPolarity.dll` 必须是 **Native C++ DLL**。
2. 对外只能导出稳定 C ABI，Host 不接触 CLR、Santec Managed 类型或 OpenCV 类型。
3. 优先使用 Santec 提供的 Native `STDCALL` SDK，但必须先证明其覆盖 RD-SP Mapping。
4. 如果 Native SDK 不覆盖 RD-SP，则使用独立进程封装官方 Managed Hardware Library；不把 CLR 注入 MFC Host。
5. 如果公司政策禁止部署任何 .NET Worker，则项目必须等待 Santec 提供 Native RD-SP SDK，不能以自研 Mapping 作为绕过方案。

### 3.2 推荐架构

```text
Company MFC / C Test Host
          |
          | C ABI (__stdcall)
          v
+--------------------------------------------------+
| UDL.SantecPolarity.dll                           |
| Native C++ / Visual Studio /MD                   |
|                                                  |
|  DriverExports                                   |
|       -> PolaritySession                         |
|       -> Rlm100PolarityCoordinator               |
|       -> RlmScpiController                       |
|       -> Osx24RouteController                    |
|       -> RDSPBackend                             |
|       -> Result / Error / Log                    |
+---------------------------+----------------------+
                            |
              +-------------+-------------+
              |                           |
              v                           v
  A. OplSdkNativeBackend       B. RdspWorkerBackend
     Native STDCALL               Named Pipe / local RPC
     仅在确认完整支持后                     |
                                          v
                              SantecRdspWorker.exe
                              .NET Framework 4.8 x64
                                          |
                              Santec.Hardware.dll
                                          |
                                          v
                              RD-SP-100-SM Camera

Native RLM/OSX control (only when backend does not own them):
UDL -> TCP 5025 or VISA -> RLM-100 -> OSX-100 SW1
```

### 3.3 为什么 Worker 是 Managed 回退方案

Worker 不是公司 UDL 驱动；UDL 本体仍为 C/C++。Worker 只是隔离厂商只提供的 Managed Camera/Mapping 库，具有以下优点：

- MFC Host 和 UDL 不加载 CLR；
- x86 Host 可调用 x64 Worker；
- TIS Camera/OpenCvSharp 崩溃与 Host 隔离；
- 官方 DLL、OpenCV、Camera Runtime 版本独立部署；
- Worker 异常后可重启并重新初始化 RD-SP。

不推荐把 C++/CLI Bridge 直接加载到 MFC 量产进程，除非公司明确批准且所有组件位数完全一致。

---

## 4. SDK 选型 Gate

### 4.1 Gate A：OPL-SDK 是否真的支持 RD-SP

Santec 对 OPL-SDK 的公开描述是 Windows DLL、设备发现、多仪器管理和 `STDCALL` 跨语言接口。但“支持一般 Santec USB 仪器”不等于“支持 RD-SP Camera Mapping”。

必须从 Header、LIB、Sample 或厂商书面回复确认以下等价接口全部存在：

```text
Detect RD-SP camera
Validate RLM-100 + OSX-100 + RD-SP hardware combination
Create and initialize RD-SP composite device
Set MPO / Duplex mapping mode
Set Connector / BareFiber sensitivity
Set loss compensation
Start mapping
Collect one logical input fiber
Process expected mapping
Return observed mapping
Return detected/dark fibers
Return all diagnostic images
End / Cancel / Uninitialize
```

- **全部存在：** 采用 `OplSdkNativeBackend`。
- **任何一项缺失：** 采用 `RdspWorkerBackend`。
- **不得：** 混用半套 OPL-SDK 和自研图像算法。

### 4.2 Gate B：Santec.Hardware.dll RD-SP 路径

Santec Hardware Library 2.3.1.0 文档列出 `IRDSP`、`RDSP`、`SimulatedRDSP`、`RDSPCameraValidator`、`ERDSPModel` 和 RD-SP 硬件需求错误类型。开发时要以实际 2.3.1.0 Assembly 和 RD-P/RD-SP Sample 为准，验证：

- RD-SP 的 Factory 创建函数；
- RLM-100-01-3500 是否满足要求；
- RD-SP-100-SM Camera 被识别为正确型号；
- Advanced/Segmented Mapping 的 RD-SP 支持情况；
- 诊断图像在 2.3.1.0 中是单张还是列表；
- Worker 内部实际依赖版本。

不得照抄只创建 RD-P 的旧示例并宣称 RD-SP 已支持。

---

## 5. 确定的硬件拓扑

```text
                                    USB Camera
                           +--------------------------+
                           | RD-SP-100-SM             |
                           | Integrating Sphere       |
DUT output / receive  ---->| + Polarity Detector     |
                           +--------------------------+
                                      |
                                      | 管理/采集：官方 SDK
                                      v
+----------------+      +--------------------------+
| MFC UDL Host   |----->| Native Polarity UDL      |
+----------------+      +------------+-------------+
                                     |
                            TCP 5025 or USB/VISA
                                     |
                                     v
                         +--------------------------+
                         | RLM-100                  |
                         | 01: single output        |
                         | 3500: 1310/1550 nm       |
                         | S-09: single-mode        |
                         | FA-R: FC/APC remote head |
                         +------------+-------------+
                                      |
                                      | RLM-managed switch link
                                      v
                         +--------------------------+
                         | OSX-100                  |
                         | 1A / 1x24 / SM / FC/APC  |
                         | SW1 channels 1..24       |
                         +------------+-------------+
                                      |
                                      v
                              DUT input fiber 1..24
```

### 5.1 光路含义

一次 Mapping 只允许一个输入通道发光：

```text
RLM 1310 nm Laser
 -> RLM Output 1
 -> OSX SW1 Channel N
 -> DUT Input Fiber N
 -> DUT internal mapping
 -> DUT Output Position M
 -> RD-SP detects position M
```

循环 `N = 1..24` 后，官方算法返回：

```text
ObservedMapping[N] = M
```

### 5.2 固定与可配置项

**代码中固定的硬件能力：**

- RLM Model：RLM-100；
- Output：1；
- Polarity wavelength：1310 nm；
- Switch capacity：24；
- Fiber type：SM 9/125 µm；
- Connector：FC/APC system path；
- Detector family：RD-SP-100-SM。

**Profile 中可配置：**

- DUT Fiber Count：首版 12 或 24，但物理 OSX 最大 24；
- Logical Fiber -> SW1 Channel Route；
- MPO/Duplex Mapping Mode；
- Connector/BareFiber Sensitivity；
- Loss Compensation；
- Expected Mapping；
- Allowed Dark Channels；
- Switch settle time；
- 测试超时和重试策略。

---

## 6. 单一主控策略

### 6.1 禁止双主控

以下两条路径不得同时控制同一台 RLM-100/OSX-100：

```text
Existing UDL.SantecRLM -> SCPI
Santec.Hardware / RD-SP Composite Device -> RLM/OSX
```

每次 Polarity Session 必须只有一个 Owner。

### 6.2 两种允许模式

#### 模式 1：官方 RD-SP Composite Backend 全权控制

若官方 RD-SP API 自动管理 Laser 和 OSX：

- `UDL.SantecRLM` 释放连接；
- Native UDL 只调用 Backend 的 `Map`；
- Native UDL 不并行发送任何 SCPI；
- Backend 完成后再恢复 IL/RL Session。

#### 模式 2：Advanced/Segmented，由 Native Coordinator 控制 RLM/OSX

仅在官方明确支持时采用：

- Backend 负责 RD-SP Start/Collect/Process/End；
- Native Coordinator 负责 RLM 1310 nm、Output 1、SW1 CH1..24；
- 所有动作在同一个 Session Lock 内顺序执行；
- Worker 不能在后台再次切换同一设备。

### 6.3 跨进程设备锁

建议使用 RLM Serial 形成锁名：

```text
Global\UDL_SANTEC_RLM100_<RLM_SERIAL>
```

获取锁后才允许：Open、Laser On、Switch、Collect、Process。任何进程拿不到锁时返回 `SP_E_DEVICE_BUSY`，不得重试抢占。

---

## 7. 设备发现与严格校验

### 7.1 RLM-100 校验

连接后保存以下原始响应：

```text
*IDN?
*OPT?
:SYSTem:VERSion?
FIBER:INFO?
LASer:INFO?
POW:NUM?
POW:det#:INFO?
OUT:CLOSe?
SW1:INFO?
SW1:CLOSe?
:SYSTem:ERRor:NEXT?
```

硬校验：

- IDN/Model 必须匹配 RLM-100；
- `FIBER:INFO?` 必须为 SM；
- `LASer:INFO?` 必须包含 1310 和 1550；
- Polarity 流程只选 1310；
- `OUT:CLOSe?` 必须可设置/读回 1；
- 不允许出现 1300、1490、1625 配置；
- RLM Serial 必须与 Station 配置一致，避免连接旁边工位设备。

### 7.2 OSX-100 校验

- `SW1:INFO?` 返回值必须能证明外部 Switch 已连接；
- 逐个验证 CH1 和 CH24 均可设置并读回；
- 生产初始化不要求扫描 1..24，但首次 Commissioning 必须完整扫描；
- 若实际 Switch 被枚举为 SW2，不能自动悄悄切换；需要更新签核后的 Station Route 配置；
- 任何超出 1..24 的 Route 都在开 Laser 前拒绝。

### 7.3 RD-SP 校验

至少记录并匹配：

- Model/Friendly Name；
- Camera Serial；
- RD-SP Serial（若 API 可读）；
- SM Variant；
- Camera Runtime Version；
- Santec Library/OPL-SDK Version；
- Hardware Requirement Evaluation 结果；
- 校准/配置文件 ID（若 API 可读）。

不得只用“找到第一台 Camera”。多工位 PC 或维修环境中必须按 Serial 绑定。

### 7.4 Remote Head Detector 绑定

RLM 订货号末尾 `R` 表示 Remote Head。现有 IL/RL 驱动若仍默认固定 Detector 编号，需要修正为：

1. 调用 `POW:NUM?` 获取已连接 Detector 数；
2. 对每个 `det#` 调用 `POW:det#:INFO?`；
3. 根据 RD-SP/Remote Head Serial 与 Station 配置选择 Detector Index；
4. 将 Detector Index 写入 Session Snapshot；
5. IL 读取和 Reference 使用该 Index，不得使用硬编码 `det1`。

---

## 8. 最终 Mapping 流程

### 8.1 初始化

```text
Acquire global RLM lock
 -> Open backend
 -> Detect exact RD-SP-100-SM
 -> Open RLM-100
 -> Read and validate identity/capabilities
 -> Validate OSX-100 as SW1 1x24
 -> Bind Remote Head detector index
 -> Validate signed Profile and Route revision
 -> State = Ready
```

### 8.2 运行 Advanced/Segmented Mapping

```text
StartMapping(fiberCount)
 -> RLM remote mode
 -> OUT:CLOSe 1;*OPC?
 -> LASer:ENABle 1310;*OPC?
 -> for each configured OpticalRoute:
       SW1:CLOSe <channel>;*OPC?
       optional readback SW1:CLOSe?
       wait configured settle time
       CollectOutputMappingData(logicalFiber)
       record lightDetected + elapsed time
 -> LASer:DISABle;*OPC?
 -> ProcessMappingData(expectedMapping)
 -> Compare Expected / Observed per fiber
 -> Save all diagnostic images on mapping failure
 -> EndMapping
 -> release lock
```

### 8.3 Cleanup 顺序

所有正常、异常、Cancel 和进程退出路径都按以下顺序尝试：

```text
1. LASer:DISABle
2. 确认 LASer:ENABle? 返回关闭状态（若通信仍可用）
3. EndMapping
4. Switch 回到 Station 定义的 Safe Channel（默认不擅自定义）
5. Uninitialize RD-SP Backend
6. Close RLM session
7. Release global lock
```

Cleanup 中每一步都独立 `try/catch`，前一步失败不能阻止后续安全动作。

### 8.4 Basic Mapping 的使用限制

若官方 Basic API 默认使用前 `n` 个 Switch 通道，可以在 Sandbox 阶段验证 12/24 芯；量产仍优先 Advanced/Segmented，因为：

- 可显式记录每个 Route；
- 可验证实际 SW1 Channel；
- 可插入 Cancel 和超时；
- 可与公司现有 UDL 日志对齐；
- 可支持未来非连续 Route，而不修改 Driver。

---

## 9. Profile 与 Route

### 9.1 数据模型

```cpp
struct OpticalRoute
{
    int32_t logicalFiber;   // 1..fiberCount
    int32_t rlmOutput;      // 本机必须为 1
    int32_t switchNumber;   // 基线必须为 1
    int32_t switchChannel;  // 1..24
};

struct PolarityProfile
{
    std::string profileId;
    std::string revision;
    int32_t fiberCount;             // 12 or 24
    int32_t wavelengthNm;           // 必须 1310
    MappingMode mappingMode;        // MPO / Duplex
    MappingSensitivity sensitivity; // Connector / BareFiber
    double lossCompensationDb;
    std::vector<int32_t> expectedMapping;
    std::vector<uint8_t> allowedDarkChannels;
    std::vector<OpticalRoute> routes;
};
```

### 9.2 24 芯基线配置格式

```json
{
  "schemaVersion": 1,
  "stationHardware": {
    "rlmPartNumber": "RLM-100-01-3500-S-09-FA-R",
    "osxPartNumber": "OSX-100-1A-024-09-FA-00B",
    "detectorPartNumber": "RD-SP-100-SM"
  },
  "profileId": "MPO24_CUSTOMER_APPROVED",
  "revision": "A",
  "fiberCount": 24,
  "wavelengthNm": 1310,
  "mappingMode": "MPO",
  "sensitivity": "Connector",
  "lossCompensationDb": 0.0,
  "expectedMapping": [
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
    13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24
  ],
  "allowedDarkChannels": [],
  "routes": [
    { "logicalFiber": 1, "rlmOutput": 1, "switchNumber": 1, "switchChannel": 1 },
    { "logicalFiber": 2, "rlmOutput": 1, "switchNumber": 1, "switchChannel": 2 },
    { "logicalFiber": 24, "rlmOutput": 1, "switchNumber": 1, "switchChannel": 24 }
  ]
}
```

上面 `expectedMapping` 只是直通格式示例，不代表已经确认的 Type A。实际 Type A/B/C 必须由 Golden DUT、Connector Key 方向、A/B 端观察方向和公司编号规则共同确定。

### 9.3 Type A/B/C 规则

驱动中禁止：

```cpp
if (type == TYPE_A) { /* hard-coded mapping */ }
```

正确方式：

```text
Product/Test Plan
 -> Profile ID + Revision
 -> signed expectedMapping[]
 -> Driver executes and compares
```

Profile 必须包含审批人、审批日期、产品号、DUT 两端方向照片或文档引用，以及 SHA-256。UDL 记录 Profile Hash，防止结果无法追溯。

### 9.4 Profile 校验

开 Laser 前拒绝以下配置：

- Hardware Part Number 与 Station 不一致；
- `fiberCount` 不是 12/24 或超过 24；
- `wavelengthNm != 1310`；
- 任一 Route 的 `rlmOutput != 1`；
- 任一 Route 的 `switchNumber != 1`；
- Switch Channel 不在 1..24；
- Logical Fiber 重复或缺失；
- Expected Mapping 长度不等于 Fiber Count；
- Expected Output 重复但 Profile 未声明特殊工艺；
- Dark Channel 与 Expected Mapping 冲突；
- 未签名或 Revision 未批准。

---

## 10. Native C ABI

### 10.1 ABI 要求

- `extern "C"`；
- `__stdcall`；
- 固定宽度整数；
- UTF-8 字符串；
- 不透明 Handle；
- POD Struct；
- Struct 带 `structSize` 和 `version`；
- 两阶段 Buffer API；
- 导出函数不得抛出 C++/SEH/Managed 异常；
- 不跨边界传 `CString`、`std::string`、`std::vector`、`bool`、COM Pointer 或 Managed Handle。

### 10.2 建议公开接口

```cpp
#pragma once
#include <stdint.h>

#ifdef UDL_SANTEC_POLARITY_EXPORTS
#define SP_API extern "C" __declspec(dllexport)
#else
#define SP_API extern "C" __declspec(dllimport)
#endif
#define SP_CALL __stdcall

typedef void* SP_HANDLE;

typedef enum SP_STATUS
{
    SP_OK                         = 0,
    SP_E_INVALID_ARGUMENT        = -1,
    SP_E_INVALID_STATE           = -2,
    SP_E_DEVICE_BUSY             = -3,
    SP_E_PROFILE_MISMATCH        = -4,

    SP_E_RDSP_NOT_FOUND          = -10,
    SP_E_RLM_NOT_FOUND           = -11,
    SP_E_RLM_MODEL_MISMATCH      = -12,
    SP_E_RLM_OPTION_MISMATCH     = -13,
    SP_E_OSX_NOT_FOUND           = -14,
    SP_E_OSX_CHANNEL_MISMATCH    = -15,
    SP_E_REMOTE_HEAD_MISMATCH    = -16,
    SP_E_HARDWARE_NOT_SUPPORTED  = -17,

    SP_E_TIMEOUT                 = -20,
    SP_E_CANCELLED               = -21,
    SP_E_COMMUNICATION           = -22,
    SP_E_INVALID_RESPONSE        = -23,
    SP_E_DEVICE_ERROR            = -24,

    SP_E_AMBIENT_LIGHT           = -30,
    SP_E_MAPPING_FAILED          = -31,
    SP_E_DIAGNOSTIC_SAVE_FAILED  = -32,

    SP_E_BACKEND_NOT_SUPPORTED   = -40,
    SP_E_BACKEND_VERSION         = -41,
    SP_E_WORKER_UNAVAILABLE      = -42,
    SP_E_INTERNAL                = -100
} SP_STATUS;

typedef enum SP_FIBER_STATUS
{
    SP_FIBER_PASS = 0,
    SP_FIBER_WRONG_MAPPING,
    SP_FIBER_MISSING,
    SP_FIBER_UNEXPECTED,
    SP_FIBER_DUPLICATE,
    SP_FIBER_DARK_ALLOWED,
    SP_FIBER_AMBIGUOUS,
    SP_FIBER_ACQUISITION_ERROR
} SP_FIBER_STATUS;

#pragma pack(push, 8)
typedef struct SP_OPEN_OPTIONS
{
    uint32_t structSize;
    uint32_t version;
    const char* stationIdUtf8;
    const char* rlmSerialUtf8;
    const char* rdspSerialUtf8;
    const char* profilePathUtf8;
    uint32_t connectTimeoutMs;
    uint32_t operationTimeoutMs;
    uint32_t mappingTimeoutMs;
} SP_OPEN_OPTIONS;

typedef struct SP_HARDWARE_SNAPSHOT
{
    uint32_t structSize;
    uint32_t version;
    char rlmPartNumber[64];
    char rlmSerial[64];
    char rlmFirmware[32];
    char osxPartNumber[64];
    char rdspPartNumber[64];
    char rdspSerial[64];
    int32_t rlmOutputCount;
    int32_t switchNumber;
    int32_t switchChannelCount;
    int32_t polarityWavelengthNm;
    int32_t remoteDetectorIndex;
} SP_HARDWARE_SNAPSHOT;

typedef struct SP_FIBER_RESULT
{
    int32_t inputFiber;
    int32_t switchChannel;
    int32_t expectedOutput;
    int32_t observedOutput; // unknown = 0
    int32_t lightDetected;  // 0/1
    SP_FIBER_STATUS status;
    uint32_t acquisitionMs;
} SP_FIBER_RESULT;

typedef struct SP_RESULT_SUMMARY
{
    uint32_t structSize;
    uint32_t version;
    int32_t overallPass;
    uint32_t fiberCount;
    uint32_t passCount;
    uint32_t failCount;
    uint32_t diagnosticImageCount;
    uint64_t elapsedMs;
} SP_RESULT_SUMMARY;
#pragma pack(pop)

SP_API SP_STATUS SP_CALL SP_Create(SP_HANDLE* handle);
SP_API SP_STATUS SP_CALL SP_Destroy(SP_HANDLE handle);
SP_API SP_STATUS SP_CALL SP_Open(SP_HANDLE handle, const SP_OPEN_OPTIONS* options);
SP_API SP_STATUS SP_CALL SP_GetHardwareSnapshot(
    SP_HANDLE handle, SP_HARDWARE_SNAPSHOT* snapshot);
SP_API SP_STATUS SP_CALL SP_RunMapping(SP_HANDLE handle);
SP_API SP_STATUS SP_CALL SP_Cancel(SP_HANDLE handle);
SP_API SP_STATUS SP_CALL SP_GetSummary(
    SP_HANDLE handle, SP_RESULT_SUMMARY* summary);
SP_API SP_STATUS SP_CALL SP_GetFiberResultCount(
    SP_HANDLE handle, uint32_t* count);
SP_API SP_STATUS SP_CALL SP_GetFiberResults(
    SP_HANDLE handle, SP_FIBER_RESULT* buffer,
    uint32_t capacity, uint32_t* written);
SP_API SP_STATUS SP_CALL SP_GetDiagnosticPathUtf8(
    SP_HANDLE handle, uint32_t index, char* buffer,
    uint32_t capacity, uint32_t* required);
SP_API SP_STATUS SP_CALL SP_GetLastErrorUtf8(
    SP_HANDLE handle, char* buffer,
    uint32_t capacity, uint32_t* required);
SP_API SP_STATUS SP_CALL SP_Close(SP_HANDLE handle);
```

### 10.3 与现有 UDL 的兼容

推荐独立生成 `UDL.SantecPolarity.dll`，不修改现有 `UDL.SantecRLM` 的：

- 27 个既有 Export；
- `.def` Ordinal；
- `MeasurementResult` Layout；
- `rawData[10]`；
- 现有 IL/RL Host 调用。

如果公司要求最终合并为一个 DLL，也必须只追加新 Export，且通过 ABI Regression Test 证明旧程序二进制兼容。

---

## 11. Native 内部接口

```cpp
class IRdspBackend
{
public:
    virtual ~IRdspBackend() = default;

    virtual Status DetectAndOpen(const BackendOpenOptions&) = 0;
    virtual Status ValidateHardware(HardwareSnapshot&) = 0;
    virtual Status Configure(const PolarityProfile&) = 0;
    virtual Status StartMapping(int32_t fiberCount) = 0;
    virtual Status Collect(int32_t logicalFiber, bool& lightDetected) = 0;
    virtual Status Process(
        const std::vector<int32_t>& expected,
        BackendMappingResult& result) = 0;
    virtual Status EndMapping() noexcept = 0;
    virtual Status Cancel() noexcept = 0;
    virtual void Close() noexcept = 0;
};

class IRlm100Controller
{
public:
    virtual Status Open(const RlmConnection&) = 0;
    virtual Status ReadIdentity(RlmIdentity&) = 0;
    virtual Status SelectOutput1() = 0;
    virtual Status Enable1310() = 0;
    virtual Status DisableLaser() noexcept = 0;
    virtual Status SelectSwitchChannel(int32_t channel) = 0;
    virtual Status ReadSwitchChannel(int32_t& channel) = 0;
    virtual Status DrainErrorQueue(DeviceErrors&) = 0;
    virtual void Close() noexcept = 0;
};
```

Worker/Managed 类型、`Task`、`CancellationToken`、OpenCV `Mat` 和厂商异常都不得越过 Backend 边界。

---

## 12. SCPI 实现基准

附件 RLM 手册说明：

- Ethernet 为 TCP 5025；
- USB 使用 USBTMC/VISA；
- Write 以 LF (`\n`) 结束；
- RLM 同步执行命令；
- 可用 `*OPC?` 等待操作完成；
- `READ:RL?` 在某些模式可能超过 10 秒，IL/RL Read Timeout 至少 15 秒。

本机极性相关命令：

| 目的 | 命令 |
|---|---|
| 身份 | `*IDN?` |
| 选件 | `*OPT?` |
| Firmware/SCPI | `:SYSTem:VERSion?` |
| Fiber | `FIBER:INFO?` |
| Laser 能力 | `LASer:INFO?` |
| 开 1310 nm | `LASer:ENABle 1310;*OPC?` |
| 关 Laser | `LASer:DISABle;*OPC?` |
| 单 Output | `OUT:CLOSe 1;*OPC?` |
| Output Readback | `OUT:CLOSe?` |
| OSX 信息 | `SW1:INFO?` |
| 选择通道 | `SW1:CLOSe <1..24>;*OPC?` |
| Switch Readback | `SW1:CLOSe?` |
| Detector 数 | `POW:NUM?` |
| Detector 信息 | `POW:det#:INFO?` |
| 错误队列 | `:SYSTem:ERRor:NEXT?` |

### 12.1 通信硬化

- `send()` 循环直到完整发送；
- Query 按 LF/协议边界组帧；
- 空字符串绝不能转为 `0.0`；
- Timeout、Disconnected、ProtocolError、DeviceError、InvalidResponse 分开；
- 同一个 RLM Session 串行发送；
- SCPI 原始日志要脱敏但保留命令、响应、耗时；
- Laser On 后的任何异常立即进入 Cleanup；
- 不在 UI Thread 执行网络/Camera Blocking 调用。

---

## 13. MFC Host 集成

### 13.1 调用线程

```text
MFC UI Thread
   -> Start button
   -> Create worker thread / task
       -> SP_Open
       -> SP_RunMapping
       -> PostMessage(progress/result)
   -> Cancel button
       -> SP_Cancel
```

UI Thread 不直接调用长时间 Mapping。

### 13.2 字符串

UDL 使用 UTF-8；MFC 转换示例：

```cpp
CStringW Utf8ToCStringW(const char* utf8);
std::string CStringWToUtf8(const CStringW& value);
```

不要依赖系统 ACP，避免中文 Station/Profile/Error 信息乱码。

### 13.3 动态加载

公司若用 `LoadLibrary/GetProcAddress`：

- 启动时校验所有必需 Export；
- 调用 `SP_Create` 后检查 ABI Version；
- 缺任一函数立即阻止测试；
- 不允许因 DLL 版本不匹配而降级成“只返回 PASS/FAIL”。

---

## 14. 状态机

```text
Created
  -> Opening
  -> DetectingRDSP
  -> ConnectingRLM
  -> ValidatingRLM100
  -> ValidatingOSX24
  -> BindingRemoteHead
  -> Ready
  -> StartingMapping
  -> LaserOn
  -> AcquiringCH1..CH24
  -> LaserOff
  -> Processing
  -> Completed
  -> CleaningUp
  -> Ready

Any State
  -> Cancelling or Faulted
  -> CleaningUp
  -> Ready or Disconnected
```

非法调用必须返回 `SP_E_INVALID_STATE`，例如：

- 未 Open 就 Run；
- Run 中再次 Run；
- Run 中切换 Profile；
- Faulted 未 Cleanup 就重试；
- Close 和 Run 并发。

---

## 15. 结果模型

每条结果至少包含：

```text
Test ID
DUT SN
Profile ID / Revision / Hash
RLM Part Number / Serial / Firmware
OSX Part Number / SW Number / Channel Count
RD-SP Part Number / Serial / SDK Version
Input Fiber
Physical Switch Channel
Expected Output
Observed Output
Light Detected
Fiber Status
Acquisition Time
Overall Status
Diagnostic Image Paths
Error Code / Error Message
Cleanup Status / Laser Off Confirmation
```

逐芯状态至少：

```text
Pass
WrongMapping
MissingFiber
UnexpectedFiber
DuplicateFiber
DarkAllowed
Ambiguous
AcquisitionError
```

严禁只返回：

```text
Polarity = PASS/FAIL
```

否则无法支持 MIMS/MES Failure Analysis。

---

## 16. 错误映射与诊断

### 16.1 厂商异常映射

| 厂商/底层情况 | Native 错误 |
|---|---|
| Camera not found | `SP_E_RDSP_NOT_FOUND` |
| Hardware requirement failed | `SP_E_HARDWARE_NOT_SUPPORTED` |
| Ambient light detected | `SP_E_AMBIENT_LIGHT` |
| Mapping cannot be generated | `SP_E_MAPPING_FAILED` |
| RLM/OSX timeout | `SP_E_TIMEOUT` |
| Invalid response | `SP_E_INVALID_RESPONSE` |
| Worker crash/disconnect | `SP_E_WORKER_UNAVAILABLE` |
| User cancel | `SP_E_CANCELLED` |

### 16.2 诊断图像版本隔离

Santec Hardware Library 的版本记录显示诊断图像 API 曾从单张改为图像列表，而旧 Quick Start 示例仍可能显示单张属性。Backend 必须：

- 适配实际 2.3.1.0 API；
- 保存所有 Detector Image；
- 使用唯一文件名；
- Native 层只暴露 `imageCount` 和按索引读取路径；
- Host 不依赖厂商属性名称。

### 16.3 文件策略

```text
Diagnostics/
  YYYYMMDD/
    <Station>_<TestId>_<DutSn>/
      mapping_01.png
      mapping_02.png
      result.json
      hardware.json
      scpi.log
```

设置磁盘配额和保留周期。PASS 默认不保存原始图像，FAIL/Exception 必须保存；具体以质量要求为准。

---

## 17. 安全要求

RLM 手册指出激光不可见，直视工作中的光纤端面可能严重伤眼。

软件强制措施：

- Laser Guard 构造后默认 Off；
- 只有在 RD-SP 已 Start、Switch 已验证后才 On；
- 每次 Collect 后检查 Cancel；
- Process 前先关 Laser；
- `finally`/RAII 再次关 Laser；
- Worker Watchdog 在 Host 断开时执行 Stop/Uninitialize；
- Cleanup 结果写日志；
- 关 Laser 失败视为设备安全故障，阻止下一次测试。

操作要求：

- 不得直视连接器或裸纤；
- 测试中禁止拔插 DUT；
- RD-SP 端适配器/遮光环境按厂商要求安装；
- 维修模式必须有独立 Laser Warning。

---

## 18. 部署基线

### 18.1 Native UDL

- Visual Studio Toolset 与公司 UDL 统一；
- Runtime 统一 `/MD` 或公司指定设置；
- Win32/x64 分包，禁止混放 DLL；
- `.def` 固定 Export Name/Ordinal；
- PDB 内部归档，不随普通生产包泄露；
- Authenticode 签名按公司发布流程。

### 18.2 Managed Worker 回退方案

官方 Hardware Library 2.3.1.0 文档列出的依赖包括：

- .NET Framework 4.8；
- `Santec.Hardware.dll`；
- `Santec.Core.dll`；
- `System.Text.Json`；
- `Zeroconf`；
- `NGettext`；
- USB RLM 时需要 VISA；Ethernet 本身不需要 VISA。

RD-P/RD-SP 相关依赖还包括：

- `OpenCvSharp4.Windows`；
- `Nito.AsyncEx.Context`；
- `TIS.Imaging.ICImagingControl35`。

官方文档指出 Hardware Library 是针对 `OpenCvSharp4 4.6.0.20220608` 编译，后续版本出现 Breaking Change。必须锁定实际验证版本，不允许 NuGet 自动升级。

### 18.3 位数

PoC 第一天确认：

```text
MFC Host bitness
Native UDL bitness
OPL-SDK bitness（若使用）
Worker bitness
Santec.Hardware.dll load behavior
TIS Camera Runtime bitness
OpenCvSharp native runtime bitness
VISA Runtime bitness
```

若 Host 是 x86 而 Camera/SDK 只稳定支持 x64，则固定使用 x64 Worker，不尝试将 x64 DLL 载入 x86 Host。

### 18.4 版本清单

发布 Manifest 至少包含：

```text
RLM-100 part number / serial / firmware
OSX-100 part number / serial (if available)
RD-SP-100-SM serial
Native UDL file version + SHA-256
Worker protocol version
Santec SDK/DLL version + SHA-256
Camera runtime version
OpenCV version
VISA version
Profile/Route revision + SHA-256
```

---

## 19. 测试计划

### 19.1 无硬件单元测试

1. ABI Size/Offset/Calling Convention。
2. UTF-8 与 MFC `CStringW` 转换。
3. Profile Hard Validation。
4. 只允许 Output 1、SW1、CH1..24、1310 nm。
5. Straight、Reverse、Cross、Missing、Dark、Duplicate Mapping Golden Cases。
6. State Machine 非法顺序。
7. 每个状态 Cancel。
8. Timeout/Disconnect/Invalid Response。
9. Worker Protocol 版本不一致。
10. 所有异常的 Laser/Mapping Cleanup 调用顺序。

### 19.2 SCPI 模拟测试

Fake RLM 必须覆盖：

- Partial Send/Receive；
- 延迟 `*OPC?`；
- `SW1:CLOSe?` 不匹配；
- `LASer:INFO?` 缺少 1310；
- 错连 RLM-110/其他 RLM；
- `FIBER:INFO?` 返回 MM；
- `POW:det#:INFO?` 没有目标 RD-SP；
- Error Queue 非零；
- Laser Disable Timeout。

### 19.3 实机 PoC

#### PoC-1：硬件身份证明

输出并归档：

```text
RLM *IDN? / *OPT? / Firmware / Fiber / Lasers
OSX SW1:INFO? / CH1 / CH24 Readback
RD-SP Model / Serial / Camera / SDK
Remote Head Detector Index
```

退出条件：三台设备均与公司料号和 Station 配置一致。

#### PoC-2：官方 RD-SP Sample

- 创建的必须是 RD-SP，不是 RD-P；
- 12 芯 MPO Mapping 成功；
- Wrong Mapping 能输出 Observed Mapping；
- Mapping 失败能保存全部诊断图像；
- Cancel 后可再次初始化。

#### PoC-3：24 芯 Advanced Mapping

- `OUT=1`；
- `SW1 CH1..24`；
- Laser=1310 nm；
- 逐芯 Collect；
- Expected/Observed 共 24 项；
- 重复 30 次无 Camera 丢失、资源泄漏和 Laser 遗留。

#### PoC-4：Remote Head IL/RL 联调

虽然本方案重点是 Polarity，但 RD-SP 是组合积分球 Detector。必须验证：

- 现有 IL/RL 驱动绑定的是 RD-SP Remote Detector；
- 参考值与 Route/Detector Serial 关联；
- Polarity 完成后不拔 DUT 即可进入 IL/RL；
- 两个 Driver 通过 Session Lock 交接控制权；
- 不出现旧 Driver 与 RD-SP Backend 同时控制 RLM。

#### PoC-5：故障注入

- 拔 RD-SP USB；
- 断 RLM Ethernet/USB；
- 断 OSX Link；
- CH Readback 不一致；
- Ambient Light；
- 中途 Cancel；
- Worker Kill；
- Mapping Failure；
- Diagnostic Disk Full；
- Laser Disable 失败。

每项检查：错误码、日志、图像、Laser、EndMapping、锁释放和下一次恢复。

### 19.4 Golden DUT

至少准备：

- 24 芯正确 Mapping DUT；
- 单对 Cross DUT；
- Missing Fiber DUT；
- Dark Channel DUT；
- Type A/B/C 各一个；
- 高损耗边界样品。

Type A/B/C Golden Mapping 必须由工艺、测试、质量共同签核。

---

## 20. 验收标准

### 20.1 硬件锁定

- 仅接受 `RLM-100-01-3500-S-09-FA-R`；
- 仅接受 `OSX-100-1A-024-09-FA-00B`；
- 仅接受 `RD-SP-100-SM`；
- 不匹配时阻止测试，不弹出“是否继续”。

### 20.2 功能

- 支持 12/24 芯；
- 极性波长固定 1310 nm；
- 24 个 Switch Channel 可配置并读回；
- 返回完整 Expected/Observed；
- 返回逐芯状态；
- 支持 Custom Mapping 和经验证的 Dark Channel；
- Mapping 失败保存完整诊断；
- 支持 Cancel/Timeout/Recovery。

### 20.3 安全与可靠性

- 任意异常路径都执行 Laser Off；
- Laser Off 失败阻止下一次测试；
- 无双主控；
- 100 次连续测试无持续 Handle/Memory 增长；
- Worker/Host 异常后可恢复；
- Timeout 不得变成空值、0 或 PASS；
- 设备/Route/Profile 全可追溯。

### 20.4 性能

不沿用 RLM-110 的 `<25 s` 指标，也不把 RLM-100 数据表的 12 芯双波长 IL/RL `30 s` 当成 Polarity-only 保证。

在最终硬件上记录：

- 12 芯 Polarity-only P50/P95；
- 24 芯 Polarity-only P50/P95；
- 24 芯 Polarity + IL/RL P50/P95；
- 不同 Loss Compensation；
- PASS 与 Mapping Failure；
- Diagnostic Image 保存开/关。

由生产节拍要求确定最终 SLA。

---

## 21. 实施阶段

### Phase 0：配置冻结与 SDK Gate（2-3 天）

- 拍摄并归档三台设备铭牌；
- 获取 Serial、Firmware、Cable/Port 拓扑；
- 获取 OPL-SDK Header/Lib/Sample；
- 判断 OPL-SDK 是否完整支持 RD-SP；
- 若不支持，冻结 Worker 架构；
- 确认公司是否允许随 Native UDL 部署隔离 Worker；
- 固定 SDK/Camera/OpenCV/VISA 版本。

**退出条件：** Backend 路线唯一确定。

### Phase 1：RD-SP Sandbox（3-5 天）

- 识别 RD-SP-100-SM；
- 验证 RLM-100 Hardware Requirements；
- 跑通 12 芯 Basic 和 Advanced；
- 获取 Observed Mapping/Detected Fibers/Diagnostic Images。

**退出条件：** 官方算法在公司实机上稳定工作。

### Phase 2：Native 24 芯核心（5-8 天）

- Native C ABI；
- RLM-100/OSX-100 严格校验；
- SW1 CH1..24 Route；
- 1310 nm；
- Start/Collect/Process/End；
- RAII、Global Lock、日志。

**退出条件：** 24 芯 Golden DUT 稳定输出 Observed Mapping。

### Phase 3：MFC UDL 集成（5-8 天）

- UI Worker Thread；
- Cancel/Progress；
- MIMS/MES Result DTO；
- 与 `UDL.SantecRLM` Session 交接；
- IL/RL Remote Head 绑定。

**退出条件：** 单次连接完成公司目标 Test Flow。

### Phase 4：量产硬化（5-10 天）

- 100 次循环；
- 故障注入；
- 安装包/Manifest/Hash；
- Golden Profile 审批；
- Recovery SOP；
- 软件/测试/工艺/质量联合验收。

---

## 22. 厂商确认清单

1. `RD-SP-100-SM` 在 Santec Hardware Library 2.3.1.0 中的准确 Factory/API 示例是什么？
2. OPL-SDK 是否覆盖 RD-SP Start/Collect/Process/End 和诊断图像？
3. `RLM-100-01-3500-S-09-FA-R` 是否满足 RD-SP 的硬件要求？最低 Firmware 是什么？
4. `OSX-100-1A-024-09-FA-00B` 接到 RLM 后是否固定枚举为 SW1？
5. RD-SP Advanced/Segmented 模式下是否允许用户自己控制 RLM/OSX？
6. RD-SP-100-SM 推荐使用 1310 nm 是否正确？
7. RD-SP Loss Compensation 的单位、范围和公司光路推荐值是什么？
8. 2.3.1.0 Mapping Failure 应读取哪个诊断图像属性？
9. 24 芯、Custom Mapping 和 Dark Channel 的最大/限制条件是什么？
10. RD-SP Camera Serial、Device Serial 和 Remote Detector Serial 是否相同？
11. RD-SP 作为 Remote Integrating Sphere 时，RLM `POW:det#` 应如何绑定？
12. Cancel/Timeout 后推荐的 End/Uninitialize/Recovery 顺序是什么？
13. SDK 是否线程安全？一个 PC 可否连接多个工位的 RD-SP？
14. Native/Managed DLL、Camera Runtime、Sample 的再发布许可是什么？
15. 是否提供 RD-SP 模拟设备或 Golden Image 数据集？

---

## 23. 风险登记

| 风险 | 影响 | 对策 |
|---|---:|---|
| OPL-SDK 不覆盖 RD-SP Mapping | 高 | Phase 0 Gate；改用隔离 Worker |
| 公司禁止任何 .NET 部署 | 高 | 要求 Santec Native SDK；项目暂停，不自研算法 |
| 旧 RLM PDF 只写 RD-P | 中 | 用公司实物料号 + 当前 RD-SP 官方资料 + 实际 SDK 验证 |
| RD-SP API 沿用 RDP 命名造成误创建 | 高 | 校验实际 Model/Serial；不以类名推断硬件 |
| Remote Head Detector Index 不固定 | 高 | `POW:NUM?` + `POW:det#:INFO?` 按 Serial 绑定 |
| OSX 实际枚举不是 SW1 | 高 | Commissioning 明确验证并签核 Station Route |
| 双主控 RLM/OSX | 高 | Global Lock + Session Owner 状态机 |
| 误用 1550/1300 做 Mapping | 中 | Profile/代码双重限制 1310 |
| Type A/B/C 方向误解 | 高 | Golden DUT + Expected Mapping 签核 |
| Worker/Camera 崩溃导致 Laser 未关 | 极高 | Native Watchdog、RAII、故障注入、硬件恢复 SOP |
| OpenCvSharp/TIS 版本冲突 | 高 | 固定版本与 SHA-256，不自动升级 |
| Diagnostic Images 占满磁盘 | 中 | 配额、FAIL-only、保留策略 |

---

## 24. 最终决策摘要

> 公司最终方案应建立在 `RLM-100-01-3500-S-09-FA-R + OSX-100-1A-024-09-FA-00B + RD-SP-100-SM` 的固定组合上。Native `UDL.SantecPolarity.dll` 对外保持 C ABI 和 Visual Studio C++ MFC 风格；RLM 固定单输出、极性固定 1310 nm、OSX 固定 SW1 1x24、Detector 固定 RD-SP Remote Head。驱动采用官方 RD-SP Mapping 能力，不自研相机算法；优先使用经验证覆盖 RD-SP 的 Native OPL-SDK，否则通过隔离 Worker 使用官方 Hardware Library。Type A/B/C 以签核后的 Expected Mapping Profile 实现，所有测试通过单一 Coordinator 和全局设备锁避免双主控，并在任何退出路径强制关闭 Laser、结束 Mapping、保存逐芯结果与诊断图像。

---

## 25. 依据与参考

1. `RLM-100-C-E_Ver1-6_CODE-202506-MB-KT-CPY.pdf`，Version 1.6，2025-06：RLM-100/OSX-100 规格、Remote Interface、订货编码和 RD-P 历史选项。
2. `RLM-User-Manual_2025.8.18.pdf`：RLM TCP 5025、USBTMC/VISA、LF 终止、同步 SCPI、Laser、Detector、Output、Switch 和 IL/RL 命令。
3. Santec RD-SP-100 官方产品页：组合积分球与极性检测、RLM/OSX 集成、Custom Mapping、Dark Channel。  
   <https://inst.santec.com/products/test-and-measurement/polaritytester/rd-sp-100>
4. Santec RD-SP 产品发布说明：RD-SP 与 RLM/OSX 组合并在同一测试阶段完成 Polarity、IL、RL。  
   <https://inst.santec.com/news/rd-sp>
5. Santec RLM-100 官方产品页：RLM-100、OSX、Polarity、Remote Detector 和自动化能力。  
   <https://inst.santec.com/products/test-and-measurement/ilrlmeter/rlm-100>
6. Santec OSX-100 官方产品页：1xN 配置、USB/Ethernet、RLM 接管 Switch 和生产自动化。  
   <https://inst.santec.com/products/test-and-measurement/switch/osx-100>
7. Santec Hardware Library Programming Guide 2.3.1.0：RD-P/RD-SP API 类型、Advanced Mapping、依赖项和版本差异。  
   <https://santec-inst.files.svdcdn.com/production/Santec-Hardware-Library-Programming-Guide-2.3.1.0.pdf>
