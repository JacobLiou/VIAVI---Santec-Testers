#pragma once

// ---------------------------------------------------------------------------
// C 风格 DLL 导出 API，用于外部系统集成
// （MIMS / Mlight / MAS 或其他通过 LoadLibrary/GetProcAddress 调用的程序）
//
// 所有函数使用 HANDLE 来封装 C++ 驱动对象。
// ---------------------------------------------------------------------------

#include "OSXTypes.h"

#ifdef UDLOSX_EXPORTS
#define OSX_C_API extern "C" __declspec(dllexport)
#else
#define OSX_C_API extern "C" __declspec(dllimport)
#endif

// ---------------------------------------------------------------------------
// 生命周期
// ---------------------------------------------------------------------------

// 创建 OSX 驱动实例。返回 HANDLE（不透明指针）。
OSX_C_API HANDLE WINAPI OSX_CreateDriver(const char* ip, int port);

// 销毁驱动实例并释放资源。
OSX_C_API void WINAPI OSX_DestroyDriver(HANDLE hDriver);

// ---------------------------------------------------------------------------
// 连接
// ---------------------------------------------------------------------------

OSX_C_API BOOL WINAPI OSX_Connect(HANDLE hDriver);
OSX_C_API void WINAPI OSX_Disconnect(HANDLE hDriver);
OSX_C_API BOOL WINAPI OSX_Reconnect(HANDLE hDriver);
OSX_C_API BOOL WINAPI OSX_IsConnected(HANDLE hDriver);

// ---------------------------------------------------------------------------
// 设备识别
// ---------------------------------------------------------------------------

OSX_C_API BOOL WINAPI OSX_GetDeviceInfo(HANDLE hDriver, OSXSwitch::CDeviceInfo* info);
OSX_C_API int  WINAPI OSX_CheckError(HANDLE hDriver, char* message, int messageSize);
OSX_C_API BOOL WINAPI OSX_GetSystemVersion(HANDLE hDriver, char* version, int versionSize);

// ---------------------------------------------------------------------------
// 模块管理
// ---------------------------------------------------------------------------

OSX_C_API int  WINAPI OSX_GetModuleCount(HANDLE hDriver);
OSX_C_API int  WINAPI OSX_GetModuleCatalog(HANDLE hDriver, OSXSwitch::CModuleInfo* modules, int maxCount);
OSX_C_API BOOL WINAPI OSX_GetModuleInfo(HANDLE hDriver, int moduleIndex, OSXSwitch::CModuleInfo* info);
OSX_C_API BOOL WINAPI OSX_SelectModule(HANDLE hDriver, int moduleIndex);
OSX_C_API BOOL WINAPI OSX_SelectNextModule(HANDLE hDriver);
OSX_C_API int  WINAPI OSX_GetSelectedModule(HANDLE hDriver);

// ---------------------------------------------------------------------------
// 通道切换
// ---------------------------------------------------------------------------

OSX_C_API BOOL WINAPI OSX_SwitchChannel(HANDLE hDriver, int channel);
OSX_C_API BOOL WINAPI OSX_SwitchNext(HANDLE hDriver);
OSX_C_API int  WINAPI OSX_GetCurrentChannel(HANDLE hDriver);
OSX_C_API int  WINAPI OSX_GetChannelCount(HANDLE hDriver);

// ---------------------------------------------------------------------------
// 多模块路由
// ---------------------------------------------------------------------------

OSX_C_API BOOL WINAPI OSX_RouteChannel(HANDLE hDriver, int moduleIndex, int channel);
OSX_C_API int  WINAPI OSX_GetRouteChannel(HANDLE hDriver, int moduleIndex);
OSX_C_API BOOL WINAPI OSX_RouteAllModules(HANDLE hDriver, int channel);
OSX_C_API BOOL WINAPI OSX_SetCommonInput(HANDLE hDriver, int moduleIndex, int common);
OSX_C_API int  WINAPI OSX_GetCommonInput(HANDLE hDriver, int moduleIndex);
OSX_C_API BOOL WINAPI OSX_HomeModule(HANDLE hDriver, int moduleIndex);

// ---------------------------------------------------------------------------
// 控制
// ---------------------------------------------------------------------------

OSX_C_API BOOL WINAPI OSX_SetLocalMode(HANDLE hDriver, BOOL local);
OSX_C_API BOOL WINAPI OSX_GetLocalMode(HANDLE hDriver);
OSX_C_API BOOL WINAPI OSX_SendNotification(HANDLE hDriver, int icon, const char* message);
OSX_C_API BOOL WINAPI OSX_Reset(HANDLE hDriver);

// ---------------------------------------------------------------------------
// 网络配置
// ---------------------------------------------------------------------------

OSX_C_API BOOL WINAPI OSX_GetNetworkInfo(HANDLE hDriver, char* ip, char* gateway,
                                         char* netmask, char* hostname, char* mac,
                                         int bufSize);

// ---------------------------------------------------------------------------
// 操作同步
// ---------------------------------------------------------------------------

OSX_C_API BOOL WINAPI OSX_WaitForOperation(HANDLE hDriver, int timeoutMs);

// ---------------------------------------------------------------------------
// 原始 SCPI
// ---------------------------------------------------------------------------

OSX_C_API BOOL WINAPI OSX_SendCommand(HANDLE hDriver, const char* command,
                                      char* response, int responseSize);

// ---------------------------------------------------------------------------
// 日志
// ---------------------------------------------------------------------------

typedef void (WINAPI *OSXLogCallback)(int level, const char* source, const char* message);
OSX_C_API void WINAPI OSX_SetLogCallback(OSXLogCallback callback);

// ---------------------------------------------------------------------------
// VISA / USB 扩展
// ---------------------------------------------------------------------------

// 创建 OSX 驱动实例（扩展版本，支持指定通信类型）
// commType: 0=TCP, 2=USB(VISA)
// address: TCP 模式为 IP 地址，USB 模式为 VISA 资源字符串
OSX_C_API HANDLE WINAPI OSX_CreateDriverEx(const char* address, int port, int commType);

// 枚举可用的 VISA 资源（USB 设备）
// buffer: 接收以分号分隔的资源字符串列表
// bufferSize: 缓冲区大小
// 返回值: 找到的资源数量
OSX_C_API int WINAPI OSX_EnumerateVisaResources(char* buffer, int bufferSize);
