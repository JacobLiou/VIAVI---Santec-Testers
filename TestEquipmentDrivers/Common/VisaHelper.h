#pragma once

// ---------------------------------------------------------------------------
// VisaHelper.h -- 仅头文件的 VISA 动态加载器
//
// 通过 LoadLibrary/GetProcAddress 动态加载 VISA 运行时库（visa32.dll 或 visa64.dll），
// 避免编译时对任何 VISA SDK 的硬依赖。
//
// 支持的 VISA 实现：R&S VISA、NI VISA、Keysight VISA 等
// ---------------------------------------------------------------------------

#include <windows.h>
#include <string>
#include <vector>
#include <cstring>

namespace VisaHelper {

// ---------------------------------------------------------------------------
// VISA 基本类型定义（与 visa.h 兼容）
// ---------------------------------------------------------------------------
typedef unsigned long  ViUInt32;
typedef long           ViInt32;
typedef unsigned short ViUInt16;
typedef short          ViInt16;
typedef char           ViChar;
typedef ViChar*        ViString;
typedef const ViChar*  ViConstString;
typedef unsigned char  ViBoolean;
typedef unsigned char* ViBuf;
typedef ViUInt32       ViStatus;
typedef ViUInt32       ViSession;
typedef ViUInt32       ViObject;
typedef ViUInt32       ViAttr;
typedef ViString       ViRsrc;
typedef ViUInt32       ViAccessMode;
typedef ViUInt32       ViFindList;
typedef ViUInt32       ViAttrState;

// ---------------------------------------------------------------------------
// VISA 常量
// ---------------------------------------------------------------------------
static const ViSession VI_NULL           = 0;
static const ViStatus  VI_SUCCESS        = 0;
static const ViAccessMode VI_NO_LOCK     = 0;
static const ViUInt32  VI_FIND_BUFLEN    = 256;

static const ViAttr VI_ATTR_TMO_VALUE    = 0x3FFF001A;
static const ViAttr VI_ATTR_SEND_END_EN  = 0x3FFF0016;
static const ViAttr VI_ATTR_TERMCHAR_EN  = 0x3FFF0038;
static const ViAttr VI_ATTR_TERMCHAR     = 0x3FFF0018;
static const ViAttr VI_ATTR_RET_COUNT    = 0x3FFF4026;
static const ViAttr VI_ATTR_RSRC_NAME    = 0xBFFF0002;

static const ViUInt32 VI_TMO_INFINITE    = 0xFFFFFFFF;

// ---------------------------------------------------------------------------
// VISA 函数指针类型定义
// ---------------------------------------------------------------------------
typedef ViStatus (__stdcall *PFN_viOpenDefaultRM)(ViSession* vi);
typedef ViStatus (__stdcall *PFN_viOpen)(ViSession sesn, ViRsrc rsrcName,
                                         ViAccessMode accessMode, ViUInt32 openTimeout,
                                         ViSession* vi);
typedef ViStatus (__stdcall *PFN_viClose)(ViObject vi);
typedef ViStatus (__stdcall *PFN_viWrite)(ViSession vi, ViBuf buf, ViUInt32 count,
                                          ViUInt32* retCount);
typedef ViStatus (__stdcall *PFN_viRead)(ViSession vi, ViBuf buf, ViUInt32 count,
                                         ViUInt32* retCount);
typedef ViStatus (__stdcall *PFN_viSetAttribute)(ViObject vi, ViAttr attrName,
                                                  ViAttrState attrValue);
typedef ViStatus (__stdcall *PFN_viGetAttribute)(ViObject vi, ViAttr attrName,
                                                  void* attrValue);
typedef ViStatus (__stdcall *PFN_viStatusDesc)(ViObject vi, ViStatus status,
                                                ViChar desc[]);
typedef ViStatus (__stdcall *PFN_viFindRsrc)(ViSession sesn, ViString expr,
                                              ViFindList* findList, ViUInt32* retcnt,
                                              ViChar desc[]);
typedef ViStatus (__stdcall *PFN_viFindNext)(ViFindList findList, ViChar desc[]);
typedef ViStatus (__stdcall *PFN_viClear)(ViSession vi);

// ---------------------------------------------------------------------------
// CVisaLoader -- 动态加载 VISA 运行时并解析函数地址
// ---------------------------------------------------------------------------
class CVisaLoader
{
public:
    PFN_viOpenDefaultRM pfnOpenDefaultRM;
    PFN_viOpen          pfnOpen;
    PFN_viClose         pfnClose;
    PFN_viWrite         pfnWrite;
    PFN_viRead          pfnRead;
    PFN_viSetAttribute  pfnSetAttribute;
    PFN_viGetAttribute  pfnGetAttribute;
    PFN_viStatusDesc    pfnStatusDesc;
    PFN_viFindRsrc      pfnFindRsrc;
    PFN_viFindNext      pfnFindNext;
    PFN_viClear         pfnClear;

