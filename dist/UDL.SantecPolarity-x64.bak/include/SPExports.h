#pragma once

#include "SPTypes.h"

// Exports are declared via UDL.SantecPolarity.def (avoid LNK4197 double-export).
#ifdef UDLSANTECPOLARITY_EXPORTS
#define SP_C_API extern "C"
#else
#define SP_C_API extern "C" __declspec(dllimport)
#endif

#define SP_CALL __stdcall

SP_C_API SP_STATUS SP_CALL SP_Create(SP_HANDLE* handle);
SP_C_API SP_STATUS SP_CALL SP_Destroy(SP_HANDLE handle);
SP_C_API SP_STATUS SP_CALL SP_Open(SP_HANDLE handle, const SP_OPEN_OPTIONS* options);
SP_C_API SP_STATUS SP_CALL SP_GetHardwareSnapshot(
    SP_HANDLE handle, SP_HARDWARE_SNAPSHOT* snapshot);
SP_C_API SP_STATUS SP_CALL SP_RunMapping(SP_HANDLE handle);
SP_C_API SP_STATUS SP_CALL SP_Cancel(SP_HANDLE handle);
SP_C_API SP_STATUS SP_CALL SP_GetSummary(
    SP_HANDLE handle, SP_RESULT_SUMMARY* summary);
SP_C_API SP_STATUS SP_CALL SP_GetFiberResultCount(
    SP_HANDLE handle, uint32_t* count);
SP_C_API SP_STATUS SP_CALL SP_GetFiberResults(
    SP_HANDLE handle, SP_FIBER_RESULT* buffer,
    uint32_t capacity, uint32_t* written);
SP_C_API SP_STATUS SP_CALL SP_GetDiagnosticPathUtf8(
    SP_HANDLE handle, uint32_t index, char* buffer,
    uint32_t capacity, uint32_t* required);
SP_C_API SP_STATUS SP_CALL SP_GetLastErrorUtf8(
    SP_HANDLE handle, char* buffer,
    uint32_t capacity, uint32_t* required);
SP_C_API SP_STATUS SP_CALL SP_Close(SP_HANDLE handle);
