#pragma once

// FROZEN STUB ABI — invented POLARITY:* TCP contract only.
// Do NOT load this DLL in production. Production path:
// TestEquipmentDrivers/UDL.SantecPolarity (RLM-100 + OSX-100 + RD-SP-100-SM).
//
// Public C ABI. Callers do not need C++ runtime types.
// All structures are caller-allocated and versioned by size.

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#ifdef SANTEC_POLARITY_DRIVER_EXPORTS
#define SP_API extern "C" __declspec(dllexport)
#else
#define SP_API extern "C" __declspec(dllimport)
#endif
#define SP_CALL __stdcall

typedef void* SP_HANDLE;

enum SP_ResultCode
{
    SP_OK = 0,
    SP_E_INVALID_ARGUMENT = -1,
    SP_E_INVALID_HANDLE = -2,
    SP_E_NOT_CONNECTED = -3,
    SP_E_TRANSPORT = -4,
    SP_E_PROTOCOL = -5,
    SP_E_TIMEOUT = -6,
    SP_E_CANCELLED = -7,
    SP_E_BUFFER_TOO_SMALL = -8
};

enum SP_TestState
{
    SP_STATE_IDLE = 0,
    SP_STATE_RUNNING = 1,
    SP_STATE_COMPLETE = 2,
    SP_STATE_CANCELLED = 3,
    SP_STATE_ERROR = 4
};

enum SP_PolarityType
{
    SP_POLARITY_A = 0,
    SP_POLARITY_B = 1,
    SP_POLARITY_C = 2
};

typedef struct SP_TestSummary
{
    unsigned int size;
    int passed;
    int coreCount;
    int passedCores;
    int failedCores;
    char status[32];
} SP_TestSummary;

typedef struct SP_CoreResult
{
    unsigned int size;
    int inputCore;
    int expectedOutput;
    int observedOutput;
    int lightDetected;
    char status[32];
} SP_CoreResult;

SP_API int SP_CALL SP_Create(SP_HANDLE* handle);
SP_API int SP_CALL SP_Destroy(SP_HANDLE handle);
SP_API int SP_CALL SP_Connect(SP_HANDLE handle, const char* host, unsigned short port,
                              unsigned long connectTimeoutMs, unsigned long ioTimeoutMs);
SP_API int SP_CALL SP_Disconnect(SP_HANDLE handle);
SP_API int SP_CALL SP_IsConnected(SP_HANDLE handle, int* connected);
SP_API int SP_CALL SP_GetIdentity(SP_HANDLE handle, char* buffer, unsigned int bufferSize);
SP_API int SP_CALL SP_Configure(SP_HANDLE handle, int coreCount, int polarityType);
SP_API int SP_CALL SP_StartTest(SP_HANDLE handle);
SP_API int SP_CALL SP_GetTestState(SP_HANDLE handle, int* state);
SP_API int SP_CALL SP_GetSummary(SP_HANDLE handle, SP_TestSummary* summary);
SP_API int SP_CALL SP_GetCoreResult(SP_HANDLE handle, int inputCore, SP_CoreResult* result);
SP_API int SP_CALL SP_RunTest(SP_HANDLE handle, int coreCount, int polarityType,
                              unsigned long overallTimeoutMs, SP_TestSummary* summary,
                              SP_CoreResult* coreResults, int coreResultCapacity);
SP_API int SP_CALL SP_Cancel(SP_HANDLE handle);
SP_API int SP_CALL SP_GetLastError(SP_HANDLE handle, char* buffer, unsigned int bufferSize);
