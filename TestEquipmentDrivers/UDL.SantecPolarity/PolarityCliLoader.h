#pragma once

#include "..\Common\PolarityCliApi.h"
#include <Windows.h>

// Dynamically loads UDL.SantecPolarity.Cli.dll next to this module.

class CPolarityCliLoader
{
public:
    typedef int (__stdcall *Fn_Create)(SP_CLI_HANDLE*);
    typedef int (__stdcall *Fn_Destroy)(SP_CLI_HANDLE);
    typedef int (__stdcall *Fn_Open)(SP_CLI_HANDLE, const SP_CLI_OPEN_PARAMS*);
    typedef int (__stdcall *Fn_Close)(SP_CLI_HANDLE);
    typedef int (__stdcall *Fn_GetSnapshot)(SP_CLI_HANDLE, SP_CLI_SNAPSHOT*);
    typedef int (__stdcall *Fn_RunMapping)(SP_CLI_HANDLE, const int32_t*, uint32_t);
    typedef int (__stdcall *Fn_Cancel)(SP_CLI_HANDLE);
    typedef int (__stdcall *Fn_GetSummary)(SP_CLI_HANDLE, SP_CLI_SUMMARY*);
    typedef int (__stdcall *Fn_GetFiberCount)(SP_CLI_HANDLE, uint32_t*);
    typedef int (__stdcall *Fn_GetFibers)(SP_CLI_HANDLE, SP_CLI_FIBER_RESULT*, uint32_t, uint32_t*);
    typedef int (__stdcall *Fn_GetDiagnosticPath)(SP_CLI_HANDLE, uint32_t, char*, uint32_t, uint32_t*);
    typedef int (__stdcall *Fn_GetLastErrorMsg)(SP_CLI_HANDLE, char*, uint32_t, uint32_t*);

    CPolarityCliLoader();
    ~CPolarityCliLoader();

    bool Load();
    void Unload();
    bool IsLoaded() const { return m_module != NULL; }
    const char* LoaderError() const { return m_error; }

    Fn_Create Create;
    Fn_Destroy Destroy;
    Fn_Open Open;
    Fn_Close Close;
    Fn_GetSnapshot GetSnapshot;
    Fn_RunMapping RunMapping;
    Fn_Cancel Cancel;
    Fn_GetSummary GetSummary;
    Fn_GetFiberCount GetFiberCount;
    Fn_GetFibers GetFibers;
    Fn_GetDiagnosticPath GetDiagnosticPath;
    Fn_GetLastErrorMsg GetLastErrorMsg;

private:
    bool Resolve(const char* name, FARPROC* out);
    void ClearFns();

    HMODULE m_module;
    char m_error[256];
};
