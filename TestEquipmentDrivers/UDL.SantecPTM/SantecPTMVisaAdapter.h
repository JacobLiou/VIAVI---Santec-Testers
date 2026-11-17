#pragma once

#include "ISantecPTMCommAdapter.h"
#include "../Common/VisaHelper.h"

namespace SantecPTM {

class PTM_API CSantecPTMVisaAdapter : public ISantecPTMCommAdapter
{
public:
    CSantecPTMVisaAdapter();
    virtual ~CSantecPTMVisaAdapter();

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

} // namespace SantecPTM
