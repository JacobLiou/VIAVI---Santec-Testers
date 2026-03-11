#pragma once

#include "IViaviPCT4AllCommAdapter.h"
#include "PCT4AllTypes.h"

namespace ViaviPCT4All {

class PCT4ALL_API CViaviPCT4AllTcpAdapter : public IViaviPCT4AllCommAdapter
{
public:
    CViaviPCT4AllTcpAdapter();
    virtual ~CViaviPCT4AllTcpAdapter();

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

} // namespace ViaviPCT4All
