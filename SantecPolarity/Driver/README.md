# UDL.SantecPolarity.Driver — FROZEN STUB

**Not for production.** Speaks invented `POLARITY:*` SCPI only.
Production polarity UDL lives in `TestEquipmentDrivers/UDL.SantecPolarity/`.

Self-contained native C++ DLL for an assumed integrated polarity tester controlled only by TCP/SCPI.
It does not include or reference files outside this directory.

## Build

Open `UDL.SantecPolarity.Driver.sln` in Visual Studio 2015 Update 3. Select Win32 or x64 and Debug or Release.
The project uses toolset v140 and Windows 8.1 SDK.

## API

Include `SantecPolarityDriver.h`, link the generated import library, and use the `SP_*` C ABI.
A normal sequence is `SP_Create`, `SP_Connect`, `SP_GetIdentity`, `SP_RunTest`, `SP_Disconnect`, `SP_Destroy`.
The DLL always tries `LASER:DISABLE` before a normal disconnect.

Do **not** load this DLL from the production Molex host.
