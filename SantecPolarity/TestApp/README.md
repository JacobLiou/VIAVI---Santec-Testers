# UDL.SantecPolarity.TestApp — FROZEN STUB GUI

**Not for production hardware.** Talks invented `POLARITY:*` to the local simulator.

Standalone native Win32 GUI for an assumed integrated polarity tester. It contains its own TCP/SCPI client and has no build-time or runtime dependency on the driver project.

## Use

1. Start the simulator (not real RLM/RD-SP hardware).
2. Enter the IP address and TCP port (default 5025), then click Connect.
3. Select 12 or 24 cores and polarity Type A, B, or C.
4. Select a deterministic simulator profile.
5. Click Run Test. Results are collected on a worker thread; Cancel requests `POLARITY:TEST:ABORT`.

The tool sends `LASER:DISABLE` on normal disconnect and shutdown.
