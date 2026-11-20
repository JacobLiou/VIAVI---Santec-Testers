#pragma once

#include "ScpiTcpClient.h"
#include "SantecPolarityDriver.h"

#include <windows.h>
#include <string>

class PolarityDriver
{
public:
    PolarityDriver();
    ~PolarityDriver();

    int Connect(const char* host, unsigned short port, unsigned long connectTimeoutMs,
                unsigned long ioTimeoutMs);
    int Disconnect();
    bool IsConnected();
    int GetIdentity(std::string& identity);
    int Configure(int coreCount, int polarityType);
    int StartTest();
    int GetTestState(int& state);
    int GetSummary(SP_TestSummary& summary);
    int GetCoreResult(int inputCore, SP_CoreResult& result);
    int RunTest(int coreCount, int polarityType, unsigned long overallTimeoutMs,
                SP_TestSummary& summary, SP_CoreResult* coreResults, int capacity);
    int Cancel();
    std::string LastError();

private:
    PolarityDriver(const PolarityDriver&);
    PolarityDriver& operator=(const PolarityDriver&);

    int WriteLocked(const std::string& command);
    int QueryLocked(const std::string& command, std::string& response);
    int Fail(int code, const std::string& message);
    static bool ParseInteger(const std::string& text, int& value);
    static void CopyText(char* destination, size_t capacity, const std::string& text);

    CRITICAL_SECTION m_lock;
    ScpiTcpClient m_client;
    std::string m_lastError;
    volatile LONG m_cancelRequested;
};
