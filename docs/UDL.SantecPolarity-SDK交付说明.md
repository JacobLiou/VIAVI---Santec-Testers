# UDL.SantecPolarity SDK 交付说明

面向两类读者：

| 角色 | 本文用途 |
|---|---|
| **驱动开发（你）** | 功能已合入后的**真机调试清单**，以及向同事交包前的自检 |
| **集成软件（同事）** | 拿到 zip / 源码后如何部署、如何 `LoadLibrary`、有哪些硬约束 |

接口细节（结构体、状态码、导出函数）见同目录 [UDL.SantecPolarity-API.md](UDL.SantecPolarity-API.md)。  
可直接交付的二进制包由 `TestEquipmentDrivers/UDL.SantecPolarity.Pack/pack-polarity-dist.ps1` 生成。

---

## 1. 产品与边界（先对齐）

| 项 | 说明 |
|---|---|
| 驱动 DLL | `UDL.SantecPolarity.dll`（C ABI，`SP_*`，`__stdcall`） |
| 目标硬件 | **RLM-100 + OSX-100（1×24）+ RD-SP-100-SM**（公司量产 BOM） |
| 实现形态 | Native 外壳 + 同进程 `UDL.SantecPolarity.Cli`（C++/CLI → **Santec.Hardware 2.3.1.0**） |
| Host 加载 | 只 `LoadLibrary("UDL.SantecPolarity.dll")`，**不要**直接加载 Cli / Hardware / Core |
| 位数 | Host、本 DLL、Cli、Hardware **必须同位数**（x64 或 Win32/x86，勿混用） |
| 非目标 | 独立 PTM（`UDL.SantecPTM`）、已冻结的虚构 `POLARITY:*` stub（`SantecPolarity/`） |

与 `UDL.SantecRLM` 同时跑时：同一 RLM 序列号互斥，锁名  
`Global\UDL_SANTEC_RLM100_<serial>`。极性 Session（`SP_Open`～`SP_Close`）期间禁止 RLM 驱动再主控同一台。

---

## 2. 驱动侧：真机调试清单（功能完成后的下一步）

目标：在**不改业务 Host** 的前提下，用交付包自带的 `PolarityTestApp.exe` 把链路跑通，再交给同事。

### 2.1 环境一次装齐

- [ ] Windows 10/11，位数与包一致（日常联调建议 **x64**）
- [ ] .NET Framework **4.8**
- [ ] VC++ 2015–2022 运行库（与包位数一致）
- [ ] RD-SP USB 相机驱动 / Imaging Control（按 Santec 安装说明）
- [ ] RLM / OSX 可达（网线 TCP 默认 **5025**，或已装 VISA 的 USB）
- [ ] 解压 `dist/UDL.SantecPolarity-x64.zip`（或 Win32 包），**整份 `bin` 勿拆散**

### 2.2 用 PolarityTestApp 冒烟

在 `bin` 目录启动 `PolarityTestApp.exe`（工作目录必须是 `bin`，否则依赖 DLL 找不到）。

| 步骤 | 操作 | 期望 |
|---|---|---|
| 1 | DLL 路径 `UDL.SantecPolarity.dll` → **Load** | Status: DLL Loaded |
| 2 | **Create** | `SP_Create` OK |
| 3 | 填 **Station**（可选）、**RLM SN**（建议填真机序列号）、RDSP SN / Profile（可空）→ **Open** | `SP_Open` OK；失败看 Log / `SP_GetLastErrorUtf8` |
| 4 | **Snapshot** | 能看到 RLM / OSX / RDSP 部件号、串号、通道数等 |
| 5 | **Run Mapping** | 返回 `SP_OK` 或**明确**设备类错误（不是缺 DLL / 类型初始化失败） |
| 6 | **Get Results** | 纤芯表有行；Summary 有 pass/fail/elapsed |
| 7 | **Close** → **Destroy** → **Unload** | 无崩溃；RLM 锁已释放（可再 Open 或让 RLM 驱动连接） |

说明：TestApp 里 **Run Mapping 会卡住 UI**，调试可接受；同事的产线 Host **必须**把 `SP_RunMapping` 放到工作线程。

### 2.3 真机调试常见失败对照

| 现象 | 优先排查 |
|---|---|
| Load 失败 / 缺导出 | 位数不对；路径不是 `bin`；DLL 不是本包产物 |
| Open：`SP_E_DEVICE_BUSY` | 另一进程/线程已占用同 RLM SN（RLM TestApp / 上一 Session 未 Close） |
| Open：找不到设备 / 通信失败 | IP/VISA、线缆、相机 USB、防火墙；RLM SN 是否写对 |
| Open / Mapping：缺程序集 / 类型初始化 | `bin` 被拆散；未使用包内 `PolarityTestApp.exe.config` / Host 未合并 bindingRedirect |
| Mapping 超时 / 取消 | 光路、光纤未插好、环境光；可试 Cancel 后看 LastError |
| 与 RLM 驱动抢设备 | 极性 Open 期间不要再连同一台 RLM；先 Close 再给 RLM |

