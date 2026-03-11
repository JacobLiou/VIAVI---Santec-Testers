#pragma once

#include "PCT4AllTypes.h"

#ifdef UDLVIAVIPCT4ALL_EXPORTS
#define PCT4_C_API extern "C" __declspec(dllexport)
#else
#define PCT4_C_API extern "C" __declspec(dllimport)
#endif

// ===========================================================================
// C 风格 DLL 导出 API -- UDL.ViaviPCT4All
//
// 前缀: PCT4_ 用于区分
// 所有函数使用 HANDLE 封装 C++ 驱动对象
// ===========================================================================

// ---- 驱动生命周期 ----
PCT4_C_API HANDLE WINAPI PCT4_CreateDriver(const char* type, const char* ip, int port, int slot);
PCT4_C_API HANDLE WINAPI PCT4_CreateDriverEx(const char* type, const char* address,
                                              int port, int slot, int commType);
PCT4_C_API void WINAPI PCT4_DestroyDriver(HANDLE hDriver);

// ---- 连接 ----
PCT4_C_API BOOL WINAPI PCT4_Connect(HANDLE hDriver);
PCT4_C_API void WINAPI PCT4_Disconnect(HANDLE hDriver);
PCT4_C_API BOOL WINAPI PCT4_Initialize(HANDLE hDriver);
PCT4_C_API BOOL WINAPI PCT4_IsConnected(HANDLE hDriver);

// ---- 设备信息 ----
PCT4_C_API BOOL WINAPI PCT4_GetIdentification(HANDLE hDriver,
                                               ViaviPCT4All::CIdentificationInfo* info);
PCT4_C_API BOOL WINAPI PCT4_GetCassetteInfo(HANDLE hDriver,
                                              ViaviPCT4All::CCassetteInfo* info);
PCT4_C_API int  WINAPI PCT4_CheckError(HANDLE hDriver, char* message, int messageSize);

// ---- 原始 SCPI ----
PCT4_C_API BOOL WINAPI PCT4_SendCommand(HANDLE hDriver, const char* command,
                                         char* response, int responseSize);
PCT4_C_API BOOL WINAPI PCT4_SendWrite(HANDLE hDriver, const char* command);

// ---- 测量控制 ----
PCT4_C_API BOOL WINAPI PCT4_StartMeasurement(HANDLE hDriver);
PCT4_C_API BOOL WINAPI PCT4_StopMeasurement(HANDLE hDriver);
PCT4_C_API int  WINAPI PCT4_GetMeasurementState(HANDLE hDriver);
PCT4_C_API BOOL WINAPI PCT4_WaitForIdle(HANDLE hDriver, int timeoutMs);
PCT4_C_API BOOL WINAPI PCT4_MeasureReset(HANDLE hDriver);

// ---- 配置 ----
PCT4_C_API BOOL WINAPI PCT4_SetFunction(HANDLE hDriver, int mode);
PCT4_C_API int  WINAPI PCT4_GetFunction(HANDLE hDriver);
PCT4_C_API BOOL WINAPI PCT4_SetWavelength(HANDLE hDriver, int wavelength);
PCT4_C_API BOOL WINAPI PCT4_SetSourceList(HANDLE hDriver, const char* wavelengths);
PCT4_C_API BOOL WINAPI PCT4_SetAveragingTime(HANDLE hDriver, int seconds);
PCT4_C_API BOOL WINAPI PCT4_SetRange(HANDLE hDriver, int range);
PCT4_C_API BOOL WINAPI PCT4_SetILOnly(HANDLE hDriver, int state);
PCT4_C_API BOOL WINAPI PCT4_SetConnection(HANDLE hDriver, int mode);
PCT4_C_API BOOL WINAPI PCT4_SetBiDir(HANDLE hDriver, int state);

// ---- PATH / 光路控制 ----
PCT4_C_API BOOL WINAPI PCT4_SetChannel(HANDLE hDriver, int group, int channel);
PCT4_C_API int  WINAPI PCT4_GetChannel(HANDLE hDriver, int group);
PCT4_C_API BOOL WINAPI PCT4_SetPathList(HANDLE hDriver, int sw, const char* channels);
PCT4_C_API BOOL WINAPI PCT4_SetLaunch(HANDLE hDriver, int port);

// ---- 日志 ----
typedef void (WINAPI *PCT4LogCallback)(int level, const char* source, const char* message);
PCT4_C_API void WINAPI PCT4_SetLogCallback(PCT4LogCallback callback);

// ---- VISA 枚举 ----
PCT4_C_API int WINAPI PCT4_EnumerateVisaResources(char* buffer, int bufferSize);
