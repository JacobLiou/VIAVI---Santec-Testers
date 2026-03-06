#include "stdafx.h"
#include "ViaviPCTVisaAdapter.h"
#include <stdexcept>

using namespace VisaHelper;

namespace ViaviPCT {

CViaviPCTVisaAdapter::CViaviPCTVisaAdapter()
    : m_defaultRM(VI_NULL)
    , m_session(VI_NULL)
    , m_bufferSize(4096)
{
}

CViaviPCTVisaAdapter::~CViaviPCTVisaAdapter()
{
    Close();
}

bool CViaviPCTVisaAdapter::Open(const std::string& address, int port, double timeout)
{
    (void)port;

    if (!m_visa.LoadVisa())
        throw std::runtime_error(
            "无法加载 VISA 运行时库。请安装 R&S VISA、NI VISA 或 Keysight VISA。");

    ViStatus status = m_visa.pfnOpenDefaultRM(&m_defaultRM);
    if (status != VI_SUCCESS)
    {
        std::string desc = m_visa.GetStatusDescription(VI_NULL, status);
        throw std::runtime_error("viOpenDefaultRM 失败: " + desc);
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
        throw std::runtime_error("viOpen 失败 (" + address + "): " + desc);
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

void CViaviPCTVisaAdapter::Close()
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

bool CViaviPCTVisaAdapter::IsOpen() const
{
    return m_session != VI_NULL;
}

std::string CViaviPCTVisaAdapter::SendQuery(const std::string& command)
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
        throw std::runtime_error("viRead 失败: " + desc);
    }

    buf[retCount] = '\0';
    std::string data(buf, retCount);

    while (!data.empty() && (data.back() == '\n' || data.back() == '\r'))
        data.pop_back();

    return data;
}

void CViaviPCTVisaAdapter::SendWrite(const std::string& command)
{
    if (m_session == VI_NULL)
        throw std::runtime_error("Viavi PCT VISA adapter 未连接。");

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
        throw std::runtime_error("viWrite 失败: " + desc);
    }
}

void CViaviPCTVisaAdapter::SetReadTimeout(DWORD timeoutMs)
{
    if (m_session != VI_NULL && m_visa.pfnSetAttribute)
    {
        m_visa.pfnSetAttribute(m_session, VI_ATTR_TMO_VALUE,
                                static_cast<ViAttrState>(timeoutMs));
    }
}

} // namespace ViaviPCT