### 2.4 调试通过的最低标准（再交同事）

1. 同一台工位：**Open → Snapshot → RunMapping → GetResults → Close** 至少成功 1 次。  
2. 断线重连 / 再次 Open 不崩溃。  
3. 故意填错 RLM SN 或拔相机：返回明确错误码 + `SP_GetLastErrorUtf8` 可读，Host 不崩。  
4. （若同机有 RLM 驱动）极性 Close 后，RLM 驱动能再次连接同序列号。

---

## 3. 向同事交付什么

### 3.1 推荐交付物（二进制包，优先）

把下列 zip **按 Host 位数**交给集成同事（不要混位数）：

| 文件 | 内容 |
|---|---|
| `dist/UDL.SantecPolarity-x64.zip` | 64-bit：`bin` + `include` + `docs` + README |
| `dist/UDL.SantecPolarity-Win32.zip` | 32-bit（x86）同上 |
| `dist/UDL.SantecPolarity-Both.zip` | 双平台合集（`x64/`、`Win32/` 子目录） |

重新打包装命令：

```powershell
cd TestEquipmentDrivers\UDL.SantecPolarity.Pack
powershell -ExecutionPolicy Bypass -File .\pack-polarity-dist.ps1
# 默认 Release + Both，会编 Cli / Native / PolarityTestApp 并收依赖
```

### 3.2 `bin` 里有什么（同事只需关心用法）

| 类别 | 代表文件 | 同事是否直接使用 |
|---|---|---|
| 驱动入口 | `UDL.SantecPolarity.dll` | **是** — 唯一 `LoadLibrary` 目标 |
| 调试工具 | `PolarityTestApp.exe` + `.exe.config` | 可选 — 联调 / 对照 |
| 同进程桥 | `UDL.SantecPolarity.Cli.dll` | 否 — 由驱动内部加载，须同目录 |
| 钉死版本 | `Santec.Hardware.dll`（2.3.1.0）、`Santec.Core.dll` | 否 — 同目录依赖 |
| 托管/原生依赖 | OpenCvSharp*、Zeroconf、TIS.Imaging.*、Nito.*、System.Text.Json… | 否 — 同目录，**勿删勿换版本** |
| 配置样例 | `Host.exe.config.sample` | **是** — 合并进业务 `YourApp.exe.config` |
| 头文件 | `../include/SPTypes.h`、`SPExports.h` | **是** — 拷到工程或对照写 loader |
| 文档 | `../docs/*.md`、包根 `README.md` | **是** |

原则：**`bin` 整目录拷贝到 Host EXE 同目录（或你们约定的驱动目录），保持相对关系。**

### 3.3 若同事还要源码（次要）

一般业务集成**不需要**编驱动。若需要对照或二次打包，可额外给仓库路径：

| 路径 | 说明 |
|---|---|
| `TestEquipmentDrivers/UDL.SantecPolarity/` | Native `SP_*` 外壳 |
| `TestEquipmentDrivers/UDL.SantecPolarity.Cli/` | C++/CLI + `libs/`（Hardware x64/x86、Core、TIS） |
| `TestEquipmentDrivers/PolarityTestApp/` | 动态加载示例（等同同事应写的 loader） |
| `TestEquipmentDrivers/UDL.SantecPolarity.Pack/` | 打包脚本 |
| `docs/UDL.SantecPolarity-API.md` | 接口权威说明 |
| `docs/UDL.SantecPolarity-SDK交付说明.md` | 本文 |

构建基线：Visual Studio 2022，`Release|x64`（或 `Release|Win32`）。业务同事日常只需 zip，不必装整套 VS。

### 3.4 交包时建议附带的一句话清单

发给同事时可复制：

```text
附件：UDL.SantecPolarity-<x64|Win32>.zip
1) 解压后把整个 bin 放到你们 Host EXE 同目录
2) 只 LoadLibrary(UDL.SantecPolarity.dll)，头文件见 include/
3) 把 bin/Host.exe.config.sample 的 assemblyBinding 合并进 YourApp.exe.config
4) SP_RunMapping 必须工作线程；建议 SP_Open 传 rlmSerialUtf8
5) 联调可先跑 bin/PolarityTestApp.exe；接口见 docs/UDL.SantecPolarity-API.md
6) 运行时：.NET 4.8 + 对应位数 VC++ 红包 + RD-SP 相机驱动
```

