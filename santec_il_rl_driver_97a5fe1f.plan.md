---
name: Santec IL/RL Driver
overview: Based on extensive online research, there are multiple official Santec resources that can directly accelerate the implementation of the CSantecDriver for RLM-100 and ILM-100. The plan outlines all available resources and the implementation approach.
todos:
  - id: download-samples
    content: Download RLM-100 sample code ZIP, BRM-100 sample ZIP, IDN Connection sample, and Santec Programming Guide PDF from inst.santec.com/resources/programming
    status: pending
  - id: analyze-protocol
    content: "Analyze sample code to extract: TCP port, SCPI commands, response format, measurement workflow for IL and RL"
    status: pending
  - id: implement-driver
    content: Fill in CSantecDriver skeleton methods in SantecDriver.cpp based on discovered SCPI commands
    status: pending
  - id: dual-device
    content: Handle RLM-100 vs ILM-100 differences (model detection or subclassing)
    status: pending
  - id: santec-simulator
    content: Create SantecSimulator console project for offline testing
    status: pending
isProject: false
---

# Santec IL/RL Driver Implementation Plan

## Available Online Resources (Priority Order)

### 1. Official Sample Code (MOST DIRECTLY USEFUL)

Santec provides **official C# and Python sample code** for both devices on their [Programming and Drivers](https://inst.santec.com/resources/programming) page:

- **RLM-100 Sample Code** (C# + Python): [Download ZIP](https://santec-inst.files.svdcdn.com/production/RLM-Samples.zip?dm=1768835134)
- **BRM-100 Sample Code** (C#): [Download ZIP](https://santec-inst.files.svdcdn.com/production/BRM-Samples.zip?dm=1768834927) -- shares similar IL/RL measurement patterns
- **IDN Connection Sample** (Python): [Download ZIP](https://santec-inst.files.svdcdn.com/production/IDN-Connection-Sample.zip?dm=1768835060) -- basic SCPI connection example

These samples will reveal the **exact SCPI command set, TCP port, protocol format, and measurement workflow** for IL/RL operations.

### 2. Santec.Hardware.dll and Programming Guide

- **Santec.Hardware Library v1.23.0.0**: [Download DLL](https://santec-inst.files.svdcdn.com/production/Files/Programming/Santec.Hardware-1.23.0.0.zip?dm=1768835004) -- official .NET wrapper for all Santec instruments
- **Programming Guide v1.23.0.0**: [Download PDF](https://santec-inst.files.svdcdn.com/production/Files/Programming/Santec-Hardware-Library-Programming-Guide-1.23.0.0.pdf?dm=1768835004) -- full API documentation with command reference

The Programming Guide is a PDF document containing the complete SCPI command reference for all Santec instruments including RLM-100 and ILM-100. **This is the most authoritative command reference.**

### 3. GitHub: VS_Instrument_DLL_Sample (C++/MFC Reference)

[santec-corporation/VS_Instrument_DLL_Sample](https://github.com/santec-corporation/VS_Instrument_DLL_Sample) contains:

- `Instrument_DLL_Sample_MFC/` -- **C++ MFC sample** using VS2015, same tech stack as our project
- `Instrument_DLL_Sample_CPP/` -- C++ console sample
- `librayForMFCInstrument/` -- MFC helper library

While this sample targets TSL/MPM/PCU/OSU instruments (not directly RLM/ILM), the **communication patterns, DLL usage, and SCPI handling code** are transferable and show how Santec instruments are controlled from C++.

### 4. SCPI Terminal Software

[SCPI Command Terminal v2.1.8.0](https://inst.santec.com/products/test-and-measurement/software/terminal) -- Santec's official tool with **built-in SCPI command lists for each instrument type**. Can be used for:

- Discovering the exact commands supported by RLM-100 and ILM-100
- Testing commands interactively before coding
- Verifying Ethernet/USB connectivity

### 5. User Manuals

Available at [inst.santec.com/resources/usermanuals](https://inst.santec.com/resources/usermanuals):

- **RLM-100 User Manual** (M-RL1-001-05)
- **ILM-100 User Manual** (MnOP815-RevE)

These typically contain SCPI command appendices and remote control setup instructions.

### 6. Tutorials

From [inst.santec.com/resources/tutorials](https://inst.santec.com/resources/tutorials):

- "Getting Started with the RLM"
- "Accessing and Navigating the RLM Webpage"
- "How to Reference and Measure BR SM BRM" (similar workflow)

---

## Implementation Approach

### Step 1: Download and Analyze Sample Code

- Download the RLM-100 and BRM-100 sample ZIPs
- Extract the SCPI command sequences for: connect, identify, reference, measure, get results
- Document the TCP port number, termination characters, response format

### Step 2: Download and Study the Programming Guide PDF

- Focus on chapters covering RLM-100 and ILM-100
- Extract the full SCPI command table for measurement operations
- Note any device-specific initialization sequences

### Step 3: Implement CSantecDriver Methods

Fill in the skeleton methods in [SantecDriver.cpp](TestEquipmentDrivers/UDL.ViaviNSantecTester/SantecDriver.cpp) based on discovered commands:

- `Initialize()` -- identity query, system reset, calibration check
- `ConfigureWavelengths()` -- set measurement wavelength (1310/1550)
- `TakeReference()` -- execute reference/calibration procedure
- `TakeMeasurement()` -- trigger IL/RL measurement
- `WaitForCompletion()` -- poll measurement status
- `GetResults()` -- retrieve IL and RL values
- `GetDeviceInfo()` -- parse `*IDN?` response
- `CheckError()` -- query `SYST:ERR?`

### Step 4: Dual Device Support

Since there are both RLM-100 and ILM-100, consider:

- Whether they share the same SCPI command set (likely similar with minor differences)
- Whether to use a single `CSantecDriver` with model detection, or subclass for each

### Step 5: Build a Santec Simulator

Similar to the VIAVI simulator, create a `SantecSimulator` project for testing without hardware.

---

## Recommended Immediate Actions

1. **Download these 3 files now** (they contain all needed protocol details):
  - RLM-100 Sample Code ZIP
  - Santec Programming Guide PDF
  - IDN Connection Sample ZIP
2. **Install the SCPI Terminal** on a PC with access to the Santec devices to interactively test commands
3. **Connect to a real device** using the SCPI Terminal to verify connectivity and explore available commands before writing any driver code

