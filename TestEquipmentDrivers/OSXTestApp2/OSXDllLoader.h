#pragma once

// ---------------------------------------------------------------------------
// OSXDllLoader -- UDL.OSX.dll 的纯动态绑定加载器
//
// 所有与驱动的交互都通过 LoadLibrary / GetProcAddress 进行。
// 不包含 UDL.OSX 的任何 .h 文件；不链接任何 .lib 文件。
// 这是生产框架将使用的模式。
// ---------------------------------------------------------------------------

#include <Windows.h>
#include <string>

// ---------------------------------------------------------------------------
// C兼容结构体（与 UDL.OSX 导出二进制兼容）
// ---------------------------------------------------------------------------

struct OSXDeviceInfo
{
    char manufacturer[64];
    char model[64];
    char serialNumber[64];
    char firmwareVersion[64];
};

struct OSXModuleInfo
{
    int  index;
    char catalogEntry[128];
    char detailedInfo[256];
    int  configType;
    int  channelCount;
    int  currentChannel;
    int  currentCommon;
};

// ---------------------------------------------------------------------------
// 函数指针类型定义，匹配 UDL.OSX C 导出接口
// ---------------------------------------------------------------------------

typedef HANDLE (WINAPI *PFN_OSX_CreateDriver)(const char* ip, int port);
typedef void   (WINAPI *PFN_OSX_DestroyDriver)(HANDLE hDriver);

typedef BOOL (WINAPI *PFN_OSX_Connect)(HANDLE hDriver);
typedef void (WINAPI *PFN_OSX_Disconnect)(HANDLE hDriver);
typedef BOOL (WINAPI *PFN_OSX_Reconnect)(HANDLE hDriver);
typedef BOOL (WINAPI *PFN_OSX_IsConnected)(HANDLE hDriver);

typedef BOOL (WINAPI *PFN_OSX_GetDeviceInfo)(HANDLE hDriver, OSXDeviceInfo* info);
typedef int  (WINAPI *PFN_OSX_CheckError)(HANDLE hDriver, char* message, int messageSize);
typedef BOOL (WINAPI *PFN_OSX_GetSystemVersion)(HANDLE hDriver, char* version, int versionSize);

typedef int  (WINAPI *PFN_OSX_GetModuleCount)(HANDLE hDriver);
typedef int  (WINAPI *PFN_OSX_GetModuleCatalog)(HANDLE hDriver, OSXModuleInfo* modules, int maxCount);
typedef BOOL (WINAPI *PFN_OSX_GetModuleInfo)(HANDLE hDriver, int moduleIndex, OSXModuleInfo* info);
typedef BOOL (WINAPI *PFN_OSX_SelectModule)(HANDLE hDriver, int moduleIndex);
typedef BOOL (WINAPI *PFN_OSX_SelectNextModule)(HANDLE hDriver);
typedef int  (WINAPI *PFN_OSX_GetSelectedModule)(HANDLE hDriver);

typedef BOOL (WINAPI *PFN_OSX_SwitchChannel)(HANDLE hDriver, int channel);
typedef BOOL (WINAPI *PFN_OSX_SwitchNext)(HANDLE hDriver);
typedef int  (WINAPI *PFN_OSX_GetCurrentChannel)(HANDLE hDriver);
typedef int  (WINAPI *PFN_OSX_GetChannelCount)(HANDLE hDriver);

typedef BOOL (WINAPI *PFN_OSX_RouteChannel)(HANDLE hDriver, int moduleIndex, int channel);
typedef int  (WINAPI *PFN_OSX_GetRouteChannel)(HANDLE hDriver, int moduleIndex);
typedef BOOL (WINAPI *PFN_OSX_RouteAllModules)(HANDLE hDriver, int channel);
typedef BOOL (WINAPI *PFN_OSX_SetCommonInput)(HANDLE hDriver, int moduleIndex, int common);
typedef int  (WINAPI *PFN_OSX_GetCommonInput)(HANDLE hDriver, int moduleIndex);
typedef BOOL (WINAPI *PFN_OSX_HomeModule)(HANDLE hDriver, int moduleIndex);

typedef BOOL (WINAPI *PFN_OSX_SetLocalMode)(HANDLE hDriver, BOOL local);
typedef BOOL (WINAPI *PFN_OSX_GetLocalMode)(HANDLE hDriver);
typedef BOOL (WINAPI *PFN_OSX_SendNotification)(HANDLE hDriver, int icon, const char* message);
typedef BOOL (WINAPI *PFN_OSX_Reset)(HANDLE hDriver);

