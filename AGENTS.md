# AGENTS.md

本文件为 AI 编程助手（Claude Code / Cursor / Devin 等）在本仓库工作时提供指引。请先完整阅读本文件再动手。

## 项目概述

Windows C++（Win32/MFC）测试设备驱动项目，为 VIAVI 与 Santec 光通信测试仪器提供统一的设备驱动层（UDL, Unified Driver Layer）。

- **语言/技术栈**: C++ (Win32)、MFC Dialog、Winsock (`ws2_32`)、VISA 动态加载
- **构建系统**: Visual Studio 2022 解决方案，MSBuild，平台 Win32/x64，配置 Debug/Release
- **协议**: SCPI over TCP/IP（`\n` 结尾）或 USB（VISA）
- **目标平台**: 仅 Windows（依赖 Win32 API、MFC、`LoadLibrary` 动态加载）

## 构建与验证命令

```bash
# 构建（Git Bash；cmd/PowerShell 中把 //p: 换成 /p:）
MSBUILD="/c/Program Files/Microsoft Visual Studio/2022/Professional/MSBuild/Current/Bin/MSBuild.exe"
"$MSBUILD" "TestEquipmentDrivers/TestEquipmentDrivers.sln" //p:Configuration=Debug //p:Platform=x64 //m

# 单个工程（例如只编驱动）
"$MSBUILD" "TestEquipmentDrivers/UDL.SantecPTM/UDL.SantecPTM.vcxproj" //p:Configuration=Debug //p:Platform=x64
```

- 首选平台/配置：**Debug|x64**（日常开发与联调基线）
- 产物输出在各工程下的 `x64/`、`Debug/` 目录（已 gitignore，勿提交）
- 无单元测试框架；验证方式是**模拟器联调**（见下文「验证流程」）

## 仓库结构

```
TestEquipmentDrivers/
├── TestEquipmentDrivers.sln      # VS 解决方案（解决方案文件夹：Drivers / SImulators / UITestApps）
├── Common/VisaHelper.h           # 共享：仅头文件的 VISA 动态加载器（viOpen/viWrite/viRead…）
├── UDL.SantecRLM/                # 驱动 DLL：Santec RLM 回损计（参考实现）
├── UDL.SantecOSX/                # 驱动 DLL：Santec OSX-100 光开关
├── UDL.SantecPTM/                # 驱动 DLL：Santec PTM 极性测试仪
├── UDL.ViaviPCT/                 # 驱动 DLL：VIAVI MAP-PCT
├── UDL.ViaviPCT4All/             # 驱动 DLL：VIAVI PCT4All
├── UDL.ViaviOSW/                 # 驱动 DLL：VIAVI OSW 光开关
├── SantecSimulator/ OSXSimulator/ PTMSimulator/     # TCP SCPI 模拟器（console）
├── ViaviPCTSimulator/ ViaviOSWSimulator/
├── DriverTestApp3/ OSXTestApp2/ GMSApp/             # MFC 调试工具（UI Test Apps）
├── PCTApp/ PCTMaxApp/ PTMTestApp/ SantecApp/ ViaviApp/
docs/                             # PDF 设备手册（协议的唯一权威来源）+ UDL.*-API.md 接口文档
```

## 架构模式：设备「三件套」

每个设备由三个工程组成，新增设备时**三件套缺一不可**：

1. **驱动 DLL（UDL.<Vendor><Device>）** — C ABI 导出，被上位机 `LoadLibrary` + `GetProcAddress` 动态加载
2. **模拟器（<Device>Simulator）** — console TCP SCPI server，模拟真机应答，支撑无硬件联调
3. **调试工具（<Device>TestApp）** — MFC Dialog，通过各自的 `<Device>DllLoader.h` 手写函数指针绑定驱动全部导出

通信适配器内置于驱动工程：`TcpAdapter`（Winsock，Santec 默认端口 5025；VIAVI PCT 为 8301，以代码为准）与 `VisaAdapter`（基于 `Common/VisaHelper.h`）。

## 驱动 C ABI 规范（必须严格遵守）

- `extern "C" __stdcall (WINAPI)`，导出经 `.def` 文件声明（避免 LNK4197 重复导出，勿改用 `__declspec(dllexport)`）
- **函数前缀按设备隔离**：`PTM_` / `OSX_` / `PCT_` 等（RLM 例外，用无前缀 `CreateDriver`；前缀存在的理由是同一进程可加载多个驱动 DLL，避免符号冲突）
- 句柄为不透明 `HANDLE`；生命周期：`<P>_CreateDriverEx(type, address, port, slot, commType)` → 各功能调用 → `<P>_DestroyDriver`
- Factory type 字符串双别名，如 `"ptm"` / `"santecptm"`
- CommType：`0=TCP`，`2=USB(VISA)`；GPIB/DLL 仅打日志返回 false
- 标配导出集：`CreateDriver` / `CreateDriverEx` / `DestroyDriver` / `Connect` / `Disconnect` / `Initialize` / `IsConnected` / `GetDeviceInfo` / `CheckError` / `SendCommand`（SCPI 透传）/ `SetLogCallback` / `EnumerateVisaResources`
- 出参缓冲区一律 `char* buffer, int bufferSize` 模式；结构体（`CDeviceInfo`、结果结构等）定义在 `*Types.h`，固定长度字符数组，保持 C 兼容

