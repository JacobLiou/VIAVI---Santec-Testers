# RD-SP SDK / 可编程集成 — 供应商索取清单

发往 Santec（或本地代理）的技术请求模板。目标：在公司 Native UDL（非直接写 XNS）中驱动  
`RLM-100-01-3500-S-09-FA-R` + `OSX-100-1A-024-09-FA-00B` + `RD-SP-100-SM` 完成极性 Mapping。

请供应商书面确认下列各项；收到前 `UDL.SantecPolarity` 仅保持 ABI 骨架，不宣称 Mapping 可用。

## 1. 硬件与软件版本

- [ ] 确认上述三件套为支持组合（含 Remote Head `R`）
- [ ] 推荐 NI-VISA / Camera Runtime / Cable Assembly / XNS 版本矩阵
- [ ] RD-SP-100-SM 与 RD-P-100 的 API 差异说明（不得套用仅 RD-P 的示例）

## 2. Gate A — Native OPL-SDK（优先）

请提供 header / lib / sample，并书面确认是否全部覆盖：

- [ ] Detect / 绑定指定序列号的 RD-SP Camera
- [ ] 校验 RLM-100 + OSX-100 + RD-SP 组合
- [ ] 创建并初始化 RD-SP composite device
- [ ] 设置 MPO / Duplex Mapping Mode、Sensitivity、Loss Compensation
- [ ] Start / Collect（单逻辑纤）/ Process Expected Mapping / End / Cancel
- [ ] 返回 Observed Mapping、Dark Channel、诊断图像
- [ ] STDCALL / C 可调用，无需 Host 加载 CLR

若任一项缺失，请明确声明，我们将采用 Gate B。

## 3. Gate B — Managed Hardware Library + Worker

- [ ] `Santec.Hardware.dll`（含版本，例如 2.3.1.0）及 RD-SP 相关类型文档（`IRDSP` / Factory）
- [ ] RD-SP Sample（非仅 RD-P）
- [ ] 是否允许独立进程 Worker 经 Named Pipe 调用（公司 Host 为 Native/MFC，不注入 CLR）
- [ ] Worker 场景下 Laser/OSX 由谁主控（composite 全权 vs Advanced/Segmented 由我方 SCPI）

## 4. 与 XNS / Cable Assembly 的关系

- [ ] 是否存在公开/半公开 API，可在不启动 Cable Assembly UI 的情况下跑同一 Test Plan
- [ ] XNS REST/本地 API 文档（若公司未来要对接 MES）
- [ ] 明确：极性 Mapping **是否有任何 SCPI**（我方交叉验证结论：无；请确认）

## 5. 交付物形式

请提供：

1. SDK 安装包或下载链接 + 许可条款  
2. 英文或中文 API 参考 PDF  
3. 最小可编译 Sample（C/C++ 或 C#）  
4. 对上表勾选结果的书面回复（邮件即可）

## 我方联系上下文（可附在邮件）

- 已有：`UDL.SantecRLM` / `UDL.SantecOSX`（SCPI over TCP/VISA）  
- 不采用：自研相机 Mapping；虚构 `POLARITY:*` SCPI；第一版直接写 XNS UI 自动化  
- 参考资料：供应商交付的 XNS & Cable Assembly 使用方法 PDF；公司 Final Plan
