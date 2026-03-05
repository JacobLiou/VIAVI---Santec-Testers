#pragma once

#include "stdafx.h"

// ---------------------------------------------------------------------------
// Santec RL1 模拟器
//
// TCP 服务器（默认端口 5025），响应官方 SCPI 命令，
// 遵循 Santec RL1 协议，参照 RLM 用户手册 M-RL1-001-07。
//
// 模拟功能：
//   - 设备标识 (*IDN?)
//   - 激光器控制 (LAS:ENAB, LAS:DISAB, LAS:INFO?)
//   - 光纤信息 (FIBER:INFO?)
//   - 同步 IL/RL 测量 (READ:IL:det#?, READ:RL?)
//   - 参考 (REF:RL, REF:IL:det#)
//   - 配置 (DUT:LENGTH, RL:SENS, RL:GAIN, RL:POSB, OUT:CLOS, SW#:CLOS)
//   - 控制 (LCL, AUTO:ENAB)
//   - 错误查询 (SYST:ERR?)
//   - 功率计 (POW:NUM?, READ:POW:det#?, READ:POW:MON?)
// ---------------------------------------------------------------------------

class CSantecSimServer
{
public:
    enum SimModel { SIM_RL1, SIM_ILM_100 };

    CSantecSimServer();
    ~CSantecSimServer();

    bool Start(int port = 5025, SimModel model = SIM_RL1);
    void Stop();
    bool IsRunning() const { return m_running; }

    void SetErrorMode(bool enabled)     { m_errorMode = enabled; }
    bool GetErrorMode() const           { return m_errorMode; }
    void SetMeasDelayMs(int ms)         { m_measDelayMs = ms; }
    int  GetMeasDelayMs() const         { return m_measDelayMs; }
    void SetVerbose(bool enabled)       { m_verbose = enabled; }
    bool GetVerbose() const             { return m_verbose; }
    void SetModel(SimModel m)           { m_model = m; }
    SimModel GetModel() const           { return m_model; }
    int  GetCommandCount() const        { return m_commandCount; }

private:
    static DWORD WINAPI ServerThreadProc(LPVOID param);
    void RunServer();
    void HandleClient(SOCKET clientSocket);
    std::string ProcessCommand(const std::string& cmd);

    // 数据生成
    double GenerateIL(int channel, double wavelength);
    double GenerateRL(int channel, double wavelength);
    double GenerateRLTotal(int channel, double wavelength);
    double GenerateLength();

    void Log(const char* fmt, ...);

    // 服务器状态
    bool            m_running;
    int             m_port;
    SOCKET          m_listenSocket;
    HANDLE          m_serverThread;
    SimModel        m_model;

    // 设备状态（镜像驱动程序发送的内容）
    int             m_enabledLaser;         // 0=禁用，或波长（nm）
    std::string     m_fiberType;            // "SM" 或 "MM"
    std::vector<int> m_supportedWavelengths;
    int             m_dutLengthBin;         // 100、1500 或 4000
    std::string     m_rlSensitivity;        // "fast" 或 "standard"
    std::string     m_rlGain;               // "normal" 或 "low"
    std::string     m_rlPosB;               // "eof" 或 "zero"
    int             m_outputChannel;
    int             m_sw1Channel;
    int             m_sw2Channel;
    bool            m_localMode;
    bool            m_autoStart;
    double          m_dutIL;
    int             m_measDelayMs;

    // 参考状态
    bool            m_rlReferenced;
    double          m_refMTJ1Length;
    double          m_refMTJ2Length;
    struct ILRef { int wavelength; double value; };
    std::vector<ILRef> m_ilRefs;

    // 控制
    bool            m_errorMode;
    bool            m_verbose;
    int             m_commandCount;

    // 错误队列
    std::vector<std::string> m_errorQueue;

    std::mutex      m_mutex;
};
