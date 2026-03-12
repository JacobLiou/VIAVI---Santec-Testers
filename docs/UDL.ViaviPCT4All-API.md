# UDL.ViaviPCT4All API 文档

## 概述

`UDL.ViaviPCT4All.dll` 提供 Viavi MAP PCT/mORL 的 C 风格导出接口（前缀 `PCT4_`），支持：

- TCP 通信（默认端口 `8301`）
- VISA 通信（通过 `PCT4_CreateDriverEx` 指定）
- 原始 SCPI 透传（未封装命令可直接发送）

调用约定为 `WINAPI (__stdcall)`，句柄类型为 `HANDLE`。

---

## 导出函数总览

当前导出函数（与 `PCT4AllExports.h` / `.def` 对齐）：

### 1) 驱动生命周期

```c
HANDLE WINAPI PCT4_CreateDriver(const char* type, const char* ip, int port, int slot);
HANDLE WINAPI PCT4_CreateDriverEx(const char* type, const char* address, int port, int slot, int commType);
void   WINAPI PCT4_DestroyDriver(HANDLE hDriver);
```

- `type` 支持：`"viavi"`, `"pct"`, `"morl"`, `"pct4all"`, `"map"`（不区分大小写）
- `port <= 0` 时使用默认端口 `8301`
- `slot` 参数当前版本未参与驱动创建逻辑（保留参数）
- 失败返回 `NULL`

### 2) 连接管理

```c
BOOL WINAPI PCT4_Connect(HANDLE hDriver);
void WINAPI PCT4_Disconnect(HANDLE hDriver);
BOOL WINAPI PCT4_Initialize(HANDLE hDriver);
BOOL WINAPI PCT4_IsConnected(HANDLE hDriver);
```

- `PCT4_Initialize` 主要执行远程模式切换和配置读取（内部会发送 `*REM`、`:CONFig?`）

### 3) 设备信息与错误

```c
BOOL WINAPI PCT4_GetIdentification(HANDLE hDriver, ViaviPCT4All::CIdentificationInfo* info);
BOOL WINAPI PCT4_GetCassetteInfo(HANDLE hDriver, ViaviPCT4All::CCassetteInfo* info);
int  WINAPI PCT4_CheckError(HANDLE hDriver, char* message, int messageSize);
```

- `PCT4_CheckError` 返回错误码，`0` 通常表示无错误
- 无效句柄/异常场景返回 `-1`

### 4) 原始 SCPI

```c
BOOL WINAPI PCT4_SendCommand(HANDLE hDriver, const char* command, char* response, int responseSize);
BOOL WINAPI PCT4_SendWrite(HANDLE hDriver, const char* command);
```

- `PCT4_SendCommand`：发送查询并写入 `response`
- `PCT4_SendWrite`：发送写命令（无响应）

### 5) 测量控制

```c
BOOL WINAPI PCT4_StartMeasurement(HANDLE hDriver);
BOOL WINAPI PCT4_StopMeasurement(HANDLE hDriver);
int  WINAPI PCT4_GetMeasurementState(HANDLE hDriver);
BOOL WINAPI PCT4_WaitForIdle(HANDLE hDriver, int timeoutMs);
BOOL WINAPI PCT4_MeasureReset(HANDLE hDriver);
```

- `PCT4_StartMeasurement` 非阻塞
- `PCT4_WaitForIdle` 内部轮询状态，遇到 `FAULT` 或超时返回 `FALSE`

### 6) 配置

```c
BOOL WINAPI PCT4_SetFunction(HANDLE hDriver, int mode);
int  WINAPI PCT4_GetFunction(HANDLE hDriver);
BOOL WINAPI PCT4_SetWavelength(HANDLE hDriver, int wavelength);
BOOL WINAPI PCT4_SetSourceList(HANDLE hDriver, const char* wavelengths);
BOOL WINAPI PCT4_SetAveragingTime(HANDLE hDriver, int seconds);
BOOL WINAPI PCT4_SetRange(HANDLE hDriver, int range);
BOOL WINAPI PCT4_SetILOnly(HANDLE hDriver, int state);
BOOL WINAPI PCT4_SetConnection(HANDLE hDriver, int mode);
BOOL WINAPI PCT4_SetBiDir(HANDLE hDriver, int state);
```

### 7) 光路/Path 控制

