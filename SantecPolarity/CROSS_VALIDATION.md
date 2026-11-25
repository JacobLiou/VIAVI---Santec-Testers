# 极性驱动交叉验证结论（2026-11）

## 总判定

公司量产极性路径是 **RLM-100 + OSX-100 + RD-SP-100-SM**，由 XNS / Cable Assembly 或自研 Native UDL 编排官方 Mapping；**不是**一体机 `POLARITY:*` SCPI，也**不是**单独 PTM 就能覆盖当前硬件基线。

| 仓库路径 | 结论 |
|---|---|
| 本目录 `Driver`/`Simulator`/`TestApp`（虚构 `POLARITY:*`） | **错误路径**；已冻结为模拟器 stub，禁止量产验收 |
| `TestEquipmentDrivers/UDL.SantecPTM` | 仅当 BOM 含独立 PTM 时有效；相对当前 RD-SP 基线为产品选错 |
| `TestEquipmentDrivers/UDL.SantecPolarity` | **量产方向**骨架；待供应商 RD-SP SDK/API 后实现 Mapping |
| `RLM100_OSX100_RDSP100SM_Polarity_Cpp_MFC_UDL_Final_Plan.md` | 公司硬件与架构基线 |

## 证据摘要

1. 供应商 `doc/` 下 XNS & Cable Assembly 使用方法 PDF：极性需 RD-SP/RD-P；USB 为主；全文无 SCPI。
2. Santec 官网 RD-SP：接 RLM+OSX 一步测极性+IL/RL；Cable Assembly/XNS 为产线软件。
3. `docs/PTM-User-Manual.pdf`：独立 PTM 有真实 SCPI（`TEST:CON#` / `TEST:RESult?`），属另一产品线。
4. Final Plan：锁定公司料号，排除 PTM/RD-P；极性 = 编排 + 官方 Mapping，不是新增 `READ:POL?`。

## 后续门禁

1. 冻结本目录虚构协议（已完成）。
2. 向供应商索取 RD-SP SDK/API（见 `docs/RD-SP-SDK-供应商索取清单.md`）。
3. 在 `UDL.SantecPolarity` 中复用 `UDL.SantecRLM` / `UDL.SantecOSX` 经验，按 Final Plan 实现 Coordinator；未拿到 SDK 前 `SP_Open`/`SP_RunMapping` 返回 `SP_E_BACKEND_NOT_SUPPORTED`。
4. `UDL.SantecPTM` 标注为非当前 BOM；勿并入 RD-SP 产线验收。
