#include "stdafx.h"
#include "PolaritySession.h"
#include <cstring>

namespace {

void CopyCStr(char* dest, size_t destSize, const char* src)
{
    if (!dest || destSize == 0)
        return;
    dest[0] = '\0';
    if (!src)
        return;
    const size_t n = std::strlen(src);
    const size_t copy = (n < destSize - 1) ? n : (destSize - 1);
    if (copy > 0)
        std::memcpy(dest, src, copy);
    dest[copy] = '\0';
}

void ZeroSnapshot(SP_HARDWARE_SNAPSHOT* s)
{
    std::memset(s, 0, sizeof(*s));
    s->structSize = sizeof(SP_HARDWARE_SNAPSHOT);
    s->version = 1;
    s->rlmOutputCount = 1;
    s->switchNumber = 1;
    s->switchChannelCount = 24;
    s->polarityWavelengthNm = 1310;
    s->remoteDetectorIndex = -1;
    CopyCStr(s->rlmPartNumber, sizeof(s->rlmPartNumber), "RLM-100-01-3500-S-09-FA-R");
    CopyCStr(s->osxPartNumber, sizeof(s->osxPartNumber), "OSX-100-1A-024-09-FA-00B");
    CopyCStr(s->rdspPartNumber, sizeof(s->rdspPartNumber), "RD-SP-100-SM");
}

void ZeroSummary(SP_RESULT_SUMMARY* s)
{
    std::memset(s, 0, sizeof(*s));
    s->structSize = sizeof(SP_RESULT_SUMMARY);
    s->version = 1;
}

} // namespace

CPolaritySession::CPolaritySession()
    : m_opened(false)
{
    ZeroSnapshot(&m_snapshot);
    ZeroSummary(&m_summary);
    SetError("RD-SP Mapping backend not linked; await vendor SDK (SP_E_BACKEND_NOT_SUPPORTED)");
}

CPolaritySession::~CPolaritySession()
{
    Close();
}

void CPolaritySession::SetError(const char* utf8)
{
    m_lastError = utf8 ? utf8 : "";
}

SP_STATUS CPolaritySession::CopyUtf8(const std::string& src, char* buffer, uint32_t capacity, uint32_t* required)
{
    const uint32_t need = static_cast<uint32_t>(src.size() + 1);
    if (required)
        *required = need;
    if (!buffer || capacity == 0)
        return SP_E_INVALID_ARGUMENT;
    if (capacity < need)
        return SP_E_INVALID_ARGUMENT;
    std::memcpy(buffer, src.c_str(), need);
    return SP_OK;
}

SP_STATUS CPolaritySession::Open(const SP_OPEN_OPTIONS* options)
{
    if (!options || options->structSize < sizeof(SP_OPEN_OPTIONS))
        return SP_E_INVALID_ARGUMENT;
    if (m_opened)
        return SP_E_INVALID_STATE;

    // Intentionally do not open RLM/OSX/RD-SP until Gate A/B SDK proof exists.
    // Reuse UDL.SantecRLM / UDL.SantecOSX SCPI patterns after backend selection.
    SetError(
        "SP_Open blocked: RD-SP Mapping SDK/Worker not integrated yet. "
        "See docs/RD-SP-SDK-供应商索取清单.md and Final Plan Gate A/B.");
    return SP_E_BACKEND_NOT_SUPPORTED;
}

SP_STATUS CPolaritySession::Close()
{
    m_opened = false;
    m_fibers.clear();
    m_diagnosticPaths.clear();
    ZeroSummary(&m_summary);
    return SP_OK;
}

SP_STATUS CPolaritySession::GetHardwareSnapshot(SP_HARDWARE_SNAPSHOT* snapshot)
{
    if (!snapshot || snapshot->structSize < sizeof(SP_HARDWARE_SNAPSHOT))
        return SP_E_INVALID_ARGUMENT;
    // Baseline part numbers are known from company BOM even before SDK arrives.
    *snapshot = m_snapshot;
    return SP_OK;
}

SP_STATUS CPolaritySession::RunMapping()
{
    SetError("SP_RunMapping blocked: RD-SP Mapping backend not available");
    return SP_E_BACKEND_NOT_SUPPORTED;
}

SP_STATUS CPolaritySession::Cancel()
{
    return SP_OK;
}

SP_STATUS CPolaritySession::GetSummary(SP_RESULT_SUMMARY* summary)
{
    if (!summary || summary->structSize < sizeof(SP_RESULT_SUMMARY))
        return SP_E_INVALID_ARGUMENT;
    *summary = m_summary;
    return SP_OK;
}

SP_STATUS CPolaritySession::GetFiberResultCount(uint32_t* count)
{
    if (!count)
        return SP_E_INVALID_ARGUMENT;
    *count = static_cast<uint32_t>(m_fibers.size());
    return SP_OK;
}

SP_STATUS CPolaritySession::GetFiberResults(SP_FIBER_RESULT* buffer, uint32_t capacity, uint32_t* written)
{
    if (!written)
        return SP_E_INVALID_ARGUMENT;
    *written = 0;
    if (m_fibers.empty())
        return SP_OK;
    if (!buffer || capacity == 0)
        return SP_E_INVALID_ARGUMENT;
    const uint32_t n = static_cast<uint32_t>((capacity < m_fibers.size()) ? capacity : m_fibers.size());
    for (uint32_t i = 0; i < n; ++i)
        buffer[i] = m_fibers[i];
    *written = n;
    return SP_OK;
}

SP_STATUS CPolaritySession::GetDiagnosticPathUtf8(uint32_t index, char* buffer, uint32_t capacity, uint32_t* required)
{
    if (index >= m_diagnosticPaths.size())
        return SP_E_INVALID_ARGUMENT;
    return CopyUtf8(m_diagnosticPaths[index], buffer, capacity, required);
}

SP_STATUS CPolaritySession::GetLastErrorUtf8(char* buffer, uint32_t capacity, uint32_t* required)
{
    return CopyUtf8(m_lastError, buffer, capacity, required);
}