    CVisaLoader()
        : m_hModule(NULL)
        , pfnOpenDefaultRM(NULL), pfnOpen(NULL), pfnClose(NULL)
        , pfnWrite(NULL), pfnRead(NULL)
        , pfnSetAttribute(NULL), pfnGetAttribute(NULL)
        , pfnStatusDesc(NULL)
        , pfnFindRsrc(NULL), pfnFindNext(NULL)
        , pfnClear(NULL)
    {}

    ~CVisaLoader()
    {
        UnloadVisa();
    }

    bool LoadVisa()
    {
        if (m_hModule)
            return true;

#ifdef _WIN64
        const char* dllNames[] = { "visa64.dll", "visa32.dll", "RsVisa64.dll", NULL };
#else
        const char* dllNames[] = { "visa32.dll", "visa64.dll", "RsVisa32.dll", NULL };
#endif

        for (int i = 0; dllNames[i] != NULL; ++i)
        {
            m_hModule = ::LoadLibraryA(dllNames[i]);
            if (m_hModule)
                break;
        }

        if (!m_hModule)
            return false;

        pfnOpenDefaultRM = (PFN_viOpenDefaultRM)::GetProcAddress(m_hModule, "viOpenDefaultRM");
        pfnOpen          = (PFN_viOpen)::GetProcAddress(m_hModule, "viOpen");
        pfnClose         = (PFN_viClose)::GetProcAddress(m_hModule, "viClose");
        pfnWrite         = (PFN_viWrite)::GetProcAddress(m_hModule, "viWrite");
        pfnRead          = (PFN_viRead)::GetProcAddress(m_hModule, "viRead");
        pfnSetAttribute  = (PFN_viSetAttribute)::GetProcAddress(m_hModule, "viSetAttribute");
        pfnGetAttribute  = (PFN_viGetAttribute)::GetProcAddress(m_hModule, "viGetAttribute");
        pfnStatusDesc    = (PFN_viStatusDesc)::GetProcAddress(m_hModule, "viStatusDesc");
        pfnFindRsrc      = (PFN_viFindRsrc)::GetProcAddress(m_hModule, "viFindRsrc");
        pfnFindNext      = (PFN_viFindNext)::GetProcAddress(m_hModule, "viFindNext");
        pfnClear         = (PFN_viClear)::GetProcAddress(m_hModule, "viClear");

        if (!pfnOpenDefaultRM || !pfnOpen || !pfnClose || !pfnWrite || !pfnRead)
        {
            UnloadVisa();
            return false;
        }

        return true;
    }

    void UnloadVisa()
    {
        if (m_hModule)
        {
            ::FreeLibrary(m_hModule);
            m_hModule = NULL;
        }
        pfnOpenDefaultRM = NULL;
        pfnOpen          = NULL;
        pfnClose         = NULL;
        pfnWrite         = NULL;
        pfnRead          = NULL;
        pfnSetAttribute  = NULL;
        pfnGetAttribute  = NULL;
        pfnStatusDesc    = NULL;
        pfnFindRsrc      = NULL;
        pfnFindNext      = NULL;
        pfnClear         = NULL;
    }

    bool IsLoaded() const { return m_hModule != NULL; }

    // 枚举匹配 pattern 的 VISA 资源（例如 "USB?*INSTR"）
    std::vector<std::string> FindResources(const std::string& pattern = "USB?*INSTR")
    {
        std::vector<std::string> results;

        if (!IsLoaded() || !pfnFindRsrc || !pfnFindNext || !pfnOpenDefaultRM || !pfnClose)
            return results;

        ViSession defaultRM = VI_NULL;
        if (pfnOpenDefaultRM(&defaultRM) != VI_SUCCESS)
            return results;

        ViFindList findList = 0;
        ViUInt32 retcnt = 0;
        ViChar desc[VI_FIND_BUFLEN];
        memset(desc, 0, sizeof(desc));

        ViStatus status = pfnFindRsrc(defaultRM, (ViString)pattern.c_str(),
                                       &findList, &retcnt, desc);
        if (status == VI_SUCCESS && retcnt > 0)
        {
            results.push_back(std::string(desc));
            for (ViUInt32 i = 1; i < retcnt; ++i)
            {
                memset(desc, 0, sizeof(desc));
                if (pfnFindNext(findList, desc) == VI_SUCCESS)
                    results.push_back(std::string(desc));
            }
            pfnClose(findList);
        }

        pfnClose(defaultRM);
        return results;
    }

    // 获取 VISA 错误描述
    std::string GetStatusDescription(ViSession vi, ViStatus status)
    {
        if (!pfnStatusDesc)
            return "Unknown VISA error";

        ViChar desc[256];
        memset(desc, 0, sizeof(desc));
        pfnStatusDesc(vi, status, desc);
        return std::string(desc);
    }

private:
    HMODULE m_hModule;

    CVisaLoader(const CVisaLoader&);
    CVisaLoader& operator=(const CVisaLoader&);
};

} // namespace VisaHelper
