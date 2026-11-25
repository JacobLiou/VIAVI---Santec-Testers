# UDL.SantecPolarity — production native shell

Host-facing **pure Native** C ABI (`SP_*`) for company polarity baseline:

`RLM-100` + `OSX-100 1x24` + `RD-SP-100-SM`

## Architecture

| Layer | Role |
|---|---|
| This DLL | Native shell: handles, RLM station lock, LoadLibrary Cli |
| `UDL.SantecPolarity.Cli` | C++/CLI → Santec.Hardware Mapping only |
| `UDL.SantecRLM` | SCPI IL/RL; shares `Global\UDL_SANTEC_RLM100_<serial>` lock |

MFC Host only `LoadLibrary` **this** DLL. Do not reference C# from Host.

## Deploy

Build `UDL.SantecPolarity` (ProjectReference builds Cli). Ensure output folder contains:

- `UDL.SantecPolarity.dll`
- `UDL.SantecPolarity.Cli.dll`
- `Santec.Hardware.dll` (2.3.1.0 matching bitness)
- `Santec.Core.dll`

## Notes

- Call `SP_RunMapping` from a **worker thread** (not UI).
- Same RLM cannot be owned by polarity and IL/RL at once (device lock).
- Not the frozen invented-`POLARITY:*` stub under `SantecPolarity/Driver/`.
