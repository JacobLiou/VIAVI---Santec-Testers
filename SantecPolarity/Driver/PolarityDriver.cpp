#include "PolarityDriver.h"

#include <cstring>
#include <cstdlib>
#include <sstream>
#include <vector>

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

    std::vector<std::string> SplitCsv(const std::string& value)
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
}

PolarityDriver::PolarityDriver() : m_cancelRequested(0)
{
    InitializeCriticalSection(&m_lock);
}

PolarityDriver::~PolarityDriver()
{
    Disconnect();
    DeleteCriticalSection(&m_lock);
}

int PolarityDriver::Connect(const char* host, unsigned short port,
                            unsigned long connectTimeoutMs, unsigned long ioTimeoutMs)
{
    if (host == NULL || *host == '\0' || port == 0)
        return Fail(SP_E_INVALID_ARGUMENT, "A valid host and port are required");

    LockGuard guard(m_lock);
    std::string error;
    m_client.SetIoTimeout(ioTimeoutMs < 1000 ? 1000 : ioTimeoutMs);
    if (!m_client.Connect(host, port, connectTimeoutMs < 100 ? 100 : connectTimeoutMs, error))
        return Fail(SP_E_TRANSPORT, error);
    m_lastError.clear();
    return SP_OK;
}

int PolarityDriver::Disconnect()
{
    LockGuard guard(m_lock);
    if (m_client.IsConnected())
    {
        std::string ignored;
        m_client.Write("LASER:DISABLE", ignored);
        m_client.Disconnect();
    }
    return SP_OK;
}

bool PolarityDriver::IsConnected()
{
    LockGuard guard(m_lock);
    return m_client.IsConnected();
}

int PolarityDriver::GetIdentity(std::string& identity)
{
    LockGuard guard(m_lock);
    return QueryLocked("*IDN?", identity);
}

int PolarityDriver::Configure(int coreCount, int polarityType)
{
    if (coreCount != 12 && coreCount != 24)
        return Fail(SP_E_INVALID_ARGUMENT, "Core count must be 12 or 24");
    if (polarityType < SP_POLARITY_A || polarityType > SP_POLARITY_C)
        return Fail(SP_E_INVALID_ARGUMENT, "Polarity type must be A, B, or C");

    LockGuard guard(m_lock);
    std::ostringstream cores;
    cores << "POLARITY:CONFIGURE:CORES " << coreCount;
    int result = WriteLocked(cores.str());
    if (result != SP_OK)
        return result;
    const char type = static_cast<char>('A' + polarityType);
    std::string typeCommand("POLARITY:CONFIGURE:TYPE ");
    typeCommand.push_back(type);
    return WriteLocked(typeCommand);
}

int PolarityDriver::StartTest()
{
    InterlockedExchange(&m_cancelRequested, 0);
    LockGuard guard(m_lock);
    int result = WriteLocked("*CLS");
    if (result != SP_OK)
        return result;
    return WriteLocked("POLARITY:TEST:START");
}

int PolarityDriver::GetTestState(int& state)
{
    LockGuard guard(m_lock);
    std::string response;
    const int result = QueryLocked("POLARITY:TEST:STATUS?", response);
    if (result != SP_OK)
        return result;
    if (response == "IDLE") state = SP_STATE_IDLE;
    else if (response == "RUNNING") state = SP_STATE_RUNNING;
    else if (response == "COMPLETE") state = SP_STATE_COMPLETE;
    else if (response == "CANCELLED") state = SP_STATE_CANCELLED;
    else if (response == "ERROR") state = SP_STATE_ERROR;
    else return Fail(SP_E_PROTOCOL, "Unknown test state: " + response);
    return SP_OK;
}

int PolarityDriver::GetSummary(SP_TestSummary& summary)
{
    LockGuard guard(m_lock);
    std::string response;
    int result = QueryLocked("POLARITY:RESULT:SUMMARY?", response);
    if (result != SP_OK)
        return result;
    const std::vector<std::string> fields = SplitCsv(response);
    int total = 0, passed = 0, failed = 0;
    if (fields.size() != 4 || !ParseInteger(fields[1], total) ||
        !ParseInteger(fields[2], passed) || !ParseInteger(fields[3], failed))
        return Fail(SP_E_PROTOCOL, "Malformed summary response: " + response);

    ZeroMemory(&summary, sizeof(summary));
    summary.size = sizeof(summary);
    summary.passed = (fields[0] == "PASS") ? 1 : 0;
    summary.coreCount = total;
    summary.passedCores = passed;
    summary.failedCores = failed;
    CopyText(summary.status, sizeof(summary.status), fields[0]);
    return SP_OK;
}

