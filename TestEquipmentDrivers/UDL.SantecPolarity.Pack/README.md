# UDL.SantecPolarity 打包

```powershell
cd TestEquipmentDrivers\UDL.SantecPolarity.Pack

# 默认：Release + Both (x64 与 Win32)，含 PolarityTestApp
powershell -ExecutionPolicy Bypass -File .\pack-polarity-dist.ps1

# 只打 x64
powershell -ExecutionPolicy Bypass -File .\pack-polarity-dist.ps1 -Configuration Release -Platform x64

# 只打 Win32 (x86)
powershell -ExecutionPolicy Bypass -File .\pack-polarity-dist.ps1 -Platform Win32

# 已编译过，只组装
.\pack-polarity-dist.ps1 -Configuration Release -Platform Both -SkipBuild
```

## 产物

| 路径 | 说明 |
|---|---|
| `dist/UDL.SantecPolarity-x64/` + `.zip` | 64-bit 包（`bin` 含 `PolarityTestApp.exe`） |
| `dist/UDL.SantecPolarity-Win32/` + `.zip` | 32-bit 包 |
| `dist/UDL.SantecPolarity-Both/` + `.zip` | 双平台合集（`-Platform Both` 时） |

## 冒烟

1. 解压对应位数包，进入 `bin`。
2. 运行 `PolarityTestApp.exe` → Load → Create → Open → Snapshot / Run Mapping。
