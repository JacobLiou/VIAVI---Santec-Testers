#pragma once

#include "IViaviPCT4AllCommAdapter.h"
#include "PCT4AllTypes.h"
#include "../Common/VisaHelper.h"

namespace ViaviPCT4All {

class PCT4ALL_API CViaviPCT4AllVisaAdapter : public IViaviPCT4AllCommAdapter
{
public:
    CViaviPCT4AllVisaAdapter();
    virtual ~CViaviPCT4AllVisaAdapter();

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

} // namespace ViaviPCT4All
