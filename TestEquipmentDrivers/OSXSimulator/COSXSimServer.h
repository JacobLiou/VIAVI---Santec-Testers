#pragma once

#include "stdafx.h"

// ---------------------------------------------------------------------------
// OSX 光开关模拟器
//
// TCP 服务器（默认端口 5025），响应官方 SCPI 命令，
// 遵循 OSX 协议，参照 OSX 用户手册 M-OSX_SX1-001.04。
//
// 模拟功能：
//   - 标准 SCPI (表 4): *IDN?, *RST, *CLS, *OPC?, 状态, 系统
//   - OSX 开关命令 (表 5): CLOSe, 模块, 路由, LCL, 通知
//   - 异步操作模型: STAT:OPER:COND? 在切换期间返回忙碌状态
//   - 多模块支持，可配置不同配置
// ---------------------------------------------------------------------------

class COSXSimServer
{
public:
    struct SimModule
    {
        std::string name;           // 例如 "SX 1Ax24"
        int  configType;            // 0=1A, 1=2A, 2=2B, 3=2C
        int  channelCount;
        int  currentChannel;
        int  currentCommon;         // 用于 2A/2C 配置

        SimModule()
            : configType(0), channelCount(24)
            , currentChannel(1), currentCommon(0)
        {}
    };

    COSXSimServer();
    ~COSXSimServer();

    bool Start(int port = 5025);
    void Stop();
    bool IsRunning() const { return m_running; }

    // 配置
    void SetSwitchDelayMs(int ms)    { m_switchDelayMs = ms; }
    int  GetSwitchDelayMs() const    { return m_switchDelayMs; }
    void SetVerbose(bool enabled)    { m_verbose = enabled; }
    bool GetVerbose() const          { return m_verbose; }
    void SetErrorMode(bool enabled)  { m_errorMode = enabled; }
    bool GetErrorMode() const        { return m_errorMode; }
    int  GetCommandCount() const     { return m_commandCount; }
    int  GetModuleCount() const      { return static_cast<int>(m_modules.size()); }

    // 模块管理
    void AddModule(const std::string& name, int configType, int channelCount);
    void ClearModules();
    const std::vector<SimModule>& GetModules() const { return m_modules; }

private:
    static DWORD WINAPI ServerThreadProc(LPVOID param);
    void RunServer();
    void HandleClient(SOCKET clientSocket);
    std::string ProcessCommand(const std::string& cmd);

    void Log(const char* fmt, ...);

    // 异步操作跟踪
    void BeginOperation();
    bool IsOperationBusy() const;

    // 服务器状态
    bool            m_running;
    int             m_port;
    SOCKET          m_listenSocket;
    HANDLE          m_serverThread;

    // 设备状态
    std::vector<SimModule> m_modules;
    int             m_selectedModule;
    bool            m_localMode;
    int             m_switchDelayMs;

    // 模拟网络设置
    std::string     m_ipAddress;
    std::string     m_gateway;
    std::string     m_netmask;
    std::string     m_hostname;
    std::string     m_macAddress;

    // 异步操作计时
    DWORD           m_operationStartTick;
    int             m_operationDurationMs;

    // 控制
    bool            m_errorMode;
    bool            m_verbose;
    int             m_commandCount;

    // 错误队列
    std::vector<std::string> m_errorQueue;

    // 状态寄存器
    int             m_eseRegister;
    int             m_sreRegister;

    std::mutex      m_mutex;
};
