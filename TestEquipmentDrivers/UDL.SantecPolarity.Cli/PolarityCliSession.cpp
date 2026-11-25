// Avoid Win32 IServiceProvider clash with System::IServiceProvider under /clr.
#define IServiceProvider IServiceProvider_Win32
#include <Windows.h>
#undef IServiceProvider

#include "PolarityCliSession.h"
#include <cstring>
#include <msclr/marshal_cppstd.h>

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Diagnostics;
using namespace System::Runtime::InteropServices;
using namespace Santec::Hardware::Devices::General;
using namespace Santec::Hardware::Devices::Polarity;
using namespace Santec::Hardware::Devices::ReturnLoss;
using namespace Santec::Hardware::Exceptions;
using namespace Santec::Hardware::Services;

namespace {

const int kSP_OK = 0;
const int kSP_E_INVALID_ARGUMENT = -1;
const int kSP_E_INVALID_STATE = -2;
const int kSP_E_RDSP_NOT_FOUND = -10;
const int kSP_E_RLM_NOT_FOUND = -11;
const int kSP_E_HARDWARE_NOT_SUPPORTED = -17;
const int kSP_E_CANCELLED = -21;
const int kSP_E_MAPPING_FAILED = -31;
const int kSP_E_INTERNAL = -100;
const int kSP_FIBER_PASS = 0;
const int kSP_FIBER_WRONG_MAPPING = 1;
const int kSP_FIBER_MISSING = 2;

void CopyNarrow(char* dest, size_t destSize, String^ src)
{
    if (!dest || destSize == 0)
        return;
    dest[0] = '\0';
    if (src == nullptr)
        return;
    msclr::interop::marshal_context ctx;
    std::string s = ctx.marshal_as<std::string>(src);
    strncpy_s(dest, destSize, s.c_str(), _TRUNCATE);
}

String^ FromUtf8(const char* p)
{
    if (!p || !*p)
        return nullptr;
    return Marshal::PtrToStringAnsi(IntPtr(const_cast<char*>(p)));
}

} // namespace

PolarityCliSession::PolarityCliSession()
    : m_opened(false)
    , m_cancel(false)
    , m_fiberCount(12)
{
    std::memset(&m_snapshot, 0, sizeof(m_snapshot));
    m_snapshot.structSize = sizeof(m_snapshot);
    m_snapshot.rlmOutputCount = 1;
    m_snapshot.switchNumber = 1;
    m_snapshot.switchChannelCount = 24;
    m_snapshot.polarityWavelengthNm = 1310;
    m_snapshot.remoteDetectorIndex = -1;
    std::memset(&m_summary, 0, sizeof(m_summary));
    m_summary.structSize = sizeof(m_summary);
}

PolarityCliSession::~PolarityCliSession()
{
    CleanupHardware();
}

void PolarityCliSession::SetError(String^ msg)
{
    if (msg == nullptr)
    {
        m_lastError.clear();
        return;
    }
    msclr::interop::marshal_context ctx;
    m_lastError = ctx.marshal_as<std::string>(msg);
}

int PolarityCliSession::CopyToUtf8(String^ src, char* buffer, uint32_t capacity, uint32_t* required)
{
    msclr::interop::marshal_context ctx;
    std::string s = (src == nullptr) ? std::string() : ctx.marshal_as<std::string>(src);
    const uint32_t need = static_cast<uint32_t>(s.size() + 1);
    if (required)
        *required = need;
    if (!buffer || capacity == 0)
        return kSP_E_INVALID_ARGUMENT;
    if (capacity < need)
        return kSP_E_INVALID_ARGUMENT;
    std::memcpy(buffer, s.c_str(), need);
    return kSP_OK;
}

void PolarityCliSession::CleanupHardware()
{
    try
    {
        if (static_cast<IRDP^>(m_rdp) != nullptr)
        {
            try { safe_cast<IDevice^>(m_rdp)->Uninitialize(); } catch (...) {}
            m_rdp = nullptr;
        }
        if (static_cast<IRLM^>(m_rlm) != nullptr)
        {
            try { WaitTask(m_rlm->TurnOffLaserAsync()); } catch (...) {}
            try { safe_cast<IDevice^>(m_rlm)->Uninitialize(); } catch (...) {}
            m_rlm = nullptr;
        }
    }
    catch (...) {}
    m_factory = nullptr;
    m_detect = nullptr;
    m_opened = false;
}

