#include "ScpiSession.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sstream>

namespace
{
    std::string Trim(const std::string& value)
    {
        size_t first = 0;
        while (first < value.size() && std::isspace(static_cast<unsigned char>(value[first])))
            ++first;
        size_t last = value.size();
        while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1])))
            --last;
        return value.substr(first, last - first);
    }

    std::string Upper(const std::string& value)
    {
        std::string result(value);
        for (size_t i = 0; i < result.size(); ++i)
            result[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(result[i])));
        return result;
    }

    bool StartsWith(const std::string& value, const char* prefix)
    {
        const std::string text(prefix);
        return value.size() >= text.size() && value.compare(0, text.size(), text) == 0;
    }
}

ScpiSession::ScpiSession(SOCKET socket)
    : m_socket(socket), m_coreCount(24), m_polarityType('A'), m_profile("PASS"),
      m_state("IDLE"), m_latencyMs(350), m_completeAt(0), m_errorCode(0)
{
}

void ScpiSession::Run()
{
    std::string line;
    while (ReceiveLine(line))
    {
        std::string response;
        bool hasResponse = false;
        Execute(line, response, hasResponse);
        if (hasResponse && !SendLine(response))
            break;
    }
    shutdown(m_socket, SD_BOTH);
    closesocket(m_socket);
}

bool ScpiSession::ReceiveLine(std::string& line)
{
    for (;;)
    {
        const size_t newline = m_receiveBuffer.find('\n');
        if (newline != std::string::npos)
        {
            line.assign(m_receiveBuffer, 0, newline);
            m_receiveBuffer.erase(0, newline + 1);
            if (!line.empty() && line[line.size() - 1] == '\r')
                line.erase(line.size() - 1);
            return true;
        }
        char buffer[2048];
        const int count = recv(m_socket, buffer, sizeof(buffer), 0);
        if (count <= 0)
            return false;
        m_receiveBuffer.append(buffer, static_cast<size_t>(count));
        if (m_receiveBuffer.size() > 64U * 1024U)
            return false;
    }
}

bool ScpiSession::SendLine(const std::string& line)
{
    const std::string wire = line + "\n";
    size_t sent = 0;
    while (sent < wire.size())
    {
        const int count = send(m_socket, wire.data() + sent,
                               static_cast<int>(wire.size() - sent), 0);
        if (count <= 0)
            return false;
        sent += static_cast<size_t>(count);
    }
    return true;
}