typedef BOOL (WINAPI *PFN_OSX_GetNetworkInfo)(HANDLE hDriver, char* ip, char* gateway,
                                              char* netmask, char* hostname, char* mac,
                                              int bufSize);

typedef BOOL (WINAPI *PFN_OSX_WaitForOperation)(HANDLE hDriver, int timeoutMs);

typedef BOOL (WINAPI *PFN_OSX_SendCommand)(HANDLE hDriver, const char* command,
                                           char* response, int responseSize);

typedef void (WINAPI *PFN_OSXLogCallback)(int level, const char* source, const char* message);
typedef void (WINAPI *PFN_OSX_SetLogCallback)(PFN_OSXLogCallback callback);

// VISA / USB 扩展
typedef HANDLE (WINAPI *PFN_OSX_CreateDriverEx)(const char* address, int port, int commType);
typedef int    (WINAPI *PFN_OSX_EnumerateVisaResources)(char* buffer, int bufferSize);

// ---------------------------------------------------------------------------
// COSXDllLoader 类
// ---------------------------------------------------------------------------

class COSXDllLoader
{
public:
    COSXDllLoader()
        : m_hDll(NULL)
        , m_hDriver(NULL)
        , pfnCreateDriver(NULL)
        , pfnDestroyDriver(NULL)
        , pfnConnect(NULL)
        , pfnDisconnect(NULL)
        , pfnReconnect(NULL)
        , pfnIsConnected(NULL)
        , pfnGetDeviceInfo(NULL)
        , pfnCheckError(NULL)
        , pfnGetSystemVersion(NULL)
        , pfnGetModuleCount(NULL)
        , pfnGetModuleCatalog(NULL)
        , pfnGetModuleInfo(NULL)
        , pfnSelectModule(NULL)
        , pfnSelectNextModule(NULL)
        , pfnGetSelectedModule(NULL)
        , pfnSwitchChannel(NULL)
        , pfnSwitchNext(NULL)
        , pfnGetCurrentChannel(NULL)
        , pfnGetChannelCount(NULL)
        , pfnRouteChannel(NULL)
        , pfnGetRouteChannel(NULL)
        , pfnRouteAllModules(NULL)
        , pfnSetCommonInput(NULL)
        , pfnGetCommonInput(NULL)
        , pfnHomeModule(NULL)
        , pfnSetLocalMode(NULL)
        , pfnGetLocalMode(NULL)
        , pfnSendNotification(NULL)
        , pfnReset(NULL)
        , pfnGetNetworkInfo(NULL)
        , pfnWaitForOperation(NULL)
        , pfnSendCommand(NULL)
        , pfnSetLogCallback(NULL)
        , pfnCreateDriverEx(NULL)
        , pfnEnumerateVisaResources(NULL)
    {
    }

    ~COSXDllLoader()
    {
        DestroyDriver();
        UnloadDll();
    }

    // -----------------------------------------------------------------------
    // DLL 生命周期管理
    // -----------------------------------------------------------------------