---

## 4. 同事集成指南（SDK 用法）

### 4.1 最小调用顺序

```text
SP_Create
  → SP_Open(&options)          // 建议填 rlmSerialUtf8
  → SP_GetHardwareSnapshot(...) // 可选，确认工位
  → SP_RunMapping()            // 必须工作线程
  → SP_GetSummary / SP_GetFiberResultCount / SP_GetFiberResults
  → SP_Close
  → SP_Destroy
```

失败时调用 `SP_GetLastErrorUtf8` 取可读信息；不要忽略返回的 `SP_STATUS`。

### 4.2 `SP_OPEN_OPTIONS`（交包常用字段）

| 字段 | 建议 |
|---|---|
| `structSize` / `version` | 填 `sizeof` 与 `1` |
| `stationIdUtf8` | 工位名，可空 |
| `rlmSerialUtf8` | **建议必填**真机序列号（锁与选设备） |
| `rdspSerialUtf8` | 多相机时指定；单台可空 |
| `profilePathUtf8` | 有工位 profile 再填；可空 |
| `connectTimeoutMs` / `operationTimeoutMs` / `mappingTimeoutMs` | 按产线 SLA；Mapping 建议 ≥ 120s |

结构体定义见 `include/SPTypes.h`。

### 4.3 动态加载约定

- 与现有 RLM/OSX/PCT TestApp 相同：`LoadLibrary` + `GetProcAddress("SP_Create")` 等。  
- 参考实现：`TestEquipmentDrivers/PolarityTestApp/SantecPolarityDllLoader.h`。  
- 导出列表：`SPExports.h` / `UDL.SantecPolarity.def`。  
- **不要**静态链接导入库到业务工程（保持与其它 UDL 一致：纯动态加载）。

### 4.4 Host `.exe.config`（必做）

托管依赖有 bindingRedirect。将包内 `bin/Host.exe.config.sample` 的  
`<runtime><assemblyBinding>...</assemblyBinding></runtime>`  
**合并**进 `YourApp.exe.config`（文件名必须等于 Host EXE 名）。  
PolarityTestApp 已自带同内容的 `PolarityTestApp.exe.config`，可作对照。

### 4.5 线程与生命周期

| 规则 | 原因 |
|---|---|
| `SP_RunMapping` 在工作线程 | Mapping 可达数十秒～数分钟，阻塞 UI / 主调度 |
| 同一 `SP_HANDLE` 不要并行乱调 | Session 有内部状态；Cancel 可从其它线程请求 |
| 进程退出前 `Close` + `Destroy` | 关激光、释放全局 RLM 锁、卸 Cli |
| 一个进程可加载多个 UDL DLL | 前缀 `SP_` 已与 `PTM_` / RLM 等隔离 |

### 4.6 结果怎么用

- `SP_RESULT_SUMMARY`：总判定、纤芯数、pass/fail、耗时。  
- `SP_FIBER_RESULT[]`：逐芯期望/实测/有光/状态枚举。  
- 诊断图路径：`SP_GetDiagnosticPathUtf8`（若有保存）。  
- 解析失败或字段含义以真机确认为准时：**保留 Summary/逐芯原始语义**，业务侧展示可后改。

---

## 5. 双方分工与变更约定

| 事项 | 驱动侧 | 集成侧 |
|---|---|---|
| 真机 Mapping 是否通 | 用 PolarityTestApp 验收 | 按同样步骤在 Host 复现 |
| ABI / 导出符号变更 | 改 `SPTypes`/`SPExports` 并升文档版本，通知集成 | 同步 loader / 头文件 |
| 依赖 DLL 增减 | 更新打包脚本与本交付说明 | 整包替换 `bin`，勿只换一个 DLL |
| Hardware 版本 | **钉死 2.3.1.0**，禁止私自替换 | 不要用 Samples 目录旧 DLL 覆盖 |
| RLM 并行 | 文档化锁行为 | Open 极性前停掉同 SN 的 RLM 测量 |

---

## 6. 相关文档索引

| 文档 | 用途 |
|---|---|
| [UDL.SantecPolarity-API.md](UDL.SantecPolarity-API.md) | C ABI、结构体、状态码 |
| 交付包内 `README.md` | 打开 zip 后的最短说明 |
| `TestEquipmentDrivers/UDL.SantecPolarity.Pack/README.md` | 如何重新打包 |
| `AGENTS.md`（仓库根） | 本仓库驱动三件套与工程约定 |

---

## 7. 修订记录

| 日期 | 说明 |
|---|---|
| 2026-11-25 | 初版：真机调试清单 + 二进制/依赖交付 + 同事集成要点；含 x64/Win32 与 PolarityTestApp |
