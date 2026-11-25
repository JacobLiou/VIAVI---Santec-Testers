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
    strncpy_s(dest, destSize, src, _TRUNCATE);
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
    , m_fiberCount(12)
    , m_cliHandle(NULL)
{
    ZeroSnapshot(&m_snapshot);
    ZeroSummary(&m_summary);
    SetError("");
}

CPolaritySession::~CPolaritySession()
{
    Close();
}

void CPolaritySession::SetError(const char* utf8)
{
    m_lastError = utf8 ? utf8 : "";
}

void CPolaritySession::PullCliError()
{
    if (!m_cli.IsLoaded() || !m_cli.GetLastErrorMsg || !m_cliHandle)
        return;
    char buf[512];
    uint32_t required = 0;
    if (m_cli.GetLastErrorMsg(m_cliHandle, buf, sizeof(buf), &required) == SP_OK && buf[0])
        SetError(buf);
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

void CPolaritySession::BuildDefaultExpected(uint32_t fiberCount)
{
    m_expectedMap.clear();
    m_expectedMap.reserve(fiberCount);
    for (uint32_t i = 1; i <= fiberCount; ++i)
        m_expectedMap.push_back(static_cast<int32_t>(i));
}

SP_STATUS CPolaritySession::Open(const SP_OPEN_OPTIONS* options)
{
    if (!options || options->structSize < sizeof(SP_OPEN_OPTIONS))
        return SP_E_INVALID_ARGUMENT;
    if (m_opened)
        return SP_E_INVALID_STATE;

    m_rlmSerial = options->rlmSerialUtf8 ? options->rlmSerialUtf8 : "";
    if (!m_rlmLock.TryAcquire(m_rlmSerial.c_str(), 0))
    {
        SetError("RLM device lock busy (Global\\UDL_SANTEC_RLM100_*); another session owns this RLM");
        return SP_E_DEVICE_BUSY;
    }

    if (!m_cli.Load())
    {
        SetError(m_cli.LoaderError());
        m_rlmLock.Release();
        return SP_E_WORKER_UNAVAILABLE;
    }

    const int createRc = m_cli.Create(&m_cliHandle);
    if (createRc != SP_OK || !m_cliHandle)
    {
        SetError("SP_Cli_Create failed");
        m_cli.Unload();
        m_rlmLock.Release();
        return createRc != SP_OK ? static_cast<SP_STATUS>(createRc) : SP_E_INTERNAL;
    }

    m_fiberCount = 12;
    // Prefer 24 when station uses full OSX 1x24; callers may pass fiber count via profile later.
    // For now: if mappingTimeoutMs field abused? Keep 12 default; profilePath reserved.
    BuildDefaultExpected(m_fiberCount);

    SP_CLI_OPEN_PARAMS params;
    std::memset(&params, 0, sizeof(params));
    params.structSize = sizeof(params);
    params.version = 1;
    params.stationIdUtf8 = options->stationIdUtf8;
    params.rlmSerialUtf8 = options->rlmSerialUtf8;
    params.rdspSerialUtf8 = options->rdspSerialUtf8;
    params.fiberCount = m_fiberCount;
    params.connectTimeoutMs = options->connectTimeoutMs;
    params.mappingTimeoutMs = options->mappingTimeoutMs;

    const int openRc = m_cli.Open(m_cliHandle, &params);
    if (openRc != SP_OK)
    {
        PullCliError();
        if (m_lastError.empty())
            SetError("SP_Cli_Open failed");
        m_cli.Destroy(m_cliHandle);
        m_cliHandle = NULL;
        m_cli.Unload();
        m_rlmLock.Release();
        return static_cast<SP_STATUS>(openRc);
    }

    SP_CLI_SNAPSHOT snap;
    std::memset(&snap, 0, sizeof(snap));
    snap.structSize = sizeof(snap);
    if (m_cli.GetSnapshot(m_cliHandle, &snap) == SP_OK)
    {
        CopyCStr(m_snapshot.rlmPartNumber, sizeof(m_snapshot.rlmPartNumber), snap.rlmPartNumber);
        CopyCStr(m_snapshot.rlmSerial, sizeof(m_snapshot.rlmSerial), snap.rlmSerial);
        CopyCStr(m_snapshot.rlmFirmware, sizeof(m_snapshot.rlmFirmware), snap.rlmFirmware);
        CopyCStr(m_snapshot.osxPartNumber, sizeof(m_snapshot.osxPartNumber), snap.osxPartNumber);
        CopyCStr(m_snapshot.rdspPartNumber, sizeof(m_snapshot.rdspPartNumber), snap.rdspPartNumber);
        CopyCStr(m_snapshot.rdspSerial, sizeof(m_snapshot.rdspSerial), snap.rdspSerial);
        m_snapshot.rlmOutputCount = snap.rlmOutputCount;
        m_snapshot.switchNumber = snap.switchNumber;
        m_snapshot.switchChannelCount = snap.switchChannelCount;
        m_snapshot.polarityWavelengthNm = snap.polarityWavelengthNm;
        m_snapshot.remoteDetectorIndex = snap.remoteDetectorIndex;
        if (snap.switchChannelCount > 0 && snap.switchChannelCount <= 24)
        {
            m_fiberCount = static_cast<uint32_t>(snap.switchChannelCount >= 24 ? 24 : 12);
            if (params.fiberCount == 12 || params.fiberCount == 24)
                m_fiberCount = params.fiberCount;
            BuildDefaultExpected(m_fiberCount);
        }
    }

    m_opened = true;
    SetError("");
    return SP_OK;
}

SP_STATUS CPolaritySession::Close()
{
    if (m_cliHandle && m_cli.IsLoaded())
    {
        if (m_cli.Close)
            m_cli.Close(m_cliHandle);
        if (m_cli.Destroy)
            m_cli.Destroy(m_cliHandle);
        m_cliHandle = NULL;
    }
    m_cli.Unload();
    m_rlmLock.Release();
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
    *snapshot = m_snapshot;
    return SP_OK;
}

SP_STATUS CPolaritySession::RunMapping()
{
    if (!m_opened || !m_cliHandle || !m_cli.RunMapping)
        return SP_E_INVALID_STATE;

    const int rc = m_cli.RunMapping(
        m_cliHandle,
        m_expectedMap.empty() ? NULL : &m_expectedMap[0],
        static_cast<uint32_t>(m_expectedMap.size()));
    if (rc != SP_OK)
    {
        PullCliError();
        if (m_lastError.empty())
            SetError("SP_Cli_RunMapping failed");
        return static_cast<SP_STATUS>(rc);
    }

    SP_CLI_SUMMARY sum;
    std::memset(&sum, 0, sizeof(sum));
    sum.structSize = sizeof(sum);
    if (m_cli.GetSummary(m_cliHandle, &sum) == SP_OK)
    {
        m_summary.overallPass = sum.overallPass;
        m_summary.fiberCount = sum.fiberCount;
        m_summary.passCount = sum.passCount;
        m_summary.failCount = sum.failCount;
        m_summary.diagnosticImageCount = sum.diagnosticImageCount;
        m_summary.elapsedMs = sum.elapsedMs;
    }

    uint32_t count = 0;
    m_cli.GetFiberCount(m_cliHandle, &count);
    m_fibers.clear();
    if (count > 0)
    {
        std::vector<SP_CLI_FIBER_RESULT> tmp(count);
        uint32_t written = 0;
        if (m_cli.GetFibers(m_cliHandle, &tmp[0], count, &written) == SP_OK)
        {
            m_fibers.resize(written);
            for (uint32_t i = 0; i < written; ++i)
            {
                m_fibers[i].inputFiber = tmp[i].inputFiber;
                m_fibers[i].switchChannel = tmp[i].switchChannel;
                m_fibers[i].expectedOutput = tmp[i].expectedOutput;
                m_fibers[i].observedOutput = tmp[i].observedOutput;
                m_fibers[i].lightDetected = tmp[i].lightDetected;
                m_fibers[i].status = static_cast<SP_FIBER_STATUS>(tmp[i].status);
                m_fibers[i].acquisitionMs = tmp[i].acquisitionMs;
            }
        }
    }

    m_diagnosticPaths.clear();
    for (uint32_t i = 0; i < m_summary.diagnosticImageCount; ++i)
    {
        char path[520];
        uint32_t required = 0;
        if (m_cli.GetDiagnosticPath(m_cliHandle, i, path, sizeof(path), &required) == SP_OK)
            m_diagnosticPaths.push_back(path);
    }

    SetError("");
    return SP_OK;
}

SP_STATUS CPolaritySession::Cancel()
{
    if (!m_cliHandle || !m_cli.Cancel)
        return SP_OK;
    return static_cast<SP_STATUS>(m_cli.Cancel(m_cliHandle));
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
