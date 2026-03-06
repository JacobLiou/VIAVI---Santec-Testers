#pragma once

#include "PCTTypes.h"
#include <string>
#include <functional>
#include <mutex>

namespace ViaviPCT {

typedef std::function<void(LogLevel level, const std::string& source, const std::string& message)> LogCallback;

class PCT_API CLogger
{
public:
    CLogger(const std::string& source);
    ~CLogger();

    void SetLevel(LogLevel level);
    void SetCallback(LogCallback callback);

    void Debug(const char* fmt, ...);
    void Info(const char* fmt, ...);
    void Warning(const char* fmt, ...);
    void Error(const char* fmt, ...);

    static void SetGlobalCallback(LogCallback callback);
    static void SetGlobalLevel(LogLevel level);

private:
    void Log(LogLevel level, const char* fmt, va_list args);

    std::string m_source;
    LogLevel m_level;
    LogCallback m_callback;

    static LogCallback s_globalCallback;
    static LogLevel s_globalLevel;
    static std::mutex s_mutex;
};

} // namespace ViaviPCT
