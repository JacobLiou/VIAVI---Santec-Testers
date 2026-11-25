# UDL.SantecPolarity.Cli

In-process C++/CLI bridge from native `UDL.SantecPolarity` to **Santec.Hardware 2.3.1.0** (RD-P/RD-SP Mapping).

MFC Host must **not** load this DLL directly. The native polarity shell loads it via `LoadLibrary` from the same directory.

## Rule

- Devices with SCPI → native UDL (RLM/OSX).
- RD-SP Mapping (no SCPI) → this Cli DLL → `Santec.Hardware`.

## Build

- Platform must match Host (x64 or Win32).
- Toolset v143, `/clr`, .NET Framework 4.8.
- Vendor libs under `libs/`:
  - `common/Santec.Core.dll`
  - `x64/Santec.Hardware.dll` or `x86/Santec.Hardware.dll` (pin **2.3.1.0**)

Post-build copies Cli + Hardware + Core beside `UDL.SantecPolarity` output.

## Runtime deps

Place next to the native polarity DLL (and ensure camera runtime / OpenCvSharp native deps from vendor samples if Mapping fails to load assemblies).

## Exports

See `../Common/PolarityCliApi.h` (`SP_Cli_*`).
