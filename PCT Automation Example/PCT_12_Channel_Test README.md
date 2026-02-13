# PCT 12 Channel Test Script

This Python script automates optical measurements using a MAP300 PCT (Passive Component Tester) for testing 12 channels of optical components.

## Features

- Automated connection to MAP300 chassis and PCT module
- Configurable ORL (Optical Return Loss) window settings
- Support for multiple wavelength measurements (1310nm, 1550nm)
- Automated reference and measurement processes
- IL (Insertion Loss) and RL (Return Loss) measurements
- Error checking functionality

## Requirements

- Python 3.x
- Network access to MAP300 chassis
- Socket communication capabilities

## Configuration

Key parameters that can be configured:

- IP Address: Set the MAP300 IP address
- Ports: 
  - PCT Port: 8300 + slot number
  - Chassis Port: 8100
- Measurement Settings:
  - Wavelengths: 1310nm, 1550nm
  - Method: Discrete Mode (2) or Integration Mode (1)
  - Origin points and offsets for measurements

## Usage

1. Connect to MAP300:
   - Script automatically connects to chassis
   - Launches Super Application
   - Establishes connection to PCT module

2. Measurement Options:
   - Zero-loss reference measurement
   - Override reference with custom IL and length values
   - Full IL and RL measurements across all channels

3. Results:
   - Displays RL measurements for each channel
   - Shows results for all configured wavelengths

## Error Handling

The script includes comprehensive error checking:
- Validates commands
- Monitors measurement states
- Reports any system errors

## Notes

- Default timeout for socket connections is 3 seconds
- Super Application launch requires approximately 7 seconds
- Supports both APC and UPC connector measurements