    bool LoadDll(const char* dllPath)
    {
        if (m_hDll)
        {
            printf("[Loader] DLL already loaded. Unload first.\n");
            return false;
        }

        m_hDll = ::LoadLibraryA(dllPath);
        if (!m_hDll)
        {
            printf("[Loader] LoadLibrary(\"%s\") failed, error=%lu\n", dllPath, GetLastError());
            return false;
        }

        int resolved = 0;
        int total = 0;

        #define LOAD_PROC(name) \
            ++total; \
            pfn##name = (PFN_OSX_##name)::GetProcAddress(m_hDll, "OSX_" #name); \
            if (pfn##name) ++resolved; \
            else printf("[Loader] WARNING: OSX_%s not found\n", #name);

        LOAD_PROC(CreateDriver);
        LOAD_PROC(DestroyDriver);
        LOAD_PROC(Connect);
        LOAD_PROC(Disconnect);
        LOAD_PROC(Reconnect);
        LOAD_PROC(IsConnected);
        LOAD_PROC(GetDeviceInfo);
        LOAD_PROC(CheckError);
        LOAD_PROC(GetSystemVersion);
        LOAD_PROC(GetModuleCount);
        LOAD_PROC(GetModuleCatalog);
        LOAD_PROC(GetModuleInfo);
        LOAD_PROC(SelectModule);
        LOAD_PROC(SelectNextModule);
        LOAD_PROC(GetSelectedModule);
        LOAD_PROC(SwitchChannel);
        LOAD_PROC(SwitchNext);
        LOAD_PROC(GetCurrentChannel);
        LOAD_PROC(GetChannelCount);
        LOAD_PROC(RouteChannel);
        LOAD_PROC(GetRouteChannel);
        LOAD_PROC(RouteAllModules);
        LOAD_PROC(SetCommonInput);
        LOAD_PROC(GetCommonInput);
        LOAD_PROC(HomeModule);
        LOAD_PROC(SetLocalMode);
        LOAD_PROC(GetLocalMode);
        LOAD_PROC(SendNotification);
        LOAD_PROC(Reset);
        LOAD_PROC(GetNetworkInfo);
        LOAD_PROC(WaitForOperation);
        LOAD_PROC(SendCommand);
        LOAD_PROC(SetLogCallback);

        // VISA 扩展（可选，不影响总数判断）
        pfnCreateDriverEx = (PFN_OSX_CreateDriverEx)::GetProcAddress(m_hDll, "OSX_CreateDriverEx");
        pfnEnumerateVisaResources = (PFN_OSX_EnumerateVisaResources)::GetProcAddress(m_hDll, "OSX_EnumerateVisaResources");

        #undef LOAD_PROC

        printf("[Loader] DLL loaded: %d/%d functions resolved.\n", resolved, total);
        if (pfnCreateDriverEx) printf("[Loader] VISA extension: OSX_CreateDriverEx available.\n");
        if (pfnEnumerateVisaResources) printf("[Loader] VISA extension: OSX_EnumerateVisaResources available.\n");
        return (resolved == total);
    }

    void UnloadDll()
    {
        if (m_hDll)
        {
            ::FreeLibrary(m_hDll);
            m_hDll = NULL;
            memset(&pfnCreateDriver, 0,
                   (char*)&pfnSetLogCallback - (char*)&pfnCreateDriver + sizeof(pfnSetLogCallback));
            printf("[Loader] DLL unloaded.\n");
        }
    }

    bool IsDllLoaded() const { return m_hDll != NULL; }

    // -----------------------------------------------------------------------
    // 驱动实例生命周期管理
    // -----------------------------------------------------------------------

    bool CreateDriver(const char* ip, int port)
    {
        if (!pfnCreateDriver) { printf("[Loader] DLL not loaded.\n"); return false; }
        if (m_hDriver) { printf("[Loader] Driver already created. Destroy first.\n"); return false; }

        m_hDriver = pfnCreateDriver(ip, port);
        if (!m_hDriver)
        {
            printf("[Loader] OSX_CreateDriver failed.\n");
            return false;
        }
        return true;
    }

    // 扩展版本：支持指定通信类型 (0=TCP, 2=USB/VISA)
    bool CreateDriverEx(const char* address, int port, int commType)
    {
        if (!pfnCreateDriverEx) { printf("[Loader] CreateDriverEx not available.\n"); return false; }
        if (m_hDriver) { printf("[Loader] Driver already created. Destroy first.\n"); return false; }

        m_hDriver = pfnCreateDriverEx(address, port, commType);
        if (!m_hDriver)
        {
            printf("[Loader] OSX_CreateDriverEx failed.\n");
            return false;
        }
        return true;
    }

    // 枚举 VISA 资源
    int EnumerateVisaResources(char* buffer, int bufferSize)
    {
        if (!pfnEnumerateVisaResources) { printf("[Loader] EnumerateVisaResources not available.\n"); return 0; }
        return pfnEnumerateVisaResources(buffer, bufferSize);
    }

    bool HasVisaSupport() const { return pfnCreateDriverEx != NULL; }

    void DestroyDriver()
    {
        if (m_hDriver && pfnDestroyDriver)
        {
            pfnDestroyDriver(m_hDriver);
            m_hDriver = NULL;
        }
    }

    HANDLE GetDriverHandle() const { return m_hDriver; }

    // -----------------------------------------------------------------------
    // 连接管理
    // -----------------------------------------------------------------------

    BOOL Connect()
    {
        if (!m_hDriver || !pfnConnect) return FALSE;
        return pfnConnect(m_hDriver);
    }

    void Disconnect()
    {
        if (m_hDriver && pfnDisconnect) pfnDisconnect(m_hDriver);
    }

    BOOL Reconnect()
    {
        if (!m_hDriver || !pfnReconnect) return FALSE;
        return pfnReconnect(m_hDriver);
    }

    BOOL IsConnected()
    {
        if (!m_hDriver || !pfnIsConnected) return FALSE;
        return pfnIsConnected(m_hDriver);
    }

    // -----------------------------------------------------------------------
    // 设备识别
    // -----------------------------------------------------------------------

    BOOL GetDeviceInfo(OSXDeviceInfo* info)
    {
        if (!m_hDriver || !pfnGetDeviceInfo || !info) return FALSE;
        return pfnGetDeviceInfo(m_hDriver, info);
    }

    int CheckError(char* message, int messageSize)
    {
        if (!m_hDriver || !pfnCheckError) return -1;
        return pfnCheckError(m_hDriver, message, messageSize);
    }

    BOOL GetSystemVersion(char* version, int versionSize)
    {
        if (!m_hDriver || !pfnGetSystemVersion) return FALSE;
        return pfnGetSystemVersion(m_hDriver, version, versionSize);
    }

    // -----------------------------------------------------------------------
    // 模块管理
    // -----------------------------------------------------------------------

    int GetModuleCount()
    {
        if (!m_hDriver || !pfnGetModuleCount) return -1;
        return pfnGetModuleCount(m_hDriver);
    }

    int GetModuleCatalog(OSXModuleInfo* modules, int maxCount)
    {
        if (!m_hDriver || !pfnGetModuleCatalog) return 0;
        return pfnGetModuleCatalog(m_hDriver, modules, maxCount);
    }

    BOOL GetModuleInfo(int moduleIndex, OSXModuleInfo* info)
    {
        if (!m_hDriver || !pfnGetModuleInfo) return FALSE;
        return pfnGetModuleInfo(m_hDriver, moduleIndex, info);
    }

    BOOL SelectModule(int moduleIndex)
    {
        if (!m_hDriver || !pfnSelectModule) return FALSE;
        return pfnSelectModule(m_hDriver, moduleIndex);
    }

    BOOL SelectNextModule()
    {
        if (!m_hDriver || !pfnSelectNextModule) return FALSE;
        return pfnSelectNextModule(m_hDriver);
    }

    int GetSelectedModule()
    {
        if (!m_hDriver || !pfnGetSelectedModule) return -1;
        return pfnGetSelectedModule(m_hDriver);
    }

    // -----------------------------------------------------------------------
    // 通道切换
    // -----------------------------------------------------------------------

    BOOL SwitchChannel(int channel)
    {
        if (!m_hDriver || !pfnSwitchChannel) return FALSE;
        return pfnSwitchChannel(m_hDriver, channel);
    }

    BOOL SwitchNext()
    {
        if (!m_hDriver || !pfnSwitchNext) return FALSE;
        return pfnSwitchNext(m_hDriver);
    }

    int GetCurrentChannel()
    {
        if (!m_hDriver || !pfnGetCurrentChannel) return -1;
        return pfnGetCurrentChannel(m_hDriver);
    }

    int GetChannelCount()
    {
        if (!m_hDriver || !pfnGetChannelCount) return -1;
        return pfnGetChannelCount(m_hDriver);
    }

    // -----------------------------------------------------------------------
    // 多模块路由
    // -----------------------------------------------------------------------

    BOOL RouteChannel(int moduleIndex, int channel)
    {
        if (!m_hDriver || !pfnRouteChannel) return FALSE;
        return pfnRouteChannel(m_hDriver, moduleIndex, channel);
    }

    int GetRouteChannel(int moduleIndex)
    {
        if (!m_hDriver || !pfnGetRouteChannel) return -1;
        return pfnGetRouteChannel(m_hDriver, moduleIndex);
    }

    BOOL RouteAllModules(int channel)
    {
        if (!m_hDriver || !pfnRouteAllModules) return FALSE;
        return pfnRouteAllModules(m_hDriver, channel);
    }

    BOOL SetCommonInput(int moduleIndex, int common)
    {
        if (!m_hDriver || !pfnSetCommonInput) return FALSE;
        return pfnSetCommonInput(m_hDriver, moduleIndex, common);
    }

    int GetCommonInput(int moduleIndex)
    {
        if (!m_hDriver || !pfnGetCommonInput) return -1;
        return pfnGetCommonInput(m_hDriver, moduleIndex);
    }

    BOOL HomeModule(int moduleIndex)
    {
        if (!m_hDriver || !pfnHomeModule) return FALSE;
        return pfnHomeModule(m_hDriver, moduleIndex);
    }

    // -----------------------------------------------------------------------
    // 控制
    // -----------------------------------------------------------------------

    BOOL SetLocalMode(BOOL local)
    {
        if (!m_hDriver || !pfnSetLocalMode) return FALSE;
        return pfnSetLocalMode(m_hDriver, local);
    }

    BOOL GetLocalMode()
    {
        if (!m_hDriver || !pfnGetLocalMode) return FALSE;
        return pfnGetLocalMode(m_hDriver);
    }

    BOOL SendNotification(int icon, const char* message)
    {
        if (!m_hDriver || !pfnSendNotification) return FALSE;
        return pfnSendNotification(m_hDriver, icon, message);
    }

    BOOL Reset()
    {
        if (!m_hDriver || !pfnReset) return FALSE;
        return pfnReset(m_hDriver);
    }

    // -----------------------------------------------------------------------
    // 网络
    // -----------------------------------------------------------------------

    BOOL GetNetworkInfo(char* ip, char* gateway, char* netmask,
                        char* hostname, char* mac, int bufSize)
    {
        if (!m_hDriver || !pfnGetNetworkInfo) return FALSE;
        return pfnGetNetworkInfo(m_hDriver, ip, gateway, netmask, hostname, mac, bufSize);
    }

    // -----------------------------------------------------------------------
    // 操作同步
    // -----------------------------------------------------------------------

    BOOL WaitForOperation(int timeoutMs)
    {
        if (!m_hDriver || !pfnWaitForOperation) return FALSE;
        return pfnWaitForOperation(m_hDriver, timeoutMs);
    }

    // -----------------------------------------------------------------------
    // 原始SCPI命令
    // -----------------------------------------------------------------------

    BOOL SendCommand(const char* command, char* response, int responseSize)
    {
        if (!m_hDriver || !pfnSendCommand) return FALSE;
        return pfnSendCommand(m_hDriver, command, response, responseSize);
    }

    // -----------------------------------------------------------------------
    // 日志
    // -----------------------------------------------------------------------

    void SetLogCallback(PFN_OSXLogCallback callback)
    {
        if (pfnSetLogCallback) pfnSetLogCallback(callback);
    }

private:
    HMODULE m_hDll;
    HANDLE  m_hDriver;

    // 函数指针 -- 在 LoadDll 时解析
    PFN_OSX_CreateDriver      pfnCreateDriver;
    PFN_OSX_DestroyDriver     pfnDestroyDriver;
    PFN_OSX_Connect           pfnConnect;
    PFN_OSX_Disconnect        pfnDisconnect;
    PFN_OSX_Reconnect         pfnReconnect;
    PFN_OSX_IsConnected       pfnIsConnected;
    PFN_OSX_GetDeviceInfo     pfnGetDeviceInfo;
    PFN_OSX_CheckError        pfnCheckError;
    PFN_OSX_GetSystemVersion  pfnGetSystemVersion;
    PFN_OSX_GetModuleCount    pfnGetModuleCount;
    PFN_OSX_GetModuleCatalog  pfnGetModuleCatalog;
    PFN_OSX_GetModuleInfo     pfnGetModuleInfo;
    PFN_OSX_SelectModule      pfnSelectModule;
    PFN_OSX_SelectNextModule  pfnSelectNextModule;
    PFN_OSX_GetSelectedModule pfnGetSelectedModule;
    PFN_OSX_SwitchChannel     pfnSwitchChannel;
    PFN_OSX_SwitchNext        pfnSwitchNext;
    PFN_OSX_GetCurrentChannel pfnGetCurrentChannel;
    PFN_OSX_GetChannelCount   pfnGetChannelCount;
    PFN_OSX_RouteChannel      pfnRouteChannel;
    PFN_OSX_GetRouteChannel   pfnGetRouteChannel;
    PFN_OSX_RouteAllModules   pfnRouteAllModules;
    PFN_OSX_SetCommonInput    pfnSetCommonInput;
    PFN_OSX_GetCommonInput    pfnGetCommonInput;
    PFN_OSX_HomeModule        pfnHomeModule;
    PFN_OSX_SetLocalMode      pfnSetLocalMode;
    PFN_OSX_GetLocalMode      pfnGetLocalMode;
    PFN_OSX_SendNotification  pfnSendNotification;
    PFN_OSX_Reset             pfnReset;
    PFN_OSX_GetNetworkInfo    pfnGetNetworkInfo;
    PFN_OSX_WaitForOperation  pfnWaitForOperation;
    PFN_OSX_SendCommand       pfnSendCommand;
    PFN_OSX_SetLogCallback    pfnSetLogCallback;

    // VISA 扩展
    PFN_OSX_CreateDriverEx          pfnCreateDriverEx;
    PFN_OSX_EnumerateVisaResources  pfnEnumerateVisaResources;
};
