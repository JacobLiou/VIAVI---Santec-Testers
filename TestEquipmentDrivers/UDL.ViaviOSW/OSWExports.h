#pragma once

// ---------------------------------------------------------------------------
// C 风格 DLL 导出 API，用于外部系统集成
//
// 所有函数使用 HANDLE 来封装 C++ 驱动对象。
// 前缀: OSW_ 用于区分不同驱动模块
// ---------------------------------------------------------------------------

#include "OSWTypes.h"

#ifdef UDLVIAVIOSW_EXPORTS
#define OSW_C_API extern "C" __declspec(dllexport)
#else
#define OSW_C_API extern "C" __declspec(dllimport)
#endif

// ---------------------------------------------------------------------------
// 驱动生命周期
// ---------------------------------------------------------------------------

// 创建驱动实例 (TCP 模式)
// ip: MAP 系统 IP 地址
// port: OSW 模块 TCP 端口（0 = 使用默认端口 8203）
OSW_C_API HANDLE WINAPI OSW_CreateDriver(const char* ip, int port);

// 创建驱动实例（扩展版本，支持指定通信类型）
// commType: 0=TCP, 1=GPIB, 2=VISA
// address: TCP 模式为 IP 地址，VISA 模式为 VISA 资源字符串
OSW_C_API HANDLE WINAPI OSW_CreateDriverEx(const char* address, int port, int commType);

// 销毁驱动实例并释放资源
OSW_C_API void WINAPI OSW_DestroyDriver(HANDLE hDriver);

// ---------------------------------------------------------------------------
// 连接
// ---------------------------------------------------------------------------

OSW_C_API BOOL WINAPI OSW_Connect(HANDLE hDriver);
OSW_C_API void WINAPI OSW_Disconnect(HANDLE hDriver);
OSW_C_API BOOL WINAPI OSW_IsConnected(HANDLE hDriver);

// ---------------------------------------------------------------------------
// 设备识别
// ---------------------------------------------------------------------------

OSW_C_API BOOL WINAPI OSW_GetDeviceInfo(HANDLE hDriver, ViaviOSW::CDeviceInfo* info);
OSW_C_API int  WINAPI OSW_CheckError(HANDLE hDriver, char* message, int messageSize);

// ---------------------------------------------------------------------------
// 开关信息
// ---------------------------------------------------------------------------

OSW_C_API int  WINAPI OSW_GetDeviceCount(HANDLE hDriver);
OSW_C_API BOOL WINAPI OSW_GetSwitchInfo(HANDLE hDriver, int deviceNum, ViaviOSW::CSwitchInfo* info);

// ---------------------------------------------------------------------------
// 通道切换
// ---------------------------------------------------------------------------

OSW_C_API BOOL WINAPI OSW_SwitchChannel(HANDLE hDriver, int deviceNum, int channel);
OSW_C_API int  WINAPI OSW_GetCurrentChannel(HANDLE hDriver, int deviceNum);
OSW_C_API int  WINAPI OSW_GetChannelCount(HANDLE hDriver, int deviceNum);

// ---------------------------------------------------------------------------
// 控制
// ---------------------------------------------------------------------------

OSW_C_API BOOL WINAPI OSW_SetLocalMode(HANDLE hDriver, BOOL local);
OSW_C_API BOOL WINAPI OSW_Reset(HANDLE hDriver);

// ---------------------------------------------------------------------------
// 操作同步
// ---------------------------------------------------------------------------

OSW_C_API BOOL WINAPI OSW_WaitForIdle(HANDLE hDriver, int timeoutMs);

// ---------------------------------------------------------------------------
// 原始 SCPI
// ---------------------------------------------------------------------------

OSW_C_API BOOL WINAPI OSW_SendCommand(HANDLE hDriver, const char* command,
                                       char* response, int responseSize);

// ---------------------------------------------------------------------------
// 日志
// ---------------------------------------------------------------------------

typedef void (WINAPI *OSWLogCallback)(int level, const char* source, const char* message);
OSW_C_API void WINAPI OSW_SetLogCallback(OSWLogCallback callback);
OSW_C_API void WINAPI OSW_SetLogCallbackEx(HANDLE hDriver, OSWLogCallback callback);

// ---------------------------------------------------------------------------
// VISA 枚举
// ---------------------------------------------------------------------------

OSW_C_API int WINAPI OSW_EnumerateVisaResources(char* buffer, int bufferSize);
