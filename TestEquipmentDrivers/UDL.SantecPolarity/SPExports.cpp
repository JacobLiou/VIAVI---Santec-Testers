#include "stdafx.h"
#include "SPExports.h"
#include "PolaritySession.h"
#include <new>

namespace {

CPolaritySession* AsSession(SP_HANDLE handle)
{
    return reinterpret_cast<CPolaritySession*>(handle);
}

} // namespace

SP_STATUS SP_CALL SP_Create(SP_HANDLE* handle)
{
    if (!handle)
        return SP_E_INVALID_ARGUMENT;
    *handle = new (std::nothrow) CPolaritySession();
    if (!*handle)
        return SP_E_INTERNAL;
    return SP_OK;
}

SP_STATUS SP_CALL SP_Destroy(SP_HANDLE handle)
{
    CPolaritySession* session = AsSession(handle);
    if (!session)
        return SP_E_INVALID_ARGUMENT;
    delete session;
    return SP_OK;
}

SP_STATUS SP_CALL SP_Open(SP_HANDLE handle, const SP_OPEN_OPTIONS* options)
{
    CPolaritySession* session = AsSession(handle);
    if (!session)
        return SP_E_INVALID_ARGUMENT;
    return session->Open(options);
}

SP_STATUS SP_CALL SP_GetHardwareSnapshot(SP_HANDLE handle, SP_HARDWARE_SNAPSHOT* snapshot)
{
    CPolaritySession* session = AsSession(handle);
    if (!session)
        return SP_E_INVALID_ARGUMENT;
    return session->GetHardwareSnapshot(snapshot);
}

SP_STATUS SP_CALL SP_RunMapping(SP_HANDLE handle)
{
    CPolaritySession* session = AsSession(handle);
    if (!session)
        return SP_E_INVALID_ARGUMENT;
    return session->RunMapping();
}

SP_STATUS SP_CALL SP_Cancel(SP_HANDLE handle)
{
    CPolaritySession* session = AsSession(handle);
    if (!session)
        return SP_E_INVALID_ARGUMENT;
    return session->Cancel();
}

SP_STATUS SP_CALL SP_GetSummary(SP_HANDLE handle, SP_RESULT_SUMMARY* summary)
{
    CPolaritySession* session = AsSession(handle);
    if (!session)
        return SP_E_INVALID_ARGUMENT;
    return session->GetSummary(summary);
}

SP_STATUS SP_CALL SP_GetFiberResultCount(SP_HANDLE handle, uint32_t* count)
{
    CPolaritySession* session = AsSession(handle);
    if (!session)
        return SP_E_INVALID_ARGUMENT;
    return session->GetFiberResultCount(count);
}

SP_STATUS SP_CALL SP_GetFiberResults(
    SP_HANDLE handle, SP_FIBER_RESULT* buffer, uint32_t capacity, uint32_t* written)
{
    CPolaritySession* session = AsSession(handle);
    if (!session)
        return SP_E_INVALID_ARGUMENT;
    return session->GetFiberResults(buffer, capacity, written);
}

SP_STATUS SP_CALL SP_GetDiagnosticPathUtf8(
    SP_HANDLE handle, uint32_t index, char* buffer, uint32_t capacity, uint32_t* required)
{
    CPolaritySession* session = AsSession(handle);
    if (!session)
        return SP_E_INVALID_ARGUMENT;
    return session->GetDiagnosticPathUtf8(index, buffer, capacity, required);
}

SP_STATUS SP_CALL SP_GetLastErrorUtf8(
    SP_HANDLE handle, char* buffer, uint32_t capacity, uint32_t* required)
{
    CPolaritySession* session = AsSession(handle);
    if (!session)
        return SP_E_INVALID_ARGUMENT;
    return session->GetLastErrorUtf8(buffer, capacity, required);
}

SP_STATUS SP_CALL SP_Close(SP_HANDLE handle)
{
    CPolaritySession* session = AsSession(handle);
    if (!session)
        return SP_E_INVALID_ARGUMENT;
    return session->Close();
}