bool ScpiSession::Execute(const std::string& originalCommand,
                          std::string& response, bool& hasResponse)
{
    const std::string command = Upper(Trim(originalCommand));
    hasResponse = false;
    UpdateTestState();

    if (command == "*IDN?")
    {
        response = "MOLEX,SANTEC-POLARITY-SIM,SIM0001,2.0.0";
        hasResponse = true;
    }
    else if (command == "*CLS")
    {
        m_errorCode = 0;
        m_errorMessage.clear();
    }
    else if (command == "*RST")
    {
        m_coreCount = 24;
        m_polarityType = 'A';
        m_profile = "PASS";
        m_state = "IDLE";
        m_results.clear();
    }
    else if (command == "*OPC?")
    {
        response = "1";
        hasResponse = true;
    }
    else if (command == "SYST:ERR?" || command == "SYSTEM:ERROR?")
    {
        std::ostringstream text;
        text << m_errorCode << ",\"" << (m_errorCode == 0 ? "No error" : m_errorMessage) << "\"";
        response = text.str();
        hasResponse = true;
        m_errorCode = 0;
        m_errorMessage.clear();
    }
    else if (command == "LASER:DISABLE" || command == "LAS:DISAB")
    {
        // Accepted for compatibility. The simulator has no physical laser.
    }
    else if (StartsWith(command, "POLARITY:CONFIGURE:CORES "))
    {
        const int cores = std::atoi(command.substr(25).c_str());
        if (cores == 12 || cores == 24)
            m_coreCount = cores;
        else
            SetError(-222, "Core count must be 12 or 24");
    }
    else if (command == "POLARITY:CONFIGURE:CORES?")
    {
        std::ostringstream text;
        text << m_coreCount;
        response = text.str();
        hasResponse = true;
    }
    else if (StartsWith(command, "POLARITY:CONFIGURE:TYPE "))
    {
        const std::string argument = Trim(command.substr(24));
        if (argument == "A" || argument == "B" || argument == "C")
            m_polarityType = argument[0];
        else
            SetError(-222, "Polarity type must be A, B, or C");
    }
    else if (command == "POLARITY:CONFIGURE:TYPE?")
    {
        response.assign(1, m_polarityType);
        hasResponse = true;
    }
    else if (StartsWith(command, "POLARITY:SIMULATOR:PROFILE "))
    {
        const std::string argument = Trim(command.substr(27));
        if (argument == "PASS" || argument == "CROSS" || argument == "OPEN" ||
            argument == "SHORT" || argument == "REVERSE")
            m_profile = argument;
        else
            SetError(-222, "Unknown simulator profile");
    }
    else if (command == "POLARITY:SIMULATOR:PROFILE?")
    {
        response = m_profile;
        hasResponse = true;
    }
    else if (StartsWith(command, "POLARITY:SIMULATOR:LATENCY "))
    {
        const long latency = std::strtol(command.substr(27).c_str(), NULL, 10);
        if (latency >= 0 && latency <= 60000)
            m_latencyMs = static_cast<unsigned long>(latency);
        else
            SetError(-222, "Latency must be 0 through 60000 ms");
    }
    else if (command == "POLARITY:TEST:START")
    {
        BuildResults();
        m_state = "RUNNING";
        m_completeAt = GetTickCount64() + m_latencyMs;
    }
    else if (command == "POLARITY:TEST:ABORT")
    {
        if (m_state == "RUNNING")
            m_state = "CANCELLED";
    }
    else if (command == "POLARITY:TEST:STATUS?")
    {
        UpdateTestState();
        response = m_state;
        hasResponse = true;
    }
    else if (command == "POLARITY:RESULT:SUMMARY?")
    {
        UpdateTestState();
        if (m_state != "COMPLETE")
        {
            response = "ERR,-230,\"Test is not complete\"";
            hasResponse = true;
        }
        else
        {
            int passed = 0;
            for (size_t i = 0; i < m_results.size(); ++i)
                if (m_results[i].status == "PASS") ++passed;
            std::ostringstream text;
            text << (passed == m_coreCount ? "PASS" : "FAIL") << ','
                 << m_coreCount << ',' << passed << ',' << (m_coreCount - passed);
            response = text.str();
            hasResponse = true;
        }
    }
    else if (StartsWith(command, "POLARITY:RESULT:CORE? "))
    {
        UpdateTestState();
        const int core = std::atoi(command.substr(22).c_str());
        if (m_state != "COMPLETE")
            response = "ERR,-230,\"Test is not complete\"";
        else if (core < 1 || core > m_coreCount)
            response = "ERR,-222,\"Core index out of range\"";
        else
        {
            const SimulatedCoreResult& item = m_results[static_cast<size_t>(core - 1)];
            std::ostringstream text;
            text << item.inputCore << ',' << item.expectedOutput << ','
                 << item.observedOutput << ',' << item.lightDetected << ',' << item.status;
            response = text.str();
        }
        hasResponse = true;
    }
    else if (!command.empty())
    {
        SetError(-113, "Undefined header: " + command);
        if (command.find('?') != std::string::npos)
        {
            response = "ERR,-113,\"Undefined header\"";
            hasResponse = true;
        }
    }
    return true;
}

void ScpiSession::UpdateTestState()
{
    if (m_state == "RUNNING" && GetTickCount64() >= m_completeAt)
        m_state = "COMPLETE";
}

void ScpiSession::BuildResults()
{
    m_results.clear();
    for (int input = 1; input <= m_coreCount; ++input)
    {
        SimulatedCoreResult item;
        item.inputCore = input;
        item.expectedOutput = ExpectedOutput(input);
        item.observedOutput = item.expectedOutput;
        item.lightDetected = 1;
        item.status = "PASS";
        m_results.push_back(item);
    }

    if (m_profile == "CROSS" && m_coreCount >= 2)
    {
        std::swap(m_results[0].observedOutput, m_results[1].observedOutput);
        m_results[0].status = "CROSS";
        m_results[1].status = "CROSS";
    }
    else if (m_profile == "OPEN")
    {
        m_results[0].observedOutput = 0;
        m_results[0].lightDetected = 0;
        m_results[0].status = "OPEN";
    }
    else if (m_profile == "SHORT" && m_coreCount >= 2)
    {
        m_results[1].observedOutput = m_results[0].expectedOutput;
        m_results[1].status = "SHORT";
    }
    else if (m_profile == "REVERSE")
    {
        for (int i = 0; i < m_coreCount; ++i)
        {
            m_results[static_cast<size_t>(i)].observedOutput = m_coreCount - i;
            m_results[static_cast<size_t>(i)].status =
                (m_results[static_cast<size_t>(i)].observedOutput ==
                 m_results[static_cast<size_t>(i)].expectedOutput) ? "PASS" : "REVERSE";
        }
    }
}

int ScpiSession::ExpectedOutput(int inputCore) const
{
    if (m_polarityType == 'A')
        return inputCore;
    if (m_polarityType == 'B')
        return m_coreCount + 1 - inputCore;
    return (inputCore % 2 == 1) ? inputCore + 1 : inputCore - 1;
}

void ScpiSession::SetError(int code, const std::string& message)
{
    m_errorCode = code;
    m_errorMessage = message;
}
