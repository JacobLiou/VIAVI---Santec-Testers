#pragma once

#include "IViaviPCTCommAdapter.h"
#include "PCTTypes.h"
#include "../Common/VisaHelper.h"

namespace ViaviPCT {

// ---------------------------------------------------------------------------
// CViaviPCTVisaAdapter -- 通过 VISA 与 Viavi MAP PCT 模块通信
//
// 使用动态加载的 VISA 运行时（R&S VISA / NI VISA / Keysight VISA）。
// 地址为 VISA 资源字符串，例如:
//   "GPIB0::13::INSTR"                       (GPIB)
//   "TCPIP::10.14.140.32::8301::SOCKET"      (VXI-11 / TCP)
// ---------------------------------------------------------------------------
class PCT_API CViaviPCTVisaAdapter : public IViaviPCTCommAdapter
{
public:
    CViaviPCTVisaAdapter();
    virtual ~CViaviPCTVisaAdapter();

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

} // namespace ViaviPCT