int PolarityCliSession::Open(const SP_CLI_OPEN_PARAMS* params)
{
    if (!params || params->structSize < sizeof(SP_CLI_OPEN_PARAMS))
        return kSP_E_INVALID_ARGUMENT;
    if (m_opened)
        return kSP_E_INVALID_STATE;

    m_cancel = false;
    m_fiberCount = params->fiberCount;
    if (m_fiberCount != 12 && m_fiberCount != 24)
        m_fiberCount = 12;

    String^ wantRlm = FromUtf8(params->rlmSerialUtf8);

    try
    {
        m_factory = gcnew RDPFactory();
        m_detect = gcnew HardwareDetectionService();

        if (!m_factory->CheckForCamera())
        {
            SetError("Could not find RD-P/RD-SP camera (CheckForCamera failed)");
            CleanupHardware();
            return kSP_E_RDSP_NOT_FOUND;
        }

        List<IRLM^>^ rlms = WaitTask(
            m_detect->DetectHardwareAsync<IRLM^>(EConnectionTypesToDetect::USB));

        IRLM^ chosen = nullptr;
        for (int i = 0; i < rlms->Count; ++i)
        {
            IRLM^ detected = rlms[i];
            WaitTask(detected->InitializeAsync());
            if (wantRlm != nullptr)
            {
                String^ sn = detected->SerialNumber;
                if (sn == nullptr || !sn->Equals(wantRlm, StringComparison::OrdinalIgnoreCase))
                {
                    safe_cast<IDevice^>(detected)->Uninitialize();
                    continue;
                }
            }
            if (m_factory->CheckIfRLMMeetsHardwareRequirements(detected))
            {
                chosen = detected;
                break;
            }
            safe_cast<IDevice^>(detected)->Uninitialize();
        }

        if (chosen == nullptr)
        {
            SetError("No RLM meets RD-P/RD-SP hardware requirements (or serial mismatch)");
            CleanupHardware();
            return kSP_E_RLM_NOT_FOUND;
        }

        m_rlm = chosen;
        m_rdp = m_factory->CreateRDP(m_rlm);
        WaitTask(m_rdp->InitializeAsync());

        CopyNarrow(m_snapshot.rlmSerial, sizeof(m_snapshot.rlmSerial), m_rlm->SerialNumber);
        CopyNarrow(m_snapshot.rlmPartNumber, sizeof(m_snapshot.rlmPartNumber), "RLM-100");
        CopyNarrow(m_snapshot.osxPartNumber, sizeof(m_snapshot.osxPartNumber), "OSX-100");
        CopyNarrow(m_snapshot.rdspPartNumber, sizeof(m_snapshot.rdspPartNumber), "RD-SP-100");
        CopyNarrow(m_snapshot.rdspSerial, sizeof(m_snapshot.rdspSerial), "");

        DeviceDetector^ detector = nullptr;
        if (m_rlm->Detectors != nullptr && m_rlm->Detectors->Count > 0)
            detector = m_rlm->Detectors[0];
        m_snapshot.isRdsp = (detector != nullptr && detector->IsRDSP) ? 1 : 0;
        m_snapshot.remoteDetectorIndex = (detector != nullptr) ? detector->Index : -1;
        m_snapshot.switchChannelCount = static_cast<int32_t>(m_fiberCount);
        m_snapshot.polarityWavelengthNm = 1310;
        m_snapshot.rlmOutputCount = 1;
        m_snapshot.switchNumber = 1;

        m_opened = true;
        SetError(nullptr);
        return kSP_OK;
    }
    catch (Exception^ ex)
    {
        SetError(ex->ToString());
        CleanupHardware();
        return kSP_E_HARDWARE_NOT_SUPPORTED;
    }
}

int PolarityCliSession::Close()
{
    CleanupHardware();
    m_fibers.clear();
    m_diagPaths.clear();
    std::memset(&m_summary, 0, sizeof(m_summary));
    m_summary.structSize = sizeof(m_summary);
    SetError(nullptr);
    return kSP_OK;
}

int PolarityCliSession::GetSnapshot(SP_CLI_SNAPSHOT* snapshot)
{
    if (!snapshot || snapshot->structSize < sizeof(SP_CLI_SNAPSHOT))
        return kSP_E_INVALID_ARGUMENT;
    *snapshot = m_snapshot;
    return kSP_OK;
}

