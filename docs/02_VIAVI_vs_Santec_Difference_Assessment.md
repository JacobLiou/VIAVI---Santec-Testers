# VIAVI vs Santec 差异评估表

> Author: Menghui | Date: 2026-02-13 (Updated: 2026-02-27)
> Status: 已基于 RLM User Manual M-RL1-001-07 及 GMS软件使用指导 完成评估

---

## 1. 核心差异对比

| 评估项目 | VIAVI MAP300 PCT | Santec RL1 | 差异点 & 影响 |
|----------|-----------------|------------|---------------|
| **通讯方式** | TCP Socket | TCP/IP (端口5025) + USB USBTMC | 两者均支持TCP，Santec额外支持USB VISA |
| **指令体系** | SCPI-like 自定义指令 | 标准 SCPI (IEEE 488.2 + 设备指令集) | Santec更标准化，但指令集完全不同 |
| **连接架构** | 两级连接 (Chassis + Module) | 单级TCP连接 (端口5025) | VIAVI 较复杂，Santec 简单直连 |
| **端口分配** | Chassis:8100, Module:8300+slot | 固定端口 5025 (SCPI-RAW per LXI) | Santec 端口固定，无需计算 |
| **启动流程** | 需先启动 Super Application (~10s) | 上电自启，SM预热30分钟/MM预热60分钟 | VIAVI有软件初始化，Santec有硬件预热 |
| **数据格式** | CSV格式, 每波长5值, RL在index 3 | READ:RL? 返回 RLA,RLB,RLTOTAL,Length (4值CSV) | 数据结构不同，各需独立解析 |
| **波长支持** | 支持多波长 (1310, 1550 等) | SM: 1310/1490/1550/1625/1650; MM: 850/1300 | Santec支持更多SM波长 |
| **通道数** | 最高12通道 (可配置) | 内部开关 (OUT:CLOSe) + 外部SX1开关 (SW#:CLOSe) | 通道管理方式不同 |
| **参考模式** | 支持自动参考 + 手动覆盖 | RL参考: REF:RL (自动/手动); IL参考: REF:IL:det# | 清零只需对IL进行，RL参考设置跳线长度 |
| **ORL 配置** | 支持积分/离散模式, 可配标记偏移 | RLTOTAL通过READ:RL?直接返回 | Santec自动计算RLTOTAL，无需额外ORL配置 |
| **错误处理** | SYST:ERR? 返回 code,message | SYST:ERR? 返回 code,"message" (标准SCPI) | 格式相同 |
| **测量状态** | MEAS:STATE? 轮询 (1=完成/2=运行/3=错误) | 同步命令 + *OPC? 同步 | Santec使用同步READ命令，无需轮询 |
| **连接器类型** | 支持 APC + UPC | SM: FC/APC; MM: FC/UPC (支持APC模式切换) | 类似 |
| **编码格式** | UTF-8 发送 / ASCII 接收 | ASCII + \n终止符 | Santec更标准 |
| **需要额外硬件** | 不需要 (直接TCP通讯) | 不需要 (TCP直连或USB VISA) | 两者均可TCP直连 |
| **测量速度** | 取决于配置 | Fast: <1.5s/波长; Standard: <5s/波长 | 可通过 RL:SENSitivity 配置 |
| **RL精度范围** | 取决于配置 | SM: 30-85dB; MM: 10-50dB | 由RL:GAIN和RL:SENSitivity控制 |
| **DUT长度** | N/A | <100m / <1500m / <4000m (DUT:LENGTH) | Santec需要配置DUT长度档位 |

## 2. Santec RL1 SCPI 指令集 (已确认)

### 2.1 标准 IEEE 488.2 指令 (Table 11)

| 指令 | 说明 |
|------|------|
| `*IDN?` | 设备识别 |
| `*RST` | 复位 |
| `*CLS` | 清除状态 |
| `*OPC?` | 操作完成查询 (返回1) |
| `*OPC` | 操作完成 |
| `*WAI` | 等待操作完成 |
| `SYST:ERR?` | 查询错误队列 |
| `SYST:VER?` | 查询SCPI版本 |
| `:STAT:OPER:COND?` | 操作状态条件寄存器 |
| `:STAT:OPER:ENAB` | 操作状态使能寄存器 |
| `:STAT:OPER?` | 操作状态事件寄存器 |
| `:STAT:QUES:COND?` | 可疑状态条件寄存器 |
| `:STAT:PRES` | 状态预设 |

### 2.2 RL1 设备指令 (Table 12)

| 指令 | 说明 |
|------|------|
| `LAS:DISAB` | 关闭所有激光器 |
| `LAS:ENAB #` | 开启指定波长的激光器 (IL模式) |
| `LAS:ENAB?` | 查询当前启用的激光器波长 |
| `LAS:INFO?` | 查询支持的激光器波长列表 |
| `FIBER:INFO?` | 查询光纤类型 (SM 或 MM) |
| `READ:RL? #` | 同步RL测量，返回 RLA,RLB,RLTOTAL,Length |
| `READ:RL? #,#,#` | 带参考长度的同步RL测量 |
| `REF:RL` | 自动测量跳线长度 (RL参考) |
| `REF:RL #,[#]` | 手动设置MTJ1/MTJ2跳线长度 |
| `REF:RL?` | 查询存储的MTJ1跳线长度 |
| `READ:IL:det#? #` | 同步IL测量，返回指定探测器的IL值 |
| `REF:IL:det# #,#` | 设置IL参考值 |
| `REF:IL:det#? #` | 查询IL参考值 |
| `POW:NUM?` | 查询连接的功率计数量 |
| `POW:det#:INFO?` | 查询探测器信息 |
| `READ:POW:det#? #` | 读取功率计读数 |
| `READ:POW:MON? #` | 读取内部参考功率计 |
| `RL:SENS #` | 设置RL灵敏度 (fast/standard) |
| `RL:SENS?` | 查询RL灵敏度 |
| `RL:GAIN <low,normal>` | 设置RL增益模式 |
| `RL:GAIN?` | 查询RL增益模式 |
| `RL:POSB #` | 设置RLB位置定义 (eof/zero) |
| `RL:POSB?` | 查询RLB位置定义 |
| `DUT:LENGTH #` | 设置DUT长度档位 (100/1500/4000) |
| `DUT:LENGTH?` | 查询DUT长度档位 |
| `DUT:IL #` | 设置DUT插入损耗补偿值 |
| `DUT:IL?` | 查询DUT插入损耗设置值 |
| `OUT:CLOS #` | 设置内部开关通道 |
| `OUT:CLOS?` | 查询内部开关通道 |
| `SW#:CLOS #` | 设置外部SX1开关通道 |
| `SW#:CLOS?` | 查询外部开关通道 |
| `SW#:INFO?` | 查询开关类型 |
| `LCL #` | 设置本地模式 (1=启用, 0=禁用) |
| `LCL?` | 查询本地模式状态 |
| `AUTO:ENAB #` | 设置自动启动模式 |
| `AUTO:ENAB?` | 查询自动启动模式 |
| `AUTO:TRIG?` | 查询自动启动条件是否满足 |
| `AUTO:TRIG:RST #,#` | 重置自动启动触发器 |
| `READ:BARC?` | 读取条形码扫描内容 |
| `READ:FACT:POW? #,#` | 读取出厂功率校准值 |
| `TEST:NOTIFY# "string"` | 推送通知到RL1触摸屏 |
| `TEST:RETRY` | 重试当前XN1测试计划 |
| `TEST:NEXT` | 保存结果并准备下一个DUT |

## 3. 对驱动架构的影响分析

### 3.1 可统一的部分 (已在 CBaseEquipmentDriver 中实现)

| 功能模块 | 说明 |
|----------|------|
| 连接状态管理 | Connect / Disconnect / Reconnect 生命周期 |
| 测量结果数据模型 | MeasurementResult 统一结构 (已扩展 RLA/RLB/RLTOTAL/Length) |
| 完整测试流程 | ConfigureWavelengths -> TakeReference -> TakeMeasurement -> GetResults 模板方法 |
| 日志记录 | 统一 CLogger 框架 |
| 错误处理模式 | CheckError + AssertNoError |
| DLL导出接口 | C-style API 统一封装 |

### 3.2 各自实现的部分 (已完成)

| 功能模块 | VIAVI 特有 | Santec RL1 特有 |
|----------|-----------|-----------------|
| 连接建立 | 两级连接 (Chassis + Module) | 单级TCP连接 (端口5025) |
| 设备初始化 | SUPER:LAUNCH PCT + 等待 | FIBER:INFO? + LAS:INFO? 发现能力 |
| 指令发送格式 | SCPI-like + `\n` 结尾 | 标准SCPI + `\n`终止; 支持`;`链接 |
| 波长配置 | SOURCE:LIST 命令 | LAS:ENAB <nm>;OPC? |
| 通道配置 | PATH:LIST + PATH:LAUNCH | OUT:CLOS <n> / SW#:CLOS <n> |
| ORL 配置 | MEAS:ORL:SETUP | 不需要 (RLTOTAL自动计算) |
| 参考/校准 | SENS:FUNC 0/1 切换模式 | REF:RL (RL参考) + REF:IL:det# (IL参考) |
| 测量触发 | MEAS:INIT + MEAS:STATE? 轮询 | READ:RL? / READ:IL:det#? (同步返回) |
| 结果解析 | CSV 5值/波长 | RLA,RLB,RLTOTAL,Length (4值CSV) |
| 速度控制 | N/A | RL:SENSitivity (fast/standard) |
| DUT配置 | N/A | DUT:LENGTH (100/1500/4000) |
| 增益控制 | N/A | RL:GAIN (low/normal) |

### 3.3 关键技术要点

| 要点 | 说明 |
|------|------|
| 同步命令 | RL1所有SCPI命令同步执行，无需轮询 |
| *OPC? 同步 | 可用 `命令;*OPC?` 链接确保操作完成 |
| 读取超时 | READ:RL? 在4km长度档+标准模式下最长可达10秒，超时需设至少15000ms |
| 命令链接 | 支持 `;` 分隔多命令，如 `LAS:ENAB 1310;*OPC?` |
| 命令终止 | 所有命令以 `\n` (换行符) 结束 |
| VISA驱动 | USB通讯需安装VISA驱动 (推荐 Rohde & Schwarz) |

## 4. 已确认的问题清单

1. **通讯方式**: TCP/IP (端口5025) 和 USB USBTMC -- **已确认**
2. **通讯协议文档**: RLM User Manual M-RL1-001-07, Programming Guide (Table 11 & 12) -- **已获取**
3. **指令集**: 标准 IEEE 488.2 + 设备特定 SCPI 指令 -- **已确认**
4. **数据输出格式**: READ:RL? 返回 RLA,RLB,RLTOTAL,Length; READ:IL:det#? 返回单值 -- **已确认**
5. **SDK/API**: 无专用SDK，使用标准SCPI通讯 + VISA驱动 -- **已确认**
6. **初始化/校准流程**: *CLS -> FIBER:INFO? -> LAS:INFO? -> 配置 -> REF:RL -> 测量 -- **已确认**
7. **多通道支持**: OUT:CLOSe (内部开关) + SW#:CLOSe (外部SX1开关) -- **已确认**
8. **示例代码**: GMS软件使用指导提供了操作流程参考 -- **已获取**

## 5. 实施状态

- [x] 收到 Santec 协议文档
- [x] 完成差异评估
- [x] 更新 SantecDriver.h/cpp 适配官方协议
- [x] 更新 SantecSimServer 模拟器适配官方协议
- [x] 更新 DriverTypes.h 数据结构
- [x] 更新 DriverExports DLL导出接口
- [x] 更新 DriverTestApp 测试界面
- [ ] 与实际硬件联调验证
