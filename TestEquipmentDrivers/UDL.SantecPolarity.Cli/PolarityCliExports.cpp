#include "PolarityCliSession.h"
#include <new>

extern "C" {

int __stdcall SP_Cli_Create(SP_CLI_HANDLE* handle)
{
    if (!handle)
        return -1;
    PolarityCliSession* s = new (std::nothrow) PolarityCliSession();
    if (!s)
        return -100;
    *handle = s;
    return 0;
}

int __stdcall SP_Cli_Destroy(SP_CLI_HANDLE handle)
{
    PolarityCliSession* s = static_cast<PolarityCliSession*>(handle);
    if (!s)
        return -1;
    delete s;
    return 0;
}

int __stdcall SP_Cli_Open(SP_CLI_HANDLE handle, const SP_CLI_OPEN_PARAMS* params)
{
    PolarityCliSession* s = static_cast<PolarityCliSession*>(handle);
    if (!s)
        return -1;
    return s->Open(params);
}

int __stdcall SP_Cli_Close(SP_CLI_HANDLE handle)
{
    PolarityCliSession* s = static_cast<PolarityCliSession*>(handle);
    if (!s)
        return -1;
    return s->Close();
}

int __stdcall SP_Cli_GetSnapshot(SP_CLI_HANDLE handle, SP_CLI_SNAPSHOT* snapshot)
{
    PolarityCliSession* s = static_cast<PolarityCliSession*>(handle);
    if (!s)
        return -1;
    return s->GetSnapshot(snapshot);
}

int __stdcall SP_Cli_RunMapping(SP_CLI_HANDLE handle, const int32_t* expectedMap, uint32_t expectedCount)
{
    PolarityCliSession* s = static_cast<PolarityCliSession*>(handle);
    if (!s)
        return -1;
    return s->RunMapping(expectedMap, expectedCount);
}

int __stdcall SP_Cli_Cancel(SP_CLI_HANDLE handle)
{
    PolarityCliSession* s = static_cast<PolarityCliSession*>(handle);
    if (!s)
        return -1;
    return s->Cancel();
}

int __stdcall SP_Cli_GetSummary(SP_CLI_HANDLE handle, SP_CLI_SUMMARY* summary)
{
    PolarityCliSession* s = static_cast<PolarityCliSession*>(handle);
    if (!s)
        return -1;
    return s->GetSummary(summary);
}

int __stdcall SP_Cli_GetFiberCount(SP_CLI_HANDLE handle, uint32_t* count)
{
    PolarityCliSession* s = static_cast<PolarityCliSession*>(handle);
    if (!s)
        return -1;
    return s->GetFiberCount(count);
}

int __stdcall SP_Cli_GetFibers(SP_CLI_HANDLE handle, SP_CLI_FIBER_RESULT* buffer, uint32_t capacity, uint32_t* written)
{
    PolarityCliSession* s = static_cast<PolarityCliSession*>(handle);
    if (!s)
        return -1;
    return s->GetFibers(buffer, capacity, written);
}

int __stdcall SP_Cli_GetDiagnosticPath(SP_CLI_HANDLE handle, uint32_t index, char* buffer, uint32_t capacity, uint32_t* required)
{
    PolarityCliSession* s = static_cast<PolarityCliSession*>(handle);
    if (!s)
        return -1;
    return s->GetDiagnosticPath(index, buffer, capacity, required);
}

int __stdcall SP_Cli_GetLastError(SP_CLI_HANDLE handle, char* buffer, uint32_t capacity, uint32_t* required)
{
    PolarityCliSession* s = static_cast<PolarityCliSession*>(handle);
    if (!s)
        return -1;
    return s->GetLastErrorMsg(buffer, capacity, required);
}

} // extern "C"
