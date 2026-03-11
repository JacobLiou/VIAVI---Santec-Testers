#include "stdafx.h"
#include "ViaviPCT4AllVisaAdapter.h"
#include <stdexcept>

using namespace VisaHelper;

namespace ViaviPCT4All {

CViaviPCT4AllVisaAdapter::CViaviPCT4AllVisaAdapter()
    : m_defaultRM(VI_NULL)
    , m_session(VI_NULL)
    , m_bufferSize(4096)
{
}

CViaviPCT4AllVisaAdapter::~CViaviPCT4AllVisaAdapter()
{
    Close();
}

bool CViaviPCT4AllVisaAdapter::Open(const std::string& address, int port, double timeout)
{
    (void)port;

    if (!m_visa.LoadVisa())
        throw std::runtime_error(
            "Cannot load VISA runtime. Please install R&S VISA, NI VISA, or Keysight VISA.");

    ViStatus status = m_visa.pfnOpenDefaultRM(&m_defaultRM);
    if (status != VI_SUCCESS)
    {
        std::string desc = m_visa.GetStatusDescription(VI_NULL, status);
        throw std::runtime_error("viOpenDefaultRM failed: " + desc);
    }

    ViUInt32 openTimeout = static_cast<ViUInt32>(timeout * 1000);

    status = m_visa.pfnOpen(m_defaultRM,
                             const_cast<ViRsrc>(address.c_str()),
                             VI_NO_LOCK,
                             openTimeout,
                             &m_session);
    if (status != VI_SUCCESS)
    {
        std::string desc = m_visa.GetStatusDescription(m_defaultRM, status);
        m_visa.pfnClose(m_defaultRM);
        m_defaultRM = VI_NULL;
        throw std::runtime_error("viOpen failed (" + address + "): " + desc);
    }

    ViUInt32 tmoMs = static_cast<ViUInt32>(timeout * 1000);
    if (m_visa.pfnSetAttribute)
    {
        m_visa.pfnSetAttribute(m_session, VI_ATTR_TMO_VALUE, tmoMs);
        m_visa.pfnSetAttribute(m_session, VI_ATTR_SEND_END_EN, 1);
        m_visa.pfnSetAttribute(m_session, VI_ATTR_TERMCHAR_EN, 1);
        m_visa.pfnSetAttribute(m_session, VI_ATTR_TERMCHAR, '\n');
    }

    if (m_visa.pfnClear)
        m_visa.pfnClear(m_session);

    return true;
}

void CViaviPCT4AllVisaAdapter::Close()
{
    if (m_session != VI_NULL && m_visa.IsLoaded())
    {
        m_visa.pfnClose(m_session);
        m_session = VI_NULL;
    }
    if (m_defaultRM != VI_NULL && m_visa.IsLoaded())
    {
        m_visa.pfnClose(m_defaultRM);
        m_defaultRM = VI_NULL;
    }
}

bool CViaviPCT4AllVisaAdapter::IsOpen() const
{
    return m_session != VI_NULL;
}

std::string CViaviPCT4AllVisaAdapter::SendQuery(const std::string& command)
{
    SendWrite(command);

    char buf[4096];
    ViUInt32 retCount = 0;

    ViStatus status = m_visa.pfnRead(m_session,
                                      reinterpret_cast<ViBuf>(buf),
                                      static_cast<ViUInt32>(sizeof(buf) - 1),
                                      &retCount);

    if (status != VI_SUCCESS && (status & 0x3FFF0000) != 0x3FFF0000)
    {
        std::string desc = m_visa.GetStatusDescription(m_session, status);
        throw std::runtime_error("viRead failed: " + desc);
    }

    buf[retCount] = '\0';
    std::string data(buf, retCount);

    while (!data.empty() && (data.back() == '\n' || data.back() == '\r'))
        data.pop_back();

    return data;
}

void CViaviPCT4AllVisaAdapter::SendWrite(const std::string& command)
{
    if (m_session == VI_NULL)
        throw std::runtime_error("Viavi PCT4All VISA adapter not connected.");

    std::string fullCmd = command + "\n";
    ViUInt32 retCount = 0;

    ViStatus status = m_visa.pfnWrite(m_session,
                                       reinterpret_cast<ViBuf>(
                                           const_cast<char*>(fullCmd.c_str())),
                                       static_cast<ViUInt32>(fullCmd.size()),
                                       &retCount);

    if (status != VI_SUCCESS)
    {
        std::string desc = m_visa.GetStatusDescription(m_session, status);
        throw std::runtime_error("viWrite failed: " + desc);
    }
}

void CViaviPCT4AllVisaAdapter::SetReadTimeout(DWORD timeoutMs)
{
    if (m_session != VI_NULL && m_visa.pfnSetAttribute)
    {
        m_visa.pfnSetAttribute(m_session, VI_ATTR_TMO_VALUE,
                                static_cast<ViAttrState>(timeoutMs));
    }
}

} // namespace ViaviPCT4All
