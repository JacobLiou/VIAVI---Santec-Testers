# UDL.SantecPolarity 业务交付包 (Win32 / x86 / Release)

打开即用：将 `bin` 目录全部文件放在一起（勿拆散）。可用 `PolarityTestApp.exe` 直接测驱动，或业务 Host `LoadLibrary`。

## 目录

| 路径 | 说明 |
|---|---|
| `bin/PolarityTestApp.exe` | MFC 调试工具（动态加载 `UDL.SantecPolarity.dll` / `SP_*`） |
| `bin/PolarityTestApp.exe.config` | 已含 bindingRedirect（勿删） |
| `bin/` | 驱动与全部运行时依赖 |
| `include/` | `SPTypes.h` / `SPExports.h`（C ABI） |
| `docs/` | 接口说明 |

## 用 PolarityTestApp 冒烟

1. 双击 `bin/PolarityTestApp.exe`（必须在 `bin` 目录启动，或把工作目录设为 `bin`）。
2. DLL 路径默认 `UDL.SantecPolarity.dll` → **Load** → **Create** → 填 RLM SN（建议）→ **Open**。
3. **Snapshot** 看硬件；**Run Mapping** 跑极性；**Get Results** 看纤芯表。
4. `SP_RunMapping` 会阻塞 UI 线程直到结束（调试可接受；产线 Host 请用工作线程）。

## 必备运行环境（业务机）

1. Windows 10/11（本包平台：**Win32 / x86**）
2. **.NET Framework 4.8**
3. **VC++ 2015-2022 x86 运行库**（[Microsoft VC++ Redistributable](https://learn.microsoft.com/cpp/windows/latest-supported-vc-redist)）
4. RD-SP 相机驱动 / Imaging Control（首次接真机按 Santec 说明安装）
5. 若使用 USB VISA：安装 NI-VISA

## Host 集成要点

1. 只 `LoadLibrary("UDL.SantecPolarity.dll")`，**不要**直接加载 Cli / Hardware。
2. `bin` 内所有 DLL 必须与 `UDL.SantecPolarity.dll` **同目录**。
3. 将 `bin/Host.exe.config.sample` 中的 `assemblyBinding` **合并**进 `YourApp.exe.config`。
4. `SP_RunMapping` 必须在**工作线程**调用（产线 Host）。
5. 与 `UDL.SantecRLM` 同时使用时：同一 RLM 串号互斥（`Global\UDL_SANTEC_RLM100_<serial>`）。

## 最小调用顺序

`SP_Create` → `SP_Open` → `SP_RunMapping` → `SP_GetSummary` / `SP_GetFiberResults` → `SP_Close` → `SP_Destroy`

## 打包信息

- 生成时间: 2026-11-25 15:41:26
- Configuration: Release
- Platform: Win32 (x86)
- Santec.Hardware: 2.3.1.0（钉死）
- 含 PolarityTestApp（SP_*）
