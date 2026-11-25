#pragma once

#include "..\Common\PolarityCliApi.h"
#include <string>
#include <vector>
#include <vcclr.h>

// Native C++ session with gcroot<> to Santec.Hardware (C++/CLI).

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Threading::Tasks;
using namespace Santec::Hardware::Devices::General;
using namespace Santec::Hardware::Devices::Polarity;
using namespace Santec::Hardware::Devices::ReturnLoss;
using namespace Santec::Hardware::Services;

class PolarityCliSession
{
public:
    PolarityCliSession();
    ~PolarityCliSession();

    int Open(const SP_CLI_OPEN_PARAMS* params);
    int Close();
    int GetSnapshot(SP_CLI_SNAPSHOT* snapshot);
    int RunMapping(const int32_t* expectedMap, uint32_t expectedCount);
    int Cancel();
    int GetSummary(SP_CLI_SUMMARY* summary);
    int GetFiberCount(uint32_t* count);
    int GetFibers(SP_CLI_FIBER_RESULT* buffer, uint32_t capacity, uint32_t* written);
    int GetDiagnosticPath(uint32_t index, char* buffer, uint32_t capacity, uint32_t* required);
    int GetLastErrorMsg(char* buffer, uint32_t capacity, uint32_t* required);

private:
    void SetError(System::String^ msg);
    int CopyToUtf8(System::String^ src, char* buffer, uint32_t capacity, uint32_t* required);
    void CleanupHardware();
    void FillResults(System::Collections::Generic::IReadOnlyList<System::Nullable<int>>^ map,
                     System::Collections::Generic::List<int>^ expected,
                     System::Collections::Generic::List<bool>^ detected);

    template <typename T>
    T WaitTask(System::Threading::Tasks::Task<T>^ task)
    {
        return task->GetAwaiter().GetResult();
    }

    void WaitTask(System::Threading::Tasks::Task^ task)
    {
        task->GetAwaiter().GetResult();
    }

    bool m_opened;
    volatile bool m_cancel;
    uint32_t m_fiberCount;
    std::string m_lastError;
    SP_CLI_SNAPSHOT m_snapshot;
    SP_CLI_SUMMARY m_summary;
    std::vector<SP_CLI_FIBER_RESULT> m_fibers;
    std::vector<std::string> m_diagPaths;

    gcroot<IRDPFactory^> m_factory;
    gcroot<IHardwareDetectionService^> m_detect;
    gcroot<IRLM^> m_rlm;
    gcroot<IRDP^> m_rdp;
};
