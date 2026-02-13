# VIAVI MAP300 PCT 通讯协议分析报告

> 基于 PCT_12_Channel_Test.py 示例代码及 README 提取

## 1. 通讯方式

| 项目 | 详情 |
|------|------|
| **协议类型** | TCP Socket |
| **指令体系** | SCPI-like (Standard Commands for Programmable Instruments) |
| **编码格式** | UTF-8 发送, ASCII 接收 |
| **指令结尾符** | `\n` (换行符) |
| **默认超时** | 3 秒 |
| **缓冲区大小** | 1024 bytes |

## 2. 连接架构（两级连接）

MAP300 采用 **两级连接** 模式：

```
[上位机] --TCP--> [Chassis Port 8100] --启动Super App--> [PCT Module Port 8300+slot]
```

### 连接步骤：
1. **先连接 Chassis**（端口 8100）
2. **发送 `SUPER:LAUNCH PCT`** 启动 Super Application
3. **等待约 7-10 秒** 让应用完全启动
4. **再连接 PCT Module**（端口 8300 + 插槽编号）

### 端口分配：
- **Chassis 管理端口**: `8100`（固定）
- **PCT 模块端口**: `8300 + slot_number`（动态，如 slot 1 = 8301）

## 3. 指令格式

### 3.1 命令分类

| 类型 | 格式 | 说明 |
|------|------|------|
| **设置命令** | `COMMAND param1,param2,...` | 无返回值 |
| **查询命令** | `COMMAND?` 或 `COMMAND? param` | 返回数据 |

### 3.2 判断是否有返回值
- 指令中包含 `?` → 有返回值，需要接收
- 指令中不含 `?` → 无返回值，仅发送

### 3.3 响应格式
- 以 `\n` 结尾
- 多值以 `,` 分隔
- 错误码格式：`code,message`

## 4. 完整指令集（从示例提取）

### 4.1 Chassis 级别指令

| 指令 | 说明 |
|------|------|
| `SUPER:LAUNCH PCT` | 启动 PCT Super Application |

### 4.2 系统/错误指令

| 指令 | 返回值 | 说明 |
|------|--------|------|
| `SYST:ERR?` | `code,message` | 查询错误状态。code=0 为无错误 |

### 4.3 光源配置指令

| 指令 | 返回值 | 说明 |
|------|--------|------|
| `SOURCE:LIST wl1, wl2` | 无 | 设置测试波长（如 1310, 1550 nm） |
| `SOURCE:LIST?` | `wl1,wl2,...` | 查询当前波长列表 |

### 4.4 路径配置指令

| 指令 | 返回值 | 说明 |
|------|--------|------|
| `PATH:LAUNCH port` | 无 | 设置发射端口（如 J1=1） |
| `PATH:LIST port,channels` | 无 | 设置测量通道（如 `1,1-12`） |
| `PATH:JUMPER:IL port,ch,value` | 无 | 设置通道 IL 覆盖值 (dB) |
| `PATH:JUMPER:IL:AUTO port,ch,flag` | 无 | 0=启用覆盖, 1=关闭覆盖(自动) |
| `PATH:JUMPER:LENGTH port,ch,value` | 无 | 设置通道长度覆盖值 (米) |
| `PATH:JUMPER:LENGTH:AUTO port,ch,flag` | 无 | 0=启用覆盖, 1=关闭覆盖(自动) |

### 4.5 ORL 测量配置

| 指令 | 参数 | 说明 |
|------|------|------|
| `MEAS:ORL:SETUP ch,method,origin,A,B` | method: 1=积分模式, 2=离散模式 | 设置 ORL 窗口 |

**参数详解：**
- `method`: 1 = Integration Mode（积分模式）, 2 = Discrete Mode（离散模式，APC 推荐）
- `origin`: 1 = A+B 锚定 DUT 起点, 2 = A+B 锚定 DUT 终点, 3 = A 从起点/B 从终点
- `Aoffset`: A 标记偏移（米），如 -0.5
- `Boffset`: B 标记偏移（米），如 0.5

### 4.6 测量控制指令

| 指令 | 返回值 | 说明 |
|------|--------|------|
| `SENS:FUNC mode` | 无 | 0=参考模式, 1=测量模式 |
| `MEAS:START` | 无 | 启动测量/参考 |
| `MEAS:STATE?` | `state` | 1=完成, 2=运行中, 3=错误 |
| `MEAS:ALL? ch,port` | CSV 数据 | 获取指定通道的全部测量结果 |

### 4.7 数据输出格式

`MEAS:ALL? channel, port` 返回格式：

```
value1,value2,value3,value4,value5[,value6,value7,value8,value9,value10]...
```

- 每个波长返回 **5 个值** 一组
- 第 4 个值（index 3）为 **RL (Return Loss)** 结果
- 如果有 N 个波长，则共返回 N×5 个值
- 数据以逗号分隔

**示例（2 个波长, 1310nm + 1550nm）：**
```
v1,v2,v3,RL_1310,v5,v6,v7,v8,RL_1550,v10
```

## 5. 标准测量流程

```
┌─────────────────────────────────────────────┐
│ 1. 连接 Chassis (TCP:8100)                  │
│ 2. SUPER:LAUNCH PCT                         │
│ 3. 等待 ~10s                                │
│ 4. 连接 PCT Module (TCP:8300+slot)          │
│ 5. 配置 ORL 窗口 (MEAS:ORL:SETUP)          │
│ 6. 设置波长 (SOURCE:LIST)                   │
│ 7. 设置发射端口 (PATH:LAUNCH)               │
│ 8. 设置测量通道 (PATH:LIST)                 │
│ 9. 配置参考 / 覆盖参考                      │
│ 10. 切换到参考模式 (SENS:FUNC 0)            │
│ 11. MEAS:START → 轮询 MEAS:STATE?           │
│ 12. 切换到测量模式 (SENS:FUNC 1)            │
│ 13. MEAS:START → 轮询 MEAS:STATE?           │
│ 14. 逐通道获取结果 (MEAS:ALL?)              │
│ 15. 解析结果数据                             │
└─────────────────────────────────────────────┘
```

## 6. 关键注意事项

1. **必须先启动 Super Application** 才能连接 PCT 模块
2. **参考测量必须在正式测量之前** 完成
3. **每次指令后建议执行 `SYST:ERR?`** 检查错误
4. **MEAS:STATE? 需要轮询**，间隔建议 100ms
5. **支持 APC 和 UPC 连接器** 测量
6. **离散模式 (Discrete Mode) 更适合 APC** 连接器

## 7. 对驱动开发的影响

| 要点 | 影响 |
|------|------|
| TCP 通讯 | 需要 socket 连接管理、断线重连 |
| 两级连接 | 需要管理 Chassis 和 Module 两个连接 |
| SCPI 指令 | 指令解析器可以通用化 |
| 轮询模式 | 需要异步/轮询机制等待测量完成 |
| 多通道多波长 | 数据解析需要支持动态通道和波长数量 |
| 错误检查 | 每步操作后需集成错误处理 |
