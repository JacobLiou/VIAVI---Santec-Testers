#pragma once

// C-compatible types for UDL.SantecPolarity (RLM-100 + OSX-100 + RD-SP-100-SM).
// ABI follows Final Plan section 10. Exports via UDL.SantecPolarity.def.

#include <stdint.h>

typedef void* SP_HANDLE;

typedef enum SP_STATUS
{
    SP_OK                        = 0,
    SP_E_INVALID_ARGUMENT        = -1,
    SP_E_INVALID_STATE           = -2,
    SP_E_DEVICE_BUSY             = -3,
    SP_E_PROFILE_MISMATCH        = -4,

    SP_E_RDSP_NOT_FOUND          = -10,
    SP_E_RLM_NOT_FOUND           = -11,
    SP_E_RLM_MODEL_MISMATCH      = -12,
    SP_E_RLM_OPTION_MISMATCH     = -13,
    SP_E_OSX_NOT_FOUND           = -14,
    SP_E_OSX_CHANNEL_MISMATCH    = -15,
    SP_E_REMOTE_HEAD_MISMATCH    = -16,
    SP_E_HARDWARE_NOT_SUPPORTED  = -17,

    SP_E_TIMEOUT                 = -20,
    SP_E_CANCELLED               = -21,
    SP_E_COMMUNICATION           = -22,
    SP_E_INVALID_RESPONSE        = -23,
    SP_E_DEVICE_ERROR            = -24,

    SP_E_AMBIENT_LIGHT           = -30,
    SP_E_MAPPING_FAILED          = -31,
    SP_E_DIAGNOSTIC_SAVE_FAILED  = -32,

    SP_E_BACKEND_NOT_SUPPORTED   = -40,
    SP_E_BACKEND_VERSION         = -41,
    SP_E_WORKER_UNAVAILABLE      = -42,
    SP_E_INTERNAL                = -100
} SP_STATUS;

typedef enum SP_FIBER_STATUS
{
    SP_FIBER_PASS = 0,
    SP_FIBER_WRONG_MAPPING,
    SP_FIBER_MISSING,
    SP_FIBER_UNEXPECTED,
    SP_FIBER_DUPLICATE,
    SP_FIBER_DARK_ALLOWED,
    SP_FIBER_AMBIGUOUS,
    SP_FIBER_ACQUISITION_ERROR
} SP_FIBER_STATUS;

#pragma pack(push, 8)
typedef struct SP_OPEN_OPTIONS
{
    uint32_t structSize;
    uint32_t version;
    const char* stationIdUtf8;
    const char* rlmSerialUtf8;
    const char* rdspSerialUtf8;
    const char* profilePathUtf8;
    uint32_t connectTimeoutMs;
    uint32_t operationTimeoutMs;
    uint32_t mappingTimeoutMs;
} SP_OPEN_OPTIONS;

typedef struct SP_HARDWARE_SNAPSHOT
{
    uint32_t structSize;
    uint32_t version;
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
} SP_HARDWARE_SNAPSHOT;

typedef struct SP_FIBER_RESULT
{
    int32_t inputFiber;
    int32_t switchChannel;
    int32_t expectedOutput;
    int32_t observedOutput; // unknown = 0
    int32_t lightDetected;  // 0/1
    SP_FIBER_STATUS status;
    uint32_t acquisitionMs;
} SP_FIBER_RESULT;

typedef struct SP_RESULT_SUMMARY
{
    uint32_t structSize;
    uint32_t version;
    int32_t overallPass;
    uint32_t fiberCount;
    uint32_t passCount;
    uint32_t failCount;
    uint32_t diagnosticImageCount;
    uint64_t elapsedMs;
} SP_RESULT_SUMMARY;
#pragma pack(pop)
