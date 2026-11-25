# UDL.SantecPolarity — production scaffold

Native C ABI DLL for company polarity baseline:

`RLM-100-01-3500-S-09-FA-R` + `OSX-100-1A-024-09-FA-00B` + `RD-SP-100-SM`

Design authority: `SantecPolarity/RLM100_OSX100_RDSP100SM_Polarity_Cpp_MFC_UDL_Final_Plan.md`.

## Current status

Scaffold only. `SP_Open` / `SP_RunMapping` return `SP_E_BACKEND_NOT_SUPPORTED`
until vendor RD-SP Mapping SDK (Gate A) or isolated Worker (Gate B) is integrated.

**Not** the frozen invented-`POLARITY:*` stub under `SantecPolarity/Driver/`.
**Not** a substitute for standalone PTM (`UDL.SantecPTM`).

## Reuse plan (after SDK arrives)

1. RLM laser / output / switch SCPI patterns from `UDL.SantecRLM` (TCP 5025 or VISA).
2. OSX route validation patterns from `UDL.SantecOSX` (`SW1` CH1..24).
3. Single global device lock; no dual-owner with IL/RL session.
4. RD-SP Mapping only via official SDK — no self-built vision.

## Build

Open `TestEquipmentDrivers.sln`, project `UDL.SantecPolarity`, preferred **Debug|x64**.

API doc: `docs/UDL.SantecPolarity-API.md` and `../docs/UDL.SantecPolarity-API.md`.
