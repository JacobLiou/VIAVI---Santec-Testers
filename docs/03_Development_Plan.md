# VIAVI & Santec 设备驱动开发计划

> Author: Menghui | Date: 2026-02-13
> Project: VIAVI/Santec IL/RL 测试设备整合至 MIMS/Mlight/MAS 系统
> Deadline: 2026-04-01

---

## 项目概况

| 项目 | 详情 |
|------|------|
| **目标** | 将 Guad 工厂 11 台 IL/RL 测试设备整合进 MIMS/Mlight/MAS 系统 |
| **设备** | VIAVI MAP300 PCT × 9 台, Santec × 2 台 |
| **负责范围** | 驱动层开发（设备通讯、控制逻辑、数据采集） |
| **参考系统** | Dimension / in-house SM24 已有驱动（已兼容 MIMS） |
| **Deadline** | 2026-04-01 |

---

## 阶段计划

### Phase 1: 协议分析与架构设计 (2/13 - 2/18)  [当前阶段]

| 任务 | 状态 | 产出 |
|------|------|------|
| 阅读 VIAVI PCT 通讯协议 & 示例代码 | ✅ 完成 | 协议分析报告 |
| 提取 VIAVI 指令集、数据格式、工作流程 | ✅ 完成 | 01_VIAVI_PCT_Protocol_Analysis.md |
| 设计统一驱动框架 (base_driver) | ✅ 完成 | driver/base_driver.py |
| 实现 VIAVI 驱动骨架 | ✅ 完成 | driver/viavi_driver.py |
| 创建 Santec 驱动骨架 | ✅ 完成 | driver/santec_driver.py |
| 输出 VIAVI vs Santec 差异评估表 | ✅ 完成 | 02_Difference_Assessment.md |
| 催收 Santec 协议文档 | ⏳ 进行中 | — |
| 确认设备物理位置（珠海是否有设备） | ⏳ 待跟进 | — |
| 参考 Dimension/SM24 现有驱动架构 | ⏳ 待跟进 | — |

### Phase 2: VIAVI 驱动开发 & 单机测试 (2/19 - 3/10)

| 任务 | 预计工时 | 说明 |
|------|----------|------|
| 完善 VIAVI 驱动所有功能 | 3天 | 基于骨架补充完整逻辑 |
| 实现完整错误处理 & 重连机制 | 2天 | 断线检测、自动重连、错误恢复 |
| 实现 IL/RL 数据解析 & 验证 | 2天 | 确认所有5个值的含义 |
| 编写单元测试 | 2天 | Mock socket 测试指令发送/响应 |
| 联机测试（如有设备） | 3天 | 与实际 MAP300 设备联调 |
| 性能优化（多通道轮询效率） | 1天 | 减少测量耗时 |

### Phase 3: Santec 驱动开发 & 单机测试 (3/10 - 3/20)

| 任务 | 预计工时 | 说明 |
|------|----------|------|
| 基于协议文档实现 Santec 驱动 | 3天 | 补充 santec_driver.py 所有方法 |
| 实现 Santec 数据解析 | 2天 | 根据 Santec 数据格式 |
| 编写单元测试 | 1天 | 与 VIAVI 测试结构对齐 |
| 联机测试（如有设备） | 3天 | 与实际 Santec 设备联调 |
| 统一接口验证 | 1天 | 确认两种设备通过 DriverFactory 无缝切换 |

### Phase 4: MIMS/Mlight/MAS 集成 & 联调 (3/20 - 3/31)

| 任务 | 预计工时 | 说明 |
|------|----------|------|
| MIMS 数据接口对接 | 2天 | 实现 format_results_for_mims 完整逻辑 |
| Mlight 系统集成 | 2天 | 驱动嵌入 Mlight 测试流程 |
| MAS 数据上传接口 | 2天 | 实现 format_results_for_mas + 上传 |
| 端到端集成测试 | 3天 | 全链路: 设备→驱动→系统→数据库 |
| Bug 修复 & 回归测试 | 2天 | — |
| 文档编写 | 1天 | 使用手册、集成说明 |

### Phase 5: 交付 (4/1)

| 任务 | 说明 |
|------|------|
| 代码 review & 合入 | — |
| 最终验收测试 | — |
| 交付给 Guolin | — |

---

## 里程碑

| 日期 | 里程碑 | 交付物 |
|------|--------|--------|
| 2/18 | M1: 架构设计完成 | 协议分析、差异评估、驱动框架 |
| 3/10 | M2: VIAVI 驱动可用 | viavi_driver.py 通过单机测试 |
| 3/20 | M3: Santec 驱动可用 | santec_driver.py 通过单机测试 |
| 3/31 | M4: 系统集成完成 | MIMS/Mlight/MAS 联调通过 |
| 4/01 | M5: 正式交付 | 全部代码 + 文档 |

---

## 风险 & 缓解

| 风险 | 可能性 | 影响 | 缓解措施 |
|------|--------|------|----------|
| Santec 协议文档延迟到达 | 高 | 高 | 先完成 VIAVI, Santec 预留框架; 催促供应商 |
| 珠海没有可测试设备 | 中 | 高 | 搭建 Mock 模拟器; 远程连接 Guad 设备 |
| MIMS/Mlight 接口文档不清晰 | 中 | 中 | 参考 Dimension/SM24 现有实现 |
| 两种设备差异过大 | 低 | 中 | 抽象层已设计好, 各自实现不影响统一接口 |
| 4/1 deadline 过紧 | 中 | 高 | 优先 VIAVI (9台); Santec (2台) 可适当延后 |

---

## 依赖项

| 依赖 | 来源 | 当前状态 |
|------|------|----------|
| VIAVI 通讯协议 & 示例代码 | Kyel 已发送 | ✅ 已收到 |
| Santec 通讯协议文档 | 供应商 | ❌ 待催收 |
| Dimension/SM24 现有驱动代码 | 内部代码库 | ⏳ 待获取 |
| MIMS 数据接口规范 | MIMS 团队 | ⏳ 待确认 |
| Mlight 集成接口规范 | Mlight 团队 | ⏳ 待确认 |
| MAS 数据上传接口规范 | MAS 团队 | ⏳ 待确认 |
| 测试设备 (物理设备或远程访问) | Guad / 珠海 | ⏳ 待确认 |

---

## 技术栈

| 项目 | 选择 | 说明 |
|------|------|------|
| 语言 | Python 3.x | 与现有系统保持一致 |
| 通讯库 | socket (标准库) | VIAVI 已确认, Santec 待定 |
| 备选通讯库 | pyvisa / ctypes | 如 Santec 使用 GPIB/DLL |
| 测试框架 | pytest | 单元测试 + 集成测试 |
| 日志 | logging (标准库) | 内置于 base_driver |
| 设计模式 | Template Method + Factory | 统一接口, 灵活扩展 |
