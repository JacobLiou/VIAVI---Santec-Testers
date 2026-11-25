#pragma once

// Cross-driver exclusive lock for one RLM-100 station.
// Mutex name: Global\UDL_SANTEC_RLM100_<serial>
// Used by UDL.SantecPolarity (and optionally UDL.SantecRLM) to avoid dual-owner.

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <string>

namespace SantecRlmLock {

inline std::wstring BuildMutexName(const char* rlmSerialUtf8)
{
    std::wstring name = L"Global\\UDL_SANTEC_RLM100_";
    if (!rlmSerialUtf8 || !*rlmSerialUtf8)
    {
        name += L"UNKNOWN";
        return name;
    }
    for (const char* p = rlmSerialUtf8; *p; ++p)
    {
        const unsigned char c = static_cast<unsigned char>(*p);
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_')
            name.push_back(static_cast<wchar_t>(c));
        else
            name.push_back(L'_');
    }
    return name;
}

class Guard
{
public:
    Guard()
        : m_mutex(NULL)
        , m_owned(false)
    {
    }

    ~Guard()
    {
        Release();
    }

    // Returns true if acquired; false if busy or create failed.
    bool TryAcquire(const char* rlmSerialUtf8, DWORD timeoutMs = 0)
    {
        Release();
        const std::wstring name = BuildMutexName(rlmSerialUtf8);
        m_mutex = CreateMutexW(NULL, FALSE, name.c_str());
        if (!m_mutex)
            return false;
        const DWORD rc = WaitForSingleObject(m_mutex, timeoutMs);
        if (rc == WAIT_OBJECT_0 || rc == WAIT_ABANDONED)
        {
            m_owned = true;
            return true;
        }
        CloseHandle(m_mutex);
        m_mutex = NULL;
        return false;
    }

    void Release()
    {
        if (m_mutex)
        {
            if (m_owned)
            {
                ReleaseMutex(m_mutex);
                m_owned = false;
            }
            CloseHandle(m_mutex);
            m_mutex = NULL;
        }
    }

    bool Owned() const { return m_owned; }

private:
    Guard(const Guard&);
    Guard& operator=(const Guard&);

    HANDLE m_mutex;
    bool m_owned;
};

} // namespace SantecRlmLock
