#pragma once

#include "IOSXCommAdapter.h"
#include "OSXTypes.h"
#include "../Common/VisaHelper.h"

namespace OSXSwitch {

// ---------------------------------------------------------------------------
// COSXVisaAdapter -- 通过 VISA (USBTMC) 与 OSX 光开关通信
//
// 使用动态加载的 VISA 运行时（R&S VISA / NI VISA / Keysight VISA）。
// 地址为 VISA 资源字符串，例如:
//   "USB0::0x1698::0x0337::SN12345::INSTR"
// ---------------------------------------------------------------------------
class OSX_API COSXVisaAdapter : public IOSXCommAdapter
{
public:
    COSXVisaAdapter();
    virtual ~COSXVisaAdapter();

    virtual bool Open(const std::string& address, int port, double timeout) override;
    virtual void Close() override;
    virtual bool IsOpen() const override;
    virtual std::string SendQuery(const std::string& command) override;
    virtual void SendWrite(const std::string& command) override;

    bool IsVisaAvailable() const { return m_visa.IsLoaded(); }

private:
    VisaHelper::CVisaLoader  m_visa;
    VisaHelper::ViSession    m_defaultRM;
    VisaHelper::ViSession    m_session;
};

} // namespace OSXSwitch
