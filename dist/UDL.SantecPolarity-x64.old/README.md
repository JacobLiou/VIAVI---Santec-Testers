# UDL.SantecPolarity 业务交付包 (x64 / Release)

打开即用：将 `bin` 目录全部文件拷贝到业务 Host EXE 同目录（或你们约定的驱动目录），按下面方式 `LoadLibrary`。

## 目录

| 路径 | 说明 |
|---|---|
| `bin/` | 驱动与全部运行时依赖（勿拆散） |
| `include/` | `SPTypes.h` / `SPExports.h`（C ABI） |
| `docs/` | 接口说明 |

## 必备运行环境（业务机）

1. Windows 10/11 x64（本包平台：**x64**）
2. **.NET Framework 4.8**
3. **VC++ 2015-2022 x64 运行库**（[Microsoft VC++ Redistributable](https://learn.microsoft.com/cpp/windows/latest-supported-vc-redist)）
4. RD-SP 相机驱动 / Imaging Control（首次接真机按 Santec 说明安装；本包已带 `TIS.Imaging.ICImagingControl35.dll`）
5. 若使用 USB VISA 仪器路径：安装 NI-VISA（本包含 `Ivi.Visa.dll` 托管封装，不替代 VISA 运行时）

## Host 集成要点

1. 只 `LoadLibrary("UDL.SantecPolarity.dll")`，**不要**直接加载 Cli / Hardware。
2. `bin` 内所有 DLL 必须与 `UDL.SantecPolarity.dll` **同目录**。
3. 将 `bin/Host.exe.config.sample` 中的 `assemblyBinding` **合并**进你们的 `YourApp.exe.config`（文件名=Host EXE）。
4. `SP_RunMapping` 必须在**工作线程**调用，禁止 UI 线程阻塞。
5. 与 `UDL.SantecRLM` 同时使用时：同一 RLM 串号互斥（`Global\UDL_SANTEC_RLM100_<serial>`）。

## 最小调用顺序

`SP_Create` → `SP_Open` → `SP_RunMapping` → `SP_GetSummary` / `SP_GetFiberResults` → `SP_Close` → `SP_Destroy`

`SP_OPEN_OPTIONS.rlmSerialUtf8` 建议填工位 RLM 序列号。

## 打包信息

- 生成时间: 2026-11-25 15:31:03
- Configuration: Release
- Platform: x64
- Santec.Hardware: 2.3.1.0（钉死）
- 本包不含：虚构 `POLARITY:*` stub、独立 PTM 驱动、XNS/Cable Assembly 安装包

## 真机冒烟（业务方）

1. 接好 RLM-100 + OSX + RD-SP USB 相机。
2. Host 加载本 DLL，`SP_Open` 成功（`IsRDSP` 应在 snapshot/日志侧可见）。
3. `SP_RunMapping` 返回 `SP_OK` 或明确设备错误（不应出现缺 DLL / 类型初始化失败）。
