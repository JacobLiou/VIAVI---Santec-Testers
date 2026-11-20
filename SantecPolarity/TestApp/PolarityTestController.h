#pragma once

#include "ScpiTcpClient.h"

#include <windows.h>
#include <string>
#include <vector>

struct AppCoreResult
{
    int inputCore;
    int expectedOutput;
    int observedOutput;
    int lightDetected;
    std::string status;
};

struct AppTestResult
{
    bool communicationSucceeded;
    bool passed;
    std::string summaryStatus;
    std::string error;
    std::vector<AppCoreResult> cores;
};

class PolarityTestController
{
public:
    PolarityTestController();
    ~PolarityTestController();

    bool Connect(const std::string& host, unsigned short port, std::string& identity, std::string& error);
    void Disconnect();
    bool IsConnected();
    AppTestResult RunTest(int coreCount, char polarityType, const std::string& simulatorProfile,
                          unsigned long overallTimeoutMs);
    void RequestCancel();

private:
    PolarityTestController(const PolarityTestController&);
    PolarityTestController& operator=(const PolarityTestController&);

    bool Write(const std::string& command, std::string& error);
    bool Query(const std::string& command, std::string& response, std::string& error);
    static std::vector<std::string> SplitCsv(const std::string& value);
    static bool ParseInteger(const std::string& text, int& value);

    CRITICAL_SECTION m_lock;
    ScpiTcpClient m_client;
    volatile LONG m_cancelRequested;
};
