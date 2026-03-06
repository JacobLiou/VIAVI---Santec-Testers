#pragma once

#include "stdafx.h"

// ---------------------------------------------------------------------------
// Viavi MAP PCT (mORL-A1) 模拟器
//
// TCP 服务器（默认端口 8301），响应 SCPI 命令，
// 遵循 MAP-PCT Programming Guide 22112369-346 协议。
//
// 模拟功能：
//   - 远程控制 (*REM)
//   - 设备标识 (*IDN?)
//   - 模块信息 (:INFO?, :CONF?)
//   - 光源/波长配置 (:SOUR:WAV, :SOUR:LIST, :SOUR:WAV:AVA?)
//   - 传感器/模式 (:SENS:FUNC, :SENS:AVG, :SENS:RANG, :SENS:ILON, :SENS:CONN, :SENS:BIDI)
//   - 路径/通道 (:PATH:CHAN)
//   - 测量控制 (:MEAS:STAR, :MEAS:STAT?, :MEAS:ABOR)
//   - 测量结果 (:MEAS:ALL?, :MEAS:IL?, :MEAS:ORL?, :MEAS:LEN?)
//   - 忙碌状态 (:BUSY?)
//   - 错误查询 (:SYST:ERR?)
// ---------------------------------------------------------------------------

class CViaviPCTSimServer
{
public:
    CViaviPCTSimServer();
    ~CViaviPCTSimServer();

    bool Start(int port = 8301);
    void Stop();
    bool IsRunning() const { return m_running; }

    void SetErrorMode(bool enabled)     { m_errorMode = enabled; }
    bool GetErrorMode() const           { return m_errorMode; }
    void SetMeasDelayMs(int ms)         { m_measDelayMs = ms; }
    int  GetMeasDelayMs() const         { return m_measDelayMs; }
    void SetVerbose(bool enabled)       { m_verbose = enabled; }
    bool GetVerbose() const             { return m_verbose; }
    int  GetCommandCount() const        { return m_commandCount; }

private:
    static DWORD WINAPI ServerThreadProc(LPVOID param);
    void RunServer();
    void HandleClient(SOCKET clientSocket);
    std::string ProcessCommand(const std::string& cmd);

    // 模拟数据生成
    double GenerateIL(int channel, double wavelength);
    double GenerateORL(int channel, double wavelength);
    double GenerateLength();
    double GeneratePower(double wavelength);

    void Log(const char* fmt, ...);

    // 服务器状态
    bool            m_running;
    int             m_port;
    SOCKET          m_listenSocket;
    HANDLE          m_serverThread;

    // 远程控制
    bool            m_remoteMode;

    // 模块配置
    int             m_deviceNum;
    std::string     m_deviceType;
    std::string     m_fiberType;
    int             m_outputs;
    int             m_sw1Channels;
    int             m_sw2Channels;
    std::vector<int> m_availableWavelengths;
    std::vector<int> m_configuredWavelengths;

    // 传感器配置
    int             m_measFunc;         // 0=参考, 1=DUT
    int             m_avgTime;          // 平均时间（秒）
    int             m_dutRange;         // DUT 范围（米）
    bool            m_ilOnly;           // IL-only 模式
    int             m_connMode;         // 连接模式（1跳线/2跳线）
    bool            m_bidi;             // 双向模式

    // 路径/通道
    int             m_group;
    int             m_channel;

    // 测量状态
    bool            m_measBusy;
    DWORD           m_measStartTime;
    int             m_measDelayMs;
    bool            m_referenced;

    // 控制
    bool            m_errorMode;
    bool            m_verbose;
    int             m_commandCount;

    // 错误队列
    std::vector<std::string> m_errorQueue;

    std::mutex      m_mutex;
};
