#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <windows.h>
#include <string>
#include <vector>

struct SimulatedCoreResult
{
    int inputCore;
    int expectedOutput;
    int observedOutput;
    int lightDetected;
    std::string status;
};

class ScpiSession
{
public:
    explicit ScpiSession(SOCKET socket);
    void Run();

private:
    bool ReceiveLine(std::string& line);
    bool SendLine(const std::string& line);
    bool Execute(const std::string& originalCommand, std::string& response, bool& hasResponse);
    void UpdateTestState();
    void BuildResults();
    int ExpectedOutput(int inputCore) const;
    void SetError(int code, const std::string& message);

    SOCKET m_socket;
    std::string m_receiveBuffer;
    int m_coreCount;
    char m_polarityType;
    std::string m_profile;
    std::string m_state;
    unsigned long m_latencyMs;
    ULONGLONG m_completeAt;
    int m_errorCode;
    std::string m_errorMessage;
    std::vector<SimulatedCoreResult> m_results;
};
