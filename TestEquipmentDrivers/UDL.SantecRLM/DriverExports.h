#pragma once

// ---------------------------------------------------------------------------
// C 风格 DLL 导出 API，用于外部系统集成
// （MIMS / Mlight / MAS 或其他通过 LoadLibrary/GetProcAddress 调用的程序）
//
// 所有函数使用 HANDLE 来封装 C++ 驱动对象。
// ---------------------------------------------------------------------------

#include "DriverTypes.h"

#ifdef UDLSANTECRLM_EXPORTS
#define DRIVER_C_API extern "C" __declspec(dllexport)
#else
#define DRIVER_C_API extern "C" __declspec(dllimport)
#endif

// 创建驱动实例。返回 HANDLE（不透明指针）。
// type: "santec" 或 "rlm"
// ip: 设备 IP 地址（例如 "10.14.132.194"）
// port: TCP 端口（0 = 使用默认端口）
// slot: 保留参数（当前忽略）
DRIVER_C_API HANDLE WINAPI CreateDriver(const char* type, const char* ip, int port, int slot);

// 销毁驱动实例并释放资源
DRIVER_C_API void WINAPI DestroyDriver(HANDLE hDriver);

// 连接到设备
DRIVER_C_API BOOL WINAPI DriverConnect(HANDLE hDriver);

// 断开与设备的连接
DRIVER_C_API void WINAPI DriverDisconnect(HANDLE hDriver);

// 连接后初始化设备
DRIVER_C_API BOOL WINAPI DriverInitialize(HANDLE hDriver);

// 检查是否已连接
DRIVER_C_API BOOL WINAPI DriverIsConnected(HANDLE hDriver);

// 配置测量波长（双精度浮点数组，单位 nm）
DRIVER_C_API BOOL WINAPI DriverConfigureWavelengths(HANDLE hDriver, double* wavelengths, int count);

// 配置测量通道（整数数组）
DRIVER_C_API BOOL WINAPI DriverConfigureChannels(HANDLE hDriver, int* channels, int count);

// 执行参考测量
// bOverride: TRUE 使用覆盖值，FALSE 使用自动测量
DRIVER_C_API BOOL WINAPI DriverTakeReference(HANDLE hDriver, BOOL bOverride,
                                             double ilValue, double lengthValue);

// 执行插入损耗/回波损耗测量
DRIVER_C_API BOOL WINAPI DriverTakeMeasurement(HANDLE hDriver);

// 将结果写入调用者预分配的数组。返回写入的结果数量。
// results: 预分配的 CMeasurementResult 数组
// maxCount: 数组大小
DRIVER_C_API int WINAPI DriverGetResults(HANDLE hDriver,
                                         SantecRLM::CMeasurementResult* results,
                                         int maxCount);

// 获取设备信息
DRIVER_C_API BOOL WINAPI DriverGetDeviceInfo(HANDLE hDriver, SantecRLM::CDeviceInfo* info);

// 检查最近的错误。返回错误代码（0 = 无错误）。
// message: 接收错误消息的缓冲区
// messageSize: 缓冲区大小
DRIVER_C_API int WINAPI DriverCheckError(HANDLE hDriver, char* message, int messageSize);

// 发送原始 SCPI 命令并接收响应
// response: 接收响应的缓冲区（仅写命令时可为 NULL）
// responseSize: 缓冲区大小
DRIVER_C_API BOOL WINAPI DriverSendCommand(HANDLE hDriver, const char* command,
                                           char* response, int responseSize);

// 设置日志回调（接收来自驱动的日志消息）
typedef void (WINAPI *DriverLogCallback)(int level, const char* source, const char* message);
DRIVER_C_API void WINAPI DriverSetLogCallback(DriverLogCallback callback);

// ---------------------------------------------------------------------------
// Santec RL1 特定配置
// ---------------------------------------------------------------------------

// 设置回波损耗灵敏度: 0=快速 (<1.5秒, RL<=75dB), 1=标准 (<5秒, RL<=85dB)
DRIVER_C_API BOOL WINAPI DriverSantecSetRLSensitivity(HANDLE hDriver, int sensitivity);

// 设置被测器件长度档位: 100、1500 或 4000（米）
DRIVER_C_API BOOL WINAPI DriverSantecSetDUTLength(HANDLE hDriver, int lengthBin);

// 设置回波损耗增益模式: 0=正常 (40-85dB), 1=低 (30-40dB)
DRIVER_C_API BOOL WINAPI DriverSantecSetRLGain(HANDLE hDriver, int gain);

// 设置本地模式: TRUE=启用（触摸屏可用），FALSE=禁用（仅远程控制）
DRIVER_C_API BOOL WINAPI DriverSantecSetLocalMode(HANDLE hDriver, BOOL enabled);

// ---------------------------------------------------------------------------
// 探测器选择
// ---------------------------------------------------------------------------

// 设置当前活动探测器编号（1=内置前面板探测器，2=外部遥控探头）
DRIVER_C_API BOOL WINAPI DriverSetDetector(HANDLE hDriver, int detectorNum);

// 获取已连接的探测器数量
DRIVER_C_API int WINAPI DriverGetDetectorCount(HANDLE hDriver);

// 获取指定探测器的信息字符串
// buffer: 接收信息的缓冲区
// bufferSize: 缓冲区大小
DRIVER_C_API BOOL WINAPI DriverGetDetectorInfo(HANDLE hDriver, int detectorNum,
                                               char* buffer, int bufferSize);

// ---------------------------------------------------------------------------
// 外部开关控制（集成模式：通过 RLM USB A 端口控制连接的 OSX 光开关）
// ---------------------------------------------------------------------------

// 设置外部开关通道（发送 SW#:CLOSe # 命令）
// switchNum: 外部开关编号（1=SW1, 2=SW2; 0=内部开关）
// channel: 目标通道号
DRIVER_C_API BOOL WINAPI DriverSetSwitchChannel(HANDLE hDriver, int switchNum, int channel);

// 查询外部开关当前通道（发送 SW#:CLOSe? 命令）
DRIVER_C_API int WINAPI DriverGetSwitchChannel(HANDLE hDriver, int switchNum);

// 查询外部开关信息（发送 SW#:INFO? 命令）
// buffer: 接收信息的缓冲区
// bufferSize: 缓冲区大小
DRIVER_C_API BOOL WINAPI DriverGetSwitchInfo(HANDLE hDriver, int switchNum,
                                             char* buffer, int bufferSize);

// ---------------------------------------------------------------------------
// VISA / USB 扩展
// ---------------------------------------------------------------------------

// 创建驱动实例（扩展版本，支持指定通信类型）
// commType: 0=TCP, 1=GPIB, 2=USB(VISA)
// address: TCP 模式为 IP 地址，USB 模式为 VISA 资源字符串
DRIVER_C_API HANDLE WINAPI CreateDriverEx(const char* type, const char* address,
                                          int port, int slot, int commType);

// 枚举可用的 VISA 资源（USB 设备）
// buffer: 接收以分号分隔的资源字符串列表
// bufferSize: 缓冲区大小
// 返回值: 找到的资源数量
DRIVER_C_API int WINAPI EnumerateVisaResources(char* buffer, int bufferSize);
