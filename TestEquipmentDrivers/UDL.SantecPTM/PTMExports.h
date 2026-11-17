#pragma once

#include "PTMTypes.h"

// Exports are declared via UDL.SantecPTM.def (avoid LNK4197 double-export).
#ifdef UDLSANTECPTM_EXPORTS
#define PTM_C_API extern "C"
#else
#define PTM_C_API extern "C" __declspec(dllimport)
#endif

// Lifecycle
PTM_C_API HANDLE WINAPI PTM_CreateDriver(const char* type, const char* ip, int port, int slot);
PTM_C_API HANDLE WINAPI PTM_CreateDriverEx(const char* type, const char* address,
                                           int port, int slot, int commType);
PTM_C_API void   WINAPI PTM_DestroyDriver(HANDLE hDriver);

// Connection
PTM_C_API BOOL WINAPI PTM_Connect(HANDLE hDriver);
PTM_C_API void WINAPI PTM_Disconnect(HANDLE hDriver);
PTM_C_API BOOL WINAPI PTM_Initialize(HANDLE hDriver);
PTM_C_API BOOL WINAPI PTM_IsConnected(HANDLE hDriver);

// Measurement
PTM_C_API int  WINAPI PTM_GetChannelCount(HANDLE hDriver);
PTM_C_API BOOL WINAPI PTM_TakeMeasurement(HANDLE hDriver, int nChannels);
PTM_C_API int  WINAPI PTM_GetResults(HANDLE hDriver,
                                     SantecPTM::CPolarityChannelResult* results,
                                     int maxCount);
PTM_C_API BOOL WINAPI PTM_GetRawResult(HANDLE hDriver, char* buffer, int bufferSize);

// Device helpers
PTM_C_API int  WINAPI PTM_Detect(HANDLE hDriver, int outputChannel);
PTM_C_API BOOL WINAPI PTM_SetLocalMode(HANDLE hDriver, BOOL enabled);
PTM_C_API BOOL WINAPI PTM_GetLocalMode(HANDLE hDriver);
PTM_C_API BOOL WINAPI PTM_LaserOff(HANDLE hDriver);
PTM_C_API BOOL WINAPI PTM_SetVfl(HANDLE hDriver, int outputChannel, int mode);
PTM_C_API BOOL WINAPI PTM_GetBarcode(HANDLE hDriver, char* buffer, int bufferSize);
PTM_C_API BOOL WINAPI PTM_Notify(HANDLE hDriver, int icon, const char* message);

// XN1 workflow
PTM_C_API BOOL WINAPI PTM_TestStart(HANDLE hDriver);
PTM_C_API BOOL WINAPI PTM_TestStatus(HANDLE hDriver, char* buffer, int bufferSize);
PTM_C_API BOOL WINAPI PTM_TestNext(HANDLE hDriver);
PTM_C_API BOOL WINAPI PTM_TestRetry(HANDLE hDriver);

// Info / diagnostics
PTM_C_API BOOL WINAPI PTM_GetDeviceInfo(HANDLE hDriver, SantecPTM::CDeviceInfo* info);
PTM_C_API int  WINAPI PTM_CheckError(HANDLE hDriver, char* message, int messageSize);
PTM_C_API BOOL WINAPI PTM_SendCommand(HANDLE hDriver, const char* command,
                                      char* response, int responseSize);

typedef void (WINAPI *PTM_LogCallback)(int level, const char* source, const char* message);
PTM_C_API void WINAPI PTM_SetLogCallback(PTM_LogCallback callback);
PTM_C_API int  WINAPI PTM_EnumerateVisaResources(char* buffer, int bufferSize);
