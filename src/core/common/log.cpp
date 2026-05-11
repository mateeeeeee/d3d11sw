#include "core/common/common.h"
#include "core/common/log.h"
#include "core/util/env.h"
#include <chrono>
#include <cstring>

namespace d3dsw {

Logger::Logger()
{
    const Char* env = GetEnvString("D3DSW_LOG_LEVEL");
    if (!env)
    {
#if defined(DEBUG_D3DSW)
        _minLevel = LogLevel::Debug;
#else
        _minLevel = LogLevel::Warning;
#endif
        return;
    }

    if (std::strcmp(env, "debug") == 0 || std::strcmp(env, "all") == 0)
    {
        _minLevel = LogLevel::Debug;
    }
    else if (std::strcmp(env, "info") == 0)
    {
        _minLevel = LogLevel::Info;
    }
    else if (std::strcmp(env, "warning") == 0 || std::strcmp(env, "warn") == 0)
    {
        _minLevel = LogLevel::Warning;
    }
    else if (std::strcmp(env, "error") == 0)
    {
        _minLevel = LogLevel::Error;
    }
    else if (std::strcmp(env, "none") == 0 || std::strcmp(env, "off") == 0)
    {
        _minLevel = static_cast<LogLevel>(static_cast<Uint8>(LogLevel::Error) + 1);
    }
}

std::string Logger::GetLogPrefix(LogLevel level)
{
    switch (level)
    {
    case LogLevel::Debug:   return "[DEBUG]";
    case LogLevel::Info:    return "[INFO]";
    case LogLevel::Warning: return "[WARNING]";
    case LogLevel::Error:   return "[ERROR]";
    }
    return "";
}

std::string Logger::GetCurrentTimeString()
{
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::tm local_time{};
#ifdef D3DSW_PLATFORM_WINDOWS
    localtime_s(&local_time, &in_time_t);
#else
    localtime_r(&in_time_t, &local_time);
#endif
    Char timestamp[64];
    std::snprintf(timestamp, sizeof(timestamp), "[%04d-%02d-%02d %02d:%02d:%02d.%03d]",
        local_time.tm_year + 1900,
        local_time.tm_mon + 1,
        local_time.tm_mday,
        local_time.tm_hour,
        local_time.tm_min,
        local_time.tm_sec,
        static_cast<Int>(ms.count()));
    return timestamp;
}

void Logger::Output(const std::string& msg)
{
    std::fprintf(stderr, "%s", msg.c_str());
#ifdef D3DSW_PLATFORM_WINDOWS
    OutputDebugStringA(msg.c_str());
#endif
}

}
