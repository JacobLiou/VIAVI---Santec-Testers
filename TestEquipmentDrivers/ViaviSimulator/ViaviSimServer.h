#pragma once

#include "stdafx.h"

// ---------------------------------------------------------------------------
// VIAVI MAP300 PCT 模拟器
//
// 模拟两个TCP服务器：
//   机箱 (端口 8100) - 接受 SUPER:LAUNCH 命令
//   PCT 模块 (端口 8301) - 处理 SCPI 测量命令
//
// 跟踪完整的协议状态：波长、通道、测量生命周期。
// 生成逼真的 IL/RL 测量数据，用于驱动程序测试。
// ---------------------------------------------------------------------------

class CViaviSimServer
{
public:
    CViaviSimServer();
    ~CViaviSimServer();

    bool Start(int chassisPort = 8100, int pctPort = 8301);
    void Stop();
    bool IsRunning() const { return m_running; }

    // 运行时控制
    void SetErrorMode(bool enabled)    { m_errorMode = enabled; }
    bool GetErrorMode() const           { return m_errorMode; }
    void SetMeasurementDelay(int ms)    { m_measDelayMs = ms; }
    int  GetMeasurementDelay() const    { return m_measDelayMs; }
    void SetVerbose(bool enabled)       { m_verbose = enabled; }
    bool GetVerbose() const             { return m_verbose; }

    // 统计信息
    int  GetCommandCount() const        { return m_commandCount; }

private:
    // 服务器线程入口点
    static DWORD WINAPI ChassisThreadProc(LPVOID param);
    static DWORD WINAPI PctThreadProc(LPVOID param);

    void RunChassisServer();
    void RunPctServer();
    void HandleClient(SOCKET clientSocket, const char* serverTag);

    // 协议处理
    std::string ProcessChassisCommand(const std::string& cmd);
    std::string ProcessPctCommand(const std::string& cmd);
    std::string GenerateMeasResults(int channel, int launchPort);

    // 日志
    void Log(const char* tag, const char* fmt, ...);
    void LogVerbose(const char* tag, const char* fmt, ...);

    // 套接字
    SOCKET m_chassisListen;
    SOCKET m_pctListen;
    int    m_chassisPort;
    int    m_pctPort;

    // 线程
    HANDLE m_hChassisThread;
    HANDLE m_hPctThread;

    // 运行时标志
    std::atomic<bool> m_running;
    std::atomic<bool> m_errorMode;
    std::atomic<bool> m_verbose;
    std::atomic<int>  m_measDelayMs;
    std::atomic<int>  m_commandCount;

    // PCT 协议状态（由 m_stateMutex 保护）
    std::mutex          m_stateMutex;
    std::vector<double> m_wavelengths;
    int                 m_launchPort;
    int                 m_sensFunc;         // 0 = 参考, 1 = 测量
    bool                m_measRunning;
    DWORD               m_measStartTick;

    // 控制台输出锁
    std::mutex m_logMutex;
};
