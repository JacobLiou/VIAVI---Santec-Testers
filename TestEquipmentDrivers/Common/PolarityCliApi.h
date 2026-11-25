#pragma once

// Native C ABI between UDL.SantecPolarity (pure native) and
// UDL.SantecPolarity.Cli (C++/CLI -> Santec.Hardware).
// Host/MFC must NOT call these; only the native polarity shell loads the Cli DLL.

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* SP_CLI_HANDLE;

typedef struct SP_CLI_OPEN_PARAMS
{
    uint32_t structSize;
    uint32_t version;
    const char* stationIdUtf8;
    const char* rlmSerialUtf8;
    const char* rdspSerialUtf8;
    uint32_t fiberCount;          // 12 or 24; 0 => 12
    uint32_t connectTimeoutMs;
    uint32_t mappingTimeoutMs;
} SP_CLI_OPEN_PARAMS;

typedef struct SP_CLI_FIBER_RESULT
{
    int32_t inputFiber;
    int32_t switchChannel;
    int32_t expectedOutput;
    int32_t observedOutput; // 0 = unknown / not detected
    int32_t lightDetected;
    int32_t status;         // SP_FIBER_STATUS values
    uint32_t acquisitionMs;
} SP_CLI_FIBER_RESULT;

typedef struct SP_CLI_SUMMARY
{
    uint32_t structSize;
    int32_t overallPass;
    uint32_t fiberCount;
    uint32_t passCount;
    uint32_t failCount;
    uint32_t diagnosticImageCount;
    uint64_t elapsedMs;
} SP_CLI_SUMMARY;

typedef struct SP_CLI_SNAPSHOT
{
    uint32_t structSize;
    char rlmPartNumber[64];
    char rlmSerial[64];
    char rlmFirmware[32];
    char osxPartNumber[64];
    char rdspPartNumber[64];
    char rdspSerial[64];
    int32_t rlmOutputCount;
    int32_t switchNumber;
    int32_t switchChannelCount;
    int32_t polarityWavelengthNm;
    int32_t remoteDetectorIndex;
    int32_t isRdsp; // 1 if IsRDSP
} SP_CLI_SNAPSHOT;

// Status codes mirror SP_STATUS in SPTypes.h
int __stdcall SP_Cli_Create(SP_CLI_HANDLE* handle);
int __stdcall SP_Cli_Destroy(SP_CLI_HANDLE handle);
int __stdcall SP_Cli_Open(SP_CLI_HANDLE handle, const SP_CLI_OPEN_PARAMS* params);
int __stdcall SP_Cli_Close(SP_CLI_HANDLE handle);
int __stdcall SP_Cli_GetSnapshot(SP_CLI_HANDLE handle, SP_CLI_SNAPSHOT* snapshot);
int __stdcall SP_Cli_RunMapping(SP_CLI_HANDLE handle,
                                const int32_t* expectedMap,
                                uint32_t expectedCount);
int __stdcall SP_Cli_Cancel(SP_CLI_HANDLE handle);
int __stdcall SP_Cli_GetSummary(SP_CLI_HANDLE handle, SP_CLI_SUMMARY* summary);
int __stdcall SP_Cli_GetFiberCount(SP_CLI_HANDLE handle, uint32_t* count);
int __stdcall SP_Cli_GetFibers(SP_CLI_HANDLE handle,
                               SP_CLI_FIBER_RESULT* buffer,
                               uint32_t capacity,
                               uint32_t* written);
int __stdcall SP_Cli_GetDiagnosticPath(SP_CLI_HANDLE handle,
                                       uint32_t index,
                                       char* buffer,
                                       uint32_t capacity,
                                       uint32_t* required);
int __stdcall SP_Cli_GetLastError(SP_CLI_HANDLE handle,
                                  char* buffer,
                                  uint32_t capacity,
                                  uint32_t* required);

#ifdef __cplusplus
}
#endif
