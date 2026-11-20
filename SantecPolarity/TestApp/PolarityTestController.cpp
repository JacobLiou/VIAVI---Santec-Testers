#include "PolarityTestController.h"

#include <cstdlib>
#include <sstream>

namespace
{
    class LockGuard
    {
    public:
        explicit LockGuard(CRITICAL_SECTION& lock) : m_lock(lock) { EnterCriticalSection(&m_lock); }
        ~LockGuard() { LeaveCriticalSection(&m_lock); }
    private:
        CRITICAL_SECTION& m_lock;
    };
}

PolarityTestController::PolarityTestController() : m_cancelRequested(0)
{
    InitializeCriticalSection(&m_lock);
    m_client.SetIoTimeout(15000);
}

PolarityTestController::~PolarityTestController()
{
    Disconnect();
    DeleteCriticalSection(&m_lock);
}

bool PolarityTestController::Connect(const std::string& host, unsigned short port,
                                     std::string& identity, std::string& error)
{
    LockGuard guard(m_lock);
    if (!m_client.Connect(host, port, 5000, error))
        return false;
    if (!m_client.Query("*IDN?", identity, error))
    {
        m_client.Disconnect();
        return false;
    }
    return true;
}

void PolarityTestController::Disconnect()
{
    LockGuard guard(m_lock);
    if (m_client.IsConnected())
    {
        std::string ignored;
        m_client.Write("LASER:DISABLE", ignored);
        m_client.Disconnect();
    }
}

bool PolarityTestController::IsConnected()
{
    LockGuard guard(m_lock);
    return m_client.IsConnected();
}

AppTestResult PolarityTestController::RunTest(int coreCount, char polarityType,
                                              const std::string& simulatorProfile,
                                              unsigned long overallTimeoutMs)
{
    AppTestResult result;
    result.communicationSucceeded = false;
    result.passed = false;
    InterlockedExchange(&m_cancelRequested, 0);

    std::ostringstream command;
    std::string error;
    command << "POLARITY:CONFIGURE:CORES " << coreCount;
    if (!Write(command.str(), error)) { result.error = error; return result; }
    command.str(""); command.clear();
    command << "POLARITY:CONFIGURE:TYPE " << polarityType;
    if (!Write(command.str(), error)) { result.error = error; return result; }

    if (!simulatorProfile.empty() && simulatorProfile != "DEVICE DEFAULT")
    {
        command.str(""); command.clear();
        command << "POLARITY:SIMULATOR:PROFILE " << simulatorProfile;
        if (!Write(command.str(), error)) { result.error = error; return result; }
    }

    if (!Write("*CLS", error) || !Write("POLARITY:TEST:START", error))
    {
        result.error = error;
        return result;
    }

    const ULONGLONG deadline = GetTickCount64() + overallTimeoutMs;
    for (;;)
    {
        if (InterlockedCompareExchange(&m_cancelRequested, 0, 0) != 0)
        {
            Write("POLARITY:TEST:ABORT", error);
            result.error = "Test cancelled";
            return result;
        }
        std::string state;
        if (!Query("POLARITY:TEST:STATUS?", state, error))
        {
            result.error = error;
            return result;
        }
        if (state == "COMPLETE")
            break;
        if (state == "CANCELLED" || state == "ERROR")
        {
            result.error = "Instrument state: " + state;
            return result;
        }
        if (GetTickCount64() >= deadline)
        {
            Write("POLARITY:TEST:ABORT", error);
            result.error = "Test timed out";
            return result;
        }
        Sleep(100);
    }

    std::string summary;
    if (!Query("POLARITY:RESULT:SUMMARY?", summary, error))
    {
        result.error = error;
        return result;
    }
    const std::vector<std::string> summaryFields = SplitCsv(summary);
    int reportedCores = 0, passedCores = 0, failedCores = 0;
    if (summaryFields.size() != 4 || !ParseInteger(summaryFields[1], reportedCores) ||
        !ParseInteger(summaryFields[2], passedCores) || !ParseInteger(summaryFields[3], failedCores) ||
        reportedCores != coreCount)
    {
        result.error = "Malformed summary: " + summary;
        return result;
    }
    result.summaryStatus = summaryFields[0];
    result.passed = summaryFields[0] == "PASS" && failedCores == 0 && passedCores == coreCount;

    for (int core = 1; core <= coreCount; ++core)
    {
        command.str(""); command.clear();
        command << "POLARITY:RESULT:CORE? " << core;
        std::string line;
        if (!Query(command.str(), line, error))
        {
            result.error = error;
            result.cores.clear();
            return result;
        }
        const std::vector<std::string> fields = SplitCsv(line);
        AppCoreResult item;
        if (fields.size() != 5 || !ParseInteger(fields[0], item.inputCore) ||
            !ParseInteger(fields[1], item.expectedOutput) ||
            !ParseInteger(fields[2], item.observedOutput) ||
            !ParseInteger(fields[3], item.lightDetected))
        {
            result.error = "Malformed core result: " + line;
            result.cores.clear();
            return result;
        }
        item.status = fields[4];
        result.cores.push_back(item);
    }
    result.communicationSucceeded = true;
    return result;
}

void PolarityTestController::RequestCancel()
{
    InterlockedExchange(&m_cancelRequested, 1);
}

bool PolarityTestController::Write(const std::string& command, std::string& error)
{
    LockGuard guard(m_lock);
    return m_client.Write(command, error);
}

bool PolarityTestController::Query(const std::string& command,
                                   std::string& response, std::string& error)
{
    LockGuard guard(m_lock);
    if (!m_client.Query(command, response, error))
        return false;
    if (response.compare(0, 4, "ERR,") == 0)
    {
        error = response;
        return false;
    }
    return true;
}

std::vector<std::string> PolarityTestController::SplitCsv(const std::string& value)
{
    std::vector<std::string> fields;
    size_t begin = 0;
    for (;;)
    {
        const size_t comma = value.find(',', begin);
        fields.push_back(value.substr(begin, comma == std::string::npos ? comma : comma - begin));
        if (comma == std::string::npos)
            break;
        begin = comma + 1;
    }
    return fields;
}

bool PolarityTestController::ParseInteger(const std::string& text, int& value)
{
    char* end = NULL;
    const long parsed = std::strtol(text.c_str(), &end, 10);
    if (end == text.c_str() || *end != '\0')
        return false;
    value = static_cast<int>(parsed);
    return true;
}