## 关键约束

- **VISA 零硬依赖**：禁止直接 `#include <visa.h>` 或链接 visa lib；必须经 `Common/VisaHelper.h` 动态加载（支持 NI/R&S/Keysight 运行时）
- **协议权威来源**：`docs/` 下的 PDF 手册（如 `PTM-User-Manual.pdf` 第 8 章 SCPI 表）。实现命令前先查手册，不确定时保留 raw 字符串
- **Raw 优先原则**：测量结果先保存原始字符串并提供 `GetRawResult`；解析器（如 `ParsePolarityMap`/`ParseDoubleList`）独立可替换，解析失败不得判驱动失败——真机格式确认后只改解析器
- **异步等待**：轮询状态寄存器（如 `STAT:OPER:COND? == 0`），间隔 10–50ms，超时放宽（约 30s）
- **解决方案文件夹**：改 sln 时新工程挂到 `Drivers` / `SImulators` / `UITestApps`（注意 `SImulators` 是历史拼写，勿"修正"）
- **MFC 工程细节**：`stdafx.h` 预编译头必须最先 include；`.rc`/`resource.h` 由 VS 生成，手工合并对话框控件时保持 `IDD_`/`IDC_` 命名一致

## 新增设备驱动工作流（克隆改造法）

标准做法是从最接近的同厂驱动克隆改名（先例：UDL.SantecRLM → UDL.SantecPTM）：

1. 复制参考驱动工程目录，重命名所有文件与类型（`DriverFactory` / `Exports` / `Types` / `ScpiCommands` / 通信适配器），生成新 GUID 加入 `.sln`
2. 精简/扩展驱动接口：删掉不适用的能力，按手册 SCPI 表新增命令封装（常量集中在 `<Device>ScpiCommands.h`）
3. 更新 `.def` 导出列表与 `<Device>DllLoader.h` 函数指针绑定
4. 克隆模拟器：console TCP server，实现 `*IDN?`（如 `SANTEC,PTM,...`）、`*CLS`、`SYST:ERR?` 及设备命令
5. 克隆 TestApp：对话框覆盖 Load/Unload、Connect、Initialize、测量、结果表格、Raw 显示、透传 SCPI、日志窗口
6. 新增 `docs/UDL.<Name>-API.md`（对照现有 API 文档格式），格式未定论处明确标注「暂定，真机确认后只改解析器」

## 验证流程（无硬件）

1. `Debug|x64` 编译通过（驱动 + 模拟器 + TestApp 三个工程）
2. 启动模拟器（如 `PTMSimulator.exe`，默认 `127.0.0.1:5025`）
3. 打开 TestApp → Load DLL → Connect → Initialize → 执行测量 → 结果表格与 Raw 区有正确输出
4. 透传 `*IDN?`、断开/重连、错误路径不崩溃

## 文档约定

- 每个驱动在 `docs/UDL.<Name>-API.md` 维护接口文档（结构体、枚举、导出函数、SCPI 映射表）；**改导出接口必须同步更新**
- PDF 手册是协议唯一权威，不可篡改；`docs/指令列表.md` 等摘录文档注明来源章节
- 用户与团队沟通使用中文；代码标识符、SCPI 命令保持英文

## Git 约定

- 提交信息使用中文，简述动机与变更（可参考历史：`feat: 基础实现Santec极性测试仪的驱动和模拟器 调试APP`）
- 不要提交构建产物（`x64/`、`Debug/`、`Release/`、`*.user` 已忽略，新增产物目录记得补 `.gitignore`）
- 未经明确要求不要推送、不要重写历史

## AI 协作注意事项

- 本项目是真机产线工具的前置驱动层，**接口稳定性优先于实现优雅**：改 C ABI 前先确认调用方（各 TestApp 的 DllLoader）
- 修改既有驱动时先读对应 `docs/UDL.*-API.md` 与 PDF 手册相关章节，再读 `SantecSimulator`/对应模拟器的应答实现，确保三方一致
- 不确定真机行为时，保持 raw 透传能力（`SendCommand`）可用，绝不猜测后隐藏错误
- 编译报错优先怀疑：字符集（MBCS/Unicode）、预编译头、`__stdcall` 签名不匹配、.def 与导出名不一致
