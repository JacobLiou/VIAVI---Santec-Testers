# UDL.SantecPolarity.Simulator

A standalone TCP/SCPI simulator for the integrated polarity tester. It does not link to the driver and does not include files outside this directory.

## Run

`UDL.SantecPolarity.Simulator.exe [port]`

The default port is 5025. Each TCP client has independent configuration and results.
Use `POLARITY:SIMULATOR:PROFILE PASS|CROSS|OPEN|SHORT|REVERSE` to choose deterministic behavior.
