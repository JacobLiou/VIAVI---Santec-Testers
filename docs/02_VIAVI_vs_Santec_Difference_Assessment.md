# VIAVI vs Santec 差异评估表

> Author: Menghui | Date: 2026-02-13
> Status: VIAVI 部分已基于示例代码分析完毕，Santec 部分待收到协议文档后补充

---

## 1. 核心差异对比

| 评估项目 | VIAVI MAP300 PCT | Santec (待确认) | 差异点 & 影响 |
|----------|-----------------|-----------------|---------------|
| **通讯方式** | TCP Socket | 待确认 (TCP/GPIB/USB/DLL?) | 若 Santec 用 GPIB/DLL，需要额外适配层 |
| **指令体系** | SCPI-like 自定义指令 | 待确认 (SCPI/proprietary?) | 指令解析器可能需要两套实现 |
| **连接架构** | 两级连接 (Chassis + Module) | 待确认 (单级/多级?) | VIAVI 较复杂，需管理两个 socket |
| **端口分配** | Chassis:8100, Module:8300+slot | 待确认 | 端口管理逻辑不同 |
| **启动流程** | 需先启动 Super Application (~10s) | 待确认 | VIAVI 有较长初始化时间 |
| **数据格式** | CSV 格式, 每波长5值, RL在index 3 | 待确认 | 数据解析器需要各自实现 |
| **波长支持** | 支持多波长 (1310, 1550 等) | 待确认 | 波长配置方式可能不同 |
| **通道数** | 最高12通道 (可配置) | 待确认 | 通道管理方式可能不同 |
| **参考模式** | 支持自动参考 + 手动覆盖 | 待确认 | 参考/校准流程可能有差异 |
| **ORL 配置** | 支持积分/离散模式, 可配标记偏移 | 待确认 | ORL 测量方法可能不同 |
| **错误处理** | SYST:ERR? 返回 code,message | 待确认 | 错误查询机制可能不同 |
| **测量状态** | MEAS:STATE? 轮询 (1=完成/2=运行/3=错误) | 待确认 | 状态查询/回调机制可能不同 |
| **连接器类型** | 支持 APC + UPC | 待确认 | 是否支持相同连接器类型 |
| **编码格式** | UTF-8 发送 / ASCII 接收 | 待确认 | 编码方式可能不同 |
| **需要额外硬件** | 不需要 (直接TCP通讯) | 待确认 | 若用 GPIB 需转换器, DLL 需特定OS |

## 2. 对驱动架构的影响分析

### 2.1 可统一的部分 (已在 base_driver.py 中实现)

| 功能模块 | 说明 |
|----------|------|
| 连接状态管理 | connect / disconnect / reconnect 生命周期 |
| 测量结果数据模型 | MeasurementResult 统一结构 |
| 完整测试流程 | configure -> reference -> measure -> results 模板方法 |
| 数据格式化输出 | MIMS / MAS 格式转换 |
| 日志记录 | 统一 logging 框架 |
| 错误处理模式 | check_error + assert_no_error |
| Context Manager | with 语句支持 |

### 2.2 必须各自实现的部分

| 功能模块 | VIAVI 特有 | Santec 特有 (预估) |
|----------|-----------|-------------------|
| 连接建立 | 两级连接 (Chassis + Module) | 可能单级连接 |
| 设备初始化 | SUPER:LAUNCH PCT + 等待 | 可能 *RST 或特定初始化 |
| 指令发送格式 | SCPI-like + `\n` 结尾 | 待确认 |
| 响应解析 | CSV 格式, 5值/波长 | 待确认 |
| 波长配置 | SOURCE:LIST 命令 | 待确认 |
| 通道配置 | PATH:LIST + PATH:LAUNCH | 待确认 |
| ORL 配置 | MEAS:ORL:SETUP | 待确认 |
| 参考/校准 | SENS:FUNC 0/1 切换模式 | 待确认 |

### 2.3 潜在风险点

| 风险 | 说明 | 缓解措施 |
|------|------|----------|
| Santec 使用 DLL 而非 TCP | 需要用 ctypes/cffi 调用 | 预留 DLL 适配层接口 |
| Santec 使用 GPIB 通讯 | 需要 GPIB 硬件转换器 + pyvisa | 确认设备通讯方式后决定 |
| 数据格式差异大 | 解析逻辑需完全重写 | base_driver 已用抽象方法隔离 |
| Santec 无法远程测试 | 需要实物设备联调 | 确认珠海是否有设备, 或搭建模拟器 |
| 时间紧迫 (4月1日deadline) | 两种设备串行开发有风险 | 优先完成 VIAVI (9台, 数量多) |

## 3. 需要向 Santec 供应商确认的问题清单

1. **通讯方式**: 支持哪些通讯方式？TCP/IP? GPIB? USB? DLL?
2. **通讯协议文档**: 是否有完整的编程手册 (Programming Manual)?
3. **指令集**: 是否基于 SCPI 标准？
4. **数据输出格式**: IL/RL 测量数据的返回格式是什么？
5. **SDK/API**: 是否提供 Python SDK 或 DLL 接口？
6. **初始化/校准流程**: 设备上电后需要哪些初始化步骤？
7. **多通道支持**: 设备是否支持多通道？如何配置？
8. **示例代码**: 是否有类似 VIAVI PCT 的自动化示例？

## 4. 下一步行动

- [ ] 收到 Santec 协议文档后，立即补充本表中 "待确认" 部分
- [ ] 根据确认结果更新驱动框架设计（特别是 santec_driver.py）
- [ ] 与 Lei 协作完成最终评估
- [ ] 提交评估表给 Guolin