```c
BOOL WINAPI PCT4_SetChannel(HANDLE hDriver, int group, int channel);
int  WINAPI PCT4_GetChannel(HANDLE hDriver, int group);
BOOL WINAPI PCT4_SetPathList(HANDLE hDriver, int sw, const char* channels);
BOOL WINAPI PCT4_SetLaunch(HANDLE hDriver, int port);
```

### 8) 日志与 VISA 枚举

```c
typedef void (WINAPI *PCT4LogCallback)(int level, const char* source, const char* message);
void WINAPI PCT4_SetLogCallback(PCT4LogCallback callback);
int  WINAPI PCT4_EnumerateVisaResources(char* buffer, int bufferSize);
```

- `PCT4_EnumerateVisaResources` 返回资源数量，`buffer` 使用 `;` 分隔资源字符串
- 若 VISA 未加载或未找到资源，返回 `0`

---

## C 兼容结构体

以下结构体定义位于 `PCT4AllTypes.h`：

```c
struct CIdentificationInfo
{
    char manufacturer[64];
    char platform[64];
    char serialNumber[64];
    char firmwareVersion[64];
};

struct CCassetteInfo
{
    char serialNumber[64];
    char partNumber[64];
    char firmwareVersion[64];
    char hardwareVersion[64];
    char assemblyDate[32];
    char description[128];
};
```

说明：

- `CMeasurementResult`、`CConnectionConfig` 也在头文件中定义，但当前并未作为 C 导出函数参数/返回值使用。

---

## 常用枚举值（来自 `PCT4AllTypes.h`）

### 通信类型（`commType`）

| 值 | 含义 |
|---|---|
| 0 | `COMM_TCP` |
| 1 | `COMM_GPIB` |
| 2 | `COMM_VISA` |

### 测量模式（`PCT4_SetFunction`）

| 值 | 含义 |
|---|---|
| 0 | `MODE_REFERENCE` |
| 1 | `MODE_DUT` |

### 测量状态（`PCT4_GetMeasurementState`）

| 值 | 含义 |
|---|---|
| 0 | `MEAS_INITIALIZING` |
| 1 | `MEAS_IDLE` |
| 2 | `MEAS_BUSY` |
| 3 | `MEAS_FAULT` |
| 4 | `MEAS_SYSTEM` |

### Range（`PCT4_SetRange`）

| 值 | 含义 |
|---|---|
| 0 | `RANGE_200M` |
| 1 | `RANGE_500M` |
| 2 | `RANGE_1KM` |
| 3 | `RANGE_2KM` |
| 4 | `RANGE_5KM` |
| 5 | `RANGE_10KM` |

### Path Group（`PCT4_SetChannel` / `PCT4_GetChannel`）

| 值 | 含义 |
|---|---|
| 1 | `GROUP_MTJ1` |
| 2 | `GROUP_MTJ2` |
| 3 | `GROUP_RECEIVE` |

### Connection Mode（`PCT4_SetConnection`）

| 值 | 含义 |
|---|---|
| 1 | `CONN_SINGLE_MTJ` |
| 2 | `CONN_DUAL_MTJ` |

### 日志级别（`PCT4_SetLogCallback`）

| 值 | 含义 |
|---|---|
| 0 | `LOG_DEBUG` |
| 1 | `LOG_INFO` |
| 2 | `LOG_WARNING` |
| 3 | `LOG_ERROR` |

---

## 典型调用流程（C API）

```c
HANDLE h = PCT4_CreateDriver("viavi", "10.14.140.32", 8301, 0);
if (!h) { /* handle error */ }

if (!PCT4_Connect(h)) { /* handle error */ }
if (!PCT4_Initialize(h)) { /* handle error */ }

PCT4_SetSourceList(h, "1310,1550");
PCT4_SetAveragingTime(h, 5);
PCT4_SetRange(h, 2);

PCT4_SetFunction(h, 0);          // REFERENCE
PCT4_StartMeasurement(h);
PCT4_WaitForIdle(h, 60000);

PCT4_SetFunction(h, 1);          // DUT
PCT4_StartMeasurement(h);
PCT4_WaitForIdle(h, 60000);

PCT4_Disconnect(h);
PCT4_DestroyDriver(h);
```

---

## C++ 接口说明

`IViaviPCT4AllDriver` 仍完整覆盖 Appendix A / Chapter 2~5 中的 SCPI 命令（`MEASure`、`PATH`、`PMAP`、`SENSe`、`SOURce`、Factory、System 等）。

若 C 导出层暂未封装某些能力，可通过：

- `PCT4_SendCommand`（Query）
- `PCT4_SendWrite`（Write）

直接透传 SCPI 指令。
