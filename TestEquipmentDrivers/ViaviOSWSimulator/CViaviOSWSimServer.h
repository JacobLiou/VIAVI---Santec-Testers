#pragma once

#include "stdafx.h"

// ---------------------------------------------------------------------------
// Viavi MAP mOSW-C1 光开关模拟器
//
// TCP 服务器（默认端口 8203），响应 SCPI 命令，
// 遵循 MAP 通用模块命令和标准 SCPI 路由命令。
//
// 模拟功能：
//   - 远程控制 (*REM)
//   - 设备标识 (*IDN?)
//   - 模块信息 (:INFO?, :DEV:INFO?, :CONF?)
//   - 通道路由 (:PROC:ROUT, :PROC:ROUT?)
//   - 忙碌状态 (:BUSY?)
//   - 复位 (:RES)
//   - 错误查询 (:SYST:ERR?)
// ---------------------------------------------------------------------------

class CViaviOSWSimServer
{
public:
    CViaviOSWSimServer();
    ~CViaviOSWSimServer();

    bool Start(int port = 8203);
    void Stop();
    bool IsRunning() const { return m_running; }

    void SetErrorMode(bool enabled)     { m_errorMode = enabled; }
    bool GetErrorMode() const           { return m_errorMode; }
    void SetSwitchDelayMs(int ms)       { m_switchDelayMs = ms; }
    int  GetSwitchDelayMs() const       { return m_switchDelayMs; }
    void SetVerbose(bool enabled)       { m_verbose = enabled; }
    bool GetVerbose() const             { return m_verbose; }
    int  GetCommandCount() const        { return m_commandCount; }

private:
    static DWORD WINAPI ServerThreadProc(LPVOID param);
    void RunServer();
    void HandleClient(SOCKET clientSocket);
    std::string ProcessCommand(const std::string& cmd);

    void Log(const char* fmt, ...);

    // 服务器状态
    bool            m_running;
    int             m_port;
    SOCKET          m_listenSocket;
    HANDLE          m_serverThread;

    // 远程控制
    bool            m_remoteMode;

    // 设备配置
    struct DeviceState
    {
        int channelCount;
        int currentChannel;
        DeviceState() : channelCount(8), currentChannel(1) {}
    };
    int             m_deviceCount;
    std::map<int, DeviceState> m_devices;

    // 开关状态
    bool            m_busy;
    DWORD           m_switchStartTime;
    int             m_switchDelayMs;

    // 控制
    bool            m_errorMode;
    bool            m_verbose;
    int             m_commandCount;

    // 错误队列
    std::vector<std::string> m_errorQueue;

    std::mutex      m_mutex;
};
