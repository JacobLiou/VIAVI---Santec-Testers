#pragma once

// ---------------------------------------------------------------------------
// C-style DLL exported API for external system integration
// (MIMS / Mlight / MAS or any other caller via LoadLibrary/GetProcAddress)
//
// All functions use HANDLE to wrap the C++ driver object.
// ---------------------------------------------------------------------------

#include "OSXTypes.h"

#ifdef UDLOSX_EXPORTS
#define OSX_C_API extern "C" __declspec(dllexport)
#else
#define OSX_C_API extern "C" __declspec(dllimport)
#endif

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

// Create an OSX driver instance. Returns HANDLE (opaque pointer).
OSX_C_API HANDLE WINAPI OSX_CreateDriver(const char* ip, int port);

// Destroy a driver instance and free resources.
OSX_C_API void WINAPI OSX_DestroyDriver(HANDLE hDriver);

// ---------------------------------------------------------------------------
// Connection
// ---------------------------------------------------------------------------

OSX_C_API BOOL WINAPI OSX_Connect(HANDLE hDriver);
OSX_C_API void WINAPI OSX_Disconnect(HANDLE hDriver);
OSX_C_API BOOL WINAPI OSX_Reconnect(HANDLE hDriver);
OSX_C_API BOOL WINAPI OSX_IsConnected(HANDLE hDriver);

// ---------------------------------------------------------------------------
// Device identification
// ---------------------------------------------------------------------------

OSX_C_API BOOL WINAPI OSX_GetDeviceInfo(HANDLE hDriver, OSXSwitch::CDeviceInfo* info);
OSX_C_API int  WINAPI OSX_CheckError(HANDLE hDriver, char* message, int messageSize);
OSX_C_API BOOL WINAPI OSX_GetSystemVersion(HANDLE hDriver, char* version, int versionSize);

// ---------------------------------------------------------------------------
// Module management
// ---------------------------------------------------------------------------

OSX_C_API int  WINAPI OSX_GetModuleCount(HANDLE hDriver);
OSX_C_API int  WINAPI OSX_GetModuleCatalog(HANDLE hDriver, OSXSwitch::CModuleInfo* modules, int maxCount);
OSX_C_API BOOL WINAPI OSX_GetModuleInfo(HANDLE hDriver, int moduleIndex, OSXSwitch::CModuleInfo* info);
OSX_C_API BOOL WINAPI OSX_SelectModule(HANDLE hDriver, int moduleIndex);
OSX_C_API BOOL WINAPI OSX_SelectNextModule(HANDLE hDriver);
OSX_C_API int  WINAPI OSX_GetSelectedModule(HANDLE hDriver);

// ---------------------------------------------------------------------------
// Channel switching
// ---------------------------------------------------------------------------

OSX_C_API BOOL WINAPI OSX_SwitchChannel(HANDLE hDriver, int channel);
OSX_C_API BOOL WINAPI OSX_SwitchNext(HANDLE hDriver);
OSX_C_API int  WINAPI OSX_GetCurrentChannel(HANDLE hDriver);
OSX_C_API int  WINAPI OSX_GetChannelCount(HANDLE hDriver);

// ---------------------------------------------------------------------------
// Multi-module routing
// ---------------------------------------------------------------------------

OSX_C_API BOOL WINAPI OSX_RouteChannel(HANDLE hDriver, int moduleIndex, int channel);
OSX_C_API int  WINAPI OSX_GetRouteChannel(HANDLE hDriver, int moduleIndex);
OSX_C_API BOOL WINAPI OSX_RouteAllModules(HANDLE hDriver, int channel);
OSX_C_API BOOL WINAPI OSX_SetCommonInput(HANDLE hDriver, int moduleIndex, int common);
OSX_C_API int  WINAPI OSX_GetCommonInput(HANDLE hDriver, int moduleIndex);
OSX_C_API BOOL WINAPI OSX_HomeModule(HANDLE hDriver, int moduleIndex);

// ---------------------------------------------------------------------------
// Control
// ---------------------------------------------------------------------------

OSX_C_API BOOL WINAPI OSX_SetLocalMode(HANDLE hDriver, BOOL local);
OSX_C_API BOOL WINAPI OSX_GetLocalMode(HANDLE hDriver);
OSX_C_API BOOL WINAPI OSX_SendNotification(HANDLE hDriver, int icon, const char* message);
OSX_C_API BOOL WINAPI OSX_Reset(HANDLE hDriver);

// ---------------------------------------------------------------------------
// Network configuration
// ---------------------------------------------------------------------------

OSX_C_API BOOL WINAPI OSX_GetNetworkInfo(HANDLE hDriver, char* ip, char* gateway,
                                         char* netmask, char* hostname, char* mac,
                                         int bufSize);

// ---------------------------------------------------------------------------
// Operation synchronization
// ---------------------------------------------------------------------------

OSX_C_API BOOL WINAPI OSX_WaitForOperation(HANDLE hDriver, int timeoutMs);

// ---------------------------------------------------------------------------
// Raw SCPI
// ---------------------------------------------------------------------------

OSX_C_API BOOL WINAPI OSX_SendCommand(HANDLE hDriver, const char* command,
                                      char* response, int responseSize);

// ---------------------------------------------------------------------------
// Logging
// ---------------------------------------------------------------------------

typedef void (WINAPI *OSXLogCallback)(int level, const char* source, const char* message);
OSX_C_API void WINAPI OSX_SetLogCallback(OSXLogCallback callback);
