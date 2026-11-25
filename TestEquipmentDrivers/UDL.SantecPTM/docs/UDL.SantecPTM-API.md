# UDL.SantecPTM 驱动接口文档

> **BOM / 验收状态（2026-11）：搁置（非当前量产基线）**  
> 公司当前极性硬件锁定为 `RLM-100 + OSX-100 + RD-SP-100-SM`（见 Final Plan 与 `docs/UDL.SantecPolarity-API.md`）。  
> 本驱动仅适用于**独立 PTM** 产品线；在 BOM 未包含 PTM 前，**勿并入 RD-SP 产线验收**。  
> 相对 PTM 手册 Table 8，命令流大体正确；`TEST:RESult?` 解析仍为暂定。

## 概述

| 属性 | 值 |
|---|---|
| DLL 名称 | `UDL.SantecPTM.dll` |
| 用途 | Santec PTM 极性测试仪驱动 |
| 目标设备 | Santec PTM Polarity Tester |
| 协议 | M-PTM_PT1-001.04 第 8 章 SCPI |
| 通信方式 | TCP/IP（默认）、USB (VISA/USBTMC) |
| 默认端口 | **5025** |
| 函数前缀 | `PTM_` |
| 调用约定 | `__stdcall` (WINAPI) |
| 加载方式 | `LoadLibrary` + `GetProcAddress` |
| Factory type | `"ptm"` / `"santecptm"` |
| 量产 BOM | **不含**（搁置；仅独立 PTM 时启用） |

---

## 测量流程

1. `PTM_CreateDriver` / `PTM_CreateDriverEx`
2. `PTM_Connect`（内部 `*IDN?`）
3. `PTM_Initialize`（`*CLS`、`CHANnel?`、`LCL 0`）
4. `PTM_TakeMeasurement(n)` → `TEST:CON{n}` → 轮询 `STAT:OPER:COND?` → `TEST:RESult?`
5. `PTM_GetResults` / `PTM_GetRawResult`

异步约定：发命令后轮询 `STAT:OPER:COND?`，返回 `0` 表示完成（超时默认 30s）。

---

## 数据结构

### CPolarityChannelResult

```c
struct CPolarityChannelResult
{
    int    outputChannel;   // 输出激光通道
    int    inputChannel;    // 检测到的输入通道
    double rawData[4];
    int    rawDataCount;
};
```

### CDeviceInfo

```c
struct CDeviceInfo
{
    char manufacturer[64];
    char model[64];
    char serialNumber[64];
    char firmwareVersion[64];
    int  slot;
};
```

---

## TEST:RESult? 解析约定（暂定）

手册未给出具体返回格式。当前驱动与 Simulator 约定：

| 格式 | 示例 |
|---|---|
| 逗号分隔 out,in 对 | `1,1,2,2,3,3,...,12,12` |
| 冒号对 | `1:1,2:2,3:3` |
| 分号分组 | 与逗号同等处理 |

解析失败时：`PTM_GetResults` 返回 0，但 `PTM_GetRawResult` 仍可取完整原始字符串。  
**真机确认后只需修改 `CSantecPTMDriver::ParsePolarityMap` 与 Simulator 输出。**

---

## C ABI 导出

### 生命周期

| 函数 | 说明 |
|---|---|
| `PTM_CreateDriver(type, ip, port, slot)` | TCP 创建 |
| `PTM_CreateDriverEx(..., commType)` | 0=TCP, 2=VISA |
| `PTM_DestroyDriver` | 销毁 |

### 连接 / 测量

| 函数 | 说明 |
|---|---|
| `PTM_Connect` / `PTM_Disconnect` / `PTM_Initialize` / `PTM_IsConnected` | 连接生命周期 |
| `PTM_GetChannelCount` | `CHANnel?` |
| `PTM_TakeMeasurement(h, nChannels)` | `nChannels<=0` 测全通道 |
| `PTM_GetResults` | 解析后的 Out→In 列表 |
| `PTM_GetRawResult` | 原始 `TEST:RESult?` 字符串 |

### 辅助

| 函数 | SCPI |
|---|---|
| `PTM_Detect` | `DETECT#?` |
| `PTM_SetLocalMode` / `PTM_GetLocalMode` | `LCL` |
| `PTM_LaserOff` | `LAS:OFF` |
| `PTM_SetVfl(ch, mode)` | mode: 0=off, 1=VFL#, 2=FVFL# |
| `PTM_GetBarcode` | `READ:BARC?` |
| `PTM_Notify` | `TEST:NOTIFY# "msg"` |
| `PTM_TestStart` / `TestStatus` / `TestNext` / `TestRetry` | XN1 流程 |
| `PTM_SendCommand` | 透传（以 `?` 结尾走 query） |
| `PTM_EnumerateVisaResources` | `USB?*INSTR` |

---

## 配套工具

| 项目 | 说明 |
|---|---|
| `PTMSimulator` | TCP SCPI 模拟器，默认 5025，`--channels 12\|24` |
| `PTMTestApp` | MFC 调试工具，动态加载 DLL |

联调：先启动 `PTMSimulator`，再打开 `PTMTestApp`，Load DLL → Connect `127.0.0.1:5025` → Initialize → Measure。
