# Integrated polarity tester TCP/SCPI protocol

## Transport

- TCP server, default port 5025.
- UTF-8/ASCII text commands.
- Every command and every query response ends with LF (`\n`). CRLF responses are accepted by both clients.
- One outstanding query per connection. Maximum accepted line length: 64 KiB.
- Recommended read timeout: at least 15000 ms.

These transport choices follow the attached RLM manual: Ethernet control uses TCP port 5025, write commands require LF, operations are synchronous, and long reads may require at least a 15000 ms timeout. The manual has no polarity-specific SCPI commands, so the `POLARITY` command tree below is an explicit integration contract for the assumed all-in-one tester; it must be confirmed or mapped to production firmware before real-hardware release.

## Standard/compatibility commands

| Command | Response or effect |
|---|---|
| `*IDN?` | Manufacturer, model, serial, firmware CSV |
| `*CLS` | Clear queued error |
| `*RST` | Restore default 24-core Type A configuration |
| `*OPC?` | `1` |
| `SYST:ERR?` | `<code>,"<message>"` |
| `LASER:DISABLE` | Safe laser-off request |

## Polarity command tree

| Command | Response or effect |
|---|---|
| `POLARITY:CONFIGURE:CORES 12|24` | Select core count |
| `POLARITY:CONFIGURE:CORES?` | `12` or `24` |
| `POLARITY:CONFIGURE:TYPE A|B|C` | Select expected mapping |
| `POLARITY:CONFIGURE:TYPE?` | `A`, `B`, or `C` |
| `POLARITY:TEST:START` | Start one measurement |
| `POLARITY:TEST:STATUS?` | `IDLE`, `RUNNING`, `COMPLETE`, `CANCELLED`, or `ERROR` |
| `POLARITY:TEST:ABORT` | Cancel running measurement |
| `POLARITY:RESULT:SUMMARY?` | `<PASS|FAIL>,<total>,<passed>,<failed>` |
| `POLARITY:RESULT:CORE? <input>` | `<input>,<expected>,<observed>,<light 0|1>,<status>` |

## Mapping definitions

- Type A: straight-through (`1->1`, `2->2`, ...).
- Type B: complete reversal (`1->N`, `2->N-1`, ...).
- Type C: adjacent pair swap (`1->2`, `2->1`, `3->4`, `4->3`, ...).

## Simulator-only commands

- `POLARITY:SIMULATOR:PROFILE PASS|CROSS|OPEN|SHORT|REVERSE`
- `POLARITY:SIMULATOR:PROFILE?`
- `POLARITY:SIMULATOR:LATENCY <0..60000>`

Do not send simulator commands to production hardware. Select `DEVICE DEFAULT` in the test application for a real instrument.
