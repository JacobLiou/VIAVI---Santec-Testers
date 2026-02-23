#include "stdafx.h"
#include "MockSocket.h"

CMockSocket::CMockSocket()
    : m_listenSocket(INVALID_SOCKET)
    , m_clientSocket(INVALID_SOCKET)
    , m_port(0)
    , m_running(false)
    , m_thread(NULL)
    , m_defaultResponse("0,No error")
{
    InitializeCriticalSection(&m_cs);
}

CMockSocket::~CMockSocket()
{
    Stop();
    DeleteCriticalSection(&m_cs);
}

bool CMockSocket::Start(int port)
{
    m_port = port;
    m_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_listenSocket == INVALID_SOCKET) return false;

    // Allow port reuse
    int opt = 1;
    setsockopt(m_listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(static_cast<u_short>(port));

    if (bind(m_listenSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
        return false;
    }

    if (listen(m_listenSocket, 1) == SOCKET_ERROR)
    {
        closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
        return false;
    }

    m_running = true;
    m_thread = CreateThread(NULL, 0, ServerThread, this, 0, NULL);
    return m_thread != NULL;
}

void CMockSocket::Stop()
{
    m_running = false;
    if (m_clientSocket != INVALID_SOCKET)
    {
        closesocket(m_clientSocket);
        m_clientSocket = INVALID_SOCKET;
    }
    if (m_listenSocket != INVALID_SOCKET)
    {
        closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
    }
    if (m_thread)
    {
        WaitForSingleObject(m_thread, 3000);
        CloseHandle(m_thread);
        m_thread = NULL;
    }
}

void CMockSocket::AddResponse(const std::string& commandPrefix, const std::string& response)
{
    CommandResponse cr;
    cr.commandPrefix = commandPrefix;
    cr.response = response;
    m_responses.push_back(cr);
}

void CMockSocket::SetDefaultResponse(const std::string& response)
{
    m_defaultResponse = response;
}

std::vector<std::string> CMockSocket::GetReceivedCommands() const
{
    return m_receivedCommands;
}

void CMockSocket::ClearReceivedCommands()
{
    EnterCriticalSection(&m_cs);
    m_receivedCommands.clear();
    LeaveCriticalSection(&m_cs);
}

DWORD WINAPI CMockSocket::ServerThread(LPVOID param)
{
    CMockSocket* self = reinterpret_cast<CMockSocket*>(param);
    self->ServerLoop();
    return 0;
}

void CMockSocket::ServerLoop()
{
    // Set accept timeout
    DWORD timeout = 500;
    setsockopt(m_listenSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    while (m_running)
    {
        m_clientSocket = accept(m_listenSocket, NULL, NULL);
        if (m_clientSocket == INVALID_SOCKET)
            continue;

        DWORD clientTimeout = 100;
        setsockopt(m_clientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&clientTimeout, sizeof(clientTimeout));

        while (m_running)
        {
            char buf[2048];
            int received = recv(m_clientSocket, buf, sizeof(buf) - 1, 0);
            if (received <= 0) break;
            buf[received] = '\0';

            std::string data(buf, received);

            // Commands may arrive concatenated; split by newline
            size_t pos = 0;
            while (pos < data.size())
            {
                size_t nlPos = data.find('\n', pos);
                std::string cmd;
                if (nlPos != std::string::npos)
                {
                    cmd = data.substr(pos, nlPos - pos);
                    pos = nlPos + 1;
                }
                else
                {
                    cmd = data.substr(pos);
                    pos = data.size();
                }

                // Trim
                while (!cmd.empty() && (cmd.back() == '\r' || cmd.back() == '\n'))
                    cmd.pop_back();
                if (cmd.empty()) continue;

                EnterCriticalSection(&m_cs);
                m_receivedCommands.push_back(cmd);
                LeaveCriticalSection(&m_cs);

                // Only respond to query commands (contain '?')
                if (cmd.find('?') != std::string::npos)
                {
                    std::string resp = FindResponse(cmd) + "\n";
                    send(m_clientSocket, resp.c_str(), static_cast<int>(resp.size()), 0);
                }
            }
        }

        if (m_clientSocket != INVALID_SOCKET)
        {
            closesocket(m_clientSocket);
            m_clientSocket = INVALID_SOCKET;
        }
    }
}

std::string CMockSocket::FindResponse(const std::string& command)
{
    for (size_t i = 0; i < m_responses.size(); ++i)
    {
        if (command.find(m_responses[i].commandPrefix) == 0)
            return m_responses[i].response;
    }
    return m_defaultResponse;
}
