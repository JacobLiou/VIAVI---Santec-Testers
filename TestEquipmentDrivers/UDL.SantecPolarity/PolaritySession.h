#pragma once

#include "SPTypes.h"
#include "PolarityCliLoader.h"
#include "..\Common\SantecRlmDeviceLock.h"
#include <string>
#include <vector>

// Pure-native shell: Host-facing SP_* session.
// RD-SP Mapping is forwarded to UDL.SantecPolarity.Cli (C++/CLI -> Santec.Hardware).

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
    void PullCliError();
    SP_STATUS CopyUtf8(const std::string& src, char* buffer, uint32_t capacity, uint32_t* required);
    void BuildDefaultExpected(uint32_t fiberCount);

    bool m_opened;
    std::string m_lastError;
    std::string m_rlmSerial;
    uint32_t m_fiberCount;
    std::vector<int32_t> m_expectedMap;
    SP_HARDWARE_SNAPSHOT m_snapshot;
    SP_RESULT_SUMMARY m_summary;
    std::vector<SP_FIBER_RESULT> m_fibers;
    std::vector<std::string> m_diagnosticPaths;

    CPolarityCliLoader m_cli;
    SP_CLI_HANDLE m_cliHandle;
    SantecRlmLock::Guard m_rlmLock;
};
