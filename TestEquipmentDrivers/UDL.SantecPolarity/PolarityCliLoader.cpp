#include "stdafx.h"
#include "PolarityCliLoader.h"
#include <cstring>

namespace {

void SetErr(char* buf, size_t n, const char* msg)
{
    if (!buf || n == 0)
        return;
    buf[0] = '\0';
    if (!msg)
        return;
    strncpy_s(buf, n, msg, _TRUNCATE);
}

// Free-function anchor for GetModuleHandleExW (member function pointers are not valid here).
void ModuleAnchor() {}

} // namespace

CPolarityCliLoader::CPolarityCliLoader()
    : Create(NULL)
    , Destroy(NULL)
    , Open(NULL)
    , Close(NULL)
    , GetSnapshot(NULL)
    , RunMapping(NULL)
    , Cancel(NULL)
    , GetSummary(NULL)
    , GetFiberCount(NULL)
    , GetFibers(NULL)
    , GetDiagnosticPath(NULL)
    , GetLastErrorMsg(NULL)
    , m_module(NULL)
{
    m_error[0] = '\0';
}

CPolarityCliLoader::~CPolarityCliLoader()
{
    Unload();
}

void CPolarityCliLoader::ClearFns()
{
    Create = NULL;
    Destroy = NULL;
    Open = NULL;
    Close = NULL;
    GetSnapshot = NULL;
    RunMapping = NULL;
    Cancel = NULL;
    GetSummary = NULL;
    GetFiberCount = NULL;
    GetFibers = NULL;
    GetDiagnosticPath = NULL;
    GetLastErrorMsg = NULL;
}

bool CPolarityCliLoader::Resolve(const char* name, FARPROC* out)
{
    *out = ::GetProcAddress(m_module, name);
    if (!*out)
    {
        SetErr(m_error, sizeof(m_error), "GetProcAddress failed for Cli export");
        return false;
    }
    return true;
}

bool CPolarityCliLoader::Load()
{
    if (m_module)
        return true;

    wchar_t path[MAX_PATH];
    HMODULE self = NULL;
    if (!GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(reinterpret_cast<void*>(&ModuleAnchor)),
            &self))
    {
        SetErr(m_error, sizeof(m_error), "GetModuleHandleExW failed");
        return false;
    }
    const DWORD n = GetModuleFileNameW(self, path, MAX_PATH);
    if (n == 0 || n >= MAX_PATH)
    {
        SetErr(m_error, sizeof(m_error), "GetModuleFileNameW failed");
        return false;
    }
    wchar_t* slash = wcsrchr(path, L'\\');
    if (!slash)
    {
        SetErr(m_error, sizeof(m_error), "Invalid module path");
        return false;
    }
    *(slash + 1) = L'\0';
    wcscat_s(path, L"UDL.SantecPolarity.Cli.dll");

    m_module = LoadLibraryW(path);
    if (!m_module)
    {
        SetErr(m_error, sizeof(m_error),
               "LoadLibrary UDL.SantecPolarity.Cli.dll failed (place Cli + Hardware deps beside native DLL)");
        return false;
    }

    FARPROC p = NULL;
    if (!Resolve("SP_Cli_Create", &p)) { Unload(); return false; }
    Create = reinterpret_cast<Fn_Create>(p);
    if (!Resolve("SP_Cli_Destroy", &p)) { Unload(); return false; }
    Destroy = reinterpret_cast<Fn_Destroy>(p);
    if (!Resolve("SP_Cli_Open", &p)) { Unload(); return false; }
    Open = reinterpret_cast<Fn_Open>(p);
    if (!Resolve("SP_Cli_Close", &p)) { Unload(); return false; }
    Close = reinterpret_cast<Fn_Close>(p);
    if (!Resolve("SP_Cli_GetSnapshot", &p)) { Unload(); return false; }
    GetSnapshot = reinterpret_cast<Fn_GetSnapshot>(p);
    if (!Resolve("SP_Cli_RunMapping", &p)) { Unload(); return false; }
    RunMapping = reinterpret_cast<Fn_RunMapping>(p);
    if (!Resolve("SP_Cli_Cancel", &p)) { Unload(); return false; }
    Cancel = reinterpret_cast<Fn_Cancel>(p);
    if (!Resolve("SP_Cli_GetSummary", &p)) { Unload(); return false; }
    GetSummary = reinterpret_cast<Fn_GetSummary>(p);
    if (!Resolve("SP_Cli_GetFiberCount", &p)) { Unload(); return false; }
    GetFiberCount = reinterpret_cast<Fn_GetFiberCount>(p);
    if (!Resolve("SP_Cli_GetFibers", &p)) { Unload(); return false; }
    GetFibers = reinterpret_cast<Fn_GetFibers>(p);
    if (!Resolve("SP_Cli_GetDiagnosticPath", &p)) { Unload(); return false; }
    GetDiagnosticPath = reinterpret_cast<Fn_GetDiagnosticPath>(p);
    if (!Resolve("SP_Cli_GetLastError", &p)) { Unload(); return false; }
    GetLastErrorMsg = reinterpret_cast<Fn_GetLastErrorMsg>(p);

    SetErr(m_error, sizeof(m_error), "");
    return true;
}

void CPolarityCliLoader::Unload()
{
    ClearFns();
    if (m_module)
    {
        FreeLibrary(m_module);
        m_module = NULL;
    }
}