void PolarityCliSession::FillResults(
    IReadOnlyList<Nullable<int>>^ map,
    List<int>^ expected,
    List<bool>^ detected)
{
    m_fibers.clear();
    uint32_t pass = 0;
    uint32_t fail = 0;
    const int n = expected->Count;
    for (int i = 0; i < n; ++i)
    {
        SP_CLI_FIBER_RESULT fr;
        std::memset(&fr, 0, sizeof(fr));
        fr.inputFiber = i + 1;
        fr.switchChannel = i + 1;
        fr.expectedOutput = expected[i];
        fr.observedOutput = 0;
        fr.lightDetected = 0;
        fr.status = kSP_FIBER_MISSING;
        fr.acquisitionMs = 0;

        if (detected != nullptr && i < detected->Count && detected[i])
            fr.lightDetected = 1;

        if (map != nullptr && i < map->Count && map[i].HasValue)
        {
            fr.observedOutput = map[i].Value;
            if (fr.observedOutput == fr.expectedOutput)
            {
                fr.status = kSP_FIBER_PASS;
                ++pass;
            }
            else
            {
                fr.status = kSP_FIBER_WRONG_MAPPING;
                ++fail;
            }
        }
        else
        {
            ++fail;
        }
        m_fibers.push_back(fr);
    }
    m_summary.fiberCount = static_cast<uint32_t>(n);
    m_summary.passCount = pass;
    m_summary.failCount = fail;
    m_summary.overallPass = (fail == 0 && n > 0) ? 1 : 0;
}

int PolarityCliSession::RunMapping(const int32_t* expectedMap, uint32_t expectedCount)
{
    if (!m_opened || static_cast<IRDP^>(m_rdp) == nullptr || static_cast<IRLM^>(m_rlm) == nullptr)
        return kSP_E_INVALID_STATE;

    m_cancel = false;
    m_diagPaths.clear();
    m_fibers.clear();

    List<int>^ expected = gcnew List<int>();
    if (expectedMap && expectedCount > 0)
    {
        for (uint32_t i = 0; i < expectedCount; ++i)
            expected->Add(expectedMap[i]);
    }
    else
    {
        for (uint32_t i = 1; i <= m_fiberCount; ++i)
            expected->Add(static_cast<int>(i));
    }

    const int fiberCount = expected->Count;
    List<bool>^ detectedFlags = gcnew List<bool>();
    for (int i = 0; i < fiberCount; ++i)
        detectedFlags->Add(false);

    Stopwatch^ sw = Stopwatch::StartNew();
    try
    {
        m_rdp->SetMappingMode(EMappingMode::MPO);
        m_rdp->SetMappingSensitivity(EMappingSensitivity::Connector);
        WaitTask(m_rdp->SetLossCompensationAsync(0));

        WaitTask(m_rdp->StartMappingPolarityAsync(fiberCount));
        WaitTask(m_rlm->SetInteractionModeAsync(EInteractionMode::Remote));

        int wavelength = 1310;
        bool foundWl = false;
        for (int wi = 0; wi < m_rlm->Wavelengths->Count; ++wi)
        {
            const int w = m_rlm->Wavelengths[wi];
            if (w == 1310 || w == 1300)
            {
                wavelength = w;
                foundWl = true;
                break;
            }
        }
        if (!foundWl && m_rlm->Wavelengths->Count > 0)
            wavelength = m_rlm->Wavelengths[0];
        m_snapshot.polarityWavelengthNm = wavelength;

        WaitTask(m_rlm->TurnOnLaserAsync(wavelength));
        WaitTask(m_rlm->ChangeOutputChannelAsync(1));

        DeviceDetector^ detector = nullptr;
        if (m_rlm->Detectors != nullptr && m_rlm->Detectors->Count > 0)
            detector = m_rlm->Detectors[0];

        for (int i = 0; i < fiberCount; ++i)
        {
            if (m_cancel)
            {
                try { WaitTask(m_rlm->TurnOffLaserAsync()); } catch (...) {}
                try { WaitTask(m_rdp->EndMappingPolarityAsync()); } catch (...) {}
                SetError("Mapping cancelled");
                return kSP_E_CANCELLED;
            }

            if (detector != nullptr && detector->IsRDSP)
            {
                try
                {
                    WaitTask(m_rlm->ReadILAsync(detector->Index, wavelength));
                    WaitTask(m_rlm->TurnOnLaserAsync(wavelength));
                }
                catch (...) {}
            }

            WaitTask(m_rlm->ChangeSwitchChannelAsync(1, i + 1));
            const bool fiberDetected = WaitTask(m_rdp->CollectOutputMappingDataAsync(i + 1));
            detectedFlags[i] = fiberDetected;
        }

        WaitTask(m_rlm->TurnOffLaserAsync());
        WaitTask(m_rlm->SetInteractionModeAsync(EInteractionMode::Local));

        List<Nullable<int>>^ expectedNullable = gcnew List<Nullable<int>>();
        for (int i = 0; i < expected->Count; ++i)
            expectedNullable->Add(Nullable<int>(expected[i]));

        IReadOnlyList<Nullable<int>>^ map = nullptr;
        try
        {
            map = m_rdp->ProcessMappingData(expectedNullable);
        }
        catch (PolarityMappingFailedException^ ex)
        {
            // Diagnostic images are OpenCvSharp::Mat in 2.3.1.0; persist in a later pass with OpenCvSharp package.
            try { WaitTask(m_rdp->EndMappingPolarityAsync()); } catch (...) {}
            SetError(ex->Message);
            m_summary.diagnosticImageCount = 0;
            sw->Stop();
            m_summary.elapsedMs = static_cast<uint64_t>(sw->ElapsedMilliseconds);
            return kSP_E_MAPPING_FAILED;
        }

        WaitTask(m_rdp->EndMappingPolarityAsync());
        FillResults(map, expected, detectedFlags);
        m_summary.diagnosticImageCount = static_cast<uint32_t>(m_diagPaths.size());
        sw->Stop();
        m_summary.elapsedMs = static_cast<uint64_t>(sw->ElapsedMilliseconds);
        SetError(nullptr);
        return kSP_OK;
    }
    catch (Exception^ ex)
    {
        try { WaitTask(m_rlm->TurnOffLaserAsync()); } catch (...) {}
        try
        {
            if (static_cast<IRDP^>(m_rdp) != nullptr)
                WaitTask(m_rdp->EndMappingPolarityAsync());
        }
        catch (...) {}
        SetError(ex->ToString());
        sw->Stop();
        m_summary.elapsedMs = static_cast<uint64_t>(sw->ElapsedMilliseconds);
        return kSP_E_INTERNAL;
    }
}

