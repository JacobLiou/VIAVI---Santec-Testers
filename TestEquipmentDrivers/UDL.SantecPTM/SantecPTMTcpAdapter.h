#pragma once

#include "ISantecPTMCommAdapter.h"

namespace SantecPTM {

class PTM_API CSantecPTMTcpAdapter : public ISantecPTMCommAdapter
{
public:
    CSantecPTMTcpAdapter();
    virtual ~CSantecPTMTcpAdapter();

    virtual bool Open(const std::string& address, int port, double timeout) override;
    virtual void Close() override;
    virtual bool IsOpen() const override;
    virtual std::string SendQuery(const std::string& command) override;
    virtual void SendWrite(const std::string& command) override;

    void SetReadTimeout(DWORD timeoutMs);

private:
    SOCKET m_socket;
    int    m_bufferSize;
};

} // namespace SantecPTM
