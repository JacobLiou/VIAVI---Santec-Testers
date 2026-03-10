#pragma once

// ---------------------------------------------------------------------------
// C 风格 DLL 导出 API，用于外部系统集成
//
// 所有函数使用 HANDLE 来封装 C++ 驱动对象。
// 前缀: PCT_ 用于区分不同驱动模块
// ---------------------------------------------------------------------------

#include "PCTTypes.h"

#ifdef UDLVIAVIPCT_EXPORTS
#define PCT_C_API extern "C" __declspec(dllexport)
#else
#define PCT_C_API extern "C" __declspec(dllimport)
#endif

// ---------------------------------------------------------------------------
// 驱动生命周期
// ---------------------------------------------------------------------------

// 创建驱动实例 (TCP 模式)
// type: "viavi", "pct", "morl"
// ip: MAP 系统 IP 地址
// port: PCT 模块 TCP 端口（0 = 使用默认端口 8301）
// slot: 保留参数
PCT_C_API HANDLE WINAPI PCT_CreateDriver(const char* type, const char* ip, int port, int slot);

// 创建驱动实例（扩展版本，支持指定通信类型）
// commType: 0=TCP, 1=GPIB, 2=VISA
// address: TCP 模式为 IP 地址，VISA 模式为 VISA 资源字符串
PCT_C_API HANDLE WINAPI PCT_CreateDriverEx(const char* type, const char* address,
                                            int port, int slot, int commType);

// 销毁驱动实例并释放资源
PCT_C_API void WINAPI PCT_DestroyDriver(HANDLE hDriver);

// ---------------------------------------------------------------------------
// 连接
// ---------------------------------------------------------------------------

PCT_C_API BOOL WINAPI PCT_Connect(HANDLE hDriver);
PCT_C_API void WINAPI PCT_Disconnect(HANDLE hDriver);
PCT_C_API BOOL WINAPI PCT_Initialize(HANDLE hDriver);
PCT_C_API BOOL WINAPI PCT_IsConnected(HANDLE hDriver);

// ---------------------------------------------------------------------------
// 配置
// ---------------------------------------------------------------------------

// 配置测量波长（双精度浮点数组，单位 nm）
PCT_C_API BOOL WINAPI PCT_ConfigureWavelengths(HANDLE hDriver, double* wavelengths, int count);

// 配置测量通道（整数数组）
PCT_C_API BOOL WINAPI PCT_ConfigureChannels(HANDLE hDriver, int* channels, int count);

// 设置测量模式: 0=参考, 1=DUT
PCT_C_API BOOL WINAPI PCT_SetMeasurementMode(HANDLE hDriver, int mode);

// 设置平均时间（秒）
PCT_C_API BOOL WINAPI PCT_SetAveragingTime(HANDLE hDriver, int seconds);

// 设置 DUT 范围（米）
PCT_C_API BOOL WINAPI PCT_SetDUTRange(HANDLE hDriver, int rangeMeters);

// ---------------------------------------------------------------------------
// 测量
// ---------------------------------------------------------------------------

// 执行参考测量
PCT_C_API BOOL WINAPI PCT_TakeReference(HANDLE hDriver, BOOL bOverride,
                                         double ilValue, double lengthValue);

// 执行 DUT 测量
PCT_C_API BOOL WINAPI PCT_TakeMeasurement(HANDLE hDriver);

// 中止正在进行的测量
PCT_C_API void WINAPI PCT_AbortMeasurement(HANDLE hDriver);

// 获取测量结果
PCT_C_API int WINAPI PCT_GetResults(HANDLE hDriver,
                                     ViaviPCT::CMeasurementResult* results,
                                     int maxCount);

// ---------------------------------------------------------------------------
// 设备信息
// ---------------------------------------------------------------------------

PCT_C_API BOOL WINAPI PCT_GetDeviceInfo(HANDLE hDriver, ViaviPCT::CDeviceInfo* info);

// 检查最近的错误
PCT_C_API int WINAPI PCT_CheckError(HANDLE hDriver, char* message, int messageSize);

// ---------------------------------------------------------------------------
// 原始命令
// ---------------------------------------------------------------------------

// 发送原始 SCPI 命令
PCT_C_API BOOL WINAPI PCT_SendCommand(HANDLE hDriver, const char* command,
                                       char* response, int responseSize);

// ---------------------------------------------------------------------------
// 日志
// ---------------------------------------------------------------------------

typedef void (WINAPI *PCTLogCallback)(int level, const char* source, const char* message);
PCT_C_API void WINAPI PCT_SetLogCallback(PCTLogCallback callback);

// ---------------------------------------------------------------------------
// VISA 枚举
// ---------------------------------------------------------------------------

// 枚举可用的 VISA 资源
PCT_C_API int WINAPI PCT_EnumerateVisaResources(char* buffer, int bufferSize);
