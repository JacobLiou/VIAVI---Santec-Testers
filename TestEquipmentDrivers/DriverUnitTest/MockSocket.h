#pragma once

#include <string>
#include <vector>
#include <queue>
#include <functional>

// Mock TCP server that runs on localhost for testing driver communication.
// It listens on a specified port and responds to SCPI commands
// according to pre-configured response rules.
class CMockSocket
{
public:
    struct CommandResponse
    {
        std::string commandPrefix;   // Match if command starts with this
        std::string response;        // Response to send back (with \n appended)
    };

    CMockSocket();
    ~CMockSocket();

    // Start the mock server on localhost:port
    bool Start(int port);
    void Stop();
    int GetPort() const { return m_port; }

    // Add a canned response rule
    void AddResponse(const std::string& commandPrefix, const std::string& response);

    // Set a default response for unmatched commands
    void SetDefaultResponse(const std::string& response);

    // Get all commands received by the mock server
    std::vector<std::string> GetReceivedCommands() const;

    // Clear received commands
    void ClearReceivedCommands();

private:
    static DWORD WINAPI ServerThread(LPVOID param);
    void ServerLoop();
    std::string FindResponse(const std::string& command);

    SOCKET m_listenSocket;
    SOCKET m_clientSocket;
    int    m_port;
    bool   m_running;
    HANDLE m_thread;

    std::vector<CommandResponse> m_responses;
    std::string m_defaultResponse;
    std::vector<std::string> m_receivedCommands;
    CRITICAL_SECTION m_cs;
};