int PolarityCliSession::Cancel()
{
    m_cancel = true;
    return kSP_OK;
}

int PolarityCliSession::GetSummary(SP_CLI_SUMMARY* summary)
{
    if (!summary || summary->structSize < sizeof(SP_CLI_SUMMARY))
        return kSP_E_INVALID_ARGUMENT;
    *summary = m_summary;
    return kSP_OK;
}

int PolarityCliSession::GetFiberCount(uint32_t* count)
{
    if (!count)
        return kSP_E_INVALID_ARGUMENT;
    *count = static_cast<uint32_t>(m_fibers.size());
    return kSP_OK;
}

int PolarityCliSession::GetFibers(SP_CLI_FIBER_RESULT* buffer, uint32_t capacity, uint32_t* written)
{
    if (!written)
        return kSP_E_INVALID_ARGUMENT;
    *written = 0;
    if (m_fibers.empty())
        return kSP_OK;
    if (!buffer || capacity == 0)
        return kSP_E_INVALID_ARGUMENT;
    const uint32_t n = (capacity < m_fibers.size()) ? capacity : static_cast<uint32_t>(m_fibers.size());
    for (uint32_t i = 0; i < n; ++i)
        buffer[i] = m_fibers[i];
    *written = n;
    return kSP_OK;
}

int PolarityCliSession::GetDiagnosticPath(uint32_t index, char* buffer, uint32_t capacity, uint32_t* required)
{
    if (index >= m_diagPaths.size())
        return kSP_E_INVALID_ARGUMENT;
    String^ s = gcnew String(m_diagPaths[index].c_str());
    return CopyToUtf8(s, buffer, capacity, required);
}

int PolarityCliSession::GetLastErrorMsg(char* buffer, uint32_t capacity, uint32_t* required)
{
    String^ s = gcnew String(m_lastError.c_str());
    return CopyToUtf8(s, buffer, capacity, required);
}
