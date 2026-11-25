# SantecPolarity — FROZEN STUB (NOT production)

**Status: FROZEN. Do not use for production acceptance or real-hardware polarity.**

This tree implements an **assumed** all-in-one polarity tester over invented
`POLARITY:*` TCP/SCPI. Cross-check against supplier XNS/Cable Assembly materials
and company Final Plan shows production polarity is:

`RLM-100 + OSX-100 + RD-SP-100-SM` (multi-instrument + official Mapping SDK)

**Not** a single-chassis `POLARITY:*` instrument, and **not** covered by this stub.

| Path | Role |
|---|---|
| This folder (`Driver` / `Simulator` / `TestApp`) | Teaching / offline sim only |
| `RLM100_OSX100_RDSP100SM_Polarity_Cpp_MFC_UDL_Final_Plan.md` | Company hardware baseline |
| `CROSS_VALIDATION.md` | Cross-validation conclusion |
| `../TestEquipmentDrivers/UDL.SantecPolarity/` | Production UDL scaffold (RLM+OSX+RD-SP) |
| `../TestEquipmentDrivers/UDL.SantecPTM/` | Separate product line (standalone PTM only) |

## What this stub still does

Three VS2015/v140 projects for local sim exercises only:

1. `Driver/UDL.SantecPolarity.Driver.sln` — C ABI DLL speaking invented `POLARITY:*`
2. `Simulator/UDL.SantecPolarity.Simulator.sln` — TCP server for that contract
3. `TestApp/UDL.SantecPolarity.TestApp.sln` — Win32 GUI against the simulator

Do **not** integrate `UDL.SantecPolarity.Driver.dll` into the production host.
Do **not** treat `SCPI_PROTOCOL.md` as a vendor protocol.

## Production direction

See Final Plan + `TestEquipmentDrivers/UDL.SantecPolarity/`. Obtain RD-SP SDK/API
from the supplier before implementing Mapping (see `docs/RD-SP-SDK-供应商索取清单.md`).