int PolarityDriver::GetCoreResult(int inputCore, SP_CoreResult& result)
{
    if (inputCore < 1 || inputCore > 24)
        return Fail(SP_E_INVALID_ARGUMENT, "Input core must be between 1 and 24");

    LockGuard guard(m_lock);
    std::ostringstream command;
    command << "POLARITY:RESULT:CORE? " << inputCore;
    std::string response;
    int queryResult = QueryLocked(command.str(), response);
    if (queryResult != SP_OK)
        return queryResult;

    const std::vector<std::string> fields = SplitCsv(response);
    int input = 0, expected = 0, observed = 0, light = 0;
    if (fields.size() != 5 || !ParseInteger(fields[0], input) ||
        !ParseInteger(fields[1], expected) || !ParseInteger(fields[2], observed) ||
        !ParseInteger(fields[3], light))
        return Fail(SP_E_PROTOCOL, "Malformed core response: " + response);

    ZeroMemory(&result, sizeof(result));
    result.size = sizeof(result);
    result.inputCore = input;
    result.expectedOutput = expected;
    result.observedOutput = observed;
    result.lightDetected = light;
    CopyText(result.status, sizeof(result.status), fields[4]);
    return SP_OK;
}

int PolarityDriver::RunTest(int coreCount, int polarityType, unsigned long overallTimeoutMs,
                            SP_TestSummary& summary, SP_CoreResult* coreResults, int capacity)
{
    if (coreResults == NULL || capacity < coreCount)
        return Fail(SP_E_INVALID_ARGUMENT, "Core result buffer is too small");
    int result = Configure(coreCount, polarityType);
    if (result != SP_OK)
        return result;
    result = StartTest();
    if (result != SP_OK)
        return result;

    const ULONGLONG deadline = GetTickCount64() + static_cast<ULONGLONG>(overallTimeoutMs);
    for (;;)
    {
        if (InterlockedCompareExchange(&m_cancelRequested, 0, 0) != 0)
        {
            Cancel();
            return Fail(SP_E_CANCELLED, "Test cancelled by caller");
        }
        int state = SP_STATE_IDLE;
        result = GetTestState(state);
        if (result != SP_OK)
            return result;
        if (state == SP_STATE_COMPLETE)
            break;
        if (state == SP_STATE_CANCELLED)
            return Fail(SP_E_CANCELLED, "Instrument cancelled the test");
        if (state == SP_STATE_ERROR)
            return Fail(SP_E_PROTOCOL, "Instrument reported a test error");
        if (GetTickCount64() >= deadline)
        {
            Cancel();
            return Fail(SP_E_TIMEOUT, "Polarity test exceeded the overall timeout");
        }
        Sleep(100);
    }

    result = GetSummary(summary);
    if (result != SP_OK)
        return result;
    for (int core = 1; core <= coreCount; ++core)
    {
        result = GetCoreResult(core, coreResults[core - 1]);
        if (result != SP_OK)
            return result;
    }
    return SP_OK;
}

int PolarityDriver::Cancel()
{
    InterlockedExchange(&m_cancelRequested, 1);
    LockGuard guard(m_lock);
    if (!m_client.IsConnected())
        return SP_OK;
    return WriteLocked("POLARITY:TEST:ABORT");
}

std::string PolarityDriver::LastError()
{
    LockGuard guard(m_lock);
    return m_lastError;
}

int PolarityDriver::WriteLocked(const std::string& command)
{
    if (!m_client.IsConnected())
        return Fail(SP_E_NOT_CONNECTED, "Not connected");
    std::string error;
    if (!m_client.Write(command, error))
        return Fail(SP_E_TRANSPORT, error);
    return SP_OK;
}

int PolarityDriver::QueryLocked(const std::string& command, std::string& response)
{
    if (!m_client.IsConnected())
        return Fail(SP_E_NOT_CONNECTED, "Not connected");
    std::string error;
    if (!m_client.Query(command, response, error))
        return Fail(SP_E_TRANSPORT, error);
    if (response.compare(0, 4, "ERR,") == 0)
        return Fail(SP_E_PROTOCOL, response);
    return SP_OK;
}

int PolarityDriver::Fail(int code, const std::string& message)
{
    m_lastError = message;
    return code;
}

bool PolarityDriver::ParseInteger(const std::string& text, int& value)
{
    char* end = NULL;
    const long parsed = std::strtol(text.c_str(), &end, 10);
    if (end == text.c_str() || *end != '\0')
        return false;
    value = static_cast<int>(parsed);
    return true;
}

void PolarityDriver::CopyText(char* destination, size_t capacity, const std::string& text)
{
    if (capacity == 0)
        return;
    const size_t count = text.size() < capacity - 1 ? text.size() : capacity - 1;
    memcpy(destination, text.data(), count);
    destination[count] = '\0';
}
