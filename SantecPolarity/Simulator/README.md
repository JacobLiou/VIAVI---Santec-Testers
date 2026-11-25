# UDL.SantecPolarity.Simulator — FROZEN STUB

**Not a real-instrument emulator for RLM/OSX/RD-SP.** Implements invented `POLARITY:*` only.

A standalone TCP/SCPI simulator for the assumed integrated polarity tester. It does not link to the driver and does not include files outside this directory.

## Run

`UDL.SantecPolarity.Simulator.exe [port]`

The default port is 5025. Each TCP client has independent configuration and results.
Use `POLARITY:SIMULATOR:PROFILE PASS|CROSS|OPEN|SHORT|REVERSE` to choose deterministic behavior.
