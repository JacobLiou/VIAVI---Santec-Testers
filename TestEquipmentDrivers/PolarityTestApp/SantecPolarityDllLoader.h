#pragma once

#include <Windows.h>
#include <stdint.h>
#include <string>
#include <vector>

#include "../UDL.SantecPolarity/SPTypes.h"

typedef SP_STATUS (__stdcall *PFN_SP_Create)(SP_HANDLE*);
typedef SP_STATUS (__stdcall *PFN_SP_Destroy)(SP_HANDLE);
typedef SP_STATUS (__stdcall *PFN_SP_Open)(SP_HANDLE, const SP_OPEN_OPTIONS*);
typedef SP_STATUS (__stdcall *PFN_SP_Close)(SP_HANDLE);
typedef SP_STATUS (__stdcall *PFN_SP_GetHardwareSnapshot)(SP_HANDLE, SP_HARDWARE_SNAPSHOT*);
typedef SP_STATUS (__stdcall *PFN_SP_RunMapping)(SP_HANDLE);
typedef SP_STATUS (__stdcall *PFN_SP_Cancel)(SP_HANDLE);
typedef SP_STATUS (__stdcall *PFN_SP_GetSummary)(SP_HANDLE, SP_RESULT_SUMMARY*);
typedef SP_STATUS (__stdcall *PFN_SP_GetFiberResultCount)(SP_HANDLE, uint32_t*);
typedef SP_STATUS (__stdcall *PFN_SP_GetFiberResults)(SP_HANDLE, SP_FIBER_RESULT*, uint32_t, uint32_t*);
typedef SP_STATUS (__stdcall *PFN_SP_GetDiagnosticPathUtf8)(SP_HANDLE, uint32_t, char*, uint32_t, uint32_t*);
typedef SP_STATUS (__stdcall *PFN_SP_GetLastErrorUtf8)(SP_HANDLE, char*, uint32_t, uint32_t*);

class CSantecPolarityDllLoader
{
public:
    CSantecPolarityDllLoader()
        : m_hDll(NULL), m_hDriver(NULL)
        , pfnCreate(NULL), pfnDestroy(NULL), pfnOpen(NULL), pfnClose(NULL)
        , pfnGetHardwareSnapshot(NULL), pfnRunMapping(NULL), pfnCancel(NULL)
        , pfnGetSummary(NULL), pfnGetFiberResultCount(NULL), pfnGetFiberResults(NULL)
        , pfnGetDiagnosticPathUtf8(NULL), pfnGetLastErrorUtf8(NULL)
    {}

    ~CSantecPolarityDllLoader()
    {
        Destroy();
        UnloadDll();
    }

    bool LoadDll(const char* dllPath)
    {
        if (m_hDll) UnloadDll();
        m_hDll = LoadLibraryA(dllPath);
        if (!m_hDll) return false;

#define LOAD(name) pfn##name = (PFN_SP_##name)GetProcAddress(m_hDll, "SP_" #name)
        LOAD(Create);
        LOAD(Destroy);
        LOAD(Open);
        LOAD(Close);
        LOAD(GetHardwareSnapshot);
        LOAD(RunMapping);
        LOAD(Cancel);
        LOAD(GetSummary);
        LOAD(GetFiberResultCount);
        LOAD(GetFiberResults);
        LOAD(GetDiagnosticPathUtf8);
        LOAD(GetLastErrorUtf8);
#undef LOAD

        if (!pfnCreate || !pfnDestroy || !pfnOpen || !pfnClose ||
            !pfnGetHardwareSnapshot || !pfnRunMapping || !pfnCancel ||
            !pfnGetSummary || !pfnGetFiberResultCount || !pfnGetFiberResults ||
            !pfnGetLastErrorUtf8)
        {
            UnloadDll();
            return false;
        }
        return true;
    }

    void UnloadDll()
    {
        Destroy();
        if (m_hDll)
        {
            FreeLibrary(m_hDll);
            m_hDll = NULL;
        }
        pfnCreate = NULL; pfnDestroy = NULL; pfnOpen = NULL; pfnClose = NULL;
        pfnGetHardwareSnapshot = NULL; pfnRunMapping = NULL; pfnCancel = NULL;
        pfnGetSummary = NULL; pfnGetFiberResultCount = NULL; pfnGetFiberResults = NULL;
        pfnGetDiagnosticPathUtf8 = NULL; pfnGetLastErrorUtf8 = NULL;
    }

