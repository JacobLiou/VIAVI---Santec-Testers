#include "SantecPolarityDriver.h"
#include "PolarityDriver.h"

#include <cstring>
#include <new>
#include <string>

namespace
{
    PolarityDriver* AsDriver(SP_HANDLE handle)
    {
        return static_cast<PolarityDriver*>(handle);
    }

    int CopyToCaller(const std::string& text, char* buffer, unsigned int bufferSize)
    {
        if (buffer == NULL || bufferSize == 0)
            return SP_E_INVALID_ARGUMENT;
        if (text.size() + 1 > bufferSize)
            return SP_E_BUFFER_TOO_SMALL;
        memcpy(buffer, text.c_str(), text.size() + 1);
        return SP_OK;
    }
}

int SP_CALL SP_Create(SP_HANDLE* handle)
{
    if (handle == NULL)
        return SP_E_INVALID_ARGUMENT;
    *handle = NULL;
    PolarityDriver* driver = new (std::nothrow) PolarityDriver();
    if (driver == NULL)
        return SP_E_INVALID_HANDLE;
    *handle = driver;
    return SP_OK;
}

int SP_CALL SP_Destroy(SP_HANDLE handle)
{
    if (handle == NULL)
        return SP_E_INVALID_HANDLE;
    delete AsDriver(handle);
    return SP_OK;
}

int SP_CALL SP_Connect(SP_HANDLE handle, const char* host, unsigned short port,
                       unsigned long connectTimeoutMs, unsigned long ioTimeoutMs)
{
    return handle ? AsDriver(handle)->Connect(host, port, connectTimeoutMs, ioTimeoutMs) : SP_E_INVALID_HANDLE;
}

int SP_CALL SP_Disconnect(SP_HANDLE handle)
{
    return handle ? AsDriver(handle)->Disconnect() : SP_E_INVALID_HANDLE;
}

int SP_CALL SP_IsConnected(SP_HANDLE handle, int* connected)
{
    if (handle == NULL)
        return SP_E_INVALID_HANDLE;
    if (connected == NULL)
        return SP_E_INVALID_ARGUMENT;
    *connected = AsDriver(handle)->IsConnected() ? 1 : 0;
    return SP_OK;
}

int SP_CALL SP_GetIdentity(SP_HANDLE handle, char* buffer, unsigned int bufferSize)
{
    if (handle == NULL)
        return SP_E_INVALID_HANDLE;
    std::string identity;
    const int result = AsDriver(handle)->GetIdentity(identity);
    return result == SP_OK ? CopyToCaller(identity, buffer, bufferSize) : result;
}

int SP_CALL SP_Configure(SP_HANDLE handle, int coreCount, int polarityType)
{
    return handle ? AsDriver(handle)->Configure(coreCount, polarityType) : SP_E_INVALID_HANDLE;
}

int SP_CALL SP_StartTest(SP_HANDLE handle)
{
    return handle ? AsDriver(handle)->StartTest() : SP_E_INVALID_HANDLE;
}

int SP_CALL SP_GetTestState(SP_HANDLE handle, int* state)
{
    if (handle == NULL)
        return SP_E_INVALID_HANDLE;
    if (state == NULL)
        return SP_E_INVALID_ARGUMENT;
    return AsDriver(handle)->GetTestState(*state);
}

int SP_CALL SP_GetSummary(SP_HANDLE handle, SP_TestSummary* summary)
{
    if (handle == NULL)
        return SP_E_INVALID_HANDLE;
    if (summary == NULL || summary->size != sizeof(SP_TestSummary))
        return SP_E_INVALID_ARGUMENT;
    return AsDriver(handle)->GetSummary(*summary);
}

int SP_CALL SP_GetCoreResult(SP_HANDLE handle, int inputCore, SP_CoreResult* result)
{
    if (handle == NULL)
        return SP_E_INVALID_HANDLE;
    if (result == NULL || result->size != sizeof(SP_CoreResult))
        return SP_E_INVALID_ARGUMENT;
    return AsDriver(handle)->GetCoreResult(inputCore, *result);
}

int SP_CALL SP_RunTest(SP_HANDLE handle, int coreCount, int polarityType,
                       unsigned long overallTimeoutMs, SP_TestSummary* summary,
                       SP_CoreResult* coreResults, int coreResultCapacity)
{
    if (handle == NULL)
        return SP_E_INVALID_HANDLE;
    if (summary == NULL || summary->size != sizeof(SP_TestSummary))
        return SP_E_INVALID_ARGUMENT;
    if (coreResults == NULL || coreResultCapacity < coreCount)
        return SP_E_INVALID_ARGUMENT;
    return AsDriver(handle)->RunTest(coreCount, polarityType, overallTimeoutMs,
                                     *summary, coreResults, coreResultCapacity);
}

int SP_CALL SP_Cancel(SP_HANDLE handle)
{
    return handle ? AsDriver(handle)->Cancel() : SP_E_INVALID_HANDLE;
}

int SP_CALL SP_GetLastError(SP_HANDLE handle, char* buffer, unsigned int bufferSize)
{
    if (handle == NULL)
        return SP_E_INVALID_HANDLE;
    return CopyToCaller(AsDriver(handle)->LastError(), buffer, bufferSize);
}
