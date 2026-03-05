#pragma once

#include "SantecDriver.h"
#include "../Common/VisaHelper.h"

namespace SantecRLM {

// ---------------------------------------------------------------------------
// CSantecVisaAdapter -- 通过 VISA (USBTMC) 与 Santec 设备通信
//
// 使用动态加载的 VISA 运行时（R&S VISA / NI VISA / Keysight VISA）。
// 地址为 VISA 资源字符串，例如:
//   "USB0::0x1698::0x0368::SN12345::INSTR"
//   "TCPIP::10.14.132.194::5025::SOCKET"
// ---------------------------------------------------------------------------
class DRIVER_API CSantecVisaAdapter : public ISantecCommAdapter
{
public:
    CSantecVisaAdapter();
    virtual ~CSantecVisaAdapter();

    virtual bool Open(const std::string& address, int port, double timeout) override;
    virtual void Close() override;
    virtual bool IsOpen() const override;
    virtual std::string SendQuery(const std::string& command) override;
    virtual void SendWrite(const std::string& command) override;

    void SetReadTimeout(DWORD timeoutMs);

    bool IsVisaAvailable() const { return m_visa.IsLoaded(); }

private:
    VisaHelper::CVisaLoader  m_visa;
    VisaHelper::ViSession    m_defaultRM;
    VisaHelper::ViSession    m_session;
    int                      m_bufferSize;
};

} // namespace SantecRLM
