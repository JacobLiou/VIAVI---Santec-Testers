# VIAVI & Santec IL/RL 测试设备驱动项目

> 将 Guad 工厂 VIAVI (9台) 和 Santec (2台) IL/RL 测试设备整合至 MIMS/Mlight/MAS 系统
> Deadline: 2026-04-01

---

## 项目结构

```
VIAVI & Santec Testers/
├── README.md                          # 本文件 - 项目总览
├── new task行动指南.md                 # 原始任务行动指南
│
├── docs/                              # 文档
│   ├── 01_VIAVI_PCT_Protocol_Analysis.md  # VIAVI 通讯协议分析报告
│   ├── 02_VIAVI_vs_Santec_Difference_Assessment.md  # 差异评估表
│   └── 03_Development_Plan.md         # 开发计划 (含里程碑/风险/依赖)
│
├── driver/                            # 统一驱动框架
│   ├── __init__.py                    # 包入口
│   ├── base_driver.py                 # 抽象基类 + 数据模型
│   ├── viavi_driver.py                # VIAVI MAP300 PCT 驱动 (已实现)
│   ├── santec_driver.py               # Santec 驱动 (骨架, 待协议文档)
│   └── driver_factory.py             # 工厂模式, 统一创建入口
│
└── PCT Automation Example/            # VIAVI 供应商提供的原始示例
    ├── PCT_12_Channel_Test.py         # Python 示例脚本
    ├── PCT_12_Channel_Test.txt        # 同上 (文本版)
    └── PCT_12_Channel_Test README.md  # 示例说明
```

## 驱动架构

```
                    ┌──────────────────────┐
                    │  MIMS / Mlight / MAS │  (上层系统)
                    └──────────┬───────────┘
                               │
                    ┌──────────▼───────────┐
                    │   BaseEquipmentDriver │  (抽象基类)
                    │  ┌─────────────────┐  │
                    │  │ - connect()     │  │
                    │  │ - disconnect()  │  │
                    │  │ - run_full_test()│ │
                    │  │ - format_*()    │  │
                    │  └─────────────────┘  │
                    └──────────┬───────────┘
                     ┌─────────┴─────────┐
              ┌──────▼──────┐     ┌──────▼──────┐
              │ ViaviPCTDriver│   │ SantecDriver │
              │  (已实现)    │     │  (骨架)     │
              └──────┬──────┘     └──────┬──────┘
                     │                    │
              ┌──────▼──────┐     ┌──────▼──────┐
              │ VIAVI MAP300 │    │ Santec 设备  │
              │ (TCP Socket) │    │ (协议待确认) │
              └─────────────┘     └─────────────┘
```

## 快速开始

```python
from driver import create_driver

# VIAVI MAP300 PCT
driver = create_driver("viavi", ip_address="10.14.132.194", slot=1)
driver.connect()
driver.initialize()

results = driver.run_full_test(
    wavelengths=[1310, 1550],
    channels=list(range(1, 13)),
    override_reference=True,
    il_value=0.1,
    length_value=3.0,
)

# 格式化为 MIMS 格式
mims_data = driver.format_results_for_mims(results)

driver.disconnect()
```

## 启动与调试

### 环境要求

- Python 3.9+
- 无需安装第三方依赖（仅使用标准库 `socket`, `logging`, `threading` 等）

### 运行测试 Demo（模拟模式，无需真实设备）

打开终端，进入项目目录：

```powershell
cd "C:\Users\menghl2\OneDrive - kochind.com\Desktop\VIAVI & Santec Testers"
```

运行全部 Demo：

```powershell
python test_demo.py
```

运行指定 Demo：

```powershell
python test_demo.py --demo 1        # Demo 1: 完整12通道双波长测试
python test_demo.py --demo 2        # Demo 2: 快速4通道单波长测试
python test_demo.py --demo 3        # Demo 3: Santec 骨架演示
python test_demo.py --demo 4        # Demo 4: 工厂模式演示
python test_demo.py --demo 5        # Demo 5: 多设备批量测试
python test_demo.py --demo 1 2      # 组合运行
```

开启详细日志（查看每条 SCPI 指令的收发）：

```powershell
python test_demo.py --demo 1 -v
```

### 连接真实设备（Live 模式）

当拿到实际 VIAVI MAP300 设备并知道 IP 后：

```powershell
python test_demo.py --live 10.14.132.194 --slot 1
```

### 在 Cursor / VS Code 中断点调试

1. 打开 `test_demo.py`
2. 在想调试的行号左侧点击添加断点，推荐位置：
   - `driver.connect()` — 查看连接过程
   - `driver.run_full_test(...)` — 查看测量流程
   - `format_results_table(results)` — 查看结果数据
   - `driver/viavi_driver.py` 中的 `send_command()` — 查看 SCPI 指令收发
3. 按 `F5` 启动调试，选择 **Python File**
4. 调试快捷键：
   - `F10` — 单步跳过
   - `F11` — 单步进入函数
   - `Shift+F11` — 跳出函数
   - `F5` — 继续到下一个断点

也可创建 `.vscode/launch.json` 固定调试配置：

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Run Test Demo",
            "type": "debugpy",
            "request": "launch",
            "program": "${workspaceFolder}/test_demo.py",
            "args": ["--demo", "1", "-v"],
            "cwd": "${workspaceFolder}",
            "console": "integratedTerminal"
        }
    ]
}
```

### 输出文件

运行后会自动在 `test_output/` 目录生成：

- `viavi_results_YYYYMMDD_HHMMSS.csv` — CSV 格式（含 PASS/FAIL 判定）
- `viavi_results_YYYYMMDD_HHMMSS.json` — JSON 格式（MIMS 兼容 + 统计摘要）

---

## 当前进度

- [x] VIAVI 通讯协议分析
- [x] 统一驱动框架设计
- [x] VIAVI 驱动骨架实现
- [x] Santec 驱动骨架（待协议文档补充）
- [x] 差异评估表
- [x] 开发计划
- [ ] 催收 Santec 协议文档
- [ ] 确认设备物理位置
- [ ] 获取 Dimension/SM24 现有驱动参考
- [ ] VIAVI 驱动完整实现 & 测试
- [ ] Santec 驱动完整实现 & 测试
- [ ] MIMS/Mlight/MAS 集成
