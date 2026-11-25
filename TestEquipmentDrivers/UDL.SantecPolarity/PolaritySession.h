#pragma once

#include "SPTypes.h"
#include <string>
#include <vector>

// Session shell for RLM-100 + OSX-100 + RD-SP-100-SM orchestration.
// Mapping backend is gated on vendor RD-SP SDK / Worker (see docs).

class CPolaritySession
{
public:
    CPolaritySession();
    ~CPolaritySession();

    SP_STATUS Open(const SP_OPEN_OPTIONS* options);
    SP_STATUS Close();
    SP_STATUS GetHardwareSnapshot(SP_HARDWARE_SNAPSHOT* snapshot);
    SP_STATUS RunMapping();
    SP_STATUS Cancel();
    SP_STATUS GetSummary(SP_RESULT_SUMMARY* summary);
    SP_STATUS GetFiberResultCount(uint32_t* count);
    SP_STATUS GetFiberResults(SP_FIBER_RESULT* buffer, uint32_t capacity, uint32_t* written);
    SP_STATUS GetDiagnosticPathUtf8(uint32_t index, char* buffer, uint32_t capacity, uint32_t* required);
    SP_STATUS GetLastErrorUtf8(char* buffer, uint32_t capacity, uint32_t* required);

private:
    void SetError(const char* utf8);
    SP_STATUS CopyUtf8(const std::string& src, char* buffer, uint32_t capacity, uint32_t* required);

    bool m_opened;
    std::string m_lastError;
    SP_HARDWARE_SNAPSHOT m_snapshot;
    SP_RESULT_SUMMARY m_summary;
    std::vector<SP_FIBER_RESULT> m_fibers;
    std::vector<std::string> m_diagnosticPaths;
};
