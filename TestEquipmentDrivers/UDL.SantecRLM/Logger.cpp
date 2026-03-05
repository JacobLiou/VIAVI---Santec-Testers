#include "stdafx.h"
#include "Logger.h"
#include <cstdarg>
#include <ctime>

namespace SantecRLM {

LogCallback CLogger::s_globalCallback = nullptr;
LogLevel CLogger::s_globalLevel = LOG_DEBUG;
std::mutex CLogger::s_mutex;

static const char* LevelToString(LogLevel level)
{
    switch (level)
    {
    case LOG_DEBUG:   return "DEBUG";
    case LOG_INFO:    return "INFO";
    case LOG_WARNING: return "WARN";
    case LOG_ERROR:   return "ERROR";
    default:          return "???";
    }
}

CLogger::CLogger(const std::string& source)
    : m_source(source)
    , m_level(LOG_DEBUG)
    , m_callback(nullptr)
{
}

CLogger::~CLogger()
{
}

void CLogger::SetLevel(LogLevel level)
{
    m_level = level;
}

void CLogger::SetCallback(LogCallback callback)
{
    m_callback = callback;
}

void CLogger::SetGlobalCallback(LogCallback callback)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    s_globalCallback = callback;
}

void CLogger::SetGlobalLevel(LogLevel level)
{
    s_globalLevel = level;
}

void CLogger::Debug(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    Log(LOG_DEBUG, fmt, args);
    va_end(args);
}

void CLogger::Info(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    Log(LOG_INFO, fmt, args);
    va_end(args);
}

void CLogger::Warning(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    Log(LOG_WARNING, fmt, args);
    va_end(args);
}

void CLogger::Error(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    Log(LOG_ERROR, fmt, args);
    va_end(args);
}

void CLogger::Log(LogLevel level, const char* fmt, va_list args)
{
    if (level < m_level && level < s_globalLevel)
        return;

    char buffer[2048];
    vsnprintf(buffer, sizeof(buffer), fmt, args);

    std::string message(buffer);

    // 实例回调优先
    LogCallback cb = m_callback;
    if (!cb)
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        cb = s_globalCallback;
    }

    if (cb)
    {
        cb(level, m_source, message);
    }
    else
    {
        // 默认：OutputDebugString
        time_t now = time(nullptr);
        struct tm tmNow;
        localtime_s(&tmNow, &now);

        char timeBuf[64];
        strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", &tmNow);

        char outBuf[2200];
        _snprintf_s(outBuf, sizeof(outBuf), "%s [%s] %s: %s\n",
            timeBuf, m_source.c_str(), LevelToString(level), buffer);
        OutputDebugStringA(outBuf);
    }
}

} // namespace SantecRLM