    bool IsLoaded() const { return m_hDll != NULL; }
    bool HasHandle() const { return m_hDriver != NULL; }

    SP_STATUS Create()
    {
        if (!pfnCreate) return SP_E_INVALID_STATE;
        Destroy();
        return pfnCreate(&m_hDriver);
    }

    SP_STATUS Destroy()
    {
        if (!m_hDriver) return SP_OK;
        SP_STATUS st = SP_OK;
        if (pfnClose) pfnClose(m_hDriver);
        if (pfnDestroy) st = pfnDestroy(m_hDriver);
        m_hDriver = NULL;
        return st;
    }

    SP_STATUS Open(const SP_OPEN_OPTIONS* options)
    {
        if (!pfnOpen || !m_hDriver) return SP_E_INVALID_STATE;
        return pfnOpen(m_hDriver, options);
    }

    SP_STATUS Close()
    {
        if (!pfnClose || !m_hDriver) return SP_E_INVALID_STATE;
        return pfnClose(m_hDriver);
    }

    SP_STATUS GetHardwareSnapshot(SP_HARDWARE_SNAPSHOT* snapshot)
    {
        if (!pfnGetHardwareSnapshot || !m_hDriver) return SP_E_INVALID_STATE;
        return pfnGetHardwareSnapshot(m_hDriver, snapshot);
    }

    SP_STATUS RunMapping()
    {
        if (!pfnRunMapping || !m_hDriver) return SP_E_INVALID_STATE;
        return pfnRunMapping(m_hDriver);
    }

    SP_STATUS Cancel()
    {
        if (!pfnCancel || !m_hDriver) return SP_E_INVALID_STATE;
        return pfnCancel(m_hDriver);
    }

    SP_STATUS GetSummary(SP_RESULT_SUMMARY* summary)
    {
        if (!pfnGetSummary || !m_hDriver) return SP_E_INVALID_STATE;
        return pfnGetSummary(m_hDriver, summary);
    }

    SP_STATUS GetFiberResultCount(uint32_t* count)
    {
        if (!pfnGetFiberResultCount || !m_hDriver) return SP_E_INVALID_STATE;
        return pfnGetFiberResultCount(m_hDriver, count);
    }

    SP_STATUS GetFiberResults(SP_FIBER_RESULT* buffer, uint32_t capacity, uint32_t* written)
    {
        if (!pfnGetFiberResults || !m_hDriver) return SP_E_INVALID_STATE;
        return pfnGetFiberResults(m_hDriver, buffer, capacity, written);
    }

    std::string GetLastErrorUtf8()
    {
        if (!pfnGetLastErrorUtf8 || !m_hDriver) return std::string();
        uint32_t required = 0;
        pfnGetLastErrorUtf8(m_hDriver, NULL, 0, &required);
        if (required == 0) return std::string();
        std::vector<char> buf(required + 1, 0);
        uint32_t unused = 0;
        pfnGetLastErrorUtf8(m_hDriver, buf.data(), required, &unused);
        return std::string(buf.data());
    }

private:
    HMODULE m_hDll;
    SP_HANDLE m_hDriver;
    PFN_SP_Create pfnCreate;
    PFN_SP_Destroy pfnDestroy;
    PFN_SP_Open pfnOpen;
    PFN_SP_Close pfnClose;
    PFN_SP_GetHardwareSnapshot pfnGetHardwareSnapshot;
    PFN_SP_RunMapping pfnRunMapping;
    PFN_SP_Cancel pfnCancel;
    PFN_SP_GetSummary pfnGetSummary;
    PFN_SP_GetFiberResultCount pfnGetFiberResultCount;
    PFN_SP_GetFiberResults pfnGetFiberResults;
    PFN_SP_GetDiagnosticPathUtf8 pfnGetDiagnosticPathUtf8;
    PFN_SP_GetLastErrorUtf8 pfnGetLastErrorUtf8;
};
