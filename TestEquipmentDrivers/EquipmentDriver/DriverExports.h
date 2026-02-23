#pragma once

// ---------------------------------------------------------------------------
// C-style DLL exported API for external system integration
// (MIMS / Mlight / MAS or any other caller via LoadLibrary/GetProcAddress)
//
// All functions use HANDLE to wrap the C++ driver object.
// ---------------------------------------------------------------------------

#include "DriverTypes.h"

#ifdef EQUIPMENTDRIVER_EXPORTS
#define DRIVER_C_API extern "C" __declspec(dllexport)
#else
#define DRIVER_C_API extern "C" __declspec(dllimport)
#endif

// Create a driver instance. Returns HANDLE (opaque pointer).
// type: "viavi" or "santec"
// ip: device IP address (e.g. "10.14.132.194")
// port: TCP port (0 = use default for the type)
// slot: slot number (VIAVI only, ignored for Santec)
DRIVER_C_API HANDLE WINAPI CreateDriver(const char* type, const char* ip, int port, int slot);

// Destroy a driver instance and free resources
DRIVER_C_API void WINAPI DestroyDriver(HANDLE hDriver);

// Connect to the device
DRIVER_C_API BOOL WINAPI DriverConnect(HANDLE hDriver);

// Disconnect from the device
DRIVER_C_API void WINAPI DriverDisconnect(HANDLE hDriver);

// Initialize the device after connection
DRIVER_C_API BOOL WINAPI DriverInitialize(HANDLE hDriver);

// Check if connected
DRIVER_C_API BOOL WINAPI DriverIsConnected(HANDLE hDriver);

// Configure measurement wavelengths (array of doubles in nm)
DRIVER_C_API BOOL WINAPI DriverConfigureWavelengths(HANDLE hDriver, double* wavelengths, int count);

// Configure measurement channels (array of ints)
DRIVER_C_API BOOL WINAPI DriverConfigureChannels(HANDLE hDriver, int* channels, int count);

// Configure ORL parameters (VIAVI specific; no-op for other types)
DRIVER_C_API BOOL WINAPI DriverConfigureORL(HANDLE hDriver, int channel, int method, int origin,
                                            double aOffset, double bOffset);

// Take reference measurement
// bOverride: TRUE to use override values, FALSE for automatic
DRIVER_C_API BOOL WINAPI DriverTakeReference(HANDLE hDriver, BOOL bOverride,
                                             double ilValue, double lengthValue);

// Take IL/RL measurement
DRIVER_C_API BOOL WINAPI DriverTakeMeasurement(HANDLE hDriver);

// Retrieve results into caller-allocated array. Returns number of results written.
// results: pre-allocated array of CMeasurementResult
// maxCount: size of the array
DRIVER_C_API int WINAPI DriverGetResults(HANDLE hDriver,
                                         EquipmentDriver::CMeasurementResult* results,
                                         int maxCount);

// Get device info
DRIVER_C_API BOOL WINAPI DriverGetDeviceInfo(HANDLE hDriver, EquipmentDriver::CDeviceInfo* info);

// Check last error. Returns error code (0 = no error).
// message: buffer to receive error message
// messageSize: size of buffer
DRIVER_C_API int WINAPI DriverCheckError(HANDLE hDriver, char* message, int messageSize);

// Send raw SCPI command and receive response
// response: buffer to receive response (can be NULL for write-only commands)
// responseSize: size of buffer
DRIVER_C_API BOOL WINAPI DriverSendCommand(HANDLE hDriver, const char* command,
                                           char* response, int responseSize);

// Set log callback (receives log messages from the driver)
typedef void (WINAPI *DriverLogCallback)(int level, const char* source, const char* message);
DRIVER_C_API void WINAPI DriverSetLogCallback(DriverLogCallback callback);
